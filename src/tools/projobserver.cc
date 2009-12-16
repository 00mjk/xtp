#include "easyjobserver.h"
#include <votca/csg/nblist.h>
#include <qmnblist.h>

EasyJObserver::EasyJObserver()
{}

EasyJObserver::~EasyJObserver()
{}

void EasyJObserver::BeginCG(Topology *top, Topology *top_atom)
{
    _qmtop->Initialize(*top);
}

void EasyJObserver::EndCG()
{}

/// evaluate current conformation

void EasyJObserver::EvalConfiguration(Topology *top, Topology *top_atom)
{
    _qmtop->Update(*top);
    

    BeadList list1;
    Topology *toptmp = dynamic_cast<Topology*>(_qmtop);
    list1.Generate(*toptmp, "*");
    QMNBList &nblist =(_qmtop->getNBList);
    nblist.setCutoff(1.0);
    nblist.Generate(list1);

    for(QMNBList::iterator iter = nblist.begin();
        iter!=nblist.end();++iter) {
        CrgUnit *crg1 = (*iter)->first;
        CrgUnit *crg2 = (*iter)->second;

        Topology atoms;
        _qmtop->AddAtomisticBeads(molid, crg1);
        _qmtop->AddAtomisticBeads(molid, crg2);
        ///write the topo somehow now.
    }
}

