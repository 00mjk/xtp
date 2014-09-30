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
 *Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// Overload of uBLAS prod function with MKL/GSL implementations
#include <votca/ctp/votca_ctp_config.h>

#include <votca/ctp/threecenters.h>

#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/math/constants/constants.hpp>
#include <votca/ctp/logger.h>
#include <votca/tools/linalg.h>

using namespace std;
using namespace votca::tools;

namespace votca {
    namespace ctp {
        namespace ub = boost::numeric::ublas;

        /*
         * Cleaning TCMatrix data and free memory
         */
        void AuxMatrix::Cleanup() {

            for (int _i = 0; _i < _matrix.size(); _i++) {
                _matrix[ _i ].resize(0, 0, false);
            }
            _matrix.clear();

        } // TCMatrix::Cleanup

        
        /*
         * Modify 3-center matrix elements consistent with use of symmetrized 
         * Coulomb interaction. 
         */
        void AuxMatrix::Symmetrize(const ub::matrix<double>& _coulomb) {

            #pragma omp parallel for
            for (int _i_occ = 0; _i_occ < this->get_mtot(); _i_occ++) {
	      // fist cast _matrix[_i_occ] to double for efficient prod() overloading
	      ub::matrix<double> _matrix_double = _matrix[ _i_occ ];
	      // ub::matrix<float> _temp = ub::prod(_coulomb, _matrix[ _i_occ ]);
	      ub::matrix<float> _temp = ub::prod(_coulomb, _matrix_double);
	      //_matrix[ _i_occ ] = ub::prod(_coulomb, _matrix[ _i_occ ]);
	      _matrix[ _i_occ ] = _temp;
            }

        } // TCMatrix::Symmetrize

        
        /*
         * Fill the 3-center object by looping over shells of GW basis set and
         * calling FillBlock, which calculates all 3-center overlap integrals
         * associated to a particular shell, convoluted with the DFT orbital
         * coefficients
         */
        void AuxMatrix::Fill(AOBasis& _gwbasis, AOBasis& _dftbasis, ub::matrix<double>& _dft_orbitals) {


            //std::vector< ub::matrix<double> > _block(this->get_mtot());
            //std::vector< ub::matrix<float> > _block(this->get_mtot());


            // loop over all shells in the GW basis and get _Mmn for that shell
            #pragma omp parallel for //private(_block)
            for ( int _is= 0; _is <  _gwbasis._aoshells.size() ; _is++ ){
            // for (vector< AOShell* >::iterator _is = _gwbasis.firstShell(); _is != _gwbasis.lastShell(); _is++) {
                //cout << " act threads: " << omp_get_thread_num( ) << " total threads " << omp_get_num_threads( ) << " max threads " << omp_get_max_threads( ) <<endl;
                AOShell* _shell = _gwbasis.getShell(_is);
                int _start = _shell->getStartIndex();
                int _end = _start + _shell->getNumFunc();


                // each element is a shell_size-by-n matrix, initialize to zero
                std::vector< ub::matrix<double> > _block(this->get_mtot());
                for (int i = 0; i < this->get_mtot(); i++) {
                    _block[i] = ub::zero_matrix<double>(_shell->getNumFunc(), this->get_ntot());
                }

                // Fill block for this shell (3-center overlap with _dft_basis + multiplication with _dft_orbitals )
                FillBlock(_block, _shell, _dftbasis, _dft_orbitals);

                // put into correct position
                for (int _m_level = 0; _m_level < this->get_mtot(); _m_level++) {
                    for (int _i_gw = 0; _i_gw < _shell->getNumFunc(); _i_gw++) {
                        for (int _n_level = 0; _n_level < this->get_ntot(); _n_level++) {

                            _matrix[_m_level](_start + _i_gw, _n_level) = _block[_m_level](_i_gw, _n_level);

                        } // n-th DFT orbital
                    } // GW basis function in shell
                } // m-th DFT orbital
            } // shells of GW basis set

        } // TCMatrix::Fill


        
        /*
         * Determines the 3-center integrals for a given shell in the aux basis
         * by calculating the 3-center overlap integral of the functions in the
         * aux shell with ALL functions in the DFT basis set (FillThreeCenterOLBlock)
         */
        
        void AuxMatrix::FillBlock(std::vector< ub::matrix<double> >& _block, AOShell* _shell, AOBasis& dftbasis) {
	  //void TCMatrix::FillBlock(std::vector< ub::matrix<float> >& _block, AOShell* _shell, AOBasis& dftbasis, ub::matrix<double>& _dft_orbitals) {

            // prepare local storage for 3-center integrals
            ub::matrix<double> _imstore = ub::zero_matrix<double>(dftbasis._AOBasisSize, dftbasis._AOBasisSize);
            //ub::matrix<float> _imstore = ub::zero_matrix<float>(this->mtotal * _shell->getNumFunc(), dftbasis._AOBasisSize);

            // alpha-loop over the "left" DFT basis function
            for (vector< AOShell* >::iterator _row = dftbasis.firstShell(); _row != dftbasis.lastShell(); _row++) {
                AOShell* _shell_row = dftbasis.getShell(_row);
                int _row_start = _shell_row->getStartIndex();
                int _row_end = _row_start + _shell_row->getNumFunc();

                // gamma-loop over the "right" DFT basis function
                for (vector< AOShell* >::iterator _col = dftbasis.firstShell(); _col != dftbasis.lastShell(); _col++) {
                    AOShell* _shell_col = dftbasis.getShell(_col);
                    int _col_start = _shell_col->getStartIndex();
                    int _col_end = _col_start + _shell_col->getNumFunc();

                    // get 3-center overlap directly as _subvector
                    ub::matrix<double> _subvector = ub::zero_matrix<double>(_shell_row->getNumFunc(), _shell->getNumFunc() * _shell_col->getNumFunc());
                    //ub::matrix<float> _subvector = ub::zero_matrix<float>(_shell_row->getNumFunc(), _shell->getNumFunc() * _shell_col->getNumFunc());
                    bool nonzero = FillThreeCenterOLBlock(_subvector, _shell, _shell_row, _shell_col);

                    // if this contributes, multiply _subvector with _dft_orbitals and place in _imstore
                    if (nonzero) {

            // and put it into the block it belongs to
                        
            for (int _aux = 0; _aux < _shell->getNumFunc; _aux++) {
                for (int _col = 0; _col < _shell_col->getNumFunc(); _col++) {
                    int _index=_shell_col->getNumFunc()*_aux+_col
                
                    for (int _row = 0; _row < _shell_row->getNumFunc(); _row++) {
                    
                        _block[_aux](_row, _col) = _subvector(_row, _index);
                    } // n-level
                } // GW basis function in shell
            } // m-level
            
        } // TCMatrix::FillBlock

        
             void TCMatrix::Prune ( int _basissize, int min, int max){

            int size1 = _matrix[0].size1();
            
            // vector needs only max entries
            _matrix.resize( max + 1 );
            
            
            
            // entries until min can be freed
            for ( int i = 0; i < min ; i++){
                //_matrix[i] = ub::zero_matrix<double>(_basissize,ntotal);
                _matrix[i].resize(0,0,false);
            }

            for ( int i=min; i < _matrix.size(); i++){
                _matrix[i].resize(size1,max+1);
            }

            
        }
        
 


    }
}

