/* 
 *            Copyright 2009-2012 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICEN_olE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "A_ol I_ol" BA_olI_ol,
 * WITHOUT WARRANTIE_ol OR CONDITION_ol OF ANY KIND, either express or implied.
 * _olee the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <votca/ctp/threecenters.h>

#include <votca/ctp/aobasis.h>
#include <string>
#include <map>
#include <vector>
#include <votca/tools/property.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/multi_array.hpp>
#include <votca/ctp/logger.h>
#include <votca/tools/linalg.h>

using namespace std;
using namespace votca::tools;

namespace votca { namespace ctp {
    namespace ub = boost::numeric::ublas;

   /* void AOMatrix::PrintIndexToFunction( AOBasis* aobasis){
        for (vector< AOShell* >::iterator _row = aobasis->firstShell(); _row != aobasis->lastShell() ; _row++ ) {
            AOShell* _shell_row = aobasis->getShell( _row );
            int _row_start = _shell_row->getStartIndex();
            string type = _shell_row->getType();
            cout << "Shell " << type << "starts at " << _row_start+1 << endl;
        }
    }*/
    
    void TCMatrix::Fill( AOBasis& gwbasis, AOBasis& dftbasis, ub::matrix<double>& _dft_orbitals ) {
        cout << "I'm supposed to fill out the Mmn multi array" << endl;

        // slicing the multiarray, and hand that to a function analog to isomn_beta
        typedef boost::multi_array<double,3>::array_view<3>::type array_view; //* array_view
        typedef boost::multi_array_types::index_range irange;
        
        // loop over all shells in the GW basis and get _Mmn for that shell
        for (vector< AOShell* >::iterator _is = gwbasis.firstShell(); _is != gwbasis.lastShell() ; _is++ ) {
            AOShell* _shell = gwbasis.getShell( _is );
            int _start = _shell->getStartIndex();
            int _end   = _start + _shell->getNumFunc();
            
            // define subview of multi array belonging to this shell -> works!
            array_view _block = this->_array[boost::indices[irange(_start,_end)][irange()][irange()]];
            

	    ub::vector<ub::matrix<double> > rr;

	    // matrix multiplications possible? no... multi_array and ublas are not compatible directly
	    // ub::matrix<double> res = ub::prod(_block,_block);

            // Fill block for this shell (3-center overlap with _dft_basis + multiplication with _dft_orbitals )
            // Can one pass array_view to procedure?
            // FillBlock( _block, _shell, _dft_basis, _dft_orbitals);
    
        }
    }
    
    void TCMatrix::Print( string _ident){
        cout << "\n" << endl;
        for ( int i =0; i< this->_aomatrix.size1(); i++){
            for ( int j =0; j< this->_aomatrix.size2(); j++){
                cout << _ident << "[" << i+1 << ":" << j+1 << "] " <<  this->_aomatrix(i,j) << endl;
            }
        }
    }
       
    void TCMatrix::getTrafo(ub::matrix<double>& _trafo, int _lmax, const double& _decay) {
        // s-functions
        _trafo(0,0) = 1.0; // s
       
        // p-functions
        if ( _lmax > 0 ){
            //cout << _trafo_row.size1() << ":" << _trafo_row.size2() << endl;
            _trafo(1,1) = 2.0*sqrt(_decay);
            _trafo(2,2) = 2.0*sqrt(_decay);
            _trafo(3,3) = 2.0*sqrt(_decay);
        }

        // d-functions
        if ( _lmax > 1 ){
            _trafo(4,5) = 4.0*_decay;             // dxz
            _trafo(5,6) = _trafo(4,5);            // dyz
            _trafo(6,4) = _trafo(4,5);            // dxy
            _trafo(7,7) = -2.0*_decay/sqrt(3.0);  // d3z2-r2 (dxx)
            _trafo(7,8) = _trafo(7,7);            // d3z2-r2 (dyy)
            _trafo(7,9) = -2.0*_trafo(7,7);       // d3z2-r2 (dzz)
            _trafo(8,7) = 2.0*_decay;             // dx2-y2 (dxx)
            _trafo(8,8) = -_trafo(8,7);           // dx2-y2 (dzz)
        }
        
        // f-functions
        if ( _lmax > 2 ){
            _trafo(9,12) = 4.0 * 2.0 *pow(_decay,1.5); // f1 (f??)
            _trafo(9,15) = -1.5 * _trafo(9,12);        // f1 (f??)
            _trafo(9,17) = _trafo(9,15);               // f1 (f??)
            
            _trafo(10,16) = 4.0 * 2.0 * sqrt(2.0)/sqrt(5.0) * pow(_decay,1.5); // f2 (f??)
            _trafo(10,10) = -0.25 * _trafo(10,16);                             // f2 f(??)
            _trafo(10,14) = _trafo(10,10);                                     // f2 f(??)
            
            _trafo(11,18) = _trafo(10,16);                                     // f3 (f??)
            _trafo(11,13) = -0.25 * _trafo(11,18);                             // f3 f(??)
            _trafo(11,11) = _trafo(11,13);                                     // f3 f(??)            
                   
            _trafo(12,13) = 3.0 * 2.0 * sqrt(2.0)/sqrt(3.0) * pow(_decay,1.5); // f4 (f??)
            _trafo(12,11) = -_trafo(12,13)/3.0;                                // f4 (f??)
            
            _trafo(13,10) = -_trafo(12,11);                                    // f5 (f??)
            _trafo(13,14) = -_trafo(12,13);                                    // f5 (f??)
            
            _trafo(14,19) = 8.0 * pow(_decay,1.5);                             // f6 (f??)
            
            _trafo(15,15) = 0.5 * _trafo(14,19);                               // f7 (f??)
            _trafo(15,17) = -_trafo(15,15);                                    // f7 (f??)
        }
        
        // g-functions
        if ( _lmax > 3 ){
            _trafo(16,22) = 8.0 * 2.0/sqrt(105.0) * pow(_decay,2.0);
            _trafo(16,21) = 3.0 * 2.0/sqrt(105.0) * pow(_decay,2.0);
            _trafo(16,20) = _trafo(16,21);
            _trafo(16,29) = -3.0 * _trafo(16,22);
            _trafo(16,31) = 2.0 * _trafo(16,21);
            _trafo(16,30) = _trafo(16,29);
            _trafo(16,5)  = _trafo(16,31);
            
             /* vv(17,:) =  (/   23,  22, 21, 30, 32, 31,   6 /) ! g
                cc(17,:) =  (/    8,  3, 3, -24, 6, -24,    6 /)
                normConst(17,:) = (/ 2.d0/sqrt(105.d0) ,2.d0  /)
              */
            _trafo(17,26) = 4.0 * 4.0*sqrt(2.0)/sqrt(21.0) * pow(_decay,2.0);
            _trafo(17,25) = -0.75 * _trafo(17,26);
            _trafo(17,33) = _trafo(17,25);
             
             /* vv(18,:) =  (/   27,  26, 34,  0,  0,  0,   3 /) ! g
                cc(18,:) =  (/    4,  -3, -3,  0,  0,  0,   3 /)
                normConst(18,:) = (/ 4.d0*sqrt(2.d0)/sqrt(21.d0) ,2.d0  /)
              */
            
            _trafo(18,28) = _trafo(17,26);
            _trafo(18,32) = _trafo(17,25);
            _trafo(18,27) = _trafo(17,25);
             
            /* vv(19,:) =  (/   29,  33, 28,  0,  0,  0,   3 /) ! g 
               cc(19,:) =  (/    4,  -3, -3,  0,  0,  0,   3 /)
               normConst(19,:) = (/ 4.d0*sqrt(2.d0)/sqrt(21.d0) ,2.d0  /)
             */
     
            _trafo(19,34) = 6.0 * 8.0/sqrt(21.0) * pow(_decay,2.0);
            _trafo(19,23) = -_trafo(19,34)/6.0;
            _trafo(19,24) = _trafo(19,23);
             
            /* vv(20,:) =  (/   35,  24, 25,  0,  0,  0,   3 /) ! g
               cc(20,:) =  (/    6,  -1, -1,  0,  0,  0,   3 /)
               normConst(20,:) = (/ 8.d0/sqrt(21.d0) ,2.d0  /)
             */
    
            _trafo(20,29) = 6.0 * 4.0/sqrt(21.0) * pow(_decay,2.0);
            _trafo(20,20) = -_trafo(20,29)/6.0;
            _trafo(20,30) = -_trafo(20,29);
            _trafo(20,21) = -_trafo(20,20);

            /* vv(21,:) =  (/   30,  21, 31, 22,  0,  0,   4 /) ! g
               cc(21,:) =  (/    6,  -1, -6, 1,  0,  0,    4 /)
               normConst(21,:) = (/ 4.d0/sqrt(21.d0) ,2.d0  /)
             */
    
            _trafo(21,25) = 4.0 * sqrt(2.0)/sqrt(3.0) * pow(_decay,2.0);
            _trafo(21,33) = -3.0 * _trafo(21,25);
             
            /* vv(22,:) =  (/   26,  34,  0,  0,  0,  0,   2 /) ! g
               cc(22,:) =  (/    1,  -3,  0,  0,  0,  0,   2 /)
               normConst(22,:) = (/ 4.d0*sqrt(2.d0)/sqrt(3.d0) ,2.d0  /)
             */
    
            _trafo(22,32) = -_trafo(21,33);
            _trafo(22,27) = -_trafo(21,25);
            
            /* vv(23,:) =  (/   33,  28,  0,  0,  0,  0,   2 /) ! g
               cc(23,:) =  (/    3,  -1,  0,  0,  0,  0,   2 /)
               normConst(23,:) = (/ 4.d0*sqrt(2.d0)/sqrt(3.d0) ,2.d0  /)
             */
    
            _trafo(23,23) = 8.0/sqrt(3.0) * pow(_decay,2.0);
            _trafo(23,24) = -_trafo(23,23);
             
            /* vv(24,:) =  (/   24,  25,  0,  0,  0,  0,   2 /) ! g 
               cc(24,:) =  (/    1,  -1,  0,  0,  0,  0,   2 /)
               normConst(24,:) = (/ 8.d0/sqrt(3.d0) ,2.d0  /)
             */
    
            _trafo(24,20) = 2.0/sqrt(3.0) * pow(_decay,2.0);
            _trafo(24,21) = _trafo(24,20);
            _trafo(24,31) = -6.0 * _trafo(24,20);
             
            /* vv(25,:) =  (/   21,  22, 32,  0,  0,  0,   3 /) ! g
               cc(25,:) =  (/    1,  1, -6,  0,  0,  0,   3  /)
               normConst(25,:) = (/ 2.d0/sqrt(3.d0) ,2.d0  /)
             */
               
        }
        
        
    }
    
     
    int TCMatrix::getBlockSize(int _lmax){
        int _block_size;
        if ( _lmax == 0 ) { _block_size = 1  ;}  // s
        if ( _lmax == 1 ) { _block_size = 4  ;}  // p
        if ( _lmax == 2 ) { _block_size = 10 ;}  // d
        if ( _lmax == 3 ) { _block_size = 20 ;}  // f
        if ( _lmax == 4 ) { _block_size = 35 ;}  // g
        
        return _block_size;
    }
    
  /* int TCCoulomb::getExtraBlockSize(int _lmax_row, int _lmax_col){
        int _block_size = _lmax_col + _lmax_row +1;
        return _block_size;
    } */
    
   
    
}}

