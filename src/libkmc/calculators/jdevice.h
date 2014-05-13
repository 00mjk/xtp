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

#ifndef __VOTCA_KMC_JDEVICE_H
#define	__VOTCA_KMC_JDEVICE_H

#include <iostream>

#include <votca/kmc/vssmgroup.h>
#include <votca/kmc/graphkmc.h>
#include <votca/kmc/node.h>
#include <votca/kmc/state.h>
#include <votca/kmc/eventinfo.h>
#include <votca/kmc/event.h>
#include <votca/kmc/events.h>
#include <votca/kmc/profile.h>
#include <votca/kmc/numoutput.h>

using namespace std;

namespace votca { namespace kmc {
    
//typedef votca::tools::vec myvec;

// LUMO level substracted!!!
   
class Jdevice : public KMCCalculator 
{
public:
    
    votca::tools::Random2 *randomvariable;
    GraphKMC* graph;
    StateReservoir* state;
    Events* events;
    Vssmgroup* vssmgroup;
    Eventinfo* eventinfo;
    Longrange* longrange;
    Bsumtree* non_injection_rates;
    Bsumtree* left_injection_rates;
    Bsumtree* right_injection_rates;
    Bsumtree* site_inject_probs;
    Numoutput* numoutput;
    
    Jdevice() {};
   ~Jdevice() {};

    void Initialize(const char *filename, Property *options, const char *outputfile);
    bool EvaluateFrame();

    double sim_time;
    
protected:
   void RunKMC(void); 
            
private:
    static const double kB   = 8.617332478E-5; // eV/K
    static const double hbar = 6.5821192815E-16; // eV*s
    static const double eps0 = 8.85418781762E-12/1.602176565E-19; // e**2/eV/m = 8.85418781762E-12 As/Vm
    static const double epsr = 3.0; // relative material permittivity
    static const double Pi   = 3.14159265358979323846;
   
};


void Jdevice::Initialize(const char *filename, Property *options, const char *outputfile) {

    std::cout << "===================================================" << endl;
    std::cout << "= Initialization phase                            =" << endl;
    std::cout << "===================================================" << endl;
    
    eventinfo = new Eventinfo();
    eventinfo->Read_device(options);
    if(!eventinfo->device) {std::cout << "WARNING: device flag set incorrectly, set to true" << endl; eventinfo->device = true;}

    //Setup random number generator
    srand(eventinfo->seed); // srand expects any integer in order to initialise the random number generator
    randomvariable = new votca::tools::Random2();
    randomvariable->init(rand(), rand(), rand(), rand());  
    
    graph = new GraphKMC();
    graph->Initialize(filename);
    std::cout << "number of nodes before graph manipulations: " << graph->Numberofnodes() << endl;
    std::cout << "simulation box size before graph manipulations: " << graph->Determine_Sim_Box_Size() << endl;
    
    if(graph->el_reorg()) { std::cout << "WARNING: zero electron reorganization energy" << endl;}
    if(graph->ho_reorg()) { std::cout << "WARNING: zero hole reorganization energy" << endl;}
    
    graph->Setup_device_graph(eventinfo->left_electrode_distance, eventinfo->right_electrode_distance, eventinfo->resize, eventinfo);
   
    eventinfo->Graph_Parameters(graph->hopdist(), graph->mindist(), graph->simboxsize(), graph->maxpairdegree(),graph->Average_hole_node_energy(), graph->Average_electron_node_energy());
    eventinfo->Set_field(eventinfo->voltage); 
    
    std::cout << "graph object initialized" << endl;
    std::cout << "max pair degree: " << graph->maxpairdegree() << endl;
    std::cout << "hopping distance: " << graph->hopdist() << endl;
    std::cout << "simulation box size: " << graph->simboxsize() << endl;
    if(eventinfo->device > 0) {
        std::cout << "number of left electrode injector nodes " << graph->left()->links().size() << endl;
        std::cout << "number of right electrode injector nodes " << graph->right()->links().size() << endl;
    }
    std::cout << "number of nodes " << graph->Numberofnodes() << endl;
    std::cout << "set potential bias is " << eventinfo->voltage << " resulting in a linear electric field of " << eventinfo->efield_x << "V/nm" << endl;
    
    longrange = new Longrange(graph,eventinfo);
    if(eventinfo->longrange_slab) longrange->Initialize_slab(graph,eventinfo);
    else longrange->Initialize(eventinfo);
   
    std::cout << "longrange profile initialized" << endl;
    std::cout << "used calculation method for longrange potential: ";
    if(eventinfo->longrange_slab) { std::cout << "slab-based" << endl;} else { std::cout << "plane-based" << endl;}    

    std::cout << endl;
    std::cout << "number of nodes per layer" << endl;
    for(int it= 0; it< eventinfo->number_layers; it++ ){
        std::cout << longrange->number_of_nodes(it) << " ";
    }
    std::cout << endl;    
    
    state = new StateReservoir();
    state->InitStateReservoir();
    std::cout << "state reservoir object initialized" << endl;
    
    site_inject_probs = new Bsumtree();
    site_inject_probs->initialize(graph->Numberofnodes());
    std::cout << "Random node injector tree initialized" << endl;

    //Random charge distribution
    int nrholes = 0;
    int nrelectrons = 0;
    
    if(eventinfo->init_charges) {
        if(eventinfo->int_charge_readout) {nrholes = eventinfo->nr_holes; nrelectrons = eventinfo->nr_electrons;}
        else {nrholes = eventinfo->ho_density*graph->Numberofnodes(); nrelectrons = eventinfo->el_density*graph->Numberofnodes();}

        if(nrholes!=0 && nrelectrons != 0) { std::cout << "Double carrier simulation" << endl;} 
        else { std::cout << "Single carrier simulation" << endl;} 
        if(nrholes+nrelectrons>graph->Numberofnodes()){ std::cout<< "WARNING: number of electrons and holes is larger than the number of available nodes" << endl;}
        if(nrholes+nrelectrons == 0) { std::cout << "No charges are initialized";}    
    
        state->Random_init_injection(nrelectrons, nrholes, site_inject_probs, graph, eventinfo, randomvariable);
        std::cout << "randomly placed : " << nrelectrons + nrholes << " charges of which " << nrelectrons << " electrons and " << nrholes << " holes" << endl;
    }
    else {
        std::cout << "no charges injected" << endl;
    }
    
    if(state->ReservoirEmpty()) state->Grow(eventinfo->growsize, eventinfo->maxpairdegree);
    std::cout << "state grown after random charge injection" << endl;
    
    non_injection_rates = new Bsumtree();
    left_injection_rates = new Bsumtree();
    right_injection_rates = new Bsumtree();
    std::cout << "rate binary tree initialized" << endl;
    
    events = new Events();
    if(eventinfo->coulomb_strength > 0.0) {
        events->Init_non_injection_meshes(eventinfo);
        std::cout << "mesh structure for Coulomb interaction calculations initialized" << endl;
    }
    events->Initialize_device_eventvector(graph,state,longrange,eventinfo);
    std::cout << "Initialize (device) eventvector" << endl;
    events->Initialize_rates(non_injection_rates, left_injection_rates, right_injection_rates, eventinfo);
    std::cout << "Fill rate binary trees" << endl;
    if(eventinfo->coulomb_strength > 0.0) {
        events->Init_injection_meshes(state, eventinfo);
        std::cout << "mesh structure for injection potential calculations initialized" << endl;
    }
    events->Initialize_after_charge_placement(graph,state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventinfo);
    std::cout << "Initialize event and rates object after initial placement of charges" << endl;
    
    vssmgroup = new Vssmgroup();
    std::cout << "vssm group initialized" << endl;

    numoutput = new Numoutput();
    numoutput->Initialize();
    std::cout << "output object initialized" << endl;

}

bool Jdevice::EvaluateFrame() {
    
    // register all QM packages (Gaussian, turbomole, etc))
    // EventFactory::RegisterAll(); 
        
    RunKMC();
    delete randomvariable;
    delete state;
    delete events;
    delete graph;
    delete eventinfo;
    delete longrange;
    delete vssmgroup;
    delete non_injection_rates;
    delete left_injection_rates;
    delete right_injection_rates;
    delete numoutput;
    delete site_inject_probs;
    exit(0);
}

void Jdevice::RunKMC() {
    
    // to check whether anti repeating methods are useful
    if(eventinfo->repeat_counting) numoutput->Repeat_count_init();
    
    // convergence criteria
    bool direct_iv_convergence = false;
    bool direct_reco_convergence = false;
    int direct_iv_counter = 0; //if the convergence criterium is counted ten times in a row, result is converged
    int direct_reco_counter = 0;

    std::cout << "total link x distance : " << graph->total_link_distance_x() << endl;
    std::cout << "average hole site energy : " << eventinfo->avholeenergy << endl;
    std::cout << "average electron site energy : " << eventinfo->avelectronenergy << endl; 
    std::cout << "standard deviation of hole site energies: " << graph->stddev_hole_node_energy() << endl;
    std::cout << "standard deviation of electron site energies: " << graph->stddev_electron_node_energy() << endl;
    std::cout << endl;
    std::cout << "===================================================" << endl;
    std::cout << "= Start of device simulation                      =" << endl;
    std::cout << "===================================================" << endl;
    std::cout << endl;
    
    if(eventinfo->viz_store)  numoutput->Init_visualisation(graph, eventinfo);
    
    sim_time = 0.0;
    for (long it = 0; it < 2*eventinfo->nr_equilsteps + eventinfo->nr_timesteps; it++) {
        
        if(ldiv(it, eventinfo->steps_update_longrange).rem == 0 && it>0){
            if(eventinfo->longrange_slab) longrange->Update_cache_slab(graph,eventinfo);
            else                          longrange->Update_cache(eventinfo);

            events->Recompute_all_events(state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventinfo);
        }

        vssmgroup->Recompute_device(non_injection_rates, left_injection_rates, right_injection_rates);

        double timestep = vssmgroup->Timestep(randomvariable);
        sim_time += timestep;
        
        Event* chosenevent;
        chosenevent = vssmgroup->Choose_event_device(events, non_injection_rates, left_injection_rates, right_injection_rates, randomvariable);
        
        if(eventinfo->viz_store && it <= eventinfo->viz_nr_timesteps) numoutput->Update_visualisation(chosenevent);
        if(eventinfo->viz_store && it == eventinfo->viz_nr_timesteps) numoutput->Print_visualisation();

        // check for direct repeats
        if(eventinfo->repeat_counting) numoutput->Repeat_count_update(chosenevent);
        
        numoutput->Update(chosenevent, sim_time, timestep); 
        events->On_execute(chosenevent, graph, state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventinfo);

        // equilibration
   
        if(it == eventinfo->nr_equilsteps || it == 2*eventinfo->nr_equilsteps) 
        {
            numoutput->Init_convergence_check(sim_time); 
            numoutput->Initialize_equilibrate();
            sim_time = 0.0;
        }
        
        // convergence checking
        
        if(ldiv(it,eventinfo->nr_reportsteps).rem==0 && it> 2*eventinfo->nr_equilsteps) numoutput->Convergence_check(sim_time, eventinfo);

        // direct output
        
        if(ldiv(it,eventinfo->nr_reportsteps).rem==0){
            numoutput->Write(it, sim_time, timestep, eventinfo);
            std::cout << endl;
            
            if(eventinfo->write_charge_profile) 
            {
                std::cout << "charge profile (it = " << it << " )" << endl;
            
                for(int i =0; i< eventinfo->number_layers; i++) {
                    std::cout << longrange->Get_cached_density(i, eventinfo) << " ";
                }
                std::cout << endl;
                std::cout << endl;
            }
        }
        
        // break out of loop
        if(numoutput->iv_conv()) {break;}
        
    }

}

}}


#endif	/* __VOTCA_KMC_JDEVICE_H */
