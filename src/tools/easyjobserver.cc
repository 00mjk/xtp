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
    QMNBList &nblist = _qmtop->nblist();

    BeadList list1;
    Topology *toptmp = dynamic_cast<Topology*>(_qmtop);
    list1.Generate(*toptmp, "*");

    nblist.setCutoff(1.0);
    nblist.Generate(list1);

    for(QMNBList::iterator iter = nblist.begin();
        iter!=nblist.end();++iter) {
        CrgUnit *crg1 = (*iter)->first;
        CrgUnit *crg2 = (*iter)->second;
        vector <double> Js = _qmtop->GetJCalc().GetJ(*crg1, *crg2);
        cout << crg1->GetId() << " "
             << crg2->GetId() << " ";
        for(int i=0; i<Js.size(); +i)
            cout << Js[i] << " ";
        cout << endl;
    }
}

