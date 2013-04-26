#include <votca/ctp/polarseg.h>


namespace votca { namespace ctp {

    
    
PolarSeg::PolarSeg(vector<APolarSite*> &psites) {    
    for (int i = 0; i < psites.size(); ++i) {
        push_back(psites[i]);
    }    
    this->CalcPos();
}
    
    
PolarSeg::~PolarSeg() {
   vector<APolarSite*> ::iterator pit;
   for (pit = begin(); pit < end(); ++pit) {         
       delete *pit;
   }
   clear();
}


void PolarSeg::CalcPos() {    
    _pos = vec(0,0,0);    
    for (int i = 0; i < this->size(); ++i) {        
        _pos += (*this)[i]->getPos();        
    }
    _pos /= double(this->size());      
}


void PolarSeg::Translate(const vec &shift) {    
    for (int i = 0; i < size(); ++i) {
        (*this)[i]->Translate(shift);
    }
    _pos += shift;
}



}}