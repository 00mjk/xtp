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


#ifndef _QMCALCULATOR_H
#define _QMCALCULATOR_H



#include <votca/ctp/topology.h>
#include <votca/ctp/progressobserver.h>

namespace CTP = votca::ctp;

namespace votca { namespace ctp {

class QMCalculator
{
public:

                    QMCalculator() { };
    virtual        ~QMCalculator() { };

    virtual string  Identify() { return "Generic calculator"; }

    virtual void    Initialize(CTP::Topology *top, Property *options) { }
    virtual bool    EvaluateFrame(CTP::Topology *top) { return true; }
    virtual void    EndEvaluate(CTP::Topology *top) { }

    void            setnThreads(int nThreads) { _nThreads = nThreads; }
    void            setProgObserver(ProgObserver< vector<Job*>, Job* > *obs) { _progObs = obs; }

protected:

    int _nThreads;
    ProgObserver< vector<Job*>, Job* > *_progObs;

};

}}

#endif /* _QMCALCULATOR_H */