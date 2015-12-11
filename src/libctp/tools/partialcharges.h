/* 
 *            Copyright 2009-2012 The VOTCA Development Team
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

#ifndef _VOTCA_CTP_PARTIALCHARGES_H
#define _VOTCA_CTP_PARTIALCHARGES_H

#include <stdio.h>
#include <votca/ctp/espfit.h>
#include <votca/ctp/mulliken.h>
#include <votca/ctp/logger.h>
#include <boost/filesystem.hpp>

namespace votca { namespace ctp {
    using namespace std;
    
class Partialcharges : public QMTool
{
public:

    Partialcharges () { };
   ~Partialcharges () { };

    string Identify() { return "partialcharges"; }

    void   Initialize(Property *options);
    bool   Evaluate();


private:
    
    string      _orbfile;
    string      _output_file;
    int         _state_no;  
    int         _openmp_threads;
    string      _state;
    string      _method;
    string      _spin;
    string      _integrationmethod;
    string      _gridsize;
    bool        _use_mps;
    bool        _use_pdb;
    bool        _use_mulliken;
    bool        _use_CHELPG;
    bool        _use_GDMA;
    bool        _use_CHELPG_SVD;
    bool        _use_ecp;
    
    Logger      _log;
    
    void Extractingcharges( Orbitals& _orbitals );

};

void Partialcharges::Initialize(Property* options) {
    _use_ecp=false;
    _use_mps=false;
    _use_pdb=false;
    
    _use_mulliken=false;
    _use_CHELPG=false;
    _use_GDMA=false;
    _use_CHELPG_SVD=false;

            // update options with the VOTCASHARE defaults   
    UpdateWithDefaults( options );
 

            string key = "options." + Identify();
            // _jobfile = options->get(key + ".file").as<string>();

            // key = "options." + Identify();
 
       
           // orbitals file or pure DFT output
           
    _orbfile      = options->get(key + ".input").as<string> ();
    _state     = options->get(key + ".state").as<string> ();
    _output_file  = options->get(key + ".output").as<string> ();
    _state_no     = options->get(key + ".statenumber").as<int> ();
    _spin     = options->get(key + ".spin").as<string> ();
    if ( options->exists(key+".ecp")) {
       _use_ecp=options->get(key + ".ecp").as<bool> ();
    }
    if ( options->exists(key+".method")) {
         _method = options->get(key + ".method").as<string> ();
         if (_method=="Mulliken")_use_mulliken=true; 
         else if(_method=="mulliken")_use_mulliken=true; 
         else if(_method=="CHELPG")_use_CHELPG=true; 
         else if(_method=="GDMA") throw std::runtime_error("GDMA not implemented yet");
         else if(_method=="CHELPG_SVD") throw std::runtime_error("CHELPG_SVD not implemented yet"); 
         else  throw std::runtime_error("Method not recognized. Only Mulliken and CHELPG implemented");
         }
    else _use_CHELPG=true;
    if (!_use_mulliken){
         _integrationmethod     = options->get(key + ".integrationmethod").as<string> ();
    }
    string data_format  = boost::filesystem::extension(_output_file);

    if (!(_integrationmethod=="numeric" || _integrationmethod=="analytic")){
        std::runtime_error("Method not recognized. Only numeric and analytic available");
    }
    if ( options->exists(key+".gridsize")) {
         _gridsize = options->get(key+".gridsize").as<string>();
         }
    else _gridsize="medium";
    if ( options->exists(key+".openmp")) {
         _openmp_threads = options->get(key+".openmp").as<int>();
         }
    else _openmp_threads=0;
              
    
    
    if (data_format==".mps")_use_mps=true; 
    else if(data_format==".pdb")_use_pdb=true;    
    else  throw std::runtime_error("Outputfile format not recognized. Export only to .pdb and .mps");
    
    // get the path to the shared folders with xml files
    char *votca_share = getenv("VOTCASHARE");    
    if(votca_share == NULL) throw std::runtime_error("VOTCASHARE not set, cannot open help files.");

    
    ub::matrix<double> M=ub::zero_matrix<double>(4,5);
    M(0,0)=1;
    M(0,4)=2;
    M(1,2)=3;
    M(3,1)=2;
    cout <<"M"<< endl;
    cout << M << endl;
    ub::matrix<double>VT=ub::zero_matrix<double>(5,5);
    ub::vector<double>S=ub::zero_vector<double>(4);
    
    linalg_singular_value_decomposition(M,VT,S);
    
    cout << "VT"<< endl;
    cout << VT<< endl;
    cout << "S"<< endl;
    cout << S<< endl;
    cout << "U"<< endl;
    cout << M<< endl;
    
    
    
    exit(0);
    
   
}

bool Partialcharges::Evaluate() {
    

    _log.setReportLevel( logDEBUG );
    _log.setMultithreading( true );
    
    _log.setPreface(logINFO,    "\n... ...");
    _log.setPreface(logERROR,   "\n... ...");
    _log.setPreface(logWARNING, "\n... ...");
    _log.setPreface(logDEBUG,   "\n... ..."); 

    LOG(logDEBUG, _log) << "Converting serialized QM data in " << _orbfile << flush;

    Orbitals _orbitals;
    // load the QM data from serialized orbitals object

    std::ifstream ifs( (_orbfile).c_str());
    LOG(logDEBUG, _log) << " Loading QM data from " << _orbfile << flush;
    boost::archive::binary_iarchive ia(ifs);
    ia >> _orbitals;
    ifs.close();

    Extractingcharges(_orbitals);
    string tag="TOOL:"+Identify()+"_"+_state+"_"+_spin+boost::lexical_cast<string>(_state_no);
    if(_use_mps){
        QMMInterface Converter;
        PolarSeg* result=Converter.Convert(_orbitals.QMAtoms());
        
        result->WriteMPS(_output_file,tag);
        }
    else if(_use_pdb){
        FILE *out;
        out = fopen(_output_file.c_str(), "w");
       _orbitals.WritePDB(out, tag); 
    }
    
    
    LOG(logDEBUG, _log) << "Written charges to " << _output_file << flush;
    
    return true;
}




void Partialcharges::Extractingcharges( Orbitals& _orbitals ){
    int threads=1;
#ifdef _OPENMP
            if ( _openmp_threads > 0 ) omp_set_num_threads(_openmp_threads); 
            threads=omp_get_max_threads();
#endif
   LOG(logDEBUG, _log) << "===== Running Partialcharges on "<< threads << " threads ===== " << flush;

        vector< QMAtom* > Atomlist =_orbitals.QMAtoms();
        ub::matrix<double> DMAT_tot;
        BasisSet bs;
        bs.LoadBasisSet(_orbitals.getDFTbasis());
        AOBasis basis;
        basis.AOBasisFill(&bs, Atomlist );
        basis.ReorderMOs(_orbitals.MOCoefficients(), _orbitals.getQMpackage(), "votca" );  
        bool _do_transition=false;
        if(_state=="transition"){
            _do_transition=true;
            if (_spin=="singlet"){
                DMAT_tot=_orbitals.TransitionDensityMatrix(_orbitals.MOCoefficients() , _orbitals.BSESingletCoefficients(), _state_no-1);
            }
            else if (_spin=="triplet"){
                DMAT_tot=_orbitals.TransitionDensityMatrix(_orbitals.MOCoefficients() , _orbitals.BSETripletCoefficients(), _state_no-1); 
            }
            else throw std::runtime_error("Spin entry not recognized");
        }
        else if (_state=="ground" || _state=="excited"){
            
        
            ub::matrix<double> &DMATGS=_orbitals.DensityMatrixGroundState(_orbitals.MOCoefficients());
            DMAT_tot=DMATGS;
            if ( _state_no > 0 && _state=="excited"){
                std::vector<ub::matrix<double> > DMAT;
                if (_spin=="singlet"){
                    DMAT = _orbitals.DensityMatrixExcitedState( _orbitals.MOCoefficients() , _orbitals.BSESingletCoefficients(), _state_no-1);

                }
                else if (_spin=="triplet"){
                    DMAT = _orbitals.DensityMatrixExcitedState( _orbitals.MOCoefficients() , _orbitals.BSETripletCoefficients(), _state_no-1);
                }
                else throw std::runtime_error("Spin entry not recognized");
                DMAT_tot=DMAT_tot-DMAT[0]+DMAT[1];
            }            
	   // Ground state + hole_contribution + electron contribution
	}
        else throw std::runtime_error("State entry not recognized");
        
        
        if (_use_mulliken) {
            Mulliken mulliken;
            mulliken.EvaluateMulliken(Atomlist, DMAT_tot, basis, bs, _do_transition);
                
        }
        else if (_use_CHELPG){         
            Espfit esp=Espfit(&_log);
            esp.setUseECPs(_use_ecp);
            if (_integrationmethod=="numeric")  {
                esp.Fit2Density(Atomlist, DMAT_tot, basis,bs,_gridsize); 
            }
            else if (_integrationmethod=="analytic")  esp.Fit2Density_analytic(Atomlist,DMAT_tot,basis);
        }
}       

}}


#endif