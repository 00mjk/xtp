#include "qmtopology.h"

QMTopology::QMTopology()
{
    _nblist = NULL;
}

QMTopology::~QMTopology()
{
    if(_nblist)
        delete _nblist;
    _nblist = NULL;
}


void QMTopology::Initialize(Topology& cg_top)
{
    CopyTopologyData(&cg_top);
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
    for(iter=_beads.begin(); iter!=_beads.end(); ++iter) {
        (*iter)->setPos((*iter_cg)->getPos());
        (*iter)->setU((*iter_cg)->getU());
        (*iter)->setV((*iter_cg)->getV());
        (*iter)->setW((*iter_cg)->getW());
        QMBead * b = dynamic_cast<QMBead*>(*iter);
        b->QMBead::UpdateCrg();
    }
}

void QMTopology::LoadListCharges(const string &file)
{
    _jcalc.Init(file);
}


void QMTopology::AddAtomisticBeads(const int & molid, const string & namecrgunit, Topology * totop){
    string uniqid=lexical_cast<string>(molid)+":"+namecrgunit;
    map <string, CrgUnit*>::iterator  itm= _mcharges.find(uniqid);
    if (itm == _mcharges.end())
        throw runtime_error(string ("could not find the molid and crgtype: ") + uniqid);
    CrgUnit * crg= itm->second;
    mol_and_orb * atoms = crg->rotate_translate_beads();

    for (int i=0;i<atoms->getN();i++){
        vec pos = atoms->GetPos(i);
        string atomtype = "QMAT-"+string( atoms->gettype(i) );
        BeadType * bt= totop->GetOrCreateBeadType(atomtype);
        int nbead = totop-> BeadCount();
        totop ->CreateBead(1, atomtype,bt,nbead, 0, 0.);
    }
    delete atoms;
}