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

#ifndef __VOTCA_KMC_VSSMGROUP_H_
#define __VOTCA_KMC_VSSMGROUP_H_


#include <votca/kmc/events.h>
#include <votca/kmc/graphdevice.h>
#include <votca/kmc/statedevice.h>
#include <votca/kmc/eventinfo.h>

namespace votca { namespace kmc {
  
using namespace std;

class Vssmgroup {
    
public:
    
    void Recompute_in_device(Events* events, Bsumtree* non_injection_rates, Bsumtree* injection_rates);
    double Timestep(votca::tools::Random2 *RandomVariable);
    Event* Choose_event(Events* events, Bsumtree* non_injection_rates, Bsumtree* injection_rates, votca::tools::Random2 *RandomVariable);
    
    votca::tools::Random2 *RandomVariable;   
 
private:

    double tot_probsum;
    
    double non_inject_probsum;
    double inject_probsum;
    
};

double Vssmgroup::Timestep(votca::tools::Random2 *RandomVariable){

    double timestep;
    
    double rand_u = 1-RandomVariable->rand_uniform();
    while(rand_u == 0) {
        rand_u = 1-RandomVariable->rand_uniform();
    }
        
    timestep = -1/tot_probsum*log(rand_u);
    return timestep;
    
}

void Vssmgroup::Recompute_in_device(Events* events, Bsumtree* non_injection_rates, Bsumtree* injection_rates){

    non_inject_probsum = non_injection_rates->compute_sum();
    inject_probsum = injection_rates->compute_sum();       
    tot_probsum = non_inject_probsum + inject_probsum;
}

Event* Vssmgroup::Choose_event(Events* events, Bsumtree* non_injection_rates, Bsumtree* injection_rates, votca::tools::Random2 *RandomVariable){

    double randn = RandomVariable->rand_uniform();    
    
    long event_ID;
    Event* chosenevent;
    
    std::cout << "random " << randn << " " <<  non_inject_probsum << " " << inject_probsum << " " << tot_probsum << endl;
    
    if(randn<inject_probsum/tot_probsum) { // injection event
        randn *= inject_probsum;
        event_ID = injection_rates->search(randn);
        chosenevent = events->get_injection_event(event_ID);
        std::cout << "injectie " << event_ID << " " << injection_rates->getrate(event_ID) << " " << endl;
        std::cout << chosenevent->id() << endl;
    }
    else {
        std::cout << " niet injectie" << endl;

        randn -= inject_probsum/tot_probsum;
        randn *= non_inject_probsum;
        event_ID = non_injection_rates->search(randn);
        chosenevent = events->get_non_injection_event(event_ID);       
    }

    return chosenevent;
}

}} 

#endif
