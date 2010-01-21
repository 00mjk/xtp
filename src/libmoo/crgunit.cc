/* 
 * File:   crgunit.cc
 * Author: ruehle
 *
 * Created on February 24, 2009, 1:59 PM
 */

#include "crgunit.h"

CrgUnit::~CrgUnit() {
    //   cout << "[crgunit.h]: Calling the crgunit destructor." << endl;
    _planes.clear();
    _positions.clear();
    _norms.clear();
}

CrgUnit::CrgUnit(vector <vec> positions, vector <vec> norms, vector <vec> planes,
        const unsigned int & id, CrgUnitType * type, const unsigned int & molId) {
    vector <vec>::iterator it_pos;
    vector <vec>::iterator it_norm;
    vector <vec>::iterator it_planes;
//    _com = vec(0., 0., 0.);
    double toll = 1E-6;
    vec xvec(1.0, 0., 0.);
    vec yvec(0., 1., 0.);
    for (it_pos = positions.begin(), it_norm = norms.begin(), it_planes = planes.begin()
            ; it_pos != positions.end(); ++it_pos, ++it_norm, ++it_planes) {
        _positions.push_back(*it_pos);
        if (abs(*it_norm) > toll && abs(*it_planes) > toll) {
            _norms.push_back(*it_norm);
            _planes.push_back(*it_planes);
        } else {
            _norms.push_back(xvec);
            _planes.push_back(yvec);
        }
//        _com = _com + *it_pos;

    }
//    _com /= positions.size();
    _molid = molId;
    _id = id;
    _type = type;

    // initialise variables derived from CrgUnitType
    _energy = _type -> GetEnergy();
}

void CrgUnit::copyCrgUnit(CrgUnit & acrg) {
    vector <vec>::iterator it_pos;
    vector <vec>::iterator it_norm;
    vector <vec>::iterator it_planes;
//    _com = acrg._com;
    _positions.clear();
    _norms.clear();
    _planes.clear();
    for (it_pos = (acrg._positions).begin(),
            it_norm = (acrg._norms).begin(),
            it_planes = (acrg._planes).begin()
            ; it_pos != (acrg._positions).end(); ++it_pos, ++it_norm, ++it_planes) {
        _positions.push_back(*it_pos);
        _norms.push_back(*it_norm);
        _planes.push_back(*it_planes);

    }
    _molid = acrg._molid;
    _id = acrg._id;
    _type = acrg._type;

    // initialise variables derived from CrgUnitType
    _energy = acrg._energy;

}

mol_and_orb * CrgUnit::rotate_translate_beads() {
    mol_and_orb *a = new mol_and_orb;
    basis_set *_indo = new basis_set;
    orb * _orb = new orb;
    a->define_bs(*_indo);
    a->cp_atompos(_type->GetCrgUnit());
    a->cp_atoms(_type->GetCrgUnit());
    _orb->init_orbitals_stripped(_type->GetOrb(), _type->GetTransOrbs().size());
    a->assign_orb(_orb);

    _type->rotate_each_bead(_positions.begin(), _norms.begin(),
            _planes.begin(), a);
    return a;
}

// this is the function called from the rate calculator on already initialised molecules

void CrgUnit::rot_two_mol(CrgUnit & two, mol_and_orb & mol1, mol_and_orb & mol2) {

    /*vector <vec> ::iterator  com1;
    vector <vec> ::iterator  com2;
    vector <vector < vec > > :: iterator pos1;
    vector <vector < vec > >:: iterator pos2;*/

    vector <vec>::iterator bestpos1;
    vector <vec>::iterator bestpos2;

    bestpos1 = _positions.begin();
    bestpos2 = two._positions.begin();

    /*double d = abs( two._com - _com );
        
    for ( com2 = (two._altcom).begin(), pos2 = (two._altpos).begin() ;
    com2 != (two._altcom).end() ; ++com2, ++pos2 ){
        vec displ = *com2 - _com;
        double d_t = abs( displ);
        if ( d_t < d ) {
            bestpos1 = _positions.begin();
            bestpos2 = pos2 -> begin();
            d=d_t;
        }
    }
        
    for ( com1 = (_altcom).begin(), pos1 = (_altpos).begin() ;
    com1 != (_altcom).end() ; ++com1, ++pos1 ){
        vec displ = two._com - *com1;
        double d_t = abs( displ);
        if ( d_t < d ) {
            bestpos1 = pos1 -> begin();
            bestpos2 = two._positions.begin();
            d=d_t;
        }
    }
        
    for ( com1 = _altcom.begin(), pos1 = _altpos.begin() ; 
    com1 != _altcom.end() ; ++com1, ++pos1 ){
        for ( com2 = (two._altcom).begin(), pos2 = (two._altpos).begin() ;
        com2 != (two._altcom).end() ; ++com2, ++pos2 ){
            vec displ = *com2 - *com1;
            double d_t = abs( displ);
            if ( d_t < d ) {
                bestpos1 = pos1 -> begin();
                bestpos2 = pos2 -> begin();
                d=d_t;
            }
        }
    }*/

    _type->rotate_each_bead(bestpos1, _norms.begin(), _planes.begin(), &mol1);
    (two._type)->rotate_each_bead(bestpos2, two._norms.begin(), two._planes.begin(), &mol2);
}

void CrgUnit::rotate(matrix mat) {
    vector <vec> ::iterator it_norm = _norms.begin();
    vector <vec> ::iterator it_plane = _planes.begin();
    for (; it_norm != _norms.end(); ++it_norm, ++it_plane) {
        vec a = *it_norm;
        vec b = *it_plane;
        *it_norm = mat * a;
        *it_plane = mat * b;
    }
    vec a;
//    vec a = mat * _com;
//    _com = a;
    vector < vec> ::iterator it_vec;
    /*for (it_vec = _altcom.begin() ; it_vec != _altcom.end(); ++it_vec) {
        a = *it_vec;
     *it_vec = mat * (*it_vec);
    }*/
    for (it_vec = _positions.begin(); it_vec != _positions.end(); ++it_vec) {
        a = *it_vec;
        *it_vec = mat * (*it_vec);
    }
    /*vector < vector < vec > > :: iterator it_altpos;
    for (it_altpos = _altpos.begin() ; it_altpos != _altpos.end() ; ++it_altpos){
        for (it_vec =  it_altpos->begin(); it_vec != it_altpos->end() ; ++it_vec ){
            a = *it_vec;
     *it_vec = mat * (*it_vec);
        }
    }*/
}
