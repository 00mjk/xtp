/*
 * File:   calc_estatics.cpp
 * Author: mayfalk
 *
 * Created on May 21, 2010, 11:21 PM
 */


#include <stdlib.h>
#include "estatics.h"
#include <math.h>
#include <list>

void CalcEstatics::Initialize(QMTopology *top, Property *options){
   
   if (options->exists("options.estatic_params.epsilon_dielectric")) {
   	_epsilon_dielectric = options->get("options.estatic_params.epsilon_dielectric").as<double>(); 
   }
   else {
	_epsilon_dielectric = 3.0;
	cout << "Warning: dielectric constant in not provided, using default" << endl;
   }
   
   if (options->exists("options.estatic_params.s_eps")) {
   	_s_eps = options->get("options.estatic_params.s_eps").as<double>();
   }
   else {
	_s_eps = 3.0;
	cout << "Warning: s_eps is not provided, using default" << endl;
   }
   
   if (options->exists("options.estatic_params.estatic_method")) {
	if ( options->get("options.estatic_params.estatic_method").as<string>() == "distdep" ) {
		_estatic_method = &CalcEstatics::dist_dep_eps;
		cout << "Doing distance-dependent-eps estatic with eps " << _epsilon_dielectric << " and s_eps " << _s_eps << endl;
	}
	else if ( options->get("options.estatic_params.estatic_method").as<string>() == "simple" ) {
   		_estatic_method = &CalcEstatics::constant_epsilon;
		cout << "Doing simple estatic with eps " << _epsilon_dielectric << endl;
	}
	else throw std::runtime_error("Error in CalcEstatics::Initialize : no such estatic method, should be simple or distdep");
   }
   else throw std::runtime_error("Error in CalcEstatics:Initialize : no estatic_method specified");
}

bool CalcEstatics::EvaluateFrame(QMTopology *top) {
    vector<CrgUnit *> lcharges = top->CrgUnits();
    Topology atop;
    atop.setBox(top->getBox());
    for (vector<CrgUnit *>::iterator itl = lcharges.begin(); itl != lcharges.end(); itl++) {
        top->AddAtomisticBeads(*itl, &atop);
    }
    cout << "Number of charge units in top " << lcharges.size() << endl;
    cout << "Number of molecules in atop " << atop.MoleculeCount() << endl;
    MoleculeContainer::iterator imol;
    for (imol = atop.Molecules().begin(); imol != atop.Molecules().end(); imol++) {
        Molecule *mol = *imol;

        //Compute the EstaticPotentialEnergy for molecule mol if it is neutral
        double neutr = CalcPot(&atop, mol);

        CrgUnit *crg = mol->getUserData<CrgUnit > ();

        //Change the charges on the molecule mol to the occupied=charged state
        top->CopyChargesOccupied(crg, mol);

	//Compute the EstaticPotentialEnergy for molecule mol if it is charged
        double crged = CalcPot(&atop, mol);

        //If the site energy is large and positive (crged >> neutr) molecule mol wants to stay neutral
        //This is consistent with marcus_rates.h since w_12 (from 1 to 2) contains (dG_12 + \lambda)^2 and dG_12=e_2-e_1
        crg->setEnergy(crged - neutr);

        //Copy back the charges as if molecule mol would be neutral so that the loop can pick up the next molecule
        top->CopyCharges(crg, mol);
//        cout << "Estatic energy [eV] for charged / neutral / crg-neutr=espilon: " << crged << " " << neutr << " " << crged - neutr << "\n";
        cout << "Estatic energy [eV] for crgunit " << crg->getId()  <<  " is " << crged - neutr << "\n";
    }

    atop.Cleanup();

    return true;
}

//Calculate Estatics

double CalcEstatics::CalcPot(Topology *atop, Molecule *mol) //wegen Übergabe per * unten ->
{
    //estatic energy including contributions from all other molecules in eV
    double pot = 0.0;
    //Coulombs constant including conversion factors (elementary charge and nm) so that pot is in eV
    double k_c = 1.602176487e-19 / (1.0e-9 * 8.854187817e-12 * 4.0 * M_PI);

    //Do loop over all other molecules imol that are not mol
    MoleculeContainer::iterator imol;
    for (imol = atop->Molecules().begin(); imol != atop->Molecules().end(); imol++) {
        if (*imol == mol) continue;
        //Check whether PBC have to be taken into account
        //We should replace getUserData by a funtion GetCom for molecules
        vec bcs = atop->BCShortestConnection(mol->getUserData<CrgUnit > ()->GetCom(), (*imol)->getUserData<CrgUnit > ()->GetCom());
        vec dist = (*imol)->getUserData<CrgUnit > ()->GetCom() - mol->getUserData<CrgUnit > ()->GetCom();
        vec diff = bcs - dist;
        
        //cout<<" s="<<s<<endl;
        for (int i = 0; i < mol->BeadCount(); i++)
            for (int j = 0; j != (*imol)->BeadCount(); j++) {
                Bead *bi = mol->getBead(i);
                Bead *bj = (*imol)->getBead(j);
                //distance vector may have to be shifted by s
                vec r_v = bi->getPos()-(bj->getPos() + diff);
                double r = abs(r_v);
                double qi = bi->getQ();
                double qj = bj->getQ();
                //cout<<bj->getPos()<<r_v<<r<<qi<<qj<<_epsilon_dielectric<<pot<<qi*qj/_epsilon_dielectric*1/r<<endl;
                pot += k_c * qi * qj / ( (this->*_estatic_method)(r) * r ); //in eV
            }
    }
    return pot;
}

double CalcEstatics::dist_dep_eps(const double &dist)
{
    return _epsilon_dielectric - (_epsilon_dielectric-1.0)*exp(-_s_eps*dist)*(1.0+_s_eps*dist+0.5*_s_eps*dist*_s_eps*dist);
}
    
double CalcEstatics::constant_epsilon(const double &dist)
{
   return _epsilon_dielectric; 
}
