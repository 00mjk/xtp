/*
 *            Copyright 2009-2016 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// UBLAS stops checking types and array bounds if this flag is defined
#define NDEBUG
#define BOOST_UBLAS_NDEBUG

#ifndef _VOTCA_XTP_DFTENGINE_H
#define	_VOTCA_XTP_DFTENGINE_H

#include <votca/xtp/segment.h>
#include <votca/xtp/orbitals.h>

#include <votca/xtp/logger.h>



#include <boost/filesystem.hpp>
#include <votca/xtp/ERIs.h>


#include <votca/xtp/numerical_integrations.h>

namespace votca { namespace xtp {
    namespace ub = boost::numeric::ublas;

        /**
         * \brief Electronic ground-state via Density-Functional Theory
         *
         * Evaluates electronic ground state in molecular systems based on
         * density functional theory with Gaussian Orbitals.
         * 
         */

class DFTENGINE 
{
public:

    DFTENGINE() { };
   ~DFTENGINE() { };

   
   
    void    Initialize( Property *options);
    std::string  Identify() { return "dftengine"; }
   
    void    CleanUp();

    void setLogger( Logger* pLog ) { _pLog = pLog; }
    
    bool Evaluate(   Orbitals* _orbitals );

    // interfaces for options getting/setting
    //bool get_do_qp_diag(){ return _do_qp_diag ;}
    //void set_do_qp_diag( bool inp ){ _do_qp_diag = inp;}
    
    
    private:

    Logger *_pLog;
    
    void Prepare( Orbitals* _orbitals );
    void SetupInvariantMatrices();
    
    
    
    void NuclearRepulsion();
    void EvolveDensityMatrix(Orbitals* _orbitals);
    
    //bool   _maverick;
    
    // program tasks
    //bool                                _do_qp_diag;
    
    // storage tasks
    //bool                                _store_qp_pert;
    
    int                                 _openmp_threads;
    
    
    std::string _outParent;
    std::string _outMonDir;
    
    // options
    std::string _dft_options;
    Property _dftengine_options; 
    
    // atoms
    std::vector<QMAtom*>                _atoms;

    // basis sets
    std::string                              _auxbasis_name;
    std::string                              _dftbasis_name;
    std::string                              _ecp_name;
    BasisSet                            _dftbasisset;
    BasisSet                            _auxbasisset;
    BasisSet                            _ecpbasisset;
    AOBasis                             _dftbasis;
    AOBasis                             _auxbasis;
    AOBasis                             _ecp;
    
    bool                                _with_ecp;
    
    // numerical integration 
    std::string                              _grid_name;
    NumericalIntegration                _gridIntegration;

    // AO Matrices
    AOOverlap                           _dftAOoverlap;
   
   // AOCoulomb                           _dftAOcoulomb;
    
    ub::matrix<double>                  _AuxAOcoulomb_inv;
    AOKinetic                           _dftAOkinetic;
    AOESP                               _dftAOESP;
    AOECP                               _dftAOECP;
    
    bool                                _with_guess;
    double                              E_nucnuc;
    
    //
    double                              _mixingparameter;
    double                              _Econverged;
    int                                 _numofelectrons;
    int                                 _max_iter;
    int                                 _this_iter;
    ub::matrix<double>                  _dftAOdmat;
    
    
    std::list< ub::matrix<double> >   _dftdmathist;
    std::list< ub::matrix<double> >   _errormatrixhist;
    //Electron repulsion integrals
    ERIs                                _ERIs;
    

    
    
    
    // exchange and correlation
    std::string                              _xc_functional_name;


    
  
    
};


}}

#endif	/* _VOTCA_XTP_DFTENGINE_H */
