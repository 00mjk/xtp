/*
 *            Copyright 2009-2017 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <votca/tools/linalg.h>
#include <votca/xtp/geometry_optimization.h>
#include <votca/xtp/forces.h>
#include <votca/xtp/bfgs-trm.h>

namespace votca {
    namespace xtp {

        void BFGSTRM::Initialize(Property *options) {

            // checking if there is only one segment
            _nsegments = _segments.size();
            if (_nsegments > 1) throw runtime_error(string("\n Geometry optimization of more than 1 conjugated segment not supported. Stopping!"));
            
            _convergence  = options->ifExistsReturnElseReturnDefault<double>(".convergence", 0.01); // units?

            // initial trust radius
            _trust_radius = options->ifExistsReturnElseReturnDefault<double>(".trust", 0.01); // units?
            
            // restart from saved history
            _restart_opt = options->ifExistsReturnElseReturnDefault<bool>(".restart", false);
            if (_restart_opt && !boost::filesystem::exists("optimization.restart")) {
                throw runtime_error(string("\n Restart requested but optimization.restart file not found!")); 
            };
            

            // fix units from Ang to Bohr
            _trust_radius = _trust_radius * tools::conv::ang2bohr; // initial trust radius in a.u.
            _trust_radius_max = 0.1;
            _spintype  = _force_engine.GetSpinType();
            _opt_state = _force_engine.GetOptState(); 
            
            _natoms = _segments[0]->Atoms().size();
            _force = ub::zero_matrix<double>(_natoms, 3);
            _force_old = ub::zero_matrix<double>(_natoms, 3);
            _xyz_shift = ub::zero_matrix<double>(_natoms, 3);
            _current_xyz = ub::zero_matrix<double>(_natoms, 3);
            _old_xyz = ub::zero_matrix<double>(_natoms, 3);
            _trial_xyz = ub::zero_matrix<double>(_natoms, 3);
            _hessian = ub::zero_matrix<double>(3 * _natoms, 3 * _natoms);
            
            // construct vectors (because?)
            _dim = 3 * _natoms;
            _previous_pos = ub::zero_vector<double>(_dim);
            _current_pos = ub::zero_vector<double>(_dim);
            _previous_gradient = ub::zero_vector<double>(_dim);
            _current_gradient = ub::zero_vector<double>(_dim);
            
            // Initial coordinates
            Segment2BFGS();
            
            return;

        }

        void BFGSTRM::Optimize() {

            CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM convergence of total energy:                 " << _convergence << " Hartree" << flush;
            CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM initial trust radius:                        " << _trust_radius << " Bohr " << flush;
            bool _converged = false;

            // in 1st iteration, get the energy of the initial configuration
            if ( _iteration == 0 ){
                _last_energy = GetEnergy(); // in Hartree
            }

            // now change geometry until convergence
            while (!_converged) {
                _iteration++;
                _update_hessian = true; // always update Hessian after each accepted step

                // calculate the forces for the present configuration
                _force_engine.Calculate(_last_energy);
                _force = _force_engine.GetForces();

                _step_accepted = false;
                while (!_step_accepted) {

                    // determine new trial configuration according to BFGS
                    BFGSStep();

                    // update coordinates in segment
                    UpdateSegment();

                    // for the updated geometry, get new reference energy
                    _new_energy = GetEnergy();
                    _energy_delta = _new_energy - _last_energy;

                    // check the energy at the trial coordinates, accept/reject step, and adjust trust radius
                    AcceptReject();
                    

                } // checking step to be trusted


                
                // after the step is accepted, we can shift the stored data   
                _last_energy = _new_energy;
                _old_xyz     = _current_xyz;
                _current_xyz = _trial_xyz;
                _force_old   = _force; 

                // Check convergence criteria
                _converged = GeometryConverged();

                // Report summary of the iteration
                Report();
                
            } // convergence

            return;

        }
        
        /* Accept/reject the new geometry and adjust trust radius, if required */
        void BFGSTRM::AcceptReject(){
            
            if (_energy_delta > 0.0) {
                        // total energy has unexpectedly increased, half the trust radius
                        _trust_radius = 0.5 * _trust_radius;
                        // Hessian should not be updated for reduced trust radius
                        _update_hessian = false;
                        CTP_LOG(ctp::logINFO,*_pLog) << " BFGS-TRM: total energy increased, this step is rejected and the new trust radius is: " << _trust_radius << endl;
                        // repeat BGFSStep with new trust radius
                    } else {
                        // total energy has decreased, we accept the step but might update the trust radius
                        _step_accepted = true;
                        CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM: step accepted" << flush;
                        // adjust trust radius, if required
                        double _tr_check = _energy_delta / _delta_energy_estimate;
                        if (_tr_check > 0.75 && 1.25 * sqrt(_norm_delta_pos) > _trust_radius) {
                            _trust_radius = 2.0 * _trust_radius;
                            // if ( _trust_radius > _trust_radius_max) _trust_radius = _trust_radius_max;
                            CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM: increasing trust radius to " << _trust_radius << endl;
                        } else if (_tr_check < 0.25) {
                            _trust_radius = 0.25 * sqrt(_norm_delta_pos);
                            CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM: reducing trust radius to " << _trust_radius << endl;
                        }

                    }
            
            
        }
        
        
        /* Get the energy for the current configuration */
        double BFGSTRM::GetEnergy(){


            _gwbse_engine.setRedirectLogger( true );
            string _logger_file = "gwbse_iteration_" + (boost::format("%1%") % _iteration ).str() + ".log";
            _gwbse_engine.setLoggerFile( _logger_file  );
            _gwbse_engine.ExcitationEnergies(_qmpackage, _segments, _orbitals);
            double _energy = _orbitals->GetTotalEnergy(_spintype, _opt_state); // in Hartree
            _gwbse_engine.setRedirectLogger( false );
            
            return _energy;
            
            
        }
        
        
        /* Report results of accepted step*/
        void BFGSTRM::Report(){
            
            // accepted step 
            CTP_LOG(ctp::logINFO, *_pLog) << (boost::format(" ======= OPTIMIZATION SUMMARY =======  ")).str() << flush;
            CTP_LOG(ctp::logINFO, *_pLog) << " At iteration  " << _iteration << flush;
            CTP_LOG(ctp::logINFO, *_pLog) << (boost::format("   Atom\t x\t  y\t  z ")).str() << flush;

            for (unsigned _i = 0; _i < _natoms; _i++) {
                CTP_LOG(ctp::logINFO, *_pLog) << (boost::format(" %1$4d    %2$+1.4f  %3$+1.4f  %4$+1.4f")
                        % _i  % (_current_xyz(_i,0) * votca::tools::conv::bohr2ang) % (_current_xyz(_i, 1)* votca::tools::conv::bohr2ang)  % (_current_xyz(_i, 2)* votca::tools::conv::bohr2ang) ).str() << flush;
            }
            
            CTP_LOG(ctp::logINFO, *_pLog) << (boost::format("   Total energy: %1$12.8f Hartree ") % _new_energy ).str() << flush;
            
            CTP_LOG(ctp::logINFO,*_pLog) << "   energy change: " << setprecision(12) << _energy_delta << " Hartree      " << _energy_converged   << flush;
            CTP_LOG(ctp::logINFO,*_pLog) << "   RMS force:     " << setprecision(12) << _RMSForce     << " Hartree/Bohr " << _RMSForce_converged << flush;
            CTP_LOG(ctp::logINFO,*_pLog) << "   Max force:     " << setprecision(12) << _MaxForce     << " Hartree/Bohr " << _MaxForce_converged << flush;
            CTP_LOG(ctp::logINFO,*_pLog) << "   RMS step:      " << setprecision(12) << _RMSStep      << " Bohr         " << _RMSStep_converged  << flush;
            CTP_LOG(ctp::logINFO,*_pLog) << "   Max step:      " << setprecision(12) << _MaxStep      << " Bohr         " << _MaxStep_converged  << flush;
            
            return;
        }
        
        /* Check convergence */
        bool BFGSTRM::GeometryConverged() {


            // checking convergence
            _energy_converged = false;
            _RMSForce_converged = false;
            _MaxForce_converged = false;
            _RMSStep_converged = false;
            _MaxStep_converged = false;

            _xyz_shift = _current_xyz - _old_xyz;

            _RMSForce = linalg_getRMS(_force);
            _MaxForce = linalg_getMax(_force, true);
            _RMSStep  = linalg_getRMS(_xyz_shift);
            _MaxStep  = linalg_getMax(_xyz_shift, true);

            if (std::abs(_energy_delta) < _convergence) _energy_converged = true;
            if (std::abs(_RMSForce) < 0.01) _RMSForce_converged = true;
            if (std::abs(_MaxForce) < 0.01) _MaxForce_converged = true;
            if (std::abs(_RMSStep) < 0.01) _RMSStep_converged = true;
            if (std::abs(_MaxStep) < 0.01) _MaxStep_converged = true;

            if (_energy_converged && _RMSForce_converged && _MaxForce_converged && _RMSStep_converged && _MaxStep_converged){
                return true;
            }else {
                return false;
            }


        }
        
        /* Update segment with new coordinates*/
        void BFGSTRM::UpdateSegment() {
            
            std::vector< ctp::Atom* > _atoms;
            std::vector< ctp::Atom* > ::iterator ait;
            _atoms = _segments[0]->Atoms();
            
            // put trial coordinates into _segment
            int _i_atom = 0;
            for (ait = _atoms.begin(); ait < _atoms.end(); ++ait) {
                // put trial coordinates (_trial_xyz is in Bohr, segments in nm)
                vec _pos_displaced(_trial_xyz(_i_atom, 0) * tools::conv::bohr2nm, _trial_xyz(_i_atom, 1) * tools::conv::bohr2nm, _trial_xyz(_i_atom, 2) * tools::conv::bohr2nm);
                (*ait)->setQMPos(_pos_displaced); // put updated coordinate into segment
                _i_atom++;
            }

            return;

        }
        
        
        /* Determine new trial coordinates according to BFGS */
        void BFGSTRM::BFGSStep() {

           
            // Remove total force to avoid CoM translation
            RemoveTotalForce();

            // Rewrite2Vectors (let's rethink that later)
            Rewrite2Vectors();
 
            // Update Hessian
            if ( _update_hessian ) UpdateHessian();

            // Get displacement of coordinates
            PredictDisplacement();
            //CTP_LOG(ctp::logINFO, *_pLog) << " Predicted displacements : " << _delta_pos << flush;

            // TRM -> trust radius check and regularization, if needed
            if ( OutsideTrustRegion(_norm_delta_pos) ) RegularizeStep();
            //CTP_LOG(ctp::logINFO, *_pLog) << " Predicted displacements : " << _delta_pos << flush;
            
            
            // expected energy change on quadratic surface
            QuadraticEnergy();
            CTP_LOG(ctp::logINFO, *_pLog) << " BFGS-TRM: estimated energy change: " << setprecision(12) << _delta_energy_estimate << endl;

            // new trial coordinated are written to _trial_xyz
            Rewrite2Matrices();
            
            /* 
             // make sure there is no CoM movement
                for ( int _i_atom = 0; _i_atom < _natoms; _i_atom++){
                 for ( int _i_cart = 0; _i_cart < 3; _i_cart++ ){
                      //int _idx = 3*_i_atom + _i_cart;
                     //_current_xyz(_i_atom,_i_cart) -= _total_shift(_i_cart)/_natoms;
                     // _trial_xyz(_i_atom,_i_cart) -= _total_shift(_i_cart)/_natoms;
                 }
             }
             */
            return;
        }

       

        /* Determine Total Force on all atoms */
        ub::vector<double> BFGSTRM::TotalForce(){
            
            ub::vector<double> _total_force(3, 0.0);
            for (unsigned _i_atom = 0; _i_atom < _natoms; _i_atom++) {
                for (unsigned _i_cart = 0; _i_cart < 3; _i_cart++) {
                    _total_force(_i_cart) += _force(_i_atom, _i_cart);
                }
            }
            return _total_force;
        }
        
        
        
        /* Adjust forces so that sum of forces is zero */
        void BFGSTRM::RemoveTotalForce(){
            
            // total force on all atoms
            ub::vector<double> _total_force = TotalForce();
            
            // zero total force
            for (int _i_atom = 0; _i_atom < _natoms; _i_atom++) {
                for (int _i_cart = 0; _i_cart < 3; _i_cart++) {
                    _force(_i_atom,_i_cart) -= _total_force(_i_cart)/_natoms;
                }
            }
            return;
        }

        /* Re-Store the matrix data into vectors (let's rethink this later) */
        void BFGSTRM::Rewrite2Vectors() {
            for (unsigned _i_atom = 0; _i_atom < _natoms; _i_atom++) {
                for (unsigned _i_cart = 0; _i_cart < 3; _i_cart++) {

                    int _idx = 3 * _i_atom + _i_cart;
                    _previous_pos(_idx)      = _old_xyz(_i_atom, _i_cart);
                    _current_pos(_idx)       = _current_xyz(_i_atom, _i_cart);
                    _previous_gradient(_idx) = -_force_old(_i_atom, _i_cart);
                    _current_gradient(_idx)  = -_force(_i_atom, _i_cart);

                }
            }

            return;

        }

        
        
        
        /* Update the Hessian */
        void BFGSTRM::UpdateHessian(){
            
            // delta is new - old
            _delta_pos = _current_pos - _previous_pos;
            _norm_delta_pos = ub::inner_prod(_delta_pos, _delta_pos);

            // we have no Hessian in the first iteration => start with something
            if (_iteration == 1) {
                for (unsigned _i = 0; _i < _dim; _i++) {
                    _hessian(_i, _i) = 1.0; // unit matrix
                }

            } else {
                /* for later iteration, we can make use of an iterative refinement of 
                * the initial Hessian based on the gradient (force) history
                */

                ub::vector<double> _delta_gradient = _current_gradient - _previous_gradient;

                // second term in BFGS update (needs current Hessian)
                ub::vector<double> _temp1 = ub::prod(_hessian, _delta_pos);
                ub::vector<double> _temp2 = ub::prod(ub::trans(_delta_pos), _hessian);
                _hessian -= ub::outer_prod(_temp1, _temp2) / ub::inner_prod(_delta_pos, _temp1);

                // first term in BFGS update
                _hessian += ub::outer_prod(_delta_gradient, _delta_gradient) / ub::inner_prod(_delta_gradient, _delta_pos);

                // symmetrize Hessian (since d2E/dxidxj should be symmetric)
                for (unsigned _i = 0; _i < _dim; _i++) {
                    for (unsigned _j = _i + 1; _j < _dim; _j++) {
                        double _sum = 0.5 * (_hessian(_i, _j) + _hessian(_j, _i));
                        _hessian(_i, _j) = _sum;
                        _hessian(_j, _i) = _sum;
                    }
                }
            } // update Hessian
            
            return;
        }
        
        /* Predict displacement of atom coordinates */
        void BFGSTRM::PredictDisplacement(){
            
            // get inverse of the Hessian
            ub::matrix<double> _hessian_inverse;
            votca::tools::linalg_invert(_hessian, _hessian_inverse);

            // new displacements for the atoms
            _delta_pos = -ub::prod(_hessian_inverse, _current_gradient);
            _norm_delta_pos = ub::inner_prod(_delta_pos, _delta_pos);
            
            return;
        }
        
        
        
        /* Check if predicted displacement leaves trust region */
        bool BFGSTRM::OutsideTrustRegion( const double& _step ){

            double _trust_radius_squared = _trust_radius * _trust_radius;
            if ( _step > _trust_radius_squared ) { 
                return true;
            } else {
                return false;
            }
            
        }
        /* Regularize step in case of prediction outside of Trust Region */
        void BFGSTRM::RegularizeStep() {
            
            double _max_step_squared = 0.0;
            
            // get eigenvalues and eigenvectors of Hessian
            ub::matrix<double> _eigenvectors;
            ub::vector<double> _eigenvalues;
            votca::tools::linalg_eigenvalues(_hessian, _eigenvalues, _eigenvectors);

            // start value for lambda  a bit lower than lowest eigenvalue of Hessian
            double _lambda;
            if (_eigenvalues(0) > 0.0) {
                _lambda = -0.05 * std::abs(_eigenvalues(0));
            } else {
                _lambda = 1.05 * _eigenvalues(0);
            }

            // for constrained step, we expect
            _max_step_squared = _norm_delta_pos;
            while ( OutsideTrustRegion(_max_step_squared) ) {
                _max_step_squared = 0.0;
                _lambda -= 0.05 * std::abs(_eigenvalues(0));
                for (int _i = 0; _i < _dim; _i++) {
                    ub::vector<double> _slice(_dim, 0.0);
                    for (int _j = 0; _j < _dim; _j++) {
                        _slice(_j) = _eigenvectors(_j, _i);
                    }

                    //cout << " forece is of dim " << _current_force.size1() << "  " << _current_force.size2() << endl;
                    double _temp = ub::inner_prod(_slice, _current_gradient);
                    //cout << " slice is of dim " << _slice.size1() << "  " << _slice.size2() << endl;            cout << " tmep is of dim " << _temp.size1() << "  " << _temp.size2() << endl;
                    // cout << " into max_step_sq " << _temp << " and  "  << ( _eigenvalues(_i) - _lambda ) << endl;
                    _max_step_squared += _temp * _temp / (_eigenvalues(_i) - _lambda) / (_eigenvalues(_i) - _lambda);
                }
            }

            //CTP_LOG(ctp::logDEBUG,_log) << " BFGS-TRM: with lambda " << _lambda << " max step sq is " << _max_step_squared << flush;

            _delta_pos = ub::zero_vector<double>(_dim);
            for (int _i = 0; _i < _dim; _i++) {
                ub::vector<double> _slice(_dim, 0.0);
                for (int _j = 0; _j < _dim; _j++) {
                    _slice(_j) = _eigenvectors(_j, _i);
                }

                _delta_pos -= _slice * ub::inner_prod(_slice, _current_gradient) / (_eigenvalues(_i) - _lambda);
            }

            _norm_delta_pos = ub::inner_prod(_delta_pos, _delta_pos); //_max_step_squared; 

            return;
        }
        
        
        /* Estimate energy change based on quadratic approximation */
        void BFGSTRM::QuadraticEnergy(){

            ub::vector<double> _temp_ene = ub::prod(_hessian, _delta_pos);
            _delta_energy_estimate = ub::inner_prod(_current_gradient, _delta_pos) + 0.5 * ub::inner_prod(_delta_pos, _temp_ene);
            return;
            
        }
        
        /* Rewrite the vector data back to matrices (to rethink) */
        void BFGSTRM::Rewrite2Matrices(){
            ub::vector<double> _new_pos = _current_pos + _delta_pos;
            //CTP_LOG(ctp::logDEBUG,_log) << "BFGS-TRM: step " << sqrt(_norm_delta_pos) << " vs TR " << sqrt(_trust_radius_squared) << flush  ;
            // update atom coordinates
            ub::vector<double> _total_shift(3, 0.0);
            for ( unsigned _i_atom = 0; _i_atom < _natoms; _i_atom++) {
                for ( unsigned _i_cart = 0; _i_cart < 3; _i_cart++) {
                    unsigned _idx = 3 * _i_atom + _i_cart;
                    _trial_xyz(_i_atom, _i_cart) = _new_pos(_idx);
                    _total_shift(_i_cart) += _delta_pos(_idx);
                }
            }
            return;
        }
        
        
        /* Segment info to xyz */
        void BFGSTRM::Segment2BFGS(){
            
               
            std::vector< ctp::Atom* > _atoms;
            std::vector< ctp::Atom* > ::iterator ait;
            _atoms = _segments[0]->Atoms();
            
            // put trial coordinates into _segment
            unsigned _i_atom = 0;
            for (ait = _atoms.begin(); ait < _atoms.end(); ++ait) {
                // put trial coordinates (_current_xyz is in Bohr, segments in nm)
                _current_xyz( _i_atom, 0 ) = (*ait)->getQMPos().getX() *  tools::conv::nm2bohr;
                _current_xyz( _i_atom, 1 ) = (*ait)->getQMPos().getY() *  tools::conv::nm2bohr;
                _current_xyz( _i_atom, 2 ) = (*ait)->getQMPos().getZ() *  tools::conv::nm2bohr;
                _i_atom++;
            }

            return;

            
            
        }

    }
}
