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

#ifndef __CTP_MULLIKEN__H
#define	__CTP_MULLIKEN__H


#include <votca/ctp/elements.h>
#include <votca/tools/property.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/multi_array.hpp>
#include <votca/ctp/aobasis.h>
#include <votca/ctp/qmatom.h>


/**
* \brief Takes a list of atoms, and the corresponding density matrix and puts out a table of partial charges
*
* 
* 
*/
using namespace std;
using namespace votca::tools;


namespace votca { namespace ctp {
    namespace ub = boost::numeric::ublas;
    
class Mulliken{
public:
    
    Mulliken(){};
   ~Mulliken(){};
    
    
    void EvaluateMulliken(vector< QMAtom* >& _atomlist, ub::matrix<double> &_dmat, AOBasis &_dftbasis, bool _do_transition);
  
   
private:
    
     Elements _elements; 
     
     
 
    
};

void Mulliken::EvaluateMulliken(vector< QMAtom* >& _atomlist, ub::matrix<double> &_dmat, AOBasis &basis, bool _do_transition){
    AOOverlap _overlap;
    // initialize overlap matrix
    _overlap.Initialize(basis._AOBasisSize);
    // Fill overlap
    _overlap.Fill(&basis);
    
    ub::matrix<double> _prodmat = ub::prod( _dmat, _overlap );
    
    vector < QMAtom* > :: iterator atom;
    int id = 0;
    
  

    for (atom = _atomlist.begin(); atom < _atomlist.end(); ++atom){
                
         id++;      
         // get element type and determine its nuclear charge
         if (!_do_transition){
             (*atom)->charge=_elements.getNucCrgECP((*atom)->type);
         }
         int crg = ElementToCharge((*atom)->type);
         // add to either fragment
         if ( id <= _frag ) {
             _nucCrgA += crg;
         } else {
             _nucCrgB += crg;
         }
    
    
    
    
        }



}


}}

#endif	/* ESPFIT_H */
