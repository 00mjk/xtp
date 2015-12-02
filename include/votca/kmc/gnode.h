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

#ifndef _VOTCA_KMC_GNODE_H
#define	_VOTCA_KMC_GNODE_H

#include <votca/tools/vec.h>
#include <votca/kmc/glink.h>

using namespace std;
using namespace votca::kmc;


namespace votca { namespace kmc {

class GNode
{
    public:
        GNode():hasdecay(true){};
        ~GNode(){};

        int id;
        int occupied;
        int injectable;
        double occupationtime;
        double escaperate;
        bool hasdecay;
        vec position;
        vector<GLink> event;
        // stuff for Coulomb interaction:
        double siteenergy;
        double reorg_intorig; // UnCnN
        double reorg_intdest; // UcNcC
    
        void AddEvent(int seg2, double rate12, vec dr, double Jeff2, double reorg_out);
        const double &getEscapeRate(){return escaperate;}
        void InitEscapeRate();
        void AddDecayEvent(double _decayrate);
};



void GNode::AddDecayEvent(double _decayrate)
{
    GLink newEvent;
    newEvent.destination = -1;
    newEvent.rate = _decayrate;
    newEvent.initialrate = _decayrate;
    newEvent.dr = vec(0,0,0);
    newEvent.Jeff2 = 0.0;
    newEvent.decayevent=true;
    newEvent.reorg_out = 0.0;
    this->event.push_back(newEvent);
    hasdecay=true;
}

void GNode::AddEvent(int seg2, double rate12, vec dr, double Jeff2, double reorg_out)
{
    GLink newEvent;
    newEvent.destination = seg2;
    newEvent.rate = rate12;
    newEvent.initialrate = rate12;
    newEvent.dr = dr;
    newEvent.Jeff2 = Jeff2;
    newEvent.decayevent=false;
    newEvent.reorg_out = reorg_out;
    this->event.push_back(newEvent);
}


void GNode::InitEscapeRate()
{
    double newEscapeRate = 0.0;
    for(unsigned int i=0; i<this->event.size();i++)
    {
        newEscapeRate += this->event[i].rate;
    }
    this->escaperate = newEscapeRate;
    // cout << "Escape rate for segment " << this->id << " was set to " << newEscapeRate << endl;
}



}}

#endif	/* _VOTCA_KMC_GNODE_H */

