#include <votca/ctp/fragment.h>

using namespace std;
namespace votca { namespace ctp {

Fragment::~Fragment() {
    vector < Atom* > ::iterator atmit;
    for (atmit = this->Atoms().begin();
          atmit < this->Atoms().end();
          atmit++) {
          delete *atmit;
    }
    _weights.clear();
    _atoms.clear();
}


void Fragment::AddAtom(Atom* atom) {
    _atoms.push_back( atom );
    atom->setFragment(this);
    _weights.push_back( atom->getWeight() );
}

void Fragment::Rotate(matrix spin, vec refPos) {
    vector <Atom*> ::iterator ait;
    for (ait = _atoms.begin(); ait < _atoms.end(); ait++) {
        vec dir = (*ait)->getQMPos() - refPos;
        dir = spin * dir;
        vec pos = refPos + dir;
        (*ait)->setQMPos(pos);        
    }
}

void Fragment::Translate(vec shift) {
    vector <Atom*> ::iterator ait;
    for (ait = _atoms.begin(); ait < _atoms.end(); ait++) {
        vec newQMPos = (*ait)->getQMPos() + shift;
        (*ait)->setQMPos(newQMPos);
    }
    this->calcPos("QM");
}

void Fragment::calcPos(string tag) {
    vec pos = vec(0,0,0);
    double totWeight = 0.0;

    for (int i = 0; i< _atoms.size(); i++) {
        if (tag == "MD") {
            pos += _atoms[i]->getPos() * _atoms[i]->getWeight();
        }
        else if (tag == "QM") {
            pos += _atoms[i]->getQMPos() * _atoms[i]->getWeight();
        }
        totWeight += _atoms[i]->getWeight();
    }

    if (tag == "MD") {
        _CoMD = pos / totWeight;
    }
    else if (tag == "QM") {
        _CoQM = pos / totWeight;
    }


}



void Fragment::Rigidify(bool Auto) {


    // +++++++++++++++++++++++++++++++++++++++++ //
    // Establish reference atoms for local frame //
    // +++++++++++++++++++++++++++++++++++++++++ //
    
    vector<Atom*> trihedron;
    
    if (Auto) {

        // Automated search for suitable atoms
        bool enough = false;
        vector< Atom* > ::iterator ait = this->Atoms().begin();

        while (!enough) {

            if (ait == this->Atoms().end()) { break; }
            Atom *atm = *ait;
            ait++;

            // When specifying mapping weights for rigid fragments,
            // mobile atoms (whose position can vary significantly with
            // respect to the rigid plane, for instance) should have a weight 0.
            // We do not want to pick those to establish the local frame.
            if (atm->getWeight() == 0.0) { continue; }
            else {
                trihedron.push_back(atm);
                if (trihedron.size() == 3) { enough = true; }
            }
        }
    }
    else {

        // Take atoms specified in mapping file <= DEFAULT
        vector<Atom*> ::iterator ait;
        for (ait = _atoms.begin(); ait < _atoms.end(); ait++) {
            if ( ! (*ait)->HasQMPart() ) { continue; }

            if ( (*ait)->getQMId() == _trihedron[0] ||
                 (*ait)->getQMId() == _trihedron[1] ||
                 (*ait)->getQMId() == _trihedron[2] ) {
                Atom *atm = *ait;
                trihedron.push_back(atm);
            }
        }
    }

    int _symmetry = trihedron.size();

    // +++++++++++++++++++++++ //
    // Construct trihedra axes //
    // +++++++++++++++++++++++ //

    vec xMD, yMD, zMD;
    vec xQM, yQM, zQM;

    if (_symmetry == 3) {

        vec r1MD = trihedron[0]->getPos();
        vec r2MD = trihedron[1]->getPos();
        vec r3MD = trihedron[2]->getPos();
        vec r1QM = trihedron[0]->getQMPos();
        vec r2QM = trihedron[1]->getQMPos();
        vec r3QM = trihedron[2]->getQMPos();

        xMD = r2MD - r1MD;
        yMD = r3MD - r1MD;
        xQM = r2QM - r1QM;
        yQM = r3QM - r1QM;

        zMD = xMD ^ yMD;
        zQM = xQM ^ yQM;

        yMD = zMD ^ xMD;
        yQM = zQM ^ xQM;

        xMD = xMD.normalize();
        yMD = yMD.normalize();
        zMD = zMD.normalize();
        xQM = xQM.normalize();
        yQM = yQM.normalize();
        zQM = zQM.normalize();

    }

    else if (_symmetry = 2) {

        vec r1MD = trihedron[0]->getPos();
        vec r2MD = trihedron[1]->getPos();
        vec r1QM = trihedron[0]->getQMPos();
        vec r2QM = trihedron[1]->getQMPos();

        xMD = r2MD - r1MD;
        xQM = r2QM - r1QM;

        // Normalising not necessary, but recommendable, when doing...
        xMD = xMD.normalize();
        xQM = xQM.normalize();

        vec yMDtmp = vec(0,0,0);
        vec yQMtmp = vec(0,0,0);

        // ... this: Check whether one of the components is equal or close to
        // zero. If so, this easily gives a second leg for the trihedron.
        if      ( xMD.getX()*xMD.getX() < 1e-6 ) { yMDtmp = vec(1,0,0); }
        else if ( xMD.getY()*xMD.getY() < 1e-6 ) { yMDtmp = vec(0,1,0); }
        else if ( xMD.getZ()*xMD.getZ() < 1e-6 ) { yMDtmp = vec(0,0,1); }
        if      ( xQM.getX()*xQM.getX() < 1e-6 ) { yQMtmp = vec(1,0,0); }
        else if ( xQM.getY()*xQM.getY() < 1e-6 ) { yQMtmp = vec(0,1,0); }
        else if ( xQM.getZ()*xQM.getZ() < 1e-6 ) { yQMtmp = vec(0,0,1); }

        if ( abs(yMDtmp) < 0.5 ) {
           // All components of xMD are unequal to zero => division is safe.
           // Choose vector from plane with xMD * inPlaneVec = 0:
           double tmp_x = 1.;
           double tmp_y = 1.;
           double tmp_z = 1/xMD.getZ() * (-xMD.getX()*tmp_x - xMD.getY()*tmp_y);
           yMDtmp = vec(tmp_x, tmp_y, tmp_z);
           yMDtmp.normalize();
        }
        if ( abs(yQMtmp) < 0.5 ) {
           double tmp_x = 1.;
           double tmp_y = 1.;
           double tmp_z = 1/xQM.getZ() * (-xQM.getX()*tmp_x - xQM.getY()*tmp_y);
           yQMtmp = vec(tmp_x, tmp_y, tmp_z);
           yQMtmp.normalize();
        }

        // Now proceed as for symmetry 3
        zMD = xMD ^ yMDtmp;
        yMD = zMD ^ xMD;
        zQM = xQM ^ yQMtmp;
        yQM = zQM ^ xQM;

        xMD.normalize();
        yMD.normalize();
        zMD.normalize();
        xQM.normalize();
        yQM.normalize();
        zQM.normalize();

    }

    else if (_symmetry = 1) {

        xMD = vec(1,0,0);
        yMD = vec(0,1,0);
        zMD = vec(0,0,1);
        xQM = vec(1,0,0);
        yQM = vec(0,1,0);
        zQM = vec(0,0,1);

    }
    
    // +++++++++++++++++ //
    // Rotation matrices //
    // +++++++++++++++++ //
    
    matrix rotMD = matrix(xMD, yMD, zMD);
    matrix rotQM = matrix(xQM, yQM, zQM);

    matrix rotateQM2MD = rotMD * rotQM.Transpose();
    _rotateQM2MD = rotateQM2MD;

    // ++++++++++++++++++ //
    // Translation vector //
    // ++++++++++++++++++ //

    this->calcPos("QM");    
    vec translateQM2MD = _CoMD - _CoQM;
    _translateQM2MD = translateQM2MD;

    // Update QM positions
    this->Translate(translateQM2MD);
    this->Rotate(rotateQM2MD, _CoQM);

}

}}