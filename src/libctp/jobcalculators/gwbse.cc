/*
 *            Copyright 2009-2012 The VOTCA Development Team
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

// Overload of uBLAS prod function with MKL/GSL implementations
#include <votca/ctp/votca_ctp_config.h>

#include "gwbse.h"

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <votca/ctp/aomatrix.h>
#include <votca/ctp/threecenters.h>
// #include <votca/ctp/logger.h>
#include <votca/ctp/qmpackagefactory.h>
#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/symmetric.hpp>
#include <votca/tools/linalg.h>

using boost::format;
using namespace boost::filesystem;

namespace votca {
    namespace ctp {
        namespace ub = boost::numeric::ublas;

        // +++++++++++++++++++++++++++++ //
        // GWBSE MEMBER FUNCTIONS         //
        // +++++++++++++++++++++++++++++ //

        void GWBSE::CleanUp() {

        }

        int GWBSE::NumFuncShell(string shell_type) {
            int _nbf;
            if (shell_type == "S") {
                _nbf = 1;
            } else if (shell_type == "P") {
                _nbf = 3;
            } else if (shell_type == "D") {
                _nbf = 5;
            } else if (shell_type == "SP") {
                _nbf = 4;
            } else if (shell_type == "SPD") {
                _nbf = 9;
            }
            return _nbf;
        }

        int GWBSE::OffsetFuncShell(string shell_type) {
            int _nbf;
            if (shell_type == "S") {
                _nbf = 0;
            } else if (shell_type == "P") {
                _nbf = 1;
            } else if (shell_type == "D") {
                _nbf = 4;
            } else if (shell_type == "SP") {
                _nbf = 0;
            } else if (shell_type == "SPD") {
                _nbf = 0;
            }
            return _nbf;
        }

        void GWBSE::Initialize(Property *options) {

            _maverick = (_nThreads == 1) ? true : false;

            /* obsolete string key = "options." + Identify();
            _jobfile = options->get(key + ".job_file").as<string>(); */

            string key = "options." + Identify() + ".job";
            _jobfile = options->get(key + ".file").as<string>();

            /*_do_input = false;
            _do_run = false;
            _do_parse = false;
            _do_trim = false;
    
            // conversion to GW 
            _do_convert = false;
            _do_gwbse_input = false;
            _do_gwbse_run = false;
            _do_gwbse_parse = false;
        

            string _package_xml = options->get(key+".package").as<string> ();

    
            string _tasks_string = options->get(key+".tasks").as<string> ();
            if (_tasks_string.find("input") != std::string::npos) _do_input = true;
            if (_tasks_string.find("run") != std::string::npos) _do_run = true;
            if (_tasks_string.find("trim") != std::string::npos) _do_trim = true;
            if (_tasks_string.find("parse") != std::string::npos) _do_parse = true;    
            // GW-BSE tasks
            if (_tasks_string.find("convert") != std::string::npos) _do_convert = true;   
            if (_tasks_string.find("gwbse_setup") != std::string::npos) _do_gwbse_input = true;
            if (_tasks_string.find("gwbse_exec") != std::string::npos) _do_gwbse_run = true;    
            if (_tasks_string.find("gwbse_read") != std::string::npos) _do_gwbse_parse = true;
    
            string _store_string = options->get(key+".store").as<string> ();
            if (_store_string.find("orbitals") != std::string::npos) _store_orbitals = true;
            if (_store_string.find("qppert") != std::string::npos) _store_qppert = true;
            if (_store_string.find("qpdiag") != std::string::npos) _store_qpdiag = true;
            if (_store_string.find("singlets") != std::string::npos) _store_singlets = true;
            if (_store_string.find("triplets") != std::string::npos) _store_triplets = true;
    
            load_property_from_xml( _package_options, _package_xml.c_str() );    
            key = "package";
            _package = _package_options.get(key+".name").as<string> ();


   
            // only required, if GWBSE is to be run
            if ( _do_gwbse_input || _do_gwbse_run || _do_gwbse_parse ){
                key = "options." + Identify();
                string _gwpackage_xml = options->get(key+".gwpackage").as<string> ();
                load_property_from_xml( _gwpackage_options, _gwpackage_xml.c_str() );  
                key = "package";
                _gwpackage = _gwpackage_options.get(key+".name").as<string> ();
            }
    
    
            // register all QM packages (Gaussian, turbomole, nwchem))
            QMPackageFactory::RegisterAll(); */
            cout << "I'm supposed to initialize GWBSE";

        }

        Job::JobResult GWBSE::EvalJob(Topology *top, Job *job, QMThread *opThread) {

            cout << "Starting GW-BSE";
            Orbitals _orbitals;
            Job::JobResult jres = Job::JobResult();
            Property _job_input = job->getInput();
            list<Property*> lSegments = _job_input.Select("segment");

            vector < Segment* > segments;
            int segId = lSegments.front()->getAttribute<int>("id");
            string segType = lSegments.front()->getAttribute<string>("type");

            Segment *seg = top->getSegment(segId);
            assert(seg->Name() == segType);
            segments.push_back(seg);

            Logger* pLog = opThread->getLogger();
            LOG(logINFO, *pLog) << TimeStamp() << " Evaluating site " << seg->getId() << flush;


	    // iteration timing tests
	    /*
	    // setup dummies
	    ub::vector< ub::matrix<float> > _array;
	    _array.resize( 102 );
	    for( int i=0 ; i<102; i++){
	      _array(i) = ub::matrix<float>(642, 516);
	      for ( int j=0; j<642; j++){
		for ( int k=0; k<516; k++){
		  _array(i)(j,k) = float(i +j +k);
		}
	      }
	    }


	    ub::vector<double> _energy_dummy;
	    _energy_dummy.resize(516);
	    ub::vector<double> _qp_energy_dummy;
	    _qp_energy_dummy.resize(516);
	    for ( int i =0 ; i< 516; i++){
	      _energy_dummy(i) = double(i);
	    }

	    ub::vector<double> _freq (642,1.5);



            // iterative refinement of qp energies
            int _max_iter = 1;
            int _bandsum = _array(0).size2(); // total number of bands
            int _gwsize  = _array(0).size1(); // size of the GW basis
            const double pi = boost::math::constants::pi<double>();
            
            LOG(logDEBUG, *pLog) << TimeStamp() << " Timing " << endl;
	    


            // initial _qp_energies are dft energies
            _qp_energy_dummy = _energy_dummy;

	    // initialize sigma to zero at the beginning of each iteration
	    ub::matrix<double> _sigma = ub::zero_matrix<double>(102,102);

	    // row index GW levels in Sigma
for ( int _m = 0 ; _m < 102 ; _m++ ){	    
	      // col index GW levels in Sigma
	      for (int _gw_level = 0; _gw_level < 102 ; _gw_level++ ){
		ub::matrix<float>& _matrix2 = _array( _gw_level );

		//		ub::matrix<double> _prodmat; // = ub::element_prod( _matrix1, _matrix2 );

		// linalg_element_prod( _matrix1, _matrix2, _prodmat);



		// loop over all functions in GW basis set
		for ( int _i_gw = 0; _i_gw < 642 ; _i_gw++ ){


		  // loop over all levels used in screening
		  for ( int _i = 0; _i < _bandsum ; _i++ ){
                    
		    double occ = 1.0;
		    if ( _i > 51 ) occ = -1.0; // sign for empty levels
                            
		    // energy denominator
		    double _denom = _qp_energy_dummy( _gw_level ) - _qp_energy_dummy( _i ) + occ*_freq( _i_gw );
                            
		    double _stab = 1.0;
		    if ( std::abs(_denom) < 0.5 ) {
		      _stab = 0.5 * ( 1.0 - cos(2.0 * pi * std::abs(_denom) ) );
		    }
                            
		    double  _factor = _matrix2(_i_gw, _i) * _freq( _i_gw ) * _freq( _i_gw) * _stab/_denom; // contains conversion factor 2!

  //ub::matrix<float>& _matrix1 = _array( _m ); 
	      //

  _sigma( _m , _gw_level ) += _factor * _array(_m)(_i_gw, _i);// * _matrix2(_i_gw,_i);

		    //		    _sigma( _m , _gw_level ) += _factor * _matrix1(_i_gw, _i) * _matrix2(_i_gw,_i);
		  } // screening levels
                  
		}// GW basis functions
                        
	      }// col GW levels
                    
	    }// row GW levels
    

            
 

            LOG(logDEBUG, *pLog) << TimeStamp() << " Timing " << endl;

	    exit(0);
	    */
	    







            // load the DFT data 
            string orb_file = (format("%1%_%2%%3%") % "molecule" % segId % ".orb").str();
            string frame_dir = "frame_" + boost::lexical_cast<string>(top->getDatabaseId());
            string edft_work_dir = "OR_FILES";
            string DIR = edft_work_dir + "/molecules_gwbse/" + frame_dir;
            std::ifstream ifs((DIR + "/" + orb_file).c_str());
            LOG(logDEBUG, *pLog) << TimeStamp() << " Loading DFT data from " << DIR << "/" << orb_file << flush;
            boost::archive::binary_iarchive ia(ifs);
            ia >> _orbitals;
            ifs.close();
            string _dft_package = _orbitals.getQMpackage();
            LOG(logDEBUG, *pLog) << TimeStamp() << " DFT data was created by " << _dft_package << flush;

            
            
            // reorder DFT data, load DFT basis set
            BasisSet dftbs;
            string dftbasis_name("ubecppol");

            AOBasis dftbasis;

            dftbs.LoadBasisSet(dftbasis_name);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Loaded DFT Basis Set " << dftbasis_name << flush;

            dftbasis.AOBasisFill(&dftbs, segments);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Filled DFT Basis of size " << dftbasis._AOBasisSize << flush;

            // set gw band range indices (-1)  => NEEDS TO FO TO OPTIONS
            gwmin = 0;
            gwmax = 2 * _orbitals.getNumberOfElectrons() -1;
            gwtotal = gwmax - gwmin +1 ;
            homo    = _orbitals.getNumberOfElectrons() - 1;

            // process the DFT data
            // a) form the expectation value of the XC functional in MOs
            ub::matrix<double> _dft_orbitals = *_orbitals.getOrbitals();
            // we have to do some cartesian -> spherical transformation for Gaussian
            ub::matrix<double> _vxc_ao;
            if ( _dft_package == "gaussian" ){

                ub::matrix<double> vxc_cart = (*_orbitals.getVxc()); 
                ub::matrix<double> _carttrafo;
                dftbasis.getTransformationCartToSpherical(_dft_package , _carttrafo );
                ub::matrix<double> _temp = ub::prod( _carttrafo, vxc_cart  );
                _vxc_ao = ub::prod( _temp, ub::trans( _carttrafo) );

            } else {
                _vxc_ao = (*_orbitals.getVxc()); 
            }

            // now get expectation values but only for those in gwmin:gwmax range
            ub::matrix<double> _mos = ub::project( _dft_orbitals ,  ub::range( gwmin , gwmax +1 ), ub::range(0, dftbasis._AOBasisSize ) );
            ub::matrix<double> _temp  = ub::prod( _vxc_ao, ub::trans( _mos ) ) ;
            _vxc  = ub::prod( _mos  , _temp );
            _vxc = 2.0 * _vxc;
            LOG(logDEBUG, *pLog) << TimeStamp() << " Calculated exchange-correlation expectation values " << flush;
            

            
            // b) reorder MO coefficients depending on the QM package used to obtain the DFT data
            if (_dft_package != "votca") {
                // get reordering vector _dft_package -> Votca 
                vector<int> neworder;
                dftbasis.getReorderVector(_dft_package, neworder);
                // and reorder rows of _orbitals->_mo_coefficients() accordingly
                AOBasis::ReorderMOs(_dft_orbitals, neworder);
                // NWChem inverted sign for xz d-orbital
                if (_dft_package == "nwchem") {
                    // get vector with multipliers, e.g. NWChem -> Votca (bloody sign for d_xz)
                    vector<int> multiplier;
                    dftbasis.getMultiplierVector(_dft_package, multiplier);
                    // and reorder rows of _orbitals->_mo_coefficients() accordingly
                    AOBasis::MultiplyMOs(_dft_orbitals, multiplier);
                }
            }
            LOG(logDEBUG, *pLog) << TimeStamp() << " Converted DFT orbital coefficient order " << flush;

            // setting up ao_overlap_matrix
            list<string> elements;
            BasisSet gwbs;
            string gwbasis_name("gwdefault");

            AOBasis gwbasis;
            bool PPM_symmetric = true; // only PPM supported


            gwbs.LoadBasisSet(gwbasis_name);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Loaded GW Basis Set " << gwbasis_name << flush;

            gwbasis.AOBasisFill(&gwbs, segments);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Filled GW Basis of size " << gwbasis._AOBasisSize << flush;

            // get overlap matrix as AOOverlap
            AOOverlap _gwoverlap;
            // initialize overlap matrix
            _gwoverlap.Initialize(gwbasis._AOBasisSize);
            // Fill overlap
            _gwoverlap.Fill(&gwbasis);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Filled GW Overlap matrix of dimension: " << _gwoverlap._aomatrix.size1() << flush;
            // _aooverlap.Print( "S" );

            // printing some debug info
            // _gwcoulomb.PrintIndexToFunction( &aobasis );

            // check eigenvalues of overlap matrix
            ub::vector<double> _eigenvalues;
            ub::matrix<double> _eigenvectors;
            _eigenvalues.resize(_gwoverlap._aomatrix.size1());
            _eigenvectors.resize(_gwoverlap._aomatrix.size1(), _gwoverlap._aomatrix.size1());
            linalg_eigenvalues(_gwoverlap._aomatrix, _eigenvalues, _eigenvectors);
            // cout << _eigenvalues << endl;
            sort(_eigenvalues.begin(), _eigenvalues.end());
            LOG(logDEBUG, *pLog) << TimeStamp() << " Smallest eigenvalue of GW Overlap matrix : " << _eigenvalues[0] << flush;



            // get Coulomb matrix as AOCoulomb
            AOCoulomb _gwcoulomb;
            // initialize Coulomb matrix
            _gwcoulomb.Initialize(gwbasis._AOBasisSize);
            // Fill Coulomb matrix
            _gwcoulomb.Fill(&gwbasis);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Filled GW Coulomb matrix of dimension: " << _gwcoulomb._aomatrix.size1() << flush;
            //_gwcoulomb.Print( "COU_in " );


            // PPM is symmetric, so we need to get the sqrt of the Coulomb matrix
            AOOverlap _gwoverlap_inverse;               // will also be needed in PPM itself
            AOOverlap _gwoverlap_cholesky_inverse;      // will also be needed in PPM itself
            if (PPM_symmetric) {
                _gwoverlap_inverse.Initialize( gwbasis._AOBasisSize);
                _gwcoulomb.Symmetrize(  _gwoverlap , gwbasis, _gwoverlap_inverse , _gwoverlap_cholesky_inverse );
            }
            LOG(logDEBUG, *pLog) << TimeStamp() << " Prepared GW Coulomb matrix for symmetric PPM " << flush;

            // calculate 3-center integrals,  convoluted with DFT eigenvectors

            // --- prepare a vector (gwdacay) of matrices (orbitals, orbitals) as container => M_mn
            mmin = 1; // lowest index occ 
            mmax = 2 * _orbitals.getNumberOfElectrons();
            nmin = 1;
            nmax = _orbitals.getNumberOfLevels();
            maxf = gwbasis.getMaxFunctions(); // maximum number of functions per shell in basis set
            mtotal = mmax - mmin + 1;
            ntotal = nmax - nmin + 1;


            // prepare 3-center integral object
            TCMatrix _Mmn;
            _Mmn.Initialize(gwbasis._AOBasisSize, mmin, mmax, nmin, nmax);
            _Mmn.Fill(gwbasis, dftbasis, _dft_orbitals);
            LOG(logDEBUG, *pLog) << TimeStamp() << " Calculated Mmn_beta (3-center-overlap x orbitals)  " << flush;
            //_Mmn.Print( "Mmn " );
            
            
            
            // for use in RPA, make a copy of _Mmn with dimensions (1:HOMO)(gwabasissize,LUMO:nmax)
            TCMatrix _Mmn_RPA;
            _Mmn_RPA.Initialize(gwbasis._AOBasisSize, mmin, _orbitals.getNumberOfElectrons() , _orbitals.getNumberOfElectrons() +1 , nmax);
            RPA_prepare_threecenters( _Mmn_RPA, _Mmn, gwbasis, _gwoverlap, _gwoverlap_inverse );
            

            // TODO: now, we can get rid of _gwoverlap_inverse
            // make _Mmn_RPA symmetric for use in RPA
            _Mmn_RPA.Symmetrize( _gwcoulomb._aomatrix  );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Prepared Mmn_beta for RPA  " << flush;
            // _Mmn_RPA.Print( "Mmn_RPA" );
            
            // make _Mmn symmetric for use in self-energy calculation
            _Mmn.Symmetrize( _gwcoulomb._aomatrix  );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Prepared Mmn_beta for self-energy  " << flush;


            // some parameters that need to be prepared by options parsing, here fixed for testing
            _shift = 0.3; // in Rydberg
            _screening_freq = ub::zero_matrix<double>(2,2); // two frequencies
            //first one
            _screening_freq(0,0) = 0.0; // real part
            _screening_freq(0,1) = 0.0; // imaginary part
            //second one
            _screening_freq(1,0) = 0.0; // real part
            _screening_freq(1,1) = 1.0; // imaginary part

            ub::vector<double> _dft_energies = 2.0*(*_orbitals.getEnergies()); // getEnergies -> Hartree, we want Ryd
            
            // one entry to epsilon for each frequency
            _epsilon.resize( _screening_freq.size1() );
            
            // for symmetric PPM, we can initialize _epsilon with the overlap matrix!
            for ( int _i_freq = 0 ; _i_freq < _screening_freq.size1() ; _i_freq++ ){
                _epsilon( _i_freq ) = _gwoverlap._aomatrix ;
            }
            // TODO: now, we can get rid of _gwoverlap
            
            // determine epsilon from RPA
            RPA_calculate_epsilon( _Mmn_RPA, _screening_freq, _shift, _dft_energies );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Calculated epsilon via RPA  " << flush;
  
            // construct PPM parameters
            PPM_construct_parameters( _gwoverlap_cholesky_inverse._aomatrix );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Constructed PPM parameters  " << flush;
            
            // prepare threecenters for Sigma
            sigma_prepare_threecenters(  _Mmn );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Prepared threecenters for sigma  " << flush;

            // set gw band range indices (-1)  => NEEDS TO FO TO OPTIONS
            gwmin = 0;
            gwmax = 2 * _orbitals.getNumberOfElectrons() -1;
            gwtotal = gwmax - gwmin +1 ;
            homo    = _orbitals.getNumberOfElectrons() - 1;
            
            // calculate exchange part of sigma
            sigma_x_setup( _Mmn );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Calculated exchange part of Sigma  " << flush;
            
            // TOCHECK: get rid of _ppm_phi?
  
            // ub::vector_range< ub::vector<double> > _edft = ub::subrange( _dft_energies, gwmin, gwmax +1 );
            
            // calculate correlation part of sigma
            sigma_c_setup( _Mmn, _dft_energies   );
            LOG(logDEBUG, *pLog) << TimeStamp() << " Calculated correlation part of Sigma  " << flush;

            /* One could also save the qp_energies directly to the orbitals object...
                       // now copy energies in map into orbitals object matrix
            (_orbitals->_QPpert_energies).resize( _levels, 5 );
            for (size_t itl=0; itl < _levels; itl++){
               for (size_t ite=0; ite<5; ite++) {
                   _orbitals->_QPpert_energies(itl,ite) = _energies[itl+1][ite];
               }
            } 
             */
            
            
            
            // Output of quasiparticle energies after all is done:
            pLog->setPreface(logINFO, "\n");
            
            LOG(logINFO,*pLog) << (format("  ====== Perturbative quasiparticle energies (Rydberg) ====== ")).str() << flush;
                        
            for ( int _i = 0 ; _i < gwtotal ; _i++ ){
                if ( (_i + gwmin) == homo ){
                    LOG(logINFO,*pLog) << (format("  HOMO  = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = %4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") % (_i+gwmin+1) % _dft_energies( _i + gwmin ) % _vxc(_i,_i) % _sigma_x(_i,_i) % _sigma_c(_i,_i) % _qp_energies(_i) ).str() << flush;
                } else if ( (_i + gwmin) == homo+1 ){
                    LOG(logINFO,*pLog) << (format("  LUMO  = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = %4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") % (_i+gwmin+1) % _dft_energies( _i + gwmin ) % _vxc(_i,_i) % _sigma_x(_i,_i) % _sigma_c(_i,_i) % _qp_energies(_i) ).str() << flush;                    
                    
                }else {
                LOG(logINFO,*pLog) << (format("  Level = %1$4d DFT = %2$+1.4f VXC = %3$+1.4f S-X = %4$+1.4f S-C = %5$+1.4f GWA = %6$+1.4f") % (_i+gwmin+1) % _dft_energies( _i + gwmin ) % _vxc(_i,_i) % _sigma_x(_i,_i) % _sigma_c(_i,_i) % _qp_energies(_i) ).str() << flush;
                }
            }

            // constructing full quasiparticle Hamiltonian and diagonalize, if requested
            FullQPHamiltonian();
            LOG(logDEBUG, *pLog) << TimeStamp() << " Full quasiparticle Hamiltonian  " << flush;
            LOG(logINFO, *pLog) << (format("  ====== Diagonalized quasiparticle energies (Rydberg) ====== ")).str() << flush;
            for (int _i = 0; _i < gwtotal; _i++) {
                if ((_i + gwmin) == homo) {
                    LOG(logINFO, *pLog) << (format("  HOMO  = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") % (_i + gwmin + 1) % _qp_energies(_i) % _qp_diag_energies(_i)).str() << flush;
                } else if ((_i + gwmin) == homo + 1) {
                    LOG(logINFO, *pLog) << (format("  LUMO  = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") % (_i + gwmin + 1) % _qp_energies(_i) % _qp_diag_energies(_i)).str() << flush;

                } else {
                    LOG(logINFO, *pLog) << (format("  Level = %1$4d PQP = %2$+1.4f DQP = %3$+1.4f ") % (_i + gwmin + 1) % _qp_energies(_i) % _qp_diag_energies(_i)).str() << flush;
                }
            }
            
            
            // constructing electron-hole interaction for BSE
            
            // set BSE band range indices (-1)  => NEEDS TO GO TO OPTIONS
            bse_vmin   = 0;
            bse_vmax   = homo;
            bse_vtotal = bse_vmax - bse_vmin +1 ;
            bse_cmin   = homo+1;
            bse_cmax   = gwmax;
            bse_ctotal = bse_cmax - bse_cmin +1 ;
            bse_size   = bse_vtotal * bse_ctotal;
            
            // calculate exchange part of eh interaction
            BSE_x_setup(  _Mmn );
            LOG(logINFO,*pLog) << TimeStamp() << " Exchange part of e-h interaction " << flush; 

            // calculate direct part of eh interaction
            BSE_d_setup ( _Mmn );
            LOG(logINFO,*pLog) << TimeStamp() << " Direct part of e-h interaction " << flush; 
            
            BSE_solve_triplets();
            LOG(logINFO,*pLog) << TimeStamp() << " Solved BSE for triplets " << flush; 
            LOG(logINFO, *pLog) << (format("  ====== 10 lowest triplet energies (eV) ====== ")).str() << flush;
            for (int _i = 0; _i < 10; _i++) {
                
                //cout << "\n" <<  _i << "  :   " << 13.605 * _bse_triplet_energies( _i ) << flush ;
                LOG(logINFO, *pLog) << (format("  T = %1$4d Omega = %2$+1.4f ") % (_i + 1) % (13.605* _bse_triplet_energies( _i ))).str() << flush;
            }

            
            BSE_solve_singlets();
            LOG(logINFO,*pLog) << TimeStamp() << " Solved BSE for singlets " << flush; 
            LOG(logINFO, *pLog) << (format("  ====== 10 lowest singlet energies (eV) ====== ")).str() << flush;
            for (int _i = 0; _i < 10; _i++) {
                       //   cout << "\n" <<  _i << "  :   " << 13.605 * _bse_singlet_energies( _i ) << flush ;
                LOG(logINFO, *pLog) << (format("  S = %1$4d Omega = %2$+1.4f ") % (_i + 1) % (13.605 * _bse_singlet_energies( _i ))).str() << flush;
            }
            
            
            
            
            LOG(logINFO,*pLog) << TimeStamp() << " Finished evaluating site " << seg->getId() << flush; 
 
            Property _job_summary;
            Property *_output_summary = &_job_summary.add("output","");
            Property *_segment_summary = &_output_summary->add("segment","");
            string segName = seg->getName();
            segId = seg->getId();
            _segment_summary->setAttribute("id", segId);
            _segment_summary->setAttribute("type", segName);
            // output of the JOB 
            jres.setOutput( _job_summary );
            jres.setStatus(Job::COMPLETE);

            // dump the LOG
            cout << *pLog;
            return jres;
        }

        
             
        void GWBSE::BSE_solve_triplets(){
            
            ub::matrix<double> _bse = -_eh_d;
            
            // add full QP Hamiltonian contributions to free transitions
            for ( size_t _v1 = 0 ; _v1 < bse_vtotal ; _v1++){
                for ( size_t _c1 = 0 ; _c1 < bse_ctotal ; _c1++){
                    size_t _index_vc = bse_ctotal * _v1 + _c1;

                    // diagonal
                    _bse( _index_vc , _index_vc ) += _vxc(_c1 + bse_cmin ,_c1 + bse_cmin ) - _vxc(_v1,_v1);

                    // v->c
                    for ( size_t _c2 = 0 ; _c2 < bse_ctotal ; _c2++){
                        size_t _index_vc2 = bse_ctotal * _v1 + _c2;
                        if ( _c1 != _c2 ){
                            _bse( _index_vc , _index_vc2 ) += _vxc(_c1+ bse_cmin ,_c2 + bse_cmin );
                        }
                    }
                    
                    // c-> v
                    for ( size_t _v2 = 0 ; _v2 < bse_vtotal ; _v2++){
                        size_t _index_vc2 = bse_ctotal * _v2 + _c1;
                        if ( _v1 != _v2 ){
                            _bse( _index_vc , _index_vc2 ) -= _vxc(_v1,_v2);
                        }
                    }
                    
                    
                }
            }
            
            
            
            _bse_triplet_energies.resize(_bse.size1());
            _bse_triplet_coefficients.resize(_bse.size1(), _bse.size1());
            linalg_eigenvalues(_bse, _bse_triplet_energies, _bse_triplet_coefficients);
            
            
        }
        
        void GWBSE::BSE_solve_singlets(){
            
            ub::matrix<double> _bse = -_eh_d + 2.0 * _eh_x;
                      // add full QP Hamiltonian contributions to free transitions
            for ( size_t _v1 = 0 ; _v1 < bse_vtotal ; _v1++){
                for ( size_t _c1 = 0 ; _c1 < bse_ctotal ; _c1++){
                    size_t _index_vc = bse_ctotal * _v1 + _c1;

                    // diagonal
                    _bse( _index_vc , _index_vc ) += _vxc(_c1 + bse_cmin ,_c1 + bse_cmin) - _vxc(_v1,_v1);

                    // v->c
                    for ( size_t _c2 = 0 ; _c2 < bse_ctotal ; _c2++){
                        size_t _index_vc2 = bse_ctotal * _v1 + _c2;
                        if ( _c1 != _c2 ){
                            _bse( _index_vc , _index_vc2 ) += _vxc(_c1 + bse_cmin ,_c2 + bse_cmin);
                        }
                    }
                    
                    // c-> v
                    for ( size_t _v2 = 0 ; _v2 < bse_vtotal ; _v2++){
                        size_t _index_vc2 = bse_ctotal * _v2 + _c1;
                        if ( _v1 != _v2 ){
                            _bse( _index_vc , _index_vc2 ) -= _vxc(_v1,_v2);
                        }
                    }
                    
                    
                }
            }
            
            
            
            
            
            
            _bse_singlet_energies.resize(_bse.size1());
            _bse_singlet_coefficients.resize(_bse.size1(), _bse.size1());
            linalg_eigenvalues(_bse, _bse_singlet_energies, _bse_singlet_coefficients);
            
            
        }
        
        
        void GWBSE::BSE_d_setup (  TCMatrix& _Mmn){
            // gwbasis size
            size_t _gwsize = _Mmn.matrix()(0).size1();

            // messy procedure, first get two matrices for occ and empty subbparts
            // store occs directly transposed
            ub::matrix<double> _storage_v = ub::zero_matrix<double>(  bse_vtotal * bse_vtotal , _gwsize );
            for ( size_t _v1 = 0; _v1 < bse_vtotal; _v1++){
                for ( size_t _v2 = 0; _v2 < bse_vtotal; _v2++){
                    size_t _index_vv = bse_vtotal * _v1 + _v2;
                    for ( size_t _i_gw = 0 ; _i_gw < _gwsize ; _i_gw++) {
                        _storage_v( _index_vv , _i_gw ) = _Mmn.matrix()( _v1 )( _i_gw , _v2  );
                    }
                }
            }
            
            
            ub::matrix<double> _storage_c = ub::zero_matrix<double>( _gwsize, bse_ctotal * bse_ctotal );
            for ( size_t _c1 = 0; _c1 < bse_ctotal; _c1++){
                for ( size_t _c2 = 0; _c2 < bse_ctotal; _c2++){
                    size_t _index_cc = bse_ctotal * _c1 + _c2;
                    for ( size_t _i_gw = 0 ; _i_gw < _gwsize ; _i_gw++) {
                        _storage_c( _i_gw , _index_cc ) = _Mmn.matrix()( _c1 + bse_cmin )( _i_gw , _c2 + bse_cmin );
                    }
                }
            }
            
            // store elements in a vtotal^2 x ctotal^2 matrix
            ub::matrix<double> _storage_prod = ub::prod( _storage_v , _storage_c );
            
            
            
            // now patch up _storage for screened interaction
            for ( size_t _i_gw = 0 ; _i_gw < _gwsize ; _i_gw++ ){  
                double _ppm_factor = sqrt( _ppm_weight( _i_gw ));
                for ( size_t _v = 0 ; _v < (bse_vtotal* bse_vtotal) ; _v++){
                    _storage_v( _v , _i_gw ) = _ppm_factor * _storage_v(_v , _i_gw );
                }
                for ( size_t _c = 0 ; _c < (bse_ctotal*bse_ctotal) ; _c++){
                    _storage_c( _i_gw , _c ) = _ppm_factor * _storage_c( _i_gw , _c  );
                }
            }
            
            // multiply and subtract from _storage_prod
            _storage_prod -= ub::prod( _storage_v , _storage_c );
            
            
            // finally resort into _eh_d and multiply by to for Rydbergs
            _eh_d = ub::zero_matrix<double>( bse_size , bse_size );
            for ( size_t _v1 = 0 ; _v1 < bse_vtotal ; _v1++){
                for ( size_t _v2 = 0 ; _v2 < bse_vtotal ; _v2++){
                    size_t _index_vv = bse_vtotal * _v1 + _v2;
                    
                    for ( size_t _c1 = 0 ; _c1 < bse_ctotal ; _c1++){
                        size_t _index_vc1 = bse_ctotal * _v1 + _c1 ;
                              
                        
                        for ( size_t _c2 = 0 ; _c2 < bse_ctotal ; _c2++){
                            size_t _index_vc2 = bse_ctotal * _v2 + _c2 ;
                            size_t _index_cc  = bse_ctotal * _c1 + _c2;

                            _eh_d( _index_vc1 , _index_vc2 ) = 2.0 * _storage_prod( _index_vv , _index_cc ); 

                            
                        }
                    }
                }
            }
            
           /*  
           // test output
           for ( size_t _v1 = 0; _v1 < bse_vtotal ; _v1++ ){
                // empty levels
                for (size_t _c1 =0 ; _c1 < bse_ctotal ; _c1++ ){
                     size_t _index_vc1 = bse_ctotal * _v1 + _c1;

            for ( size_t _v2 = 0; _v2 < bse_vtotal ; _v2++ ){
                // empty levels
                for (size_t _c2 =0 ; _c2 < bse_ctotal ; _c2++ ){
                     size_t _index_vc2 = bse_ctotal * _v2 + _c2 ;


                     cout << " EH-d: " << _index_vc1 << " : " << _index_vc2 << " : " << _v1 << " : " << _c1+bse_cmin << " : " << _v2 << " : " << _c2+bse_cmin << " = " << _eh_d(_index_vc1, _index_vc2 ) << endl;
                }

            }
                }
            
            } */
            
            
            
        }
        
        
        
        void GWBSE::BSE_x_setup(TCMatrix& _Mmn){
            
            /* unlike the fortran code, we store eh interaction directly in
             * a suitable matrix form instead of a four-index array
             */
            
            // allocate eh_x
            _eh_x = ub::zero_matrix<double>( bse_size , bse_size );
            
            // gwbasis size
            size_t _gwsize = _Mmn.matrix()(0).size1();
            
            // get a different storage for 3-center integrals we need
            ub::matrix<double> _storage = ub::zero_matrix<double>( _gwsize , bse_size);
            // occupied levels
            for ( size_t _v = 0; _v < bse_vtotal ; _v++ ){
                // empty levels
                for (size_t _c =0 ; _c < bse_ctotal ; _c++ ){
                    size_t _index_vc = bse_ctotal * _v + _c ;
                    for (size_t _i_gw = 0 ; _i_gw < _gwsize ; _i_gw++ ){
                        _storage( _i_gw, _index_vc ) = _Mmn.matrix()( _v )( _i_gw, _c + bse_cmin);
                    }
                }
            }
            
            // with this storage, _eh_x is obtained by matrix multiplication
            _eh_x = ub::prod( ub::trans( _storage ), _storage );
            _eh_x = 2.0 * _eh_x; // Rydberg
  
            
        }
        
        
        

        void GWBSE::FullQPHamiltonian(){
            
            // constructing full QP Hamiltonian, storage in vxc
            _vxc = -_vxc + _sigma_x + _sigma_c;
            // diagonal elements are given by _qp_energies
            for (int _m = 0; _m < _vxc.size1(); _m++ ){
              _vxc( _m,_m ) = _qp_energies( _m );
            }
                    
            /* diagonalize, since _vxc will be needed in BSE, and GSL
             * destroys the input array, we need to make a local copy first
             */
            
            // get eigenvalues and eigenvectors of this matrix
            ub::matrix<double> _temp = _vxc;
            _qp_diag_energies.resize(_temp.size1());
            _qp_diag_coefficients.resize(_temp.size1(), _temp.size1());
            linalg_eigenvalues(_temp, _qp_diag_energies, _qp_diag_coefficients);

            // TODO storage -> orbitals


            
        }
        
        
        void GWBSE::sigma_c_setup(TCMatrix& _Mmn, ub::vector<double>& _edft){
            
            // iterative refinement of qp energies
            int _max_iter = 5;
            int _bandsum = _Mmn.matrix()(0).size2(); // total number of bands
            int _gwsize  = _Mmn.matrix()(0).size1(); // size of the GW basis
            const double pi = boost::math::constants::pi<double>();
            

	    // get ONE reference to three center matrices
	    //ub::vector< ub::matrix<double> >& _Mmn_matrix = _Mmn.matrix();


            // initial _qp_energies are dft energies
            _qp_energies = _edft;

	    // only diagonal elements except for in final iteration
            for ( int _i_iter = 0 ; _i_iter < _max_iter-1 ; _i_iter++ ){
                
	      // initialize sigma_c to zero at the beginning of each iteration
	      _sigma_c = ub::zero_matrix<double>(gwtotal,gwtotal);

	      // loop over all GW levels
	      for (int _gw_level = 0; _gw_level < gwtotal ; _gw_level++ ){
              
		// loop over all functions in GW basis
		for ( int _i_gw = 0; _i_gw < _gwsize ; _i_gw++ ){
                    
		  // loop over all bands
		  for ( int _i = 0; _i < _bandsum ; _i++ ){
                    
		    double occ = 1.0;
		    if ( _i > homo ) occ = -1.0; // sign for empty levels
                                                    
		    // energy denominator
		    double _denom = _qp_energies( _gw_level ) - _qp_energies( _i ) + occ*_ppm_freq( _i_gw );
                    
		    double _stab = 1.0;
		    if ( std::abs(_denom) < 0.5 ) {
		      _stab = 0.5 * ( 1.0 - cos(2.0 * pi * std::abs(_denom) ) );
		    }
                            
		    double _factor = _ppm_weight( _i_gw ) * _ppm_freq( _i_gw) * _stab/_denom; // contains conversion factor 2!
		    
		    // sigma_c diagonal elements
		    _sigma_c( _gw_level , _gw_level ) += _factor * _Mmn.matrix()( _gw_level )(_i_gw, _i) *  _Mmn.matrix()( _gw_level )(_i_gw, _i);  
                            
		  }// bands
                        
		}// GW functions
		
		// update _qp_energies
		_qp_energies( _gw_level ) = _edft( _gw_level ) + _sigma_x(_gw_level, _gw_level) + _sigma_c(_gw_level,_gw_level) - _vxc(_gw_level,_gw_level);
                    
	      }// all bands
                
            } // iterations
            

	    // in final step, also calc offdiagonal elements
	    // initialize sigma_c to zero at the beginning of each iteration
	    _sigma_c = ub::zero_matrix<double>(gwtotal,gwtotal);

	      // loop over col  GW levels
	      for (int _gw_level = 0; _gw_level < gwtotal ; _gw_level++ ){
              
		// loop over all functions in GW basis
		for ( int _i_gw = 0; _i_gw < _gwsize ; _i_gw++ ){
                    

		  // loop over all bands
		  for ( int _i = 0; _i < _bandsum ; _i++ ){
                    
		    double occ = 1.0;
		    if ( _i > homo ) occ = -1.0; // sign for empty levels
                    
		    // energy denominator
		    double _denom = _qp_energies( _gw_level ) - _qp_energies( _i ) + occ*_ppm_freq( _i_gw );
                    
		    double _stab = 1.0;
		    if ( std::abs(_denom) < 0.5 ) {
		      _stab = 0.5 * ( 1.0 - cos(2.0 * pi * std::abs(_denom) ) );
		    }
                    
		    double _factor = _ppm_weight( _i_gw ) * _ppm_freq( _i_gw) *   _Mmn.matrix()( _gw_level )(_i_gw, _i) *_stab/_denom; // contains conversion factor 2!
		    
		    // loop over row GW levels
		    for ( int _m = 0 ; _m < gwtotal ; _m++) {

		    
		    // sigma_c all elements
		    _sigma_c( _m , _gw_level ) += _factor * _Mmn.matrix()( _m )(_i_gw, _i) ;  //_submat(_i_gw,_i);
	                      
		  }// bands
		}// GW functions
	      }// GW col 
	      _qp_energies( _gw_level ) = _edft( _gw_level ) + _sigma_x(_gw_level,_gw_level) + _sigma_c(_gw_level,_gw_level) - _vxc(_gw_level,_gw_level);
	    } // GW row
    
        } // sigma_c_setup

        void GWBSE::sigma_x_setup(TCMatrix& _Mmn){
        
            // initialize sigma_x
            _sigma_x = ub::zero_matrix<double>(gwtotal,gwtotal);
            int _size  = _Mmn.matrix()(0).size1();

            // band 1 loop over all GW bands
            for ( int _m1 = 0 ; _m1 < gwtotal ; _m1++ ){
                // band 2 loop over all GW bands
                for ( int _m2 = 0 ; _m2 < gwtotal ; _m2++ ){
                    // loop over all basis functions
                    for ( int _i_gw = 0 ; _i_gw < _size ; _i_gw++ ){
                        // loop over all occupied bands used in screening
                        for ( int _i_occ = 0 ; _i_occ <= homo ; _i_occ++ ){
                            
                            _sigma_x( _m1, _m2 ) -= 2.0 * _Mmn.matrix()( _m1 )( _i_gw , _i_occ ) * _Mmn.matrix()( _m2 )( _i_gw , _i_occ );
                            
                        } // occupied bands
                    } // gwbasis functions
                } // band 2
            } // band 1
        
        }



        

        void GWBSE::sigma_prepare_threecenters(TCMatrix& _Mmn){
    
            for ( int _m_band = 0 ; _m_band < _Mmn.get_mtot(); _m_band++ ){
                // get Mmn for this _m_band
                // ub::matrix<double> _temp = ub::trans(  _Mmn.matrix()( _m_band )   );
                // and multiply with _ppm_phi = eigenvectors of epsilon
                _Mmn.matrix()( _m_band ) = ub::prod(  _ppm_phi , _Mmn.matrix()( _m_band ) );
                
            }
            

        }        
        
        void GWBSE::PPM_construct_parameters(  ub::matrix<double>& _overlap_cholesky_inverse ){
            
            // multiply with L-1^t from the right
            ub::matrix<double> _overlap_cholesky_inverse_transposed = ub::trans( _overlap_cholesky_inverse );
            ub::matrix<double> _temp = ub::prod( _epsilon(0) , _overlap_cholesky_inverse_transposed );
            // multiply with L-1 from the left
            _temp = ub::prod( _overlap_cholesky_inverse, _temp );
            
            // get eigenvalues and eigenvectors of this matrix
            ub::vector<double> _eigenvalues;
            ub::matrix<double> _eigenvectors;
            _eigenvalues.resize(_temp.size1());
            _eigenvectors.resize(_temp.size1(), _temp.size1());
            linalg_eigenvalues(_temp, _eigenvalues, _eigenvectors);
            
            // multiply eigenvectors with overlap_cholesky_inverse_transpose and store as eigenvalues of epsilon
            _ppm_phi = ub::prod( _overlap_cholesky_inverse_transposed , _eigenvectors ); 

 
            
            // store PPM weights from eigenvalues
            _ppm_weight.resize( _eigenvalues.size() );
            for ( int _i = 0 ; _i <  _eigenvalues.size(); _i++   ){
                _ppm_weight(_i) = 1.0 - 1.0/_eigenvalues(_i);
            }

            // determine PPM frequencies
            _ppm_freq.resize( _eigenvalues.size() );
            // a) phi^t * epsilon(1) * phi 
            _temp = ub::prod( ub::trans( _ppm_phi ) , _epsilon(1) );
            _eigenvectors  = ub::prod( _temp ,  _ppm_phi  );
            // b) invert
            _temp = ub::zero_matrix<double>( _eigenvalues.size(),_eigenvalues.size() )  ;
            linalg_invert( _eigenvectors , _temp ); //eigenvectors is destroyed after!
            // c) PPM parameters -> diagonal elements
            for ( int _i = 0 ; _i <  _eigenvalues.size(); _i++   ){
                
                double _nom  = _temp( _i, _i ) - 1.0;
                
                // only purely imaginary frequency assumed
                if ( _screening_freq(1,0) != 0.0 ) {
                    cerr << " mixed frequency! real part: " << _screening_freq( 1, 0 ) << " imaginary part: "  << _screening_freq( 1 , 1 ) << flush;
                    exit(1);
                } else {
                    
                    double _frac = -1.0 * _nom/(_nom + _ppm_weight( _i )) * _screening_freq(1,1) * _screening_freq(1,1) ;
                    _ppm_freq ( _i ) =  sqrt( std::abs(_frac )) ;

		    if ( _ppm_weight(_i) < 1.e-5 ){
		      _ppm_weight(_i) = 0.0;
		      _ppm_freq(_i)   = 1.0;

		    }

                }

            }
            
            // will be needed transposed later
            _ppm_phi = ub::trans( _ppm_phi );
            
                   
            
        }
        
        

        void GWBSE::RPA_calculate_epsilon(TCMatrix& _Mmn_RPA, ub::matrix<double> _screening_freq, double _shift, ub::vector<double>& _dft_energies){
            
            int _size = _Mmn_RPA.matrix()(0).size1(); // size of gwbasis
            
            // loop over frequencies
            for ( int _i_freq = 0 ; _i_freq < _screening_freq.size1() ; _i_freq++ ){
                // initialize epsilon for this frequency
                // _epsilon ( _i_freq ) = ub::zero_matrix<double>(_size, _size);
                
                // loop over occupied bands -> vector index of _Mmn_RPA
                for ( int _m_band = 0; _m_band < _Mmn_RPA.get_mtot() ; _m_band++ ){
                    int index_m = _Mmn_RPA.get_mmin();

                    // a temporary matrix, that will get filled in empty bands loop
                    ub::matrix<double> _temp = ub::zero_matrix<double>( _Mmn_RPA.get_ntot(), _size );
                    
                        
                    // loop over empty bands
                    for ( int _n_band = 0 ; _n_band < _Mmn_RPA.get_ntot() ; _n_band++ ){
                        int index_n = _Mmn_RPA.get_nmin();
                        
                        
                        double _deltaE = _shift + _dft_energies( _n_band + index_n ) - _dft_energies( _m_band + index_m ); // get indices and units right!!!
                        double _energy_factor;
                        // this only works, if we have either purely real or purely imaginary frequencies
                        if ( _screening_freq( _i_freq, 0) == 0.0 ) {
                            // purely imaginary
                            _energy_factor = 8.0 * _deltaE / (_deltaE*_deltaE + _screening_freq( _i_freq, 1) *  _screening_freq( _i_freq, 1 ));
                        } else if ( _screening_freq( _i_freq, 1) == 0.0  ) {
                            // purely real
                            _energy_factor = 4.0 * (1.0 / (_deltaE - _screening_freq( _i_freq, 0 ) ) +  1.0 / (_deltaE + _screening_freq( _i_freq, 0 ) ) );
                        } else {
                            // mixed -> FAIL
                            cerr << " mixed frequency! real part: " << _screening_freq( _i_freq, 0 ) << " imaginary part: "  << _screening_freq( _i_freq, 1 ) << flush;
                            exit(1);
                        }

                        for ( int _i_gw = 0 ; _i_gw < _size ; _i_gw++ ){
                            _temp( _n_band , _i_gw ) = _energy_factor * _Mmn_RPA.matrix()( _m_band )( _i_gw , _n_band );
                        } // matrix size
                        
                    } // empty bands

                   // now multiply and add to epsilon
                   _epsilon( _i_freq ) += ub::prod( _Mmn_RPA.matrix()( _m_band ) , _temp  );

                } // occupied bands
                
            } // loop over frequencies
            
            
        }
        
        
   
    
    void GWBSE::RPA_prepare_threecenters( TCMatrix& _Mmn_RPA, TCMatrix& _Mmn_full, AOBasis& gwbasis, AOMatrix& gwoverlap, AOMatrix& gwoverlap_inverse     ){
        
        // cout << "blabla" << endl;
        
        
        // loop over m-bands in _Mmn_full
        // for ( int _m_band = 0; _m_band < _Mmn_full.matrix().size() ; _m_band++ ){
        // actually, only needed for size of _Mmn_RPA (until VBM)
        for ( int _m_band = 0; _m_band < _Mmn_RPA.matrix().size() ; _m_band++ ){
        
            ub::matrix<double> _temp = ub::prod( gwoverlap_inverse._aomatrix , _Mmn_full.matrix()( _m_band ) );

            // loop over n-bands in _Mmn_full 
            for ( int _n_band = 0; _n_band < _Mmn_full.get_ntot() ; _n_band++ ){

                double sc_plus  = 0.0;
                double sc_minus = 0.0;
                
                // loop over gwbasis shells
                for (vector< AOShell* >::iterator _is = gwbasis.firstShell(); _is != gwbasis.lastShell(); _is++) {
                    AOShell* _shell = gwbasis.getShell(_is);
                    double decay = (*_shell->firstGaussian())->decay;
                    int _lmax    = _shell->getLmax();
                    int _size    = _shell->getNumFunc();
                    int _start  = _shell->getStartIndex();

                    const double pi = boost::math::constants::pi<double>();
                    double _factor = pow((2.0 *pi/decay),0.75);
                    vector<double> chi( _size, 0.0 );
                    chi[0] = _factor;

                    // some block from the fortran code that I'm not sure we need 
                    /*
                                  if ( lmax .ge. 0 ) then    
                      if(lmax .ge. 2 ) then
                       chi(10)= 6.d0*factor/sqrt(15.d0)   
                       if( lmax .ge. 4) then
                          fak = 0.25d0*factor*sqrt(beta_gwa) 
                          ! xxxx, yyyy, zzzz
                          chi(21) = fak*3.d0
                          chi(22) = chi(21)
                          chi(23) = chi(21)
                           ! xxzz, yyzz, xxyy
                           chi(30) = fak
                          chi(31) = fak
                          chi(32) = fak
                       end if
                    endif
                   end if
                     
                     */

                    // loop over all functions in shell
                    for ( int _i_gw = 0; _i_gw < _size ; _i_gw++ ){
                        double _test = _temp( _i_gw + _start, _n_band   );
                        if ( _test > 0.0  ){
                            sc_plus += chi[ _i_gw ]* _test;
                        } else if ( _test < 0.0 ){
                            sc_minus -= chi[ _i_gw ]* _test;
                        }
                    } // end loop over functions in shell

                } // end loop over all shells

                if ( _m_band <= _Mmn_RPA.get_mmax() && _n_band >= _Mmn_RPA.get_nmin()  ){
                    
                    double target = sqrt( sc_plus * sc_minus );
                    sc_plus  = target / sc_plus;
                    sc_minus = target / sc_minus;

                    // loop over gwbasis shells
                    for (vector< AOShell* >::iterator _is = gwbasis.firstShell(); _is != gwbasis.lastShell(); _is++) {
                        AOShell* _shell = gwbasis.getShell(_is);
                        int _size    = _shell->getNumFunc();
                        int _start  = _shell->getStartIndex();
                        vector<double> chi( _size, 0.0 );
                        chi[0] = 1.0;
                        // loop over all functions in shell
                        for ( int _i_gw = 0; _i_gw < _size ; _i_gw++ ){
                            double _test = _temp( _i_gw + _start, _n_band   );
                            if ( _test > 0.0 && std::abs( chi[_i_gw] ) > 1.e-10 ){
                               _temp( _i_gw + _start, _n_band   ) = _temp( _i_gw + _start, _n_band   ) * sc_plus;
                            } else if ( _test < 0.0 && std::abs( chi[_i_gw] ) > 1.e-10  ){
                               _temp( _i_gw + _start, _n_band   ) = _temp( _i_gw + _start, _n_band   ) * sc_minus;
                            }
                        } // end loop over functions in shell
                    } // end loop over all shells
                    
                }                
                
            }// loop n-bands

            // multiply _temp with overlap
            ub::matrix<double> _temp2 = ub::prod( gwoverlap._aomatrix , _temp );
            // copy to _Mmn_RPA
            // range(start, stop) contains all indices i with start <= i < stop
            _Mmn_RPA.matrix()( _m_band ) = ub::project( _temp2, ub::range(0, gwbasis._AOBasisSize) , ub::range(_Mmn_RPA.get_nmin() - _Mmn_full.get_nmin()  , _Mmn_RPA.get_nmax() - _Mmn_full.get_nmin() +1 ));
            
            
        }// loop m-bands
        
    } // end RPA_prepare_threecenters


    }
    
 
};
