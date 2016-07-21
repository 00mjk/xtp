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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <votca/xtp/nbo.h>
#include <votca/xtp/aomatrix.h>
namespace votca { namespace xtp {

void NBO::EvaluateNBO(vector< QMAtom* >& _atomlist,  const ub::matrix<double> &_dmat,AOBasis &basis,BasisSet &bs){
    AOOverlap _overlap;
    // initialize overlap matrix
    _overlap.Initialize(basis._AOBasisSize);
    // Fill overlap
    _overlap.Fill(&basis);
    
    ub::matrix<double> _prodmat = ub::prod( _dmat, _overlap._aomatrix );
    
    
    
    ub::matrix<double> P=ub::prod(_overlap._aomatrix,_prodmat);
   
    
    
    
    vector < QMAtom* > :: iterator atom;

    int id =0;
    for (atom = _atomlist.begin(); atom < _atomlist.end(); ++atom){
                
    //TODO: Jens, fill this in
    
         Element* element = bs.getElement((*atom)->type);
         int nooffunc=0;
         for (Element::ShellIterator its = element->firstShell(); its != element->lastShell(); its++) {
             Shell* shell = (*its);
             shell->getnumofFunc();
             
         }
         //cout << id << " "<< id+nooffunc << endl;
      
    }
    //cout << id << " " << _dmat.size1() << endl;
}




void NBO::IntercenterOrthogonalisation(ub::matrix<double> &P,ub::matrix<double> &Overlap,vector< QMAtom* >& _atomlist,)


void NBO::LoadMatrices(std::string fn_projectionMatrix, std::string fn_overlapMatrix){

    //TODO: Yuriy, fill this in
    
    return;
    }

}}