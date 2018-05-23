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

#include <votca/xtp/numerical_integrations.h>
#include <votca/ctp/logger.h>
#include <votca/xtp/espfit.h>
#include <votca/xtp/aomatrix.h>

//#include <boost/progress.hpp>

#include <math.h>
#include <votca/tools/constants.h>



namespace votca { namespace xtp {

void Espfit::Fit2Density(std::vector< QMAtom* >& _atomlist,const Eigen::MatrixXd &_dmat,const AOBasis &_basis,string gridsize) {


    // setting up grid
    Grid _grid;
    _grid.setAtomlist(&_atomlist);
    _grid.setupCHELPgrid();
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() <<  " Done setting up CHELPG grid with " << _grid.getsize() << " points " << flush;

    // Calculating nuclear potential at gridpoints
    AOOverlap overlap;
    overlap.Fill(_basis);
    double N_comp=_dmat.cwiseProduct(overlap.Matrix()).sum();
   
    NumericalIntegration numway;

    numway.GridSetup(gridsize,_atomlist,&_basis);
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Setup "<<gridsize<<" Numerical Grid with "<<numway.getGridSize()<<" gridpoints."<< flush;
    double N=numway.IntegrateDensity(_dmat);
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Calculated Densities at Numerical Grid, Number of electrons is "<< N << flush;

    if(std::abs(N-N_comp)>0.001){
        CTP_LOG(ctp::logDEBUG, *_log) <<"=======================" << flush;
        CTP_LOG(ctp::logDEBUG, *_log) <<"WARNING: Calculated Densities at Numerical Grid, Number of electrons "<< N <<" is far away from the the real value "<< N_comp<<", you should increase the accuracy of the integration grid."<< flush;
        N=N_comp;
        CTP_LOG(ctp::logDEBUG, *_log) <<"WARNING: Electronnumber set to "<< N << flush;
        CTP_LOG(ctp::logDEBUG, *_log) <<"=======================" << flush;
    }

    double netcharge=getNetcharge( _atomlist,N );

    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Calculating ESP at CHELPG grid points"  << flush;
    //boost::progress_display show_progress( _grid.getsize() );
    #pragma omp parallel for
    for ( unsigned i = 0 ; i < _grid.getsize(); i++){
        _grid.getGridValues()(i)=numway.IntegratePotential(_grid.getGridPositions()[i]);
    }

    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Electron contribution calculated"  << flush;
    if (!_do_Transition){
      EvalNuclearPotential(  _atomlist,  _grid );
    }

    FitPartialCharges(_atomlist,_grid, netcharge);  
    return;
    }


void Espfit::EvalNuclearPotential(const std::vector< QMAtom* >& _atoms, Grid& _grid) {
  
    const std::vector< tools::vec >& _gridpoints = _grid.getGridPositions();
    Eigen::VectorXd& _gridvalues=_grid.getGridValues();
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Calculating ESP of nuclei at CHELPG grid points" << flush;

    for (unsigned i = 0; i < _gridpoints.size(); i++) {
        for (unsigned j = 0; j < _atoms.size(); j++) {
            const vec& posatom=_atoms[j]->getPos();
            double Znuc=_atoms[j]->getNuccharge();
            double dist_j = tools::abs(_gridpoints[i]-posatom);
            _gridvalues(i) += Znuc / dist_j;
        }
    }
    return;
}

double Espfit::getNetcharge(const std::vector< QMAtom* >& _atoms, double N ){
    double netcharge = 0.0;
      if (std::abs(N) < 0.05) {
        CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp()<< " Number of Electrons is " << N << " transitiondensity is used for fit" << flush;
        _do_Transition = true;
      } else {
        double Znuc = 0.0;
        for (unsigned j = 0; j < _atoms.size(); j++) {
          Znuc += _atoms[j]->getNuccharge();
        }

        if (std::abs(Znuc - N) < 4) {
          CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp()<< " Number of Electrons minus Nucleus charge is " << Znuc - N << "." << flush;
        } else {
          CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp()<< " Warning: Your molecule is highly ionized." << flush;
        }
        netcharge = Znuc - N;
        _do_Transition = false;
      }

      netcharge = std::round(netcharge);
      CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp()<< " Netcharge constrained to " << netcharge << flush;

      return netcharge;
}



void Espfit::Fit2Density_analytic(std::vector< QMAtom* >& _atomlist,const Eigen::MatrixXd &_dmat,const AOBasis &_basis) {
    // setting up grid
    Grid _grid;
    _grid.setAtomlist(&_atomlist);
    _grid.setupCHELPgrid();

    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() <<  " Done setting up CHELPG grid with " << _grid.getsize() << " points " << endl;
    // Calculating nuclear potential at gridpoints
    AOOverlap overlap;
    overlap.Fill(_basis);
    double N_comp=_dmat.cwiseProduct(overlap.Matrix()).sum();

    double netcharge=getNetcharge( _atomlist,N_comp );
    if(!_do_Transition){
        EvalNuclearPotential(_atomlist, _grid);
    }

    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Calculating ESP at CHELPG grid points"  << flush;
    #pragma omp parallel for
    for ( unsigned i = 0 ; i < _grid.getsize(); i++){
         AOESP _aoesp;
         _aoesp.Fill(_basis, _grid.getGridPositions()[i]);
         _grid.getGridValues()(i) -=_dmat.cwiseProduct(_aoesp.Matrix()).sum();
          }

   FitPartialCharges(_atomlist,_grid,netcharge);
  
    return;
    }

void Espfit::FitPartialCharges( std::vector< QMAtom* >& _atomlist,const Grid& _grid,double _netcharge ){
  
  const int NoOfConstraints=1;
  const int matrixSize=_atomlist.size()+NoOfConstraints;
  
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Setting up Matrices for fitting of size "<< matrixSize <<" x " << matrixSize<< flush;

    const std::vector< tools::vec >& _gridpoints=_grid.getGridPositions();
    const Eigen::VectorXd & _potential=_grid.getGridValues();
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Using "<< _atomlist.size() <<" Fittingcenters and " << _gridpoints.size()<< " Gridpoints."<< flush;

    Eigen::MatrixXd _Amat = Eigen::MatrixXd::Zero(matrixSize,matrixSize);
    Eigen::VectorXd _Bvec = Eigen::VectorXd::Zero(matrixSize);
    // setting up _Amat
    #pragma omp parallel for
    for ( int _i =0 ; _i < _Amat.rows()-NoOfConstraints; _i++){
        for ( int _j=_i; _j<_Amat.cols()-NoOfConstraints; _j++){
            for ( unsigned _k=0; _k < _gridpoints.size(); _k++){
                double dist_i = tools::abs(_atomlist[_i]->getPos()-_gridpoints[_k]);
                double dist_j = tools::abs(_atomlist[_j]->getPos()-_gridpoints[_k]);

                 _Amat(_i,_j) += 1.0/dist_i/dist_j;
            }
            _Amat(_j,_i) = _Amat(_i,_j);
        }
    }

    for ( int _i =0 ; _i < _Amat.rows(); _i++){
      _Amat(_i,_Amat.cols()-NoOfConstraints) = 1.0;
      _Amat(_Amat.rows()-NoOfConstraints,_i) = 1.0;
    }
    _Amat(_Amat.rows()-NoOfConstraints,_Amat.cols()-NoOfConstraints) = 0.0;

    // setting up Bvec
    #pragma omp parallel for
    for ( int _i =0 ; _i < _Bvec.size()-1; _i++){
        for ( unsigned _k=0; _k < _gridpoints.size(); _k++){
                double dist_i = tools::abs(_atomlist[_i]->getPos()-_gridpoints[_k]);
                _Bvec(_i) += _potential(_k)/dist_i;
        }
       }

    _Bvec(_Bvec.size()-NoOfConstraints) = _netcharge; //netcharge!!!!
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Inverting Matrices "<< flush;
    // invert _Amat
    


    Eigen::VectorXd _charges;
    if(_do_svd){
      Eigen::JacobiSVD<Eigen::MatrixXd> svd;
      svd.setThreshold(_conditionnumber);
      svd.compute(_Amat);
      _charges=svd.solve(_Bvec);
      CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << "SVD Done. "<<_Bvec.size()-svd.nonzeroSingularValues()<<" could not be fitted and are set to zero."<< flush;
    }
    else{
       _charges=_Amat.colPivHouseholderQr().solve(_Bvec);
    }
    
    //remove constraint
    _charges=_charges.segment(0,_charges.size()-NoOfConstraints);
    CTP_LOG(ctp::logDEBUG, *_log) << ctp::TimeStamp() << " Inverting Matrices done."<< flush;
   
    CTP_LOG(ctp::logDEBUG, *_log) << " Sum of fitted charges: " << _charges.sum() << flush;
    for (unsigned i=0;i<_atomlist.size();i++){
      _atomlist[i]->setPartialcharge(_charges(i));
    }
    
    // get RMSE
    double _rmse = 0.0;
    double _totalPotSq = 0.0;
    for ( unsigned _k=0 ; _k < _gridpoints.size(); _k++ ){
        double temp = 0.0;
        for ( const QMAtom* atom:_atomlist){
            double dist =  tools::abs(_gridpoints[_k]-atom->getPos());
            temp += atom->getPartialcharge()/dist;
        }
        _rmse += (_potential(_k) - temp)*(_potential(_k) - temp);
        _totalPotSq += _potential(_k)*_potential(_k);
    }
    CTP_LOG(ctp::logDEBUG, *_log) << " RMSE of fit:  " << sqrt(_rmse/_gridpoints.size()) << flush;
    CTP_LOG(ctp::logDEBUG, *_log) << " RRMSE of fit: " << sqrt(_rmse/_totalPotSq) << flush;

    return;
   }


}}
