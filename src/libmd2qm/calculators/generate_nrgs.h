/* 
 * File:   generate_nrgs.h
 * Author: lukyanov
 *
 * Created on July 21, 2010, 4:46 PM
 */

#ifndef _GENERATE_NRGS_H
#define	_GENERATE_NRGS_H

#include "qmpair.h"
#include "qmcalculator.h"
#include "votca/tools/average.h"

/// Generate Gaussian-distributed site energies, with or without spatial correlations

class GenerateNrgs : public QMCalculator
{
public:
    GenerateNrgs() {};
    ~GenerateNrgs() {};

    void Initialize(QMTopology *top, Property *options);
    bool EvaluateFrame(QMTopology *top);

    /// A matching function to overload the standard one
    bool MyMatchingFunction(Bead *bead1, Bead *bead2, const vec & r);
    
private:
    /// sigma of the gaussian distribution
    double _sigma;
    /// true - correlated energies, false - uncorrelated energies
    bool _correl;
    /// correlation cutoff for generating correlated energies
    double _cutoff;
    /// temporary object to store energies
    std::map<CrgUnit *, Average<double> > _tmp_energy;


    void AssignGaussian(QMTopology *top);
    void AssignCorrelated(QMTopology *top);

};

#endif	/* _GENERATE_NRGS_H */

