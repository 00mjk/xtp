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

#ifndef _VOTCA_KMC_NODEDEVICE_H
#define	_VOTCA_KMC_NODEDEVICE_H

#include <votca/kmc/nodesql.h>
#include <votca/kmc/eventinfo.h>

namespace votca { namespace kmc {

class NodeDevice : public NodeSQL
{
public:
    
    NodeDevice(int id, tools::vec position) : NodeSQL(id, position){};

    /// Set layer index of node (defined by profile object)
    void setLayer(int layer){_layer = layer;}

    /// Set indices of first and last layers which are contributing to the long-range potential double counting algorithm 
    void setfirstcontriblayer(int layer){_firstcontriblayer = layer;}
    void setfinalcontriblayer(int layer){_finalcontriblayer = layer;}  
    
    /// Compute and set self image coulomb potential (potential of image charges  of a charge on the charge itself)
    void Compute_Self_Image_Coulomb_Potential(double startx, double device_length, Eventinfo* eventinfo);
    void setSelfImage(double self_image) {_self_image = self_image;}
    
    /// Self image coulomb potential (potential of image charges  of a charge on the charge itself)
    const double &self_image() const { return _self_image; }     
    
    const bool &injectable() const { return _injectable; }
    /// Layer index
    const int &layer() const {return _layer;}

    const int &firstcontriblayer() const {return _firstcontriblayer;}
    const int &finalcontriblayer() const {return _finalcontriblayer;} 
    double &contrib(int layer) {return disc_coul_contrib[layer];}
    
    const int &reco() const {return _reco_rate;}
    const double &hole_occ() const {return _hole_occ;}
    const double &el_occ() const {return _el_occ;}
    
    void Add_hole_occ(double occ) { _hole_occ += occ;}
    void Add_el_occ(double occ) {_el_occ += occ;}
    void Add_reco() {_reco_rate ++;}
    
    void disc_coul_clear() {disc_coul_contrib.clear();}
    void disc_coul_set(double val) {disc_coul_contrib.push_back(val); }
    int disc_coul_size() {return disc_coul_contrib.size();}
    
    void Set_injectable(bool injectable) { _injectable = injectable;}
    
private:

    int _layer;
    double _self_image;
    double _hole_occ;
    double _el_occ;
    int _reco_rate;
    
    bool _injectable;

    int _firstcontriblayer;
    int _finalcontriblayer;
    
    vector<double> disc_coul_contrib;
    
};

void NodeDevice::Compute_Self_Image_Coulomb_Potential(double startz, double device_length, Eventinfo* eventinfo) {

    double coulpot = 0.0;
    double L = device_length;
      
    double sign;
    double distz_1;
    double distz_2;
      
    for (int i=0;i<eventinfo->number_short_range_images; i++) {
        if (div(i,2).rem==0) { // even generation
            sign = -1.0;
            distz_1 = i*L + 2*startz;
            distz_2 = (i+2)*L - 2*startz; 
        }
        else { // odd generation
            sign = 1.0;
            distz_1 = (i+1)*L;
            distz_2 = (i+1)*L;
        }
        coulpot += sign*1.0/distz_1;
        coulpot += sign*1.0/distz_2;
    }
    _self_image = coulpot;    
}

}}

#endif	/* _VOTCA_KMC_NODEDEVICE_H */

