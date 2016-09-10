/* 
 *            Copyright 2009-2016 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <votca/xtp/ERIs.h>



using namespace votca::tools;

namespace votca {
    namespace xtp {
        namespace ub = boost::numeric::ublas;
        
        
        
    void ERIs::Initialize(AOBasis &_dftbasis, AOBasis &_auxbasis){

           
           
            _threecenter.Fill( _auxbasis, _dftbasis );
            //cout << "_threeceenter(0)"<<endl;
            //cout << _threecenter.getDatamatrix(0)<<endl;

            return;
        }
        
        

  /*  void Eris::Initialize_Symmetric(AOBasis &_dftbasis,AOBasis &_auxbasis, AOCoulomb &_auxcoulomb){
        _threecenter.Fill(_auxbasis,_dftbasis);
        eigenlinalg_matrixsqrt(_auxcoulomb.Matrix());
        
        for 
        
    }    
        
 */       
        
        
      
        
        
        void ERIs::CalculateERIs (const ub::matrix<double> &DMAT, const ub::matrix<double> &_inverse_Coulomb){
            
           
            //cout << _auxAOcoulomb.Matrix()<<endl;
            //cout << "inverse Coulomb"<< endl;
            //cout << _inverse_Coulomb<<endl;
            
            _ERIs=ub::zero_matrix<double>(DMAT.size1(),DMAT.size2());
            const ub::vector<double> dmatasarray=DMAT.data();
            
            ub::matrix<double> Itilde=ub::zero_matrix<double>(_threecenter.getSize(),1);
            //cout << _threecenter.getSize() << " Size-Threecenter"<<endl;
            //check Efficiency !!!! someday 
            for ( int _i=0; _i<_threecenter.getSize();_i++){
                ub::vector<double>threecenterasarray=(_threecenter.getDatamatrix(_i)).data();
                // Trace over prod::DMAT,I(l)=componentwise product over 
                for ( unsigned _j=0; _j<threecenterasarray.size();_j++){
                    Itilde(_i,0)+=dmatasarray[_j]*threecenterasarray[_j];
                }
            }
            //cout << "Itilde " <<Itilde << endl;
            ub::matrix<double>K=ub::prod(_inverse_Coulomb,Itilde);
            //cout << "K " << K << endl;
            for ( unsigned _i = 0; _i < K.size1(); _i++){
                
            _ERIs+=_threecenter.getDatamatrix(_i)*K(_i,0);    
            //cout << "I " << _threecenter.getDatamatrix(_i) << endl;
            //cout<< "ERIs " <<_ERIs<< endl;
            }


            CalculateEnergy(dmatasarray);  
         
            
           
            
        }
        
         void ERIs::CalculateEnergy(const ub::vector<double> &dmatasarray){
            _ERIsenergy=0;
            const ub::vector<double>& ERIsasarray=_ERIs.data();
            for (unsigned _i=0;_i<ERIsasarray.size();_i++){
                _ERIsenergy+=dmatasarray[_i]*ERIsasarray[_i];
                
            }


        }
        
        
        void ERIs::printERIs(){
          for (unsigned i=0; i< _ERIs.size1(); i++){
                for (unsigned j=0; j< _ERIs.size2();j++){
                    cout << "ERIs [" << i<<":"<<j<<"]="<<_ERIs(i,j)<<endl;
                }
            }
        }
        
        
        
        
        
    }}
