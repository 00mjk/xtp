#include "qmtopology.h"
#include "qmnblist.h"

QMTopology::QMTopology()
{}

QMTopology::~QMTopology()
{}


void QMTopology::Initialize(Topology& cg_top)
{
    CopyTopologyData(&cg_top);
    this->InitChargeUnits();
}

void QMTopology::Cleanup()
{
    vector<CrgUnit *>::iterator iter;
    for(iter=_crgunits.begin(); iter!=_crgunits.end(); ++iter)
        delete *iter;
    _crgunits.clear();
    _mcharges.clear();
    Topology::Cleanup();
}

void QMTopology::Update(Topology& cg_top)
{
    BeadContainer::iterator iter;
    BeadContainer::iterator iter_cg;

    assert(cg_top.Beads().size() == _beads.size());

    _box = cg_top.getBox();
    _time = cg_top.getTime();
    _step = cg_top.getStep();

    iter_cg = cg_top.Beads().begin();
    for(iter=_beads.begin(); iter!=_beads.end(); ++iter, ++iter_cg) {
        (*iter)->setPos((*iter_cg)->getPos());
        (*iter)->setU((*iter_cg)->getU());
        (*iter)->setV((*iter_cg)->getV());
        (*iter)->setW((*iter_cg)->getW());
        QMBead * b = (QMBead*)(*iter);
        b->UpdateCrg();
    }
}

void QMTopology::LoadListCharges(const string &file)
{
    _jcalc.Initialize(file);
}


void QMTopology::AddAtomisticBeads(CrgUnit *crg, Topology * totop){
     
    mol_and_orb * atoms = crg->rotate_translate_beads();
    totop->CreateResidue("DUM");
    Molecule *mi = totop->CreateMolecule((crg)->getName());
    mi->setUserData(crg);   //mi->getUserData<CrgUnit>();
    for (int i=0;i<atoms->getN();i++){
        vec pos = unit<bohr,nm>::to(atoms->GetPos(i));
        string atomtype = string( atoms->gettype(i) )+ string("-") + lexical_cast<string>(crg->getId());
        BeadType * bt= totop->GetOrCreateBeadType(atomtype);
        Bead * bead = totop ->CreateBead(1, atomtype,bt,0, 0, 0.);
        bead->setPos(pos);
        mi->AddBead(bead,"???");
        if(crg->getType()->GetCrgUnit().getChargesNeutr()) {
            double charge_of_bead_neutral=crg->getType()->GetCrgUnit().getChargesNeutr()->mpls[i];
            bead->setQ(charge_of_bead_neutral);
        }
      
    }
    delete atoms->getorbs();
    delete atoms;
}

Bead *QMTopology::CreateBead(byte_t symmetry, string name, BeadType *type, int resnr, double m, double q)
{
    QMBead *bead = new QMBead(this, _beads.size(), type, symmetry, name, resnr, m, q);
    _beads.push_back(bead);
    return bead;
}

void QMTopology::InitChargeUnits(){
    BeadContainer::iterator itb;
    for (itb = _beads.begin() ; itb< _beads.end(); ++itb){
        QMBead * bead = dynamic_cast<QMBead *>(*itb);
        //initialise the crgunit * only if appropriate extra info is in the cg.xml file
        if ( (bead->Options()).exists("qm.crgunitname")){
            string namecrgunittype = bead->getType()->getName();
            int intpos = (bead->Options()).get("qm.position").as<int>();
            string namecrgunit = (bead->Options()).get("qm.crgunitname").as<string>();

            //determine whether it  has been created already
            int molid= bead->getMolecule()->getId();
            string molandtype = lexical_cast<string>(molid)+":"+namecrgunit;
            
            CrgUnit * acrg = GetCrgUnitByName(molandtype);
            if(acrg == NULL)
                acrg = CreateCrgUnit(molandtype, namecrgunittype, molid);
            
            bead->setCrg(acrg);
            bead->setiPos(intpos);
        }
        else{
            bead->setCrg(NULL);
        }
    }
}


// TODO: this function should not be in qmtopology!
void QMTopology::ComputeAllTransferIntegrals(){
    for(QMNBList::iterator iter = _nblist.begin();
        iter!=_nblist.end();++iter) {
        CrgUnit *crg1 = (*iter)->Crg1();
        CrgUnit *crg2 = (*iter)->Crg2();
        vector <double> Js = GetJCalc().CalcJ(*crg1, *crg2);
        (*iter)->setJs(Js);
    }
}

   // In topology.cc???
//Copy charges to either charged or neutral case

void QMTopology::CopyCharges(CrgUnit *crg, Molecule *mol)
{
    if(mol->BeadCount() != crg->getType()->GetCrgUnit().getN())
        throw std::runtime_error("QMTopology::CopyCharges: number of atoms in crgunit does not match number of beads in molecule");

    if(!crg->getType()->GetCrgUnit().getChargesNeutr())
        throw std::runtime_error("QMTopology::CopyCharges: no charges defined");

    //loop over all beads in that molecule
    for (int i = 0; i < mol->BeadCount(); i++) {
        //get charge
        double charge_of_bead_i_neutral = crg->getType()->GetCrgUnit().getChargesNeutr()->mpls[i];
        //set charge
        mol->getBead(i)->setQ(charge_of_bead_i_neutral);
    }
}

//Copy charges to either charged or neutral case
void QMTopology::CopyChargesOccupied(CrgUnit *crg, Molecule *mol)
{
    if(mol->BeadCount() != crg->getType()->GetCrgUnit().getN())
        throw std::runtime_error("QMTopology::CopyCharges: number of atoms in crgunit does not match number of beads in molecule");

    if(!crg->getType()->GetCrgUnit().getChargesCrged())
        throw std::runtime_error("QMTopology::CopyCharges: no charges defined");

    //loop over all beads in that molecule
    for (int i = 0; i < mol->BeadCount(); i++) {
        //get charge
        double charge_of_bead_i_charged = crg->getType()->GetCrgUnit().getChargesCrged()->mpls[i];
        //set charge
        mol->getBead(i)->setQ(charge_of_bead_i_charged);
    }
}
