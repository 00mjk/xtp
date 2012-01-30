/*
 * Copyright 2009-2011 The VOTCA Development Team (http://www.votca.org)
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

#include <votca/ctp/topology.h>
#include <votca/tools/globals.h>
#include <boost/lexical_cast.hpp>

namespace votca { namespace ctp {

Topology::Topology() : _db_id(-1), _hasPb(0), _bc(NULL) { }

// +++++++++++++++++++++ //
// Clean-Up, Destruct    //
// +++++++++++++++++++++ //

void Topology::CleanUp() {

    vector < Molecule* > ::iterator mit;
    for (mit = _molecules.begin(); mit < _molecules.end(); mit++) delete *mit;
    _molecules.clear();
    _segments.clear();
    _fragments.clear();
    _atoms.clear();
    if (_bc) { delete(_bc); _bc = NULL; }
    _bc = new CSG::OpenBox;
}


Topology::~Topology() {

    // clean up the list of molecules; this also deletes atoms
    vector < Molecule* > :: iterator molecule;
    for (molecule = _molecules.begin();
         molecule < _molecules.end();
         ++molecule) {
         delete *molecule;
    }
    _molecules.clear();
    _segments.clear();
    _fragments.clear();
    _atoms.clear();
}


// ++++++++ //
// Populate //
// ++++++++ //

Fragment *Topology::AddFragment(string fragment_name) {
    int fragment_id = _fragments.size() + 1;
    Fragment* fragment = new Fragment(fragment_id, fragment_name);
    _fragments.push_back(fragment);
    fragment->setTopology(this);
    return fragment;
}

Segment *Topology::AddSegment(string segment_name) {
    int segment_id = _segments.size() + 1;
    Segment* segment = new Segment(segment_id, segment_name);
    _segments.push_back(segment);
    segment->setTopology(this);
    return segment;
}

Atom *Topology::AddAtom(string atom_name) {
    int atom_id = _atoms.size() + 1;
    Atom *atom = new Atom(atom_id, atom_name);
    _atoms.push_back(atom);
    atom->setTopology(this);
    return atom;
}

Molecule *Topology::AddMolecule(string molecule_name) {
    int molecule_id = _molecules.size() + 1;
    Molecule *molecule = new Molecule(molecule_id, molecule_name);
    _molecules.push_back(molecule);
    molecule->setTopology(this);
    return molecule;
}



// +++++++++++++++++ //
// Periodic Boundary //
// +++++++++++++++++ //

void Topology::setBox(const matrix &box,
                      CSG::BoundaryCondition::eBoxtype boxtype) {

    // Determine box type automatically in case boxtype == typeAuto
    if(boxtype == CSG::BoundaryCondition::typeAuto) {
        boxtype = AutoDetectBoxType(box);
    }

    if(_hasPb) {
        if (votca::tools::globals::verbose) {
            cout << "Removing periodic box. Creating new... " << endl;
        }
        delete _bc;
    }

    switch(boxtype) {
       case CSG::BoundaryCondition::typeTriclinic:
            _bc = new CSG::TriclinicBox();
            break;
       case CSG::BoundaryCondition::typeOrthorhombic:
            _bc = new CSG::OrthorhombicBox();
            break;
       default:
            _bc = new CSG::OpenBox();
            break;
       }

    _bc->setBox(box);
    _hasPb = true;
}


CSG::BoundaryCondition::eBoxtype Topology::AutoDetectBoxType(const matrix &box){

    // Set box type to OpenBox in case "box" is the zero matrix,
    // to OrthorhombicBox in case "box" is a diagonal matrix,
    // or to TriclinicBox otherwise

    if(box.get(0,0)==0 && box.get(0,1)==0 && box.get(0,2)==0 &&
       box.get(1,0)==0 && box.get(1,1)==0 && box.get(1,2)==0 &&
       box.get(2,0)==0 && box.get(2,1)==0 && box.get(2,2)==0) {

            cout << "WARNING: No box vectors specified in trajectory."
               "Using open-box boundary conditions. " << endl;
            return CSG::BoundaryCondition::typeOpen;
    }

    else if(box.get(0,1)==0 && box.get(0,2)==0 &&
            box.get(1,0)==0 && box.get(1,2)==0 &&
            box.get(2,0)==0 && box.get(2,1)==0) {

            return CSG::BoundaryCondition::typeOrthorhombic;
    }

    else {
        return CSG::BoundaryCondition::typeTriclinic;
    }

    return CSG::BoundaryCondition::typeOpen;
}


vec Topology::PbShortestConnect(const vec &r1, const vec &r2) const {
    return _bc->BCShortestConnection(r1, r2);
}









void Topology::PrintInfo(ostream &out) {
        cout << endl;

        cout << "MD|QM Topology info "
                "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        if (_bc) {
        cout << "Periodic Box:        "  << this->getBox().get(0,0)
                                << " "   << this->getBox().get(0,1)
                                << " "   << this->getBox().get(0,2)
                                << " | " << this->getBox().get(1,0)
                                << " "   << this->getBox().get(1,1)
                                << " "   << this->getBox().get(1,2)
                                << " | " << this->getBox().get(2,0)
                                << " "   << this->getBox().get(2,1)
                                << " "   << this->getBox().get(2,2)
                                << endl;
        }
        else {
        cout << "No periodic boundary specified. " << endl;
        }
        cout << "Database ID:         " << this->getDatabaseId() << endl;
        cout << "Step number:         " << this->getStep() << endl;
        cout << "Time:                " << this->getTime() << endl;
        cout << "# Molecules          " << this->Molecules().size() << endl;
        cout << "# Segments           " << this->Segments().size() << endl;
        cout << "# Fragments          " << this->Fragments().size() << endl;
        cout << "# Atoms              " << this->Atoms().size() << endl;
        cout << endl;
}



}}
