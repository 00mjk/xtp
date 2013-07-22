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

#ifndef __VOTCA_KMC_GRAPH_H_
#define __VOTCA_KMC_GRAPH_H_

#include <votca/kmc/node.h>
#include <votca/tools/database.h>
#include <votca/tools/vec.h>


namespace votca { namespace kmc {
  
using namespace std;

class Graph {
 public:
     // TO IMPLEMENT
     // void Load(){};
     // TO IMPLEMENT
     void CreateSquareLattice(int NX, int NY, int NZ, double latconst);
     
     //const vector<Node*> &getNeighbours( Node* node, CarrierType type ) { return node->getNeighbours(type); }
   
 private:
     vector< Node* > nodes;
     
};

/* void Graph::Load() {
    
    votca::tools::Database db;
    db.Open( _filename );
    votca::tools::Statement *stmt = db.Prepare("SELECT _id-1, posX, posY, posZ FROM segments;");
    
    int node_id;
    vec nodeposition;
    
    int index = 0;
    
    while (stmt->Step() != SQLITE_DONE) {
        node_id = stmt->Column<int>(0);
        nodeposition = vec(stmt->Column<double>(1),stmt->Coulumb<double(2),stmt->Coulomb<double(3)); //positions in nm

        nodes[index]->id = node_id;
        nodes[index]->position = nodeposition;
        
        index++;
    }    
    

} */

void Graph:: CreateSquareLattice(int NX, int NY, int NZ, double latt_const) {
    
    //specify lattice types (square lattice, fcc lattice, fractal lattice?)
    //and dimensions
    
    int node_id;
    vec nodeposition;
    int index = 0;
    
    for(int ix = 0; ix<NX; ix++) {
        for(int iy=0; iy<NY; iy++) {
            for(int iz=0; iz<NZ; iz++) {
                Node *newNode = new Node();
                nodes.push_back(newNode);

                node_id = NX*NY*iz + NX*iy + ix-1;
                nodeposition = vec(latt_const*ix,latt_const*iy,latt_const*iz); //positions in nm

                nodes[index]->setID(node_id);
                nodes[index]->setPosition(latt_const*ix,latt_const*iy,latt_const*iz);
                    
                index++;
            }
        }
    }
}

}} 

#endif

