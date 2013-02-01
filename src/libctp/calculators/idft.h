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


#ifndef _CALC_INTEGRALS_DFT_H
#define	_CALC_INTEGRALS_DFT_H

#include <votca/ctp/qmcalculator.h>
#include <votca/ctp/orbitals.h>
#include <votca/tools/property.h>

namespace votca { namespace ctp {

/**
* \brief Density-functional based electronic coupling elements
*
* DFT-based electronic coupling elements for all conjugated
* segments from the neighbor list. Requires molecular orbitals in GAUSSIAN
* or TURBOMOLE format.
*
* Callname: idft
*/

class IDFT : public QMCalculator
{
public:

    IDFT() {};
   ~IDFT() {};
   
    // Topology is not needed; shall removed later with a different Apllication class
    void    Initialize(ctp::Topology *top, tools::Property *options );
    
    string  Identify() { return "IDFT"; }
    void    ParseOptionsXML( tools::Property *opt);

    void    SQRTOverlap();
    
    /*  
    void    EvalPair(Topology *top, QMPair *pair, int slot);
    void    CalculateJ(QMPair *pair);
    void    CleanUp();
*/

private:

    string _orbitalsA_file;
    string _orbitalsB_file;
    string _orbitalsAB_file;
    string _overlapAB_file;

    Orbitals _orbitalsA;
    Orbitals _orbitalsB;
    Orbitals _orbitalsAB;
   
};

}}
#endif	/* _CALC_INTEGRALS_DFT_H */
