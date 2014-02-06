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

#ifndef __VOTCA_KMC_STATEDEVICE_H_
#define __VOTCA_KMC_STATEDEVICE_H_

#include <vector>
#include <list>
#include <votca/tools/database.h>
#include <votca/tools/statement.h>
#include <votca/tools/vec.h>
#include <votca/kmc/state.h>
#include <votca/kmc/carrierdevice.h>
//include <votca/kmc/carrier.h>
//#include <votca/kmc/graphlattice.h>
//#include <votca/kmc/globaleventinfo.h>
//#include <votca/kmc/bsumtree.h>

namespace votca { namespace kmc {

class StateDevice : public State<GraphDevice, CarrierDevice> {
    
public:

    /// clear the carriers vector and the carrier reservoir and set length of both vectors equal to zero
    void InitStateDevice();
    
    /// load a state store file
    void LoadStateDevice();
    
    /// save a state store file
    void SaveStateDevice();
    
    /// initialize interactions (set to 0)
    void InitInteractions(int pre_size, int post_size);
    
    /// clear reservoir
    void InitReservoir() {carrier_reservoir.clear();}
    
    /// Buying/Selling of carrier numbers from the reservoir
    unsigned int Buy();
    void Sell(unsigned int remove_from_sim_box);
    
    /// Growing of carrier and reservoir vector
    void Grow(unsigned int nr_new_carriers, int maxpairdegree);
    
    /// Print carrier list (for debugging)
    void PrintDevice(std::ostream& out);
    
    bool ReservoirEmpty(){return carrier_reservoir.empty();}
    
private:

    vector<int> carrier_reservoir;
    
};

void StateDevice::InitStateDevice(){
    this->InitState();
    InitReservoir();
}

void StateDevice::LoadStateDevice(){
    
    // Initializes the Coulomb interactions
    
    int pre_carrier_size = this->GetCarrierSize();
    this->Load(const char* filename, GraphDevice* graph);
    int post_carrier_size = this->GetCarrierSize();

    this->InitInteractions(pre_carrier_size,post_carrier_size);
}

void StateDevice::InitInteractions(int pre_carrier_size,int post_carrier_size){

    for (unsigned int i=pre_carrier_size; i < post_carrier_size; i++) {
        CarrierDevice* probecarrier = this->_carriers[i];
        probecarrier->Init_to_Coulomb(maxpairdegree);
        probecarrier->Set_from_Coulomb(0.0);        
    }
}


unsigned int StateDevice::Buy() {
    
    unsigned int carriernr_to_sim_box = carrier_reservoir.back();
    carrier_reservoir.pop_back();
    CarrierDevice* newcarrier = this->GetCarrier(carriernr_to_sim_box);
    newcarrier->SetInBox(true);
    return carriernr_to_sim_box;
}

void StateDevice::Sell(unsigned int remove_from_sim_box) {
    
    carrier_reservoir.push_back(remove_from_sim_box);
    this->GetCarrier(remove_from_sim_box)->SetInBox(false);
    this->GetCarrier(remove_from_sim_box)->SetCarrierType((int) Reservoir);
    this->GetCarrier(remove_from_sim_box)->Reset_to_Coulomb();
    this->GetCarrier(remove_from_sim_box)->Set_from_Coulomb(0.0);
}

void StateDevice::Grow(unsigned int nr_new_carriers, int maxpairdegree) {
    
    unsigned int new_nr_carriers = this->GetCarrierSize() + nr_new_carriers;
    for (unsigned int i=this->GetCarrierSize(); i<new_nr_carriers; i++) {

        CarrierDevice* newcarrier = this->AddCarrier(i);         
        carrier_reservoir.push_back(i);
        newcarrier->SetInBox(false);
        newcarrier->SetDistance(votca::tools::vec(0.0,0.0,0.0)); //initialize the travelled distance vector
        newcarrier->SetCarrierType((int) Reservoir); //set carrier in reservoir
        newcarrier->Init_to_Coulomb(maxpairdegree);
        newcarrier->Set_from_Coulomb(0.0);
    }
}

void StateDevice::PrintDevice(std::ostream& out) {
    
    this->Print(out);
    std::cout << endl;
    std::cout << "reservoir indices: ";
    typename std::vector<int>::iterator it;   
    for(it = carrier_reservoir.begin(); it != carrier_reservoir.end(); it++) { 

        std::cout << (*it) << " ";
    }
    std::cout << endl;
}

}} 

#endif

