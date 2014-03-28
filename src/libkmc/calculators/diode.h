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

#ifndef __VOTCA_KMC_DIODE_H
#define	__VOTCA_KMC_DIODE_H

#include <iostream>

#include <votca/kmc/vssmgroup.h>
#include <votca/kmc/graphdevice.h>
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
   
class Diode : public KMCCalculator 
{
public:
    
    votca::tools::Random2 *RandomVariable;
    GraphDevice* graph;
    StateDevice* state;
    Events* events;
    Vssmgroup* vssmgroup;
    Vssmgroup* left_chargegroup;
    Vssmgroup* right_chargegroup;
    Eventinfo* eventdata;
    Longrange* longrange;
    Bsumtree* non_injection_rates;
    Bsumtree* left_injection_rates;
    Bsumtree* right_injection_rates;
    Bsumtree* site_inject_probs;

    Numoutput* numoutput;
    
    Diode() {};
   ~Diode() {};

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


void Diode::Initialize(const char *filename, Property *options, const char *outputfile) {

    //Setup random number generator
    srand(eventdata->seed); // srand expects any integer in order to initialise the random number generator
    RandomVariable = new votca::tools::Random2();
    RandomVariable->init(rand(), rand(), rand(), rand());     
    
    cout << "Initializing" << endl;
    
    eventdata = new Eventinfo();
    eventdata->Read(options);
    
    graph = new GraphDevice();
    graph->Initialize(filename);
    // form a histogram of the site energies
    
    graph->Setup_device_graph(eventdata->left_electrode_distance, eventdata->right_electrode_distance, true, eventdata);
    
//    std::cout << "min " << graph->min_hole_node_energy() << " max " << graph->max_hole_node_energy() << " av " << graph->Av_hole_node_energy() << endl;
    
    eventdata->Graph_Parameters(graph->hopdist(), graph->mindist(), graph->simboxsize(), graph->maxpairdegree(),graph->Av_hole_node_energy(), graph->Av_electron_node_energy());
    eventdata->Set_field(); // convert voltage to electric field

    std::cout << "graph initialized" << endl;
    std::cout << "max pair degree: " << graph->maxpairdegree() << endl;
    std::cout << "hopping distance: " << graph->hopdist() << endl;
    std::cout << "simulation box size: " << graph->simboxsize() << endl;
    std::cout << "number of left electrode injector nodes " << graph->left()->links().size() << endl;
    std::cout << "number of right electrode injector nodes " << graph->right()->links().size() << endl;
    std::cout << "number of nodes" << graph->Numberofnodes() << endl;    
    
    longrange = new Longrange(graph,eventdata);
    if(eventdata->longrange_slab) longrange->Initialize_slab(graph,eventdata);
    else longrange->Initialize(eventdata);
    
    std::cout << "longrange profile initialized" << endl;

    state = new StateDevice();
    state->InitStateDevice();
    
    site_inject_probs = new Bsumtree();
    // site_inject_probs->initialize(graph->Numberofnodes()); // take care of electrode nodes
    // state->Random_init_injection((int) Hole, site_inject_probs, graph, eventdata, RandomVariable);
        
    if(state->ReservoirEmpty()) state->Grow(eventdata->growsize, eventdata->maxpairdegree);
    
    std::cout << "state initialized" << endl;
    
    non_injection_rates = new Bsumtree();
    left_injection_rates = new Bsumtree();
    right_injection_rates = new Bsumtree();

    std::cout << "binary tree structures initialized" << endl;

    events = new Events();
    events->Init_non_injection_meshes(eventdata);
    events->Initialize_eventvector(graph,state,longrange,eventdata);
    events->Initialize_rates(non_injection_rates, left_injection_rates, right_injection_rates,eventdata);
    events->Init_injection_meshes(state, eventdata);
    events->Initialize_after_charge_placement(graph,state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventdata);

    std::cout << "event vectors and meshes initialized" << endl;

    vssmgroup = new Vssmgroup();
    left_chargegroup = new Vssmgroup();
    right_chargegroup = new Vssmgroup();

    numoutput = new Numoutput();
    numoutput->Initialize();

}

bool Diode::EvaluateFrame() {
    
    // register all QM packages (Gaussian, turbomole, etc))
    // EventFactory::RegisterAll(); 
        
    RunKMC();
    delete RandomVariable;
    delete state;
    delete events;
    delete graph;
    delete eventdata;
    delete longrange;
    delete vssmgroup;
    delete left_chargegroup;
    delete right_chargegroup;
    delete non_injection_rates;
    delete left_injection_rates;
    delete right_injection_rates;
    delete numoutput;
    delete site_inject_probs;
    exit(0);
}

void Diode::RunKMC() {
    
    // to check whether anti repeating methods are useful
    int repeat_counter = 0; 
    int old_from_node_id = -10;
    int old_to_node_id = -10;
    
    // convergence criteria
    bool direct_iv_convergence = false;
    bool direct_reco_convergence = false;
    int direct_iv_counter = 0; //if the convergence criterium is counted ten times in a row, result is converged
    int direct_reco_counter = 0;

    int numcharges_distrib;
    std::cout << graph->avrate() << endl;
    
    for(int it= 0; it< eventdata->number_layers; it++ ){
        std::cout << longrange->number_of_nodes(it) << " ";
    }
    std::cout << endl;
    
    vector<double> layercurrent;
    for(int it= 0; it< eventdata->number_layers; it++ ){
        layercurrent.push_back(0.0);
    }    
    
    for(int i = 0; i<graph->Numberofnodes(); i++) {
        for (int it = 0; it < graph->GetNode(i)->links().size(); it++ ) {
            graph->GetNode(i)->links()[it]->setcount(0); 
 //           std::cout << i << " " << it << " " << graph->GetNode(i)->links()[it]->id() << " " << graph->GetNode(i)->links()[it]->count() << endl;

        }
    }     
    
    std::cout << "total link x distance : " << graph->totdistancex() << endl;
    
    sim_time = 0.0;
    for (long it = 0; it < 2*eventdata->nr_equilsteps + eventdata->nr_timesteps; it++) {
//     for (long it = 0; it < 1; it++) {
    // Update longrange cache (expensive, so not done at every timestep)

        if(eventdata->device == 2) {
            // make sure the number of carriers on the left equals
            left_chargegroup->Recompute_injection(left_injection_rates);
            numcharges_distrib = floor(left_chargegroup->totprobsum()+0.5);
            if(numcharges_distrib == 0 && numoutput->holes() == 0) { numcharges_distrib = 1;} // at least one charge per time in the device
//            std::cout << "numcharges left " << left_chargegroup->totprobsum() << endl;
            
            while(numcharges_distrib > 0) { // need to fill nodes
                Event* chosencharge = left_chargegroup->Choose_injection_event(events, 0, left_injection_rates, RandomVariable);
                numoutput->Update_ohmic(chosencharge);
                events->On_execute(chosencharge, graph, state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventdata);
                left_chargegroup->Recompute_injection(left_injection_rates);
                numcharges_distrib = floor(left_chargegroup->totprobsum()+0.5);
//            std::cout << "numcharges left " << it << " " << left_chargegroup->totprobsum() << endl;

            }

            // make sure the number of carriers on the left equals
            right_chargegroup->Recompute_injection(right_injection_rates);
            numcharges_distrib = floor(right_chargegroup->totprobsum()+0.5);
 //           std::cout << "numcharges right " << right_chargegroup->totprobsum() << endl;

            // preference given to the major injecting electrode
            while(numcharges_distrib > 0) { // need to fill nodes
                Event* chosencharge = right_chargegroup->Choose_injection_event(events, 1, right_injection_rates, RandomVariable);
                numoutput->Update_ohmic(chosencharge);
                events->On_execute(chosencharge, graph, state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventdata);
                right_chargegroup->Recompute_injection(right_injection_rates);
                numcharges_distrib = floor(right_chargegroup->totprobsum()+0.5);
//           std::cout << "numcharges right " << right_chargegroup->totprobsum() << endl;

            }
        }

        if(ldiv(it, eventdata->steps_update_longrange).rem == 0 && it>0){
            if(eventdata->longrange_slab) longrange->Update_cache_slab(graph,eventdata);
            else                          longrange->Update_cache(eventdata);
            events->Recompute_all_events(state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventdata);
        }
        if(eventdata->device == 1) vssmgroup->Recompute_device(non_injection_rates, left_injection_rates, right_injection_rates);
        if(eventdata->device == 2) vssmgroup->Recompute_bulk(non_injection_rates);

        double timestep = vssmgroup->Timestep(RandomVariable);
        sim_time += timestep;

        Event* chosenevent;
        if(eventdata->device == 1) chosenevent = vssmgroup->Choose_event_device(events, non_injection_rates, left_injection_rates, right_injection_rates, RandomVariable);
        if(eventdata->device == 2) chosenevent = vssmgroup->Choose_event_bulk(events, non_injection_rates, RandomVariable);

        Link* count_link = chosenevent->link();
        int node1_id = count_link->node1()->id();
        Node* node1 = count_link->node1();
        Node* node2 = count_link->node2();
        votca::tools::vec node1_pos = node1->position();
        votca::tools::vec node2_pos = node2->position();

        if(node1->type() == (int) NormalNode) {
            if(node1_pos.x() < node2_pos.x()) {
                votca::tools::vec travelvec = chosenevent->link()->r12();
                count_link->incval(travelvec.x());
                layercurrent[dynamic_cast<NodeDevice*>(node1)->layer()] += travelvec.x();
            }
            else {
    //            for (int j = 0; j < node2->links().size(); j++ ) {
    //                if(node2->links()[j]->node2()->id() == node1_id) {
    //                    node2->links()[j]->deccount();    
    //                }
    //            }
            }
        }
        
        double maxcount = 0.0;

        if(it == 2*eventdata->nr_equilsteps + 1500000) {
//        if(it == 88888) {

            ofstream curstore;
            curstore.open("curdens"); 
            
            curstore << "{";
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                if(graph->GetNode(i)->type() == (int) NormalNode){ 
                    for (int j = 0; j < graph->GetNode(i)->links().size(); j++ ) {  
                        votca::tools::vec n1pos = graph->GetNode(i)->links()[j]->node1()->position();
                        votca::tools::vec n2pos = graph->GetNode(i)->links()[j]->node2()->position();
                        double count =  graph->GetNode(i)->links()[j]->count()/layercurrent[dynamic_cast<NodeDevice*>(node1)->layer()];
                        if(count!=0) {
                            if(graph->GetNode(i)->type() != NormalNode) {
//                                curstore << "Cylinder[{{" << n1pos.x() << "," << n2pos.y() << "," << n2pos.z() << "},{" << n2pos.x() << "," << n2pos.y() << "," << n2pos.z() << "}}," << count+0.5 << "/C],";                    
                            }
                            else if(graph->GetNode(i)->links()[j]->node2()->type() != NormalNode) {
                                curstore << "Tube[{{" << n1pos.x() << "," << n1pos.y() << "," << n1pos.z() << "},{" << n2pos.x() << "," << n1pos.y() << "," << n1pos.z() << "}}," << count << "/C],";                    
                            }
                            else {
                                curstore << "Tube[{{" << n1pos.x() << "," << n1pos.y() << "," << n1pos.z() << "},{" << n2pos.x() << "," << n2pos.y() << "," << n2pos.z() << "}}," << count << "/C],";                    
                                if(maxcount<count) maxcount = count;
                            }
                        }
                        if(count!= 0) std::cout << count << endl;
                    }
                }
            }         
            curstore << "maxcount " << maxcount << endl;
        }

//        if(it > eventdata->nr_equilsteps) {
/*        if(it > eventdata->nr_equilsteps) {
            Node* node1 = chosenevent->link()->node1();
            Node* node2 = chosenevent->link()->node2();
            votca::tools::vec dir = chosenevent->link()->r12();
            if(node1->type() == (int) NormalNode) node1->Add_velx(dir.x(),timestep); node1->Add_vely(dir.y(),timestep); node1->Add_velz(dir.z(),timestep);
//            if(node2->type() == (int) NormalNode) node2->Add_velx(-dir.x(),timestep); node2->Add_vely(-dir.y(),timestep); node2->Add_velz(-dir.z(),timestep);
//            std::cout << node1->vel_x() << " " << node2->vel_x() << " " << node1->vel_y() << " " << node2->vel_y() << " " << node1->vel_z() << " " << node2->vel_z() << endl;
        }*/
         
/*        if(it == 10*eventdata->nr_equilsteps) {
//          if(it == 10) {
            ofstream tempstore;
            const char* filename = "test";
            tempstore.open(filename);
            tempstore << "{";
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                Node* nodeprob = graph->GetNode(i);
                votca::tools::vec nodepos = nodeprob->position();
                tempstore << "{" << nodepos.x() << "," << nodepos.y() << "," << nodepos.z() << "},";
            }
            tempstore << "}";
            tempstore << endl;
            tempstore << "{";
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                Node* nodeprob = graph->GetNode(i);
                tempstore <<  nodeprob->vel_x()/sim_time << ","; 
            }                    
            tempstore << "}";
            tempstore << endl;
            tempstore << "{";
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                Node* nodeprob = graph->GetNode(i);
                tempstore <<  nodeprob->vel_y()/sim_time << ","; 
            } 
            tempstore << "}";
            tempstore << endl;
            tempstore << "{";
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                Node* nodeprob = graph->GetNode(i);
                tempstore <<  nodeprob->vel_z()/sim_time << ","; 
            } 
        }*/
        
        numoutput->Update(chosenevent, sim_time, timestep); 

        events->On_execute(chosenevent, graph, state, longrange, non_injection_rates, left_injection_rates, right_injection_rates, eventdata);

        // check for direct repeats
        
        int goto_node_id = chosenevent->link()->node2()->id();
        int from_node_id = chosenevent->link()->node1()->id();
        if(goto_node_id == old_from_node_id && from_node_id == old_to_node_id) repeat_counter++;
        old_from_node_id = from_node_id;
        old_to_node_id = goto_node_id;

        if(it == eventdata->nr_equilsteps || it == 2*eventdata->nr_equilsteps) numoutput->Init_convergence_check(sim_time);
        
        // equilibration
   
        if(it == eventdata->nr_equilsteps || it == 2*eventdata->nr_equilsteps) {
            numoutput->Initialize_equilibrate();
            sim_time = 0.0;
            for(int i = 0; i<graph->Numberofnodes(); i++) {
                for (int j = 0; j < graph->GetNode(i)->links().size(); j++ ) {  
                   graph->GetNode(i)->links()[j]->setcount(0.0); 
                }
            }
            layercurrent.clear();
            for(int j= 0; j< eventdata->number_layers; j++ ){
                layercurrent.push_back(0.0);
            }  
        }
    
        // convergence checking
        
        if(ldiv(it,10000).rem==0 && it> 2*eventdata->nr_equilsteps) numoutput->Convergence_check(sim_time, eventdata);

        // direct output
        if(ldiv(it,10000).rem==0){
            std::cout << it << " " << repeat_counter << " " << 
                         numoutput->iv_conv() << " " << numoutput->iv_count() << " " << 
                         numoutput->reco_conv() << " " << numoutput->reco_count() <<  " " << 
                         sim_time << " " << timestep << " " << vssmgroup->totprobsum() << " "  << vssmgroup->noninjectprobsum() << " "  << vssmgroup->leftinjectprobsum() << " "  << vssmgroup->rightinjectprobsum() << " " ;
            numoutput->Write(sim_time);
            std::cout << endl;
           /* std::cout << "position" << endl;
           
            for(int i =0; i< eventdata->number_layers; i++) {
                std::cout << longrange->position(i) << " ";
            }
            
            std::cout << endl;
            std::cout << "charge" << endl;
           
            for(int i =0; i< eventdata->number_layers; i++) {
                std::cout << longrange->Get_cached_density(i, eventdata) << " ";
            }
            
            std::cout << endl;
            std::cout << "pot" << endl;
           
            for(int i =0; i< eventdata->number_layers; i++) {
                std::cout << longrange->Get_cached_longrange(i) << " ";
            }            
            std::cout << endl;*/
        }
        
        // break out of loop
        if(numoutput->iv_conv() && numoutput->reco_conv()) {break;}
        
    }

}

}}


#endif	/* __VOTCA_KMC_DIODE_H */
