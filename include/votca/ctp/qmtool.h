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


#ifndef __QMTOOL__H
#define __QMTOOL__H


#include <votca/tools/property.h>
#include <votca/ctp/topology.h>
#include <votca/ctp/logger.h>
#include <boost/format.hpp>

namespace CTP = votca::ctp;

namespace votca { namespace ctp {

    using namespace boost;
    
class QMTool
{
public:

    QMTool() {
        _log.setReportLevel(logDEBUG);
        _log.setPreface(logINFO,    "\n... INF ..." );
        _log.setPreface(logERROR,   "\n... ERR ..." );
        _log.setPreface(logWARNING, "\n... WAR ..." );
        _log.setPreface(logDEBUG,   "\n... DBG ..." );  

    };
    virtual        ~QMTool() { };

    virtual string  Identify() { return "Generic tool"; }

    virtual void    Initialize(Property *options) {
    }
    
    virtual bool    Evaluate() { return true; }
    virtual bool    EndEvaluate() { return true; }

    void            setnThreads(int nThreads) { _nThreads = nThreads; }

protected:

    int _nThreads;
    Logger _log;

};

}}

#endif /* _QMTOOL_H */