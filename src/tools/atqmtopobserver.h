/* 
 * File:   atqmtopobserver.h
 * Author: james
 *
 * Created on March 10, 2010, 9:52 AM
 */

#ifndef _ATQMTOPOBSERVER_H
#define	_ATQMTOPOBSERVER_H

#include <votca/csg/cgobserver.h>
#include <moo/jcalc.h>
#include <libxml/parser.h>
#include <votca/csg/trajectorywriter.h>
#include "qmtopology.h"
#include <sys/stat.h>

/// will read in the atomistic trajectory and
/// generate corresponding coarse graines
/// and qm topologies according to the list charge definition
class AtQmObserver
    : public CGObserver
{
public:
    AtQmObserver();
    ~AtQmObserver();


    void setQMTopology(QMTopology &qmtop) {_qmtop = &qmtop; }

    /// begin coarse graining a trajectory
    void BeginCG(Topology *top, Topology *top_atom);

    /// end coarse graining a trajectory
    void EndCG();

    /// evaluate current conformation
    void EvalConfiguration(Topology *top, Topology *top_atom = 0);

protected:
    QMTopology *_qmtop;
    TrajectoryWriter *_writerCG;
    TrajectoryWriter *_writerQM;
};

#endif	/* _ATQMTOPOBSERVER_H */

