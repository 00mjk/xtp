/* 
 * File:   crgtopology.h
 * Author: james
 *
 * Created on 12 December 2009, 13:50
 */

#ifndef _QMTOPOLOGY_H
#define	_QMTOPOLOGY_H

#include <votca/csg/topology.h>
#include <votca/csg/nblist.h>
#include <moo/crgunittype.h>
#include <moo/crgunit.h>

#include "qmbead.h"

/**
    \brief topology of qmbeads

    contains the beads describing the c.o.m. of each cahrge unit
    given a CG toplogy, it should be apply the mapping for the cg beads -> qm beads and
    it should update the position of the crg unit. Crg units should be associated to
    these qm beads and not to any other.
*/

class QMTopology : public Topology
{
public:
    QMTopology();
    ~QMTopology();

    ///at each evaluate CG step we will need to reassess the QMBeads
    //int UpdateQMTopology();

    /// update the topology based on cg positons
    void Update(Topology &cg_top);

    /// \brief Cretae a new bead
    /// We overload CreateBead to create QMBead, this is needed to make
    /// CopyTopologyData work
    Bead *CreateBead(byte_t symmetry, string name, BeadType *type, int resnr, double m, double q);

    void LoadListCharges(const string &file);
protected:

    NBList *_nblist;
/*    /// initialises the map that reads in the map from beadtypes to qmbead
    void InitMap (string);
    /// pnce the map is initialised and _Cgtop assigned, call all the CreateQMBead function
    void Init();
    
    ///the underlying cg toplogy
    Topology * _cgtop;
    ///the underlying jcalc with the list of charge types in it
    JCalc * _jcalc;
    /// the list of neighbours
    NBList * _neighs;
    /// the list of QM beads
    vector <QMBead *>;
    /// creates a QM bead from the list of cg beads
    QMBead *CreateQMBead (BeadContainer &);
    /// a mapping from the moleculetype to each the mapping which associates each
    /// assembly of charge
    /// beadtypes to a crgunit type
    map < string, QMMoleculeMap * >;
*/
};

inline Bead *QMTopology::CreateBead(byte_t symmetry, string name, BeadType *type, int resnr, double m, double q)
{
    QMBead *b = new QMBead(this, _beads.size(), type, symmetry, name, resnr, m, q);
    _beads.push_back(b);
    return b;
}

#endif	/* _CRGTOPOLOGY_H */

