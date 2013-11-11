/*
 * Copyright 2009-2013 The VOTCA Development Team (http://www.votca.org)
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

#ifndef _VOTCA_KMC_LINK_H
#define	_VOTCA_KMC_LINK_H

#include <votca/tools/vec.h>

namespace votca { namespace kmc {

class Node;

/**
 * \brief A link between two nodes
 * 
 * Container for pair properties: rates, couplings, separations 
 * 
 */
class Link
{

public:
    
    const int &Id() const { return _id; }
    
    /// r2 - r1
    const votca::tools::vec &r12() const { return _r12; }    
    
    /// forward and backward rates
    const double &rate12() const { return _rate12; }
    const double &rate21() const { return _rate21; }
    
    void setnode1(Node* init_node) { node1 = init_node;}
    void setnode2(Node* final_node) { node2 = final_node;}
    
    // print Link info
    virtual void Print(std::ostream &out) {
        out << _id ;
    }
    
private:
    
    int _id;
    
    Node *node1;
    Node *node2;
    
    /// forward and backward rates
    double _rate12;
    double _rate21;
    
    /// r2 - r1
    votca::tools::vec _r12;       
};

}} 

#endif // _VOTCA_KMC_LINK_H
