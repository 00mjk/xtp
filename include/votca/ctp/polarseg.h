#ifndef __POLARSEG__H
#define	__POLARSEG__H

#include <votca/tools/vec.h>
#include <votca/ctp/apolarsite.h>
#include <votca/ctp/polarfrag.h>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/base_object.hpp>

namespace votca { namespace ctp {

class PolarNb;
    
class PolarSeg : public vector<APolarSite*>
{

public:

    PolarSeg() : _id(-1), _is_charged(true), _is_polarizable(true) {}
    PolarSeg(int id, vector<APolarSite*> &psites);
    explicit PolarSeg(PolarSeg *templ);
    explicit PolarSeg(int id) : _id(id) {}
   ~PolarSeg();

    const int &getId() { return _id; }
    const vec &getPos() { return _pos; }
    void setId(int id) { _id = id; }    
    
    // Polar fragments
    PolarFrag *AddFragment() { _pfrags.push_back(new PolarFrag(this, (int)_pfrags.size())); return _pfrags.back(); }
    vector<PolarFrag*> &PolarFrags() { return _pfrags; }
    // Local neighbor-list
    vector<PolarNb*> &PolarNbs() { return _nbs; }
    void ReservePolarNbs(int nbsize) { _nbs.reserve(nbsize); }
    void AddPolarNb(PolarSeg *pseg);
    void AddPolarNb(PolarNb *nb) { _nbs.push_back(nb); }
    void ClearPolarNbs();    
    // Position & total charge
    void Translate(const vec &shift);
    void CalcPos();    
    double CalcTotQ();
    void Coarsegrain();
    // Evaluates to "true" if ANY contained polar site has charge != 0
    void CalcIsCharged();
    bool IsCharged() { return _is_charged; }
    // Evaluates to "true" if ANY contained polar site has polarizability > 0
    void CalcIsPolarizable();
    bool IsPolarizable() { return _is_polarizable; }
    
    // File output methods
    void PrintPolarNbPDB(string outfile);
    void WriteMPS(string mpsfile, string tag="");    
    // Serialization interface
    template<class Archive>
    void serialize(Archive &arch, const unsigned int version) {
        arch & boost::serialization::base_object< vector<APolarSite*> >(*this);
        arch & _id;
        arch & _pos;
        arch & _is_charged;
        arch & _is_polarizable;
        return;
    }
    
private:

    int _id;
    vec _pos;
    bool _is_charged;
    bool _is_polarizable;
    vector<PolarFrag*> _pfrags;
    vector<PolarNb*> _nbs;


};


class PolarNb
{
public:
    // s22x = dr(nb,shift;pbc) - dr(nb,shift) + imageboxvector
    // Use s22x to obtain effective connection vector
    // dreff = top->ShortestConnect(ref,nb) + _pbcshift
    PolarNb(PolarSeg *nb, vec &dr12, vec &s22x) 
        : _nb(nb), _dr12(dr12), _s22x(s22x) {};
    PolarNb(PolarSeg *nb) : _nb(nb) {};
    PolarSeg *getNb() { return _nb; }
    vec &getR() { return _dr12; }
    vec &getS() { return _s22x; }
private:
    PolarSeg *_nb;
    vec _dr12;
    vec _s22x;
};


}}

#endif