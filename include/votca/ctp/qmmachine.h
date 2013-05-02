#ifndef __QMMACHINE__H
#define	__QMMACHINE__H


#include <votca/ctp/xjob.h>
#include <votca/ctp/xinductor.h>
#include <votca/ctp/gaussian.h>
#include <votca/ctp/orbitals.h>


namespace votca { namespace ctp {
    
class QMMachine 
{

public:

    QMMachine(XJob *job, XInductor *xind, Gaussian *qmpack,
              Property *opt, string sfx, int nst, bool mav);
   ~QMMachine() {};   
   
    void Evaluate(XJob *job);    
    void WriteQMPackInputFile(string inputFile, Gaussian *qmpack, XJob *job);
    void ConvertPSitesToQMAtoms(vector< PolarSeg* > &, vector< QMAtom* > &);
    void ConvertQMAtomsToPSites(vector< QMAtom* > &, vector< PolarSeg* > &);

private:

    XJob *_job;
    XInductor *_xind;
    Gaussian *_qmpack;
    int _subthreads;

};

    
}}

#endif