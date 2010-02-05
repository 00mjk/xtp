
#include <stdio.h>
#include <string>
#include "statesaver.h"

using namespace std;

StateSaver::StateSaver(QMTopology &qmtop) {
    _qmtop = &qmtop;
}

//void StateSaver::Open(string file)

void StateSaver::Save(string file, bool bAppend) {
    //_out = fopen(file.c_str(), bAppend ? "at" : "wt");
    _out.open(file.c_str(), ios::out | ios::binary);
    Write_PBCs();
    Write_Molecules();
    Write_QMBeads();
    Write_QMNeighbourlist();
    //Write NBL
    _out.close();
    //_out = datafile(datafile.c_str(), ios::binary|ios::out);
}

void StateSaver::Load(string file) {
    _qmtop->Cleanup();
    _qmtop->nblist().Cleanup();
    _qmtop->CreateResidue("dummy");

    _in.open(file.c_str(), ios::in | ios::binary);
    Read_PBCs();
    Read_Molecules();
    Read_QMBeads();
    Read_QMNeighbourlist();
    _in.close();
}

void StateSaver::Write_PBCs(){
    write<matrix>(_qmtop->getBox());
}

void StateSaver::Write_Molecules() {
    assert(_out.is_open());

    write<unsigned long>(_qmtop->MoleculeCount());
    for (MoleculeContainer::iterator iter = _qmtop->Molecules().begin();
            iter != _qmtop->Molecules().end() ; ++iter) {
         Molecule *mol=*iter;
    write<int>(mol->getId());
    write<string>(mol->getName());
    //write<int>(mol->BeadCount());
        }
    }
      

void StateSaver::Write_QMBeads() {
    assert(_out.is_open());

    write<unsigned long>(_qmtop->BeadCount());
    for (BeadContainer::iterator iter = _qmtop->Beads().begin();
            iter != _qmtop->Beads().end(); ++iter) {
        QMBead *bi = dynamic_cast<QMBead*> (*iter);

        write<byte_t > (bi->getSymmetry());
        write<string > (bi->getName());
        write<string > (bi->getType()->getName());
        write<int>(bi->getResnr());
        write<double>(bi->getM());
        write<double>(bi->getQ());

        write<string > (bi->GetCrgUnit()->getName());
        write<double > (bi->GetCrgUnit()->getEnergy());
        write<unsigned short>(bi->getiPos());
        write<vec > (bi->getPos());
        write<vec > (bi->getU());
        write<vec > (bi->getV());
        write<int>(bi->getMolecule()->getId());

    }
}

void StateSaver::Write_QMNeighbourlist() {
    assert(_out.is_open());


    QMNBList &nblist = _qmtop->nblist();
     
    write<int>(nblist.size());//?
    //cout <<"There are so many pairs in nblist: " <<(int)nblist.size()<<"\n";
    for(QMNBList::iterator iter = nblist.begin();
        iter!=nblist.end();++iter) {
        QMPair *pair = *iter;
        CrgUnit *crg1 = (*iter)->first;
        CrgUnit *crg2 = (*iter)->second;
        
        write<unsigned int>(crg1->getId());
        write<unsigned int>(crg2->getId());
        write<string > (crg1->getName());
        write<string > (crg2->getName());
        write<int>(pair->Js().size());
        vector<double>::iterator itj=pair->Js().begin();
        for (;itj!= pair->Js().end(); itj++){
            write<double>(*itj);
        }
        write<double>(pair->rate12());
        write<double>(pair->rate21());
        write<vec>(pair->r());
    }
    //
    //write<unsigned long>(_qmtop->BeadCount());
}

void StateSaver::Read_PBCs(){
    assert(_in.is_open());

    _qmtop->setBox(read<matrix>());
}

void StateSaver::Read_Molecules(){
    assert(_in.is_open());

    unsigned long nr_mols = read<unsigned long>();
    //cout << "Total number of mols is " << nr_mols << "\n";
    for (unsigned long i = 0; i < nr_mols; i++) {
        int molid=read<int>();
    //cout << "This molecule has id "<<molid<<"\n";
        string mol_name=read<string>();
    //cout << "This molecules has name "<<mol_name<<"\n";
    Molecule *mol = _qmtop->CreateMolecule(mol_name);
    }
        
}


void StateSaver::Read_QMBeads() {
    assert(_in.is_open());
   
    unsigned long nr_qmbeads = read<unsigned long>();
    //cout << "Total number of QMBeads is " << nr_qmbeads << "\n";
    for (unsigned long i = 0; i < nr_qmbeads; i++) {

        byte_t symmetry =       read<byte_t> ();
        string bead_name =      read<string> ();
        string type_name =      read<string> ();
        int resnr =             read<int>();
        double M =              read<double>();
        double Q =              read<double>();
        
        string crg_unit_name =  read<string> ();
        double energy = read<double>();

        unsigned short ipos =   read<unsigned short>();
        vec Pos =               read<vec> ();
        vec U =                 read<vec> ();
        vec V =                 read<vec> ();
        int molid =             read<int>();

        BeadType *type = _qmtop->GetOrCreateBeadType(type_name);

        /*cout << "Bead Symmetry " << (int)symmetry << "\n";
        cout << "Bead Name " << bead_name << "\n";
        cout << "Bead Type " << type_name << "\n";
        cout << "Residue Number " << resnr << "\n";
        cout << "Bead Mass " << M << "\n";
        cout << "Bead Charge " << Q << "\n";
        cout << "Molid  " << molid << "\n";*/

        QMBead *bead = dynamic_cast<QMBead*>(_qmtop->CreateBead(symmetry, bead_name, type, resnr, M, Q));
        _qmtop->getMolecule(molid)->AddBead(bead, bead_name);
        
        CrgUnit * acrg = _qmtop->GetCrgUnitByName(crg_unit_name);
        if(acrg == NULL)
            acrg = _qmtop->CreateCrgUnit(crg_unit_name, type_name, molid);
        acrg->setEnergy(energy);

        bead->setCrg(acrg);
        bead->setiPos(ipos);
        bead->setPos(Pos);
        bead->setU(U);
        bead->setV(V);
        bead->UpdateCrg();

        
        /*cout << "The charge unit is called " << crg_unit_name << "\n";
        cout << "This charge unit has energy " << energy <<"\n";
        cout << "This bead is at int position " << ipos << "\n";
        cout << "This bead hast position  " << U << "\n";
        cout << "This bead has U " << U << "\n";
        cout << "This bead has V " << V << "\n";*/
    }
}

void StateSaver::Read_QMNeighbourlist() {
    assert(_in.is_open());
   
    int nr_pairs = read<int>();
    //cout << "Total number of QMPairs is " << nr_pairs << "\n";
    for (int i = 0; i < nr_pairs; i++) {

        unsigned int id1=read<unsigned int>();
        unsigned int id2=read<unsigned int>();
        string crg_unit_name1 =  read<string> ();
        string crg_unit_name2 =  read<string> ();
        //cout << "This pair has charge unit ids " << id1 << " and " <<id2 <<"\n";
        
        CrgUnit *crg1=_qmtop->GetCrgUnitByName(crg_unit_name1);
        CrgUnit *crg2=_qmtop->GetCrgUnitByName(crg_unit_name2);    
        //cout << "This pair has names " << crg_unit_name1 << " and " <<crg_unit_name2 <<"\n";
        
        QMPair *pair=new QMPair(crg1,crg2, _qmtop);
        _qmtop->nblist().AddPair(pair);
        int sizeJs=read<int>();
        vector <double> Js;
        for (int s=0;s<sizeJs;s++){
            double J=read<double>();
            Js.push_back(J);
            //cout << "This pair has J " << J <<"\n";
        }
        pair->setJs(Js);

        double rate12=read<double>();
        double rate21=read<double>();
        pair->setRate12(rate12);
        pair->setRate21(rate21);
        //cout << "This pair has rates " << rate12 << " and " << rate21 <<"\n";

        pair->setR(read<vec>());
        //cout << "This pair has connecting vector r_ij = " << pair->r() << endl;
    }
}
