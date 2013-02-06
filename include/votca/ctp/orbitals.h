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

#ifndef __VOTCA_CTP_ORBITALS_H
#define	__VOTCA_CTP_ORBITALS_H

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/symmetric.hpp>
#include <votca/tools/globals.h>
#include <votca/tools/property.h>

namespace votca { namespace ctp {
    namespace ub = boost::numeric::ublas;
/**
    \brief information about an orbital
 
    The Orbital class stores orbital id, energy, MO coefficients
    
*/
class Orbitals 
{
public:   

    Orbitals();
   ~Orbitals();

    const int     &getBasisSetSize() const { return _basis_set_size; }
    const int     &getNumberOfLevels() const {return  _mo_energies.size(); }
    const int     &getNumberOfElectrons() const {return  _electrons; }
    
    const double  &getEnergy(const int level ) const { return _mo_energies[level]; }
    
    ub::symmetric_matrix<double>* getOverlap() { return &_overlap; }
    ub::matrix<double>* getOrbitals() { return &_mo_coefficients; }
    ub::vector<double>* getEnergies() { return &_mo_energies; }
    
    bool ReadOrbitalsGaussian( const char * filename );
    bool ReadOverlapGaussian( const char * filename );
    bool ParseGaussianLog( const char * filename );

    inline void setEnergy( const int &level, const double &energy ) { _mo_energies[level] = energy; }

protected:
    int                                 _basis_set_size;
    int                                 _electrons;
    
    ub::vector<double>                  _mo_energies;    
    ub::matrix<double>                  _mo_coefficients;
    ub::symmetric_matrix<double>        _overlap;
       
};


}}

#endif	/* __VOTCA_CTP_ORBITAL_H */

