/* 
 * File:   energycorr.cc
 * Author: lukyanov
 * 
 * Created on July 28, 2010, 6:03 PM
 */

#include <stdlib.h>
#include "energycorr.h"
#include <math.h>
#include <list>
#include "votca/tools/average.h"
#include "votca/csg/nblistgrid.h"

void EnergyCorr::Initialize(QMTopology* top, Property* options) {

   // read in _nbins
   if (options->exists("options.energy_corr.nbins")) {
   	_nbins = options->get("options.energy_corr.nbins").as<int>();
   }
   else {
	_nbins = 15;
	cout << "Warning: nbins for site energy distribution is not provided, using default "
             << "nbins = " << _nbins << endl;
   }

   // read in min and max
   _min = options->get("options.energy_corr.min").as<double>();
   _max = options->get("options.energy_corr.max").as<double>();

   // read in _outfile
   if (options->exists("options.energy_corr.file")) {
   	_outfile = options->get("options.energy_corr.file").as<string>();
   }
   else {
	_outfile = "energy_corr.dat";
	cout << "Warning: output filename for site energy distribution is not provided, using default "
             << "filename = " << _outfile << endl;
   }

   // initialize histograms
   _hist_corr.Initialize(_min, _max, _nbins);
   _hist_count.Initialize(_min, _max, _nbins);
   _hist_corr2.Initialize(_min, _max, _nbins);

   // set yerror = true
   _hist_corr.data().SetHasYErr(true);
}

bool EnergyCorr::EvaluateFrame(QMTopology* top) {

    Average<double> energy_av;

    // working topology = topology to do work
    Topology mytop;
    BeadList list1;
    NBListGrid mynbl;

    mytop.setBox(top->getBox());

    vector<QMCrgUnit *> lcharges = top->CrgUnits();
    vector<QMCrgUnit *>::iterator itl;
    // 1) calculate average energy
    // 2) for every charge unit of real topology make a bead for a working topology
    for (itl = lcharges.begin(); itl!=lcharges.end(); ++itl) {
        // Process data to get the average
        energy_av.Process ( (*itl)->getEnergy() );

        // make a simple spherical bead for a new topology
        BeadType *tmptype = mytop.GetOrCreateBeadType("no");
        Bead * tmpbead = mytop.CreateBead(1, "bead", tmptype, 0, 1, 0);
        // position of the bead = position of the corresponding crgunit
        tmpbead->setPos( (*itl)->GetCom() );
        // additionally it stores a pointer to the corresponding crgunit
        tmpbead->setUserData( (*itl) );
    }

    // mean energy and standard deviation for a particular snapshot
    _mean_energy = energy_av.getAvg();
    _stdev_energy = energy_av.CalcSig2();

    list1.Generate(mytop, "*");
    mynbl.SetMatchFunction(this, &EnergyCorr::MyMatchingFunction);
    mynbl.setCutoff(_max);
    mynbl.Generate( list1, false );

    return true;
}

void EnergyCorr::EndEvaluate(QMTopology* top) {
    // normalize
    _hist_corr.data().y() = element_div( _hist_corr.data().y(), _hist_count.data().y() );
    _hist_corr2.data().y() = element_div( _hist_corr2.data().y(), _hist_count.data().y() );

    // calculate error bars: UGLY!!!
    for (int i=0; i<_hist_corr.data().yerr().size(); i++) {
        _hist_corr.data().yerr(i) = sqrt ( (_hist_corr2.data().y(i) -
                _hist_corr.data().y(i)*_hist_corr.data().y(i) ) / _hist_count.data().y(i) );
    }
    
    // Save to file
    _hist_corr.data().Save(_outfile);
}

bool EnergyCorr::MyMatchingFunction(Bead *bead1, Bead *bead2, const vec & r) {

    QMCrgUnit *crg1 = bead1->getUserData<QMCrgUnit>();
    QMCrgUnit *crg2 = bead2->getUserData<QMCrgUnit>();

    double e1 = crg1->getEnergy();
    double e2 = crg2->getEnergy();
    double dist = abs(r);
//    cout << "e1= " << e1 << "  e2= " << e2 << " dist = " << dist;

    double contrib = (e1-_mean_energy)*(e2-_mean_energy)/_stdev_energy;

    _hist_count.Process( dist );
    _hist_corr.Process( dist, contrib );
    _hist_corr2.Process( dist, contrib*contrib );

    return false;
}
