#include "Md2QmEngine.h"
#include <votca/csg/boundarycondition.h>
#include <votca/tools/globals.h>

/**
 * Clears all engine template ('type') containers.
 */
Md2QmEngine::~Md2QmEngine() {

    vector < CTP::Molecule* > :: iterator molecule;

    // Clean up list of molecule types
    for (molecule = _molecule_types.begin();
         molecule < _molecule_types.end(); ++molecule){
         delete *molecule;
    }
    _molecule_types.clear();

    // clean up maps
    _map_MoleculeName_MoleculeType.clear();
    _map_MoleculeMDName_MoleculeName.clear();
}



/**
 * Reads in mapping .xml file to generate molecule, segment, fragment and
 * atom templates ('types') that contain all necessary information to
 * start mapping, except coordinates.
 * @param xmlfile
 */
void Md2QmEngine::Initialize(const string &xmlfile) {

    Property typology;
    load_property_from_xml(typology, xmlfile.c_str());

    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
    // XML to Types: Molecules => Segments => Fragments => Atoms //
    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

    string key = "topology.molecules.molecule";
    list<Property *> molecules = typology.Select(key);
    list<Property *>::iterator it_molecule;
    int molecule_id = 1;

    for ( it_molecule = molecules.begin();
          it_molecule != molecules.end();
          ++it_molecule ) {

       CTP::Molecule *molecule = AddMoleculeType(molecule_id++, *it_molecule);
       string molMdName = (*it_molecule)->get("mdname").as<string>();

       // +++++++++++++ //
       // Load segments //
       // +++++++++++++ //

       key = "segments.segment";
       list<Property *> segments = (*it_molecule)->Select(key);
       list<Property *>::iterator it_segment;
       int segment_id = 1;
       int md_atom_id = 1; // <- atom id count with respect to molecule

       for ( it_segment = segments.begin();
             it_segment != segments.end();
             ++it_segment ) {

         // Create a new segment
         CTP::Segment *segment = AddSegmentType(segment_id++, *it_segment );
         molecule->AddSegment( segment );

         // ++++++++++++++ //
         // Load fragments //
         // ++++++++++++++ //

         key = "fragments.fragment";
         list<Property *> fragments = (*it_segment)->Select(key);
         list<Property *>::iterator it_fragment;
         int fragment_id = 1;

         for ( it_fragment = fragments.begin();
               it_fragment != fragments.end();
               ++it_fragment ) {

            // Create new fragment
            CTP::Fragment* fragment=AddFragmentType(fragment_id++,*it_fragment);
            segment->AddFragment( fragment );

             // ++++++++++ //
             // Load atoms //
             // ++++++++++ //

             string mdatoms = (*it_fragment)->get("mdatoms").as<string>();
             string qmatoms = (*it_fragment)->get("qmatoms").as<string>();
             string weights = (*it_fragment)->get("weights").as<string>();

             Tokenizer tok_md_atoms(mdatoms, " ");
             Tokenizer tok_qm_atoms(qmatoms, " ");
             Tokenizer tok_weights(weights, " ");

             vector <string> md_atoms_info;
             vector <string> qm_atoms_info;
             vector <string> atom_weights;;

             tok_md_atoms.ToVector(md_atoms_info);
             tok_qm_atoms.ToVector(qm_atoms_info);
             tok_weights.ToVector(atom_weights);

             //assert(md_atoms_info.size() == qm_atoms_info.size());
             //assert(md_atoms_info.size() == atom_weights.size());

             if ( (md_atoms_info.size() != qm_atoms_info.size()) ||
                  (md_atoms_info.size() != atom_weights.size() ) ) {
                 cout << "ERROR: "
                      << "Could not allocate MD atoms to QM atoms or weights"
                      << " in segment " << segment->getName()
                      << " in molecule " << molMdName
                      << " due to inconsistent number of columns."
                      << endl;
                 cout << "NOTE: "
                      << "To define an MD atom without QM counterpart, insert "
                      << "a single ':' in the associated QM-atoms column and "
                      << "specify a mapping weight of 0."
                      << endl;
                 throw runtime_error( "Inconsistency in mapping file." );
             }

             vector<string> ::iterator it_md_atom_name;
             vector<string> ::iterator it_qm_atom_name;
             vector<string> ::iterator it_atom_weight;

             for ( it_md_atom_name = md_atoms_info.begin(),
                   it_qm_atom_name = qm_atoms_info.begin(),
                   it_atom_weight  = atom_weights.begin();
                   it_md_atom_name != md_atoms_info.end();
                   ++it_md_atom_name,
                   ++it_qm_atom_name,
                   ++ it_atom_weight) {

               // ++++++++++++++++++ //
               // Create single atom //
               // ++++++++++++++++++ //

               Tokenizer tok_md((*it_md_atom_name), ":");
               Tokenizer tok_qm((*it_qm_atom_name), ":");

               vector<string> md_atom_specs;
               vector<string> qm_atom_specs;

               tok_md.ToVector( md_atom_specs );
               tok_qm.ToVector( qm_atom_specs );

               // MD atom information
               int residue_number  = boost::lexical_cast<int>(md_atom_specs[0]);
               string residue_name = md_atom_specs[1];
               string md_atom_name = md_atom_specs[2];
               
               // QM atom information
               bool hasQMPart = false;
               int qm_atom_id = -1;
               // Check whether MD atom has QM counterpart
               if (qm_atom_specs.size() == 2) {
                   hasQMPart = true;
                   qm_atom_id = boost::lexical_cast<int>(qm_atom_specs[0]);
               }              
               
               // Mapping weight
               double weight = boost::lexical_cast<double>(*it_atom_weight);
               if (!hasQMPart && weight != 0) {
                   cout << "ERROR: "
                        << "Atom " << md_atom_name << " in residue "
                        << residue_name << " in molecule " << molMdName
                        << " has no QM counterpart despite non-zero weight. "
                        << endl;
                   throw runtime_error( "Error in mapping file" );
               }

               // Create atom
               CTP::Atom *atom = AddAtomType(molecule,
                                             residue_name, residue_number,
                                             md_atom_name, md_atom_id++,
                                             hasQMPart,    qm_atom_id,
                                             weight);

               try {
                   this->_map_mol_resNr_atm_atmType.at(molMdName)
                                                   .at(residue_number)
                                                   .at(md_atom_name);
                   // If this succeeded, atom has already been defined:
                   cout << "ERROR: "
                        << "Ambiguous atom definition in molecule "
                        << molMdName << ": "
                        << "Atom " << md_atom_name << " in residue "
                        << residue_number << " exists more than once. "
                        << endl;
                   throw runtime_error( "Ambiguity in atom definition." );
               }
               catch ( out_of_range ) {
                   this->_map_mol_resNr_atm_atmType[molMdName]
                                                   [residue_number]
                                                   [md_atom_name] = atom;
               }

               fragment->AddAtom(atom);
               segment->AddAtom(atom);
               molecule->AddAtom(atom);

             } /* exit loop over atoms */

          } /* exit loop over fragments */

       } /* exit loop over segments */

    } /* exit loop over molecules */
}



/**
 * Using the templates for molecules, segments, ... generated in
 * ::Initialize, Md2Qm maps the MD topology to the initially empty
 * MD/QM Topology. Calls ::MoleculeFactory and ::ExportMolecule.
 * @param mdtop
 * @param qmtop
 */
void Md2QmEngine::Md2Qm(CSG::Topology *mdtop, CTP::Topology *qmtop) {

    qmtop->CleanUp();

    // Create periodic box
    qmtop->setBox(mdtop->getBox());

    // Set trajectory meta data
    qmtop->setStep(mdtop->getStep());
    qmtop->setTime(mdtop->getTime());

    // Populate topology in a trickle-down manner
    // (i.e. molecules => ... ... => atoms)
    CSG::MoleculeContainer::iterator mit;
    for (mit = mdtop->Molecules().begin();
         mit != mdtop->Molecules().end();
         mit++ ) {

         // MD molecule + name
         CSG::Molecule *molMD = *mit;
         string nameMolMD = molMD->getName();

         // Find QM counterpart
         CTP::Molecule *molQM = this->MoleculeFactory(molMD);
         string nameMolQM = molQM->getName();

         // Generate and export
         CTP::Molecule *product = this->ExportMolecule(molQM, qmtop);
    }
}



/**
 * Takes MD molecule, finds QM counterpart, fills out atom positions.
 * @param molMDTemplate
 * @return molQMTemplate
 */
CTP::Molecule *Md2QmEngine::MoleculeFactory(CSG::Molecule *molMDTemplate) {

    string nameMolQM = this->getMoleculeName(molMDTemplate->getName());
    CTP::Molecule *molQMTemplate = this->getMoleculeType(nameMolQM);

    int resnrOffset = molMDTemplate->getBead(0)->getResnr();

    for (int i = 0; i < molMDTemplate->BeadCount(); i++) {
        CSG::Bead *atomMD = molMDTemplate->getBead(i);
        try {
            CTP::Atom *counterpart =
                 this->getAtomType(molMDTemplate->getName(),
                                   atomMD->getResnr()-resnrOffset+1,
                                   atomMD->getName());
            counterpart->setPos(atomMD->getPos());
        }
        catch (out_of_range) {
            cout << "WARNING: No mapping instruction found for atom "
                 << atomMD->getName() << " in residue number "
                 << atomMD->getResnr() << " in molecule "
                 << molMDTemplate->getName() << ". Skipping..."
                 << endl;
        }
    }
    return molQMTemplate;
}



/**
 * Takes QM molecule template as reference, and step by step adds 'new'
 * copies of all segments, fragments, atoms within that molecule to
 * the target topology.
 * @param refMol
 * @param qmtop
 * @return
 */
CTP::Molecule *Md2QmEngine::ExportMolecule(CTP::Molecule *refMol,
                                           CTP::Topology *qmtop) {

    CTP::Molecule *newMol = qmtop->AddMolecule(refMol->getName());

    // Note: The topology is responsible for allocating IDs to
    //       molecules, segments, ...
    
    vector<CTP::Segment *> ::iterator segIt;
    for (segIt = refMol->Segments().begin();
         segIt < refMol->Segments().end();
         segIt++) {

        CTP::Segment *refSeg = *segIt;
        CTP::Segment *newSeg = qmtop->AddSegment(refSeg->getName());

        vector<CTP::Fragment *> ::iterator fragIt;
        for (fragIt = refSeg->Fragments().begin();
             fragIt < refSeg->Fragments().end();
             fragIt++) {

            CTP::Fragment *refFrag = *fragIt;
            CTP::Fragment *newFrag = qmtop->AddFragment(refFrag->getName());

            vector<CTP::Atom *> ::iterator atomIt;
            for (atomIt = refFrag->Atoms().begin();
                 atomIt < refFrag->Atoms().end();
                 atomIt++) {

                CTP::Atom *refAtom = *atomIt;
                CTP::Atom *newAtom = qmtop->AddAtom(refAtom->getName());

                newAtom->setWeight(refAtom->getWeight());
                newAtom->setResnr(refAtom->getResnr());
                newAtom->setResname(refAtom->getResname());
                if (refAtom->HasQMPart()) {
                    newAtom->setQMPart(refAtom->getQMId());
                }
                newAtom->setPos(refAtom->getPos());

                newFrag->AddAtom(newAtom);
                newSeg->AddAtom(newAtom);
                newMol->AddAtom(newAtom);
            } /* exit loop over template atoms */
            newFrag->calcPos();
            newSeg->AddFragment(newFrag);
            newMol->AddFragment(newFrag);
        } /* exit loop over template fragments */
        newSeg->calcPos();
        newMol->AddSegment(newSeg);
    } /* exit loop over template molecules */

}



CTP::Atom *Md2QmEngine::AddAtomType(CTP::Molecule *owner,
                                    string residue_name,  int residue_number,
                                    string md_atom_name,  int md_atom_id,
                                    bool hasQMPart,       int qm_atom_id,
                                    double weight) {
    CTP::Atom* atom = new CTP::Atom(owner,
                                    residue_name,         residue_number,
                                    md_atom_name,         md_atom_id,
                                    hasQMPart,            qm_atom_id,
                                    weight);
    _atom_types.push_back(atom);
    return atom;
}

CTP::Fragment *Md2QmEngine::AddFragmentType(int fragment_id,
                                            Property *property) {
    string fragment_name = property->get("name").as<string>();
    CTP::Fragment* fragment = new CTP::Fragment(fragment_id, fragment_name);
    _fragment_types.push_back(fragment);
    return fragment;
}

CTP::Segment  *Md2QmEngine::AddSegmentType(int segment_id,
                                           Property *property) {
    string segment_name = property->get("name").as<string>();
    CTP::Segment* segment = new CTP::Segment(segment_id, segment_name);
    _segment_types.push_back(segment);
    return segment;
}

CTP::Molecule *Md2QmEngine::AddMoleculeType(int molecule_id,
                                            Property *property) {
    string molecule_name = property->get("name").as<string>();
    string molecule_mdname = property->get("mdname").as<string>();

    CTP::Molecule *molecule = new CTP::Molecule(molecule_id, molecule_name);
    _molecule_types.push_back(molecule);
    // TODO check if the name is already there
    _map_MoleculeMDName_MoleculeName[molecule_mdname] = molecule_name;
    _map_MoleculeName_MoleculeType[molecule_name] = molecule;
    return molecule;
}


CTP::Molecule *Md2QmEngine::getMoleculeType(const string &name) {
    try {
        return _map_MoleculeName_MoleculeType.at(name);
    }
    catch ( out_of_range ) {
        cout << "WARNING: Molecule '" << name
             << "' not included in mapping definition. Skipping... ";
        cout << endl;
        return NULL;
    }    
}

const string &Md2QmEngine::getMoleculeName(const string &mdname) {
    try {
        return _map_MoleculeMDName_MoleculeName.at(mdname);
    }
    catch ( out_of_range ) {
        return mdname;
    }
}

CTP::Atom *Md2QmEngine::getAtomType(const string &molMdName,
                                    int resNr, const string &mdAtomName) {
    return this->_map_mol_resNr_atm_atmType.at(molMdName)
                                           .at(resNr)
                                           .at(mdAtomName);
}


void Md2QmEngine::PrintInfo() {

    vector<CTP::Molecule*>::iterator mit;
    vector<CTP::Segment*>::iterator sit;
    vector<CTP::Fragment*>::iterator fit;
    vector<CTP::Atom*>::iterator ait;

    cout << "Summary ~~~~~"
            "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
    cout << "Created "
         << _molecule_types.size() << " molecule type(s): ";
    for (mit = _molecule_types.begin();
         mit != _molecule_types.end();
         mit++) {
         cout << "[ " << (*mit)->getName() << " ]";
    }
    cout << endl <<  "with a total of "
         << _segment_types.size()  << " segment(s), "
         << _fragment_types.size() << " fragments, "
         << _atom_types.size() << " atoms. \n" << endl;

    map < string, string > ::iterator mssit;
    for (mssit = this->_map_MoleculeMDName_MoleculeName.begin();
         mssit != this->_map_MoleculeMDName_MoleculeName.end();
         mssit ++) {
         cout << "MD [ " << mssit->first << " ] mapped to "
              << "QM [ " << mssit->second << " ] \n" << endl;
    }

    for (mit = _molecule_types.begin();
         mit != _molecule_types.end();
         mit++) {
         CTP::Molecule *mol = *mit;
         cout << "[ " << mol->getName() << " ]" << endl;

         for (sit = mol->Segments().begin();
              sit != mol->Segments().end();
              sit++) {
              CTP::Segment *seg = *sit;
              cout << " - Segment [ " << seg->getName() << " ]"
                   << " ID " << seg->getId() << endl;

              for (fit = seg->Fragments().begin();
                   fit != seg->Fragments().end();
                   fit++ ) {
                   CTP::Fragment *frag = *fit;
                   cout << "   - Fragment [ " << frag->getName() << " ]"
                        << " ID " << frag->getId() << ": "
                        << frag->Atoms().size() << " atoms " << endl;
              }
         }
    }

    if (! votca::tools::globals::verbose ) { return; }

    cout << endl << "Mapping table"
                    " ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                 << endl;
    map < string, map < int, map < string, CTP::Atom* > > > ::iterator it0;
    for (it0 = this->_map_mol_resNr_atm_atmType.begin();
         it0 != this->_map_mol_resNr_atm_atmType.end();
         it0 ++) {
         map < int, map < string, CTP::Atom* > > ::iterator it1;
         for (it1 = it0->second.begin();
              it1 != it0->second.end();
              it1++) {
              map < string, CTP::Atom* > ::iterator it2;
              for (it2 = it1->second.begin();
                   it2 != it1->second.end();
                   it2++) {

       printf( "MD Molecule %4s | Residue %2d | Atom %3s "
                               "| ID %3d => QM ID %3d \n",
               it0->first.c_str(),
               it2->second->getResnr(),
               it2->first.c_str(),
               it2->second->getId(),
               it2->second->getQMId());
             }
         }
    }
}

void Md2QmEngine::CheckProduct(CTP::Topology *outtop, const string &pdbfile) {

    string md_pdb = "md_" + pdbfile;
    string qm1_pdb = "qm_rigid_" + pdbfile;
    string qm2_pdb = "qm_conjg_" + pdbfile;
    FILE *outPDB = NULL;

    // Atomistic PDB
    outPDB = fopen(md_pdb.c_str(), "w");

    vector<CTP::Molecule*> ::iterator molIt;
    for (molIt = outtop->Molecules().begin();
         molIt < outtop->Molecules().end();
         molIt++) {
        CTP::Molecule *mol = *molIt;
        mol->WritePDB(outPDB);
    }

    fprintf(outPDB, "\n");
    fclose(outPDB);


    // Fragment PDB
    outPDB = fopen(qm1_pdb.c_str(), "w");

    vector<CTP::Segment*> ::iterator segIt;
    for (segIt = outtop->Segments().begin();
         segIt < outtop->Segments().end();
         segIt++) {
        CTP::Segment *seg = *segIt;
        seg->WritePDB(outPDB);
    }

    fprintf(outPDB, "\n");
    fclose(outPDB);


    // Segment PDB
    outPDB = fopen(qm2_pdb.c_str(), "w");
    outtop->WritePDB(outPDB);
    fprintf(outPDB, "\n");
    fclose(outPDB);


    if (votca::tools::globals::verbose) {
        cout << endl;
        this->PrintInfo();
        cout << endl;
        outtop->PrintInfo(cout);
    }
}

