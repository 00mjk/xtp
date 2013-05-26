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


#ifndef _QMCALCULATOR2_H
#define _QMCALCULATOR2_H


#include <votca/tools/property.h>
#include <votca/ctp/topology.h>

namespace CTP = votca::ctp;

namespace votca { namespace ctp {

class QMCalculator
{
public:

                    QMCalculator() {}
    virtual        ~QMCalculator() {}

    // reads-in default options from the shared folder
    void LoadDefaults(string name) {

        // get the path to the shared folders with xml files
        char *votca_share = getenv("VOTCASHARE");
        if(votca_share == NULL) throw std::runtime_error("VOTCASHARE not set, cannot open help files.");
        string xmlFile = string(getenv("VOTCASHARE")) + string("/ctp/xml/")+name+string(".xml");
        
        // load the xml description of the calculator (with default and test values)
        load_property_from_xml(_options, xmlFile);

        // override test values with default values
        _options.ResetFromDefaults();

    };

    
    virtual string  Identify() { return "Generic calculator"; }

    virtual void    Initialize(CTP::Topology *top, Property *options) { }
    virtual bool    EvaluateFrame(CTP::Topology *top) { return true; }
    virtual void    EndEvaluate(CTP::Topology *top) { }

    void            setnThreads(int nThreads) { _nThreads = nThreads; }

protected:

    int _nThreads;
    Property _options;

};

}}

#endif /* _QMCALCULATOR2_H */