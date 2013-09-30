/* 
 * Copyright 2009-2011 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/**

\page ctp ctp
Classes used for charge transport. The main container is votca::ctp::Topology. 


*/

#ifndef __VOTCA_MD2QM_VERSION_H
#define	__VOTCA_MD2QM_VERSION_H

#include <string>

namespace votca { namespace ctp {
    const std::string & CtpVersionStr();
    void HelpTextHeader(const std::string &tool_name);
}}

#endif

