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

#include "gaussian.h"
#include "votca/ctp/segment.h"
#include <boost/algorithm/string.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/format.hpp>

#include <stdio.h>
#include <iomanip>
#include <sys/stat.h>
#include <vector>

using namespace std;

namespace votca { namespace ctp {
    namespace ub = boost::numeric::ublas;
    
void Gaussian::Initialize( Property *options ) {

    // GAUSSIAN file names
    string fileName = "system";

    _xyz_file_name = fileName + ".xyz";
    _input_file_name = fileName + ".com";
    _log_file_name = fileName + ".log"; 
    _shell_file_name = fileName + ".sh";
    _orb_file_name = "fort.7" ;               

    string key = "package";
    string _name = options->get(key+".name").as<string> ();
    
    if ( _name != "gaussian" ) {
        cerr << "Tried to use " << _name << " package. ";
        throw std::runtime_error( "Wrong options file");
    }
    
    _executable =       options->get(key + ".executable").as<string> ();
    _charge =           options->get(key + ".charge").as<int> ();
    _spin =             options->get(key + ".spin").as<int> ();
    _options =          options->get(key + ".options").as<string> ();
    _memory =           options->get(key + ".memory").as<string> ();
    _threads =          options->get(key + ".threads").as<int> ();
    _chk_file_name  =   options->get(key + ".checkpoint").as<string> ();
    _scratch_dir =      options->get(key + ".scratch").as<string> ();
    _cleanup =          options->get(key + ".cleanup").as<string> ();
    
    // check if the guess keyword is present, if yes, append the guess later
    std::string::size_type iop_pos = _options.find("cards");
    if (iop_pos != std::string::npos) {
        _write_guess = true;
    } else
    {
        _write_guess = false;
    }

    // check if the pop keyword is present, if yes, get the charges and save them
    iop_pos = _options.find("pop");
    if (iop_pos != std::string::npos) {
        _get_charges = true;
    } else
    {
        _get_charges = false;
    }

    // check if the charge keyword is present, if yes, get the self energy and save it
    iop_pos = _options.find("charge");
    if (iop_pos != std::string::npos) {
        _get_self_energy = true;
        _write_charges = true;
    } else
    {
        _get_self_energy = false;
        _write_charges = false;
    }


}    

// This class should not be here ...
void Gaussian::WriteInputHeader(FILE *out, string tag) {

    if (_chk_file_name.size())
    fprintf(out, "%%chk = %s\n", _chk_file_name.c_str());
    if (_memory.size())
    fprintf(out, "%%mem = %s\n", _memory.c_str());
    fprintf(out, "%%nprocshared = %1d\n", _threads);
    fprintf(out, "%s\n", _options.c_str());
    fprintf(out, "\n");
    fprintf(out, "%s\n", tag.c_str());
    fprintf(out, "\n");
    fprintf(out, "%2d %2d\n", _charge, _spin);
    return;
}

/**
 * Prepares the com file from a vector of segments
 * Appends a guess constructed from monomer orbitals if supplied
 */
bool Gaussian::WriteInputFile( vector<Segment* > segments, Orbitals* orbitals_guess )
{
    vector< Atom* > _atoms;
    vector< Atom* > ::iterator ait;
    vector< Segment* >::iterator sit;
    
    int qmatoms = 0;

    ofstream _com_file;
    
    string _com_file_name_full = _run_dir + "/" + _input_file_name;
    
    _com_file.open ( _com_file_name_full.c_str() );
    // header 
    if ( _chk_file_name.size() != 0 ) {
        _com_file << "%chk = " << _chk_file_name << endl;
    }
    
    if ( _memory.size() != 0 ) {
        _com_file << "%mem = " << _memory << endl ;
    }
    
    if ( _threads != 0 ) {
         _com_file << "%nprocshared = "  << _threads << endl;
    }
        
    if ( _options.size() != 0 ) {
        _com_file <<  _options << endl ;
    }
    
    _com_file << endl;
    _com_file << "TITLE ";
    for (sit = segments.begin() ; sit != segments.end(); ++sit) {
        _com_file << (*sit)->getName() << " ";
    }
    _com_file << endl << endl;
      
    _com_file << setw(2) << _charge << setw(2) << _spin << endl;

    for (sit = segments.begin() ; sit != segments.end(); ++sit) {
        _atoms = (*sit)-> Atoms();

        for (ait = _atoms.begin(); ait < _atoms.end(); ++ait) {

            if ((*ait)->HasQMPart() == false) { continue; }

            vec     pos = (*ait)->getQMPos();
            string  name = (*ait)->getElement();

            //fprintf(out, "%2s %4.7f %4.7f %4.7f \n"
            _com_file << setw(3) << name.c_str() 
                      << setw(12) << setiosflags(ios::fixed) << setprecision(5) << pos.getX()*10
                      << setw(12) << setiosflags(ios::fixed) << setprecision(5) << pos.getY()*10
                      << setw(12) << setiosflags(ios::fixed) << setprecision(5) << pos.getZ()*10 
                      << endl;
        }
    } 
    
    if ( _write_guess ) {
        if ( orbitals_guess == NULL ) {
            throw std::runtime_error( "A guess for dimer orbitals has not been prepared.");
        } else {
            vector<int> _sort_index;
            
            orbitals_guess->SortEnergies( &_sort_index );
            
            _com_file << endl << "(5D15.8)" << endl;
            
            int level = 1;
            int ncolumns = 5;
            
            for ( vector< int > ::iterator soi = _sort_index.begin(); soi != _sort_index.end(); ++ soi ) {
                
                double _energy = (orbitals_guess->_mo_energies)[*soi] ;
                
                _com_file  << setw(5) << level  << " Alpha MO OE=" << FortranFormat( _energy ) << endl;
                
                ub::matrix_row< ub::matrix<double> > mr (orbitals_guess->_mo_coefficients, *soi);
                
                int column = 1;
                for (unsigned j = 0; j < mr.size (); ++j) {
                        _com_file <<  FortranFormat( mr[j] );
                        if (column == ncolumns) { _com_file << std::endl; column = 0; }
                        column++;
                }
                
                level++;
                _com_file << endl;
            } 
        }
    }
    
    if ( _write_charges ) {
        vector< QMAtom* > *qmatoms = orbitals_guess->getAtoms();
        vector< QMAtom* >::iterator it;
        
        for (it = qmatoms->begin(); it < qmatoms->end(); it++ ) {
            if ( !(*it)->from_environment ) {
            _com_file << (*it)->type << " " <<  (*it)->x << " " << (*it)->y << " " << (*it)->z << endl;
            }
        }
        
        _com_file << endl;
        
        for (it = qmatoms->begin(); it < qmatoms->end(); it++ ) {
            if ( (*it)->from_environment ) {
                boost::format fmt("%1$+1.7f %2$+1.7f %3$+1.7f %4$+1.7f");
                fmt % (*it)->x % (*it)->y % (*it)->z % (*it)->charge;
                _com_file << fmt << endl;
            }
        }
        
        _com_file << endl;
    }

    
    _com_file << endl;
    _com_file.close();
}

bool Gaussian::WriteShellScript() {
    ofstream _shell_file;
    
    string _shell_file_name_full = _run_dir + "/" + _shell_file_name;
            
    _shell_file.open ( _shell_file_name_full.c_str() );

    _shell_file << "#!/bin/tcsh" << endl ;
    _shell_file << "mkdir -p " << _scratch_dir << endl;
    _shell_file << "setenv GAUSS_SCRDIR " << _scratch_dir << endl;
    _shell_file << _executable << " " << _input_file_name << endl;    
    _shell_file.close();
}

/**
 * Runs the Gaussian job. Returns 
 */
bool Gaussian::Run()
{

    LOG(logDEBUG,*_pLog) << "Running GAUSSIAN job" << flush;
    
    if (system(NULL)) {
        // if scratch is provided, run the shell script; 
        // otherwise run gaussian directly and rely on global variables 
        string _command;
        if ( _scratch_dir.size() != 0 ) {
            _command  = "cd " + _run_dir + "; tcsh " + _shell_file_name;
        }
        else {
            _command  = "cd " + _run_dir + "; " + _executable + " " + _input_file_name;
        }
        
        int i = system ( _command.c_str() );
        LOG(logDEBUG,*_pLog) << "Finished GAUSSIAN job" << flush;
        return true;
    }
    else {
        LOG(logERROR,*_pLog) << _input_file_name << " failed to start" << flush; 
        return false;
    }
    



}

/**
 * Cleans up after the Gaussian job
 */
void Gaussian::CleanUp() {
    
    // cleaning up the generated files
    if ( _cleanup.size() != 0 ) {
        Tokenizer tok_cleanup(_cleanup, " \t\n");
        vector <string> _cleanup_info;
        tok_cleanup.ToVector(_cleanup_info);
        
        vector<string> ::iterator it;
               
        for (it = _cleanup_info.begin(); it != _cleanup_info.end(); ++it) {
            if ( *it == "com" ) {
                string file_name = _run_dir + "/" + _input_file_name;
                remove ( file_name.c_str() );
            }
            
            if ( *it == "sh" ) {
                string file_name = _run_dir + "/" + _shell_file_name;
                remove ( file_name.c_str() );
            }
            
            if ( *it == "log" ) {
                string file_name = _run_dir + "/" + _log_file_name;
                remove ( file_name.c_str() );
            }

           if ( *it == "chk" ) {
                string file_name = _run_dir + "/" + _chk_file_name;
                remove ( file_name.c_str() );
            }
            
            if ( *it == "fort.7" ) {
                string file_name = _run_dir + "/" + *it;
                remove ( file_name.c_str() );
            }            
        }
    }
    
}



/**
 * Reads in the MO coefficients from a GAUSSIAN fort.7 file
 */
bool Gaussian::ParseOrbitalsFile( Orbitals* _orbitals )
{
    std::map <int, std::vector<double> > _coefficients;
    std::map <int, double> _energies;
    
    std::string _line;
    unsigned _levels = 0;
    unsigned _level;
    unsigned _basis_size = 0;

    string _orb_file_name_full = _run_dir + "/" + _orb_file_name;
    std::ifstream _input_file( _orb_file_name_full.c_str() );
    
    if (_input_file.fail()) {
        LOG( logERROR, *_pLog ) << "File " << _orb_file_name << " with molecular orbitals is not found " << flush;
        return false;
    } else {
        LOG(logDEBUG, *_pLog) << "Reading MOs from " << _orb_file_name << flush;
    }

    // number of coefficients per line is  in the first line of the file (5D15.8)
    getline(_input_file, _line);
    std::vector<string> strs;
    boost::algorithm::split(strs, _line, boost::is_any_of("(D)"));
    //clog << strs.at(1) << endl;
    int nrecords_in_line = boost::lexical_cast<int>(strs.at(1));
    string format = strs.at(2);

    //clog << endl << "Orbital file " << filename << " has " 
    //        << nrecords_in_line << " records per line, in D"
    //        << format << " format." << endl;

    while (_input_file) {

        getline(_input_file, _line);
        // if a line has an equality sign, must be energy
        std::string::size_type energy_pos = _line.find("=");

        if (energy_pos != std::string::npos) {

            vector<string> results;
            boost::trim( _line );
            
            boost::algorithm::split(results, _line, boost::is_any_of("\t ="),
                    boost::algorithm::token_compress_on); 
            //cout << results[1] << ":" << results[2] << ":" << results[3] << ":" << results[4] << endl;
            
            _level = boost::lexical_cast<int>(results.front());
            boost::replace_first(results.back(), "D", "e");
            _energies[ _level ] = boost::lexical_cast<double>( results.back() );            
            _levels++;

        } else {
            
            while (_line.size() > 1) {
                string _coefficient;
                _coefficient.assign( _line, 0, 15 );
                boost::trim( _coefficient );
                boost::replace_first( _coefficient, "D", "e" );
                double coefficient = boost::lexical_cast<double>( _coefficient );
                _coefficients[ _level ].push_back( coefficient );
                _line.erase(0, 15);
            }
        }
    }

    // some sanity checks
    LOG( logDEBUG, *_pLog ) << "Energy levels: " << _levels << flush;

    std::map< int, vector<double> >::iterator iter = _coefficients.begin();
    _basis_size = iter->second.size();

    for (iter = _coefficients.begin()++; iter != _coefficients.end(); iter++) {
        if (iter->second.size() != _basis_size) {
            LOG( logERROR, *_pLog ) << "Error reading " << _orb_file_name << ". Basis set size change from level to level." << flush;
            return false;
        }
    }
    
    LOG( logDEBUG, *_pLog ) << "Basis set size: " << _basis_size << flush;

    // copying information to the orbitals object
    _orbitals->_basis_set_size = _basis_size;
    _orbitals->_has_basis_set_size = true;
    _orbitals->_has_mo_coefficients = true;
    _orbitals->_has_mo_energies = true;
    
   // copying energies to a matrix  
   _orbitals->_mo_energies.resize( _levels );
   _level = 1;
   for(size_t i=0; i < _orbitals->_mo_energies.size(); i++) {
         _orbitals->_mo_energies[i] = _energies[ _level++ ];
   }
   
   // copying orbitals to the matrix
   (_orbitals->_mo_coefficients).resize( _levels, _basis_size );     
   for(size_t i = 0; i < _orbitals->_mo_coefficients.size1(); i++) {
      for(size_t j = 0 ; j < _orbitals->_mo_coefficients.size2(); j++) {
         _orbitals->_mo_coefficients(i,j) = _coefficients[i+1][j];
         //cout << i << " " << j << endl;
      }
   }

    
   //cout << _mo_energies << endl;   
   //cout << _mo_coefficients << endl; 
   
   // cleanup
   _coefficients.clear();
   _energies.clear();
   
     
   LOG(logDEBUG, *_pLog) << "Done reading MOs" << flush;

   return true;
}

bool Gaussian::CheckLogFile() {
    
    // check if the log file exists
    char ch;
    ifstream _input_file((_run_dir + "/" + _log_file_name).c_str());
    
    if (_input_file.fail()) {
        LOG(logERROR,*_pLog) << "Gaussian LOG is not found" << flush;
        return false;
    };

    _input_file.seekg(0,ios_base::end);   // go to the EOF
    
    // get empty lines and end of lines out of the way
    do {
        _input_file.seekg(-2,ios_base::cur);
        _input_file.get(ch);   
        //cout << "\nChar: " << ch << endl;
    } while ( ch == '\n' || ch == ' ' || (int)_input_file.tellg() == -1 );
 
    // get the beginning of the line or the file
    do {
       _input_file.seekg(-2,ios_base::cur);
       _input_file.get(ch);   
       //cout << "\nNext Char: " << ch << " TELL G " <<  (int)_input_file.tellg() << endl;
    } while ( ch != '\n' || (int)_input_file.tellg() == -1 );
            
    string _line;            
    getline(_input_file,_line);                      // Read the current line
    //cout << "\nResult: " << _line << '\n';     // Display it
    _input_file.close();
        
    std::string::size_type self_energy_pos = _line.find("Normal termination of Gaussian");
    if (self_energy_pos == std::string::npos) {
            LOG(logERROR,*_pLog) << "Gaussian LOG is incomplete" << flush;
            return false;      
    } else {
            //LOG(logDEBUG,*_pLog) << "Gaussian LOG is complete" << flush;
            return true;
    }
}

/**
 * Parses the Gaussian Log file and stores data in the Orbitals object 
 */
bool Gaussian::ParseLogFile( Orbitals* _orbitals ) {

    string _line;
    vector<string> results;
    bool _has_occupied_levels = false;
    bool _has_unoccupied_levels = false;
    bool _has_number_of_electrons = false;
    bool _has_basis_set_size = false;
    bool _has_overlap_matrix = false;
    bool _has_charges = false;
    bool _has_coordinates = false;
    bool _has_qm_energy = false;
    bool _has_self_energy = false;
    
    int _occupied_levels = 0;
    int _unoccupied_levels = 0;
    int _number_of_electrons = 0;
    int _basis_set_size = 0;
    
    LOG(logDEBUG,*_pLog) << "Parsing " << _log_file_name << flush;
    string _log_file_name_full =  _run_dir + "/" + _log_file_name;
    // check if LOG file is complete
    if ( !CheckLogFile() ) return false;
    
    // Start parsing the file line by line
    ifstream _input_file(_log_file_name_full.c_str());
    while (_input_file) {

        getline(_input_file, _line);
        boost::trim(_line);

        /*
         * number of occupied and virtual orbitals
         * N alpha electrons      M beta electrons
         */
        std::string::size_type electrons_pos = _line.find("alpha electrons");
        if (electrons_pos != std::string::npos) {
            boost::algorithm::split(results, _line, boost::is_any_of("\t "), boost::algorithm::token_compress_on);
            _has_number_of_electrons = true;
            _number_of_electrons =  boost::lexical_cast<int>(results.front()) ;
            _orbitals->_number_of_electrons = _number_of_electrons ;
            _orbitals->_has_number_of_electrons = true;
            LOG(logDEBUG,*_pLog) << "Alpha electrons: " << _number_of_electrons << flush ;
        }

        /*
         * basis set size
         * N basis functions,  M primitive gaussians,   K cartesian basis functions
         */
        std::string::size_type basis_pos = _line.find("basis functions,");
        if (basis_pos != std::string::npos) {
            boost::algorithm::split(results, _line, boost::is_any_of("\t "), boost::algorithm::token_compress_on);
            _has_basis_set_size = true;
            _basis_set_size = boost::lexical_cast<int>(results.front());
            _orbitals->_basis_set_size = _basis_set_size ;
            _orbitals->_has_basis_set_size = true;
            LOG(logDEBUG,*_pLog) << "Basis functions: " << _basis_set_size << flush;
        }

        /*
         * energies of occupied/unoccupied levels
         * Alpha  occ.(virt.) eigenvalues -- e1 e2 e3 e4 e5
         */
        std::string::size_type eigenvalues_pos = _line.find("Alpha");
        if (eigenvalues_pos != std::string::npos) {
            
            std::list<std::string> stringList;
            //int _unoccupied_levels = 0;
            //int _occupied_levels = 0;

            while (eigenvalues_pos != std::string::npos && !_has_occupied_levels && !_has_unoccupied_levels) {
                //cout << _line << endl;

                boost::iter_split(stringList, _line, boost::first_finder("--"));

                vector<string> energies;
                boost::trim(stringList.back());

                boost::algorithm::split(energies, stringList.back(), boost::is_any_of("\t "), boost::algorithm::token_compress_on);

                if (stringList.front().find("virt.") != std::string::npos) {
                    _unoccupied_levels += energies.size();
                    energies.clear();
                }

                if (stringList.front().find("occ.") != std::string::npos) {
                    _occupied_levels += energies.size();
                    energies.clear();
                }

                getline(_input_file, _line);
                eigenvalues_pos = _line.find("Alpha");
                boost::trim(_line);

                //boost::iter_split(stringList, _line, boost::first_finder("--"));

                if (eigenvalues_pos == std::string::npos) {
                    _has_occupied_levels = true;
                    _has_unoccupied_levels = true;
                    _orbitals->_occupied_levels = _occupied_levels;
                    _orbitals->_unoccupied_levels = _unoccupied_levels;
                    _orbitals->_has_occupied_levels = true;
                    _orbitals->_has_unoccupied_levels = true;
                    LOG(logDEBUG,*_pLog) << "Occupied levels: " << _occupied_levels << flush;
                    LOG(logDEBUG,*_pLog) << "Unoccupied levels: " << _unoccupied_levels << flush;
                }
            } // end of the while loop              
        } // end of the eigenvalue parsing
        
 
         /*
         * overlap matrix
         * stored after the *** Overlap *** line
         */
        std::string::size_type overlap_pos = _line.find("*** Overlap ***");
        if (overlap_pos != std::string::npos ) {
            
            // prepare the container
            _orbitals->_has_overlap = true;
            (_orbitals->_overlap).resize( _basis_set_size );
            
            _has_overlap_matrix = true;
            //cout << "Found the overlap matrix!" << endl;   
            vector<int> _j_indeces;
            
            int _n_blocks = 1 + (( _basis_set_size - 1 ) / 5);
            //cout << _n_blocks;
            
            getline(_input_file, _line); boost::trim( _line );

            for (int _block = 0; _block < _n_blocks; _block++ ) {
                
                // first line gives the j index in the matrix
                //cout << _line << endl;
                
                boost::tokenizer<> tok( _line );
                std::transform( tok.begin(), tok.end(), std::back_inserter( _j_indeces ), &boost::lexical_cast<int,std::string> );
                //std::copy( _j_indeces.begin(), _j_indeces.end(), std::ostream_iterator<int>(std::cout,"\n") );
            
                // read the block of max _basis_size lines + the following header
                for (int i = 0; i <= _basis_set_size; i++ ) {
                    getline (_input_file, _line); 
                    //cout << _line << endl;
                    if ( std::string::npos == _line.find("D") ) break;
                    
                    // split the line on the i index and the rest
                    
                    vector<string> _row;
                    boost::trim( _line );
                    boost::algorithm::split( _row , _line, boost::is_any_of("\t "), boost::algorithm::token_compress_on); 
                   
                            
                    int _i_index = boost::lexical_cast<int>(  _row.front()  ); 
                    _row.erase( _row.begin() );
                    
                    //cout << _i_index << ":" << _line << endl ;
                    
                    std::vector<int>::iterator _j_iter = _j_indeces.begin();
                    
                    for (std::vector<string>::iterator iter = _row.begin()++; iter != _row.end(); iter++) {
                        string  _coefficient = *iter;
                       
                        boost::replace_first( _coefficient, "D", "e" );
                        //cout << boost::lexical_cast<double>( _coefficient ) << endl;
                        
                        int _j_index = *_j_iter;                                
                        //_overlap( _i_index-1 , _j_index-1 ) = boost::lexical_cast<double>( _coefficient );
                        _orbitals->_overlap( _i_index-1 , _j_index-1 ) = boost::lexical_cast<double>( _coefficient );
                        _j_iter++;
                        
                    }
 
                    
                }
                
                // clear the index for the next block
                _j_indeces.clear();        
            } // end of the blocks
            LOG(logDEBUG,*_pLog) << "Read the overlap matrix" << flush;
        } // end of the if "Overlap" found   

        
        /*
         *  Partial charges from the input file
         */
        std::string::size_type charge_pos = _line.find("Charges from ESP fit, RMS");
        
        if (charge_pos != std::string::npos && _get_charges ) {        
                LOG(logDEBUG,*_pLog) << "Getting charges" << flush;
                _has_charges = true;
                getline(_input_file, _line);
                getline(_input_file, _line);
                
                vector<string> _row;
                getline(_input_file, _line);
                boost::trim( _line );
                //cout << _line << endl;
                boost::algorithm::split( _row , _line, boost::is_any_of("\t "), boost::algorithm::token_compress_on); 
                int nfields =  _row.size();
                //cout << _row.size() << endl;
                
                while ( nfields == 3 ) {
                    int atom_id = boost::lexical_cast< int >( _row.at(0) );
                    int atom_number = boost::lexical_cast< int >( _row.at(0) );
                    string atom_type = _row.at(1);
                    double atom_charge = boost::lexical_cast< double >( _row.at(2) );
                    //if ( tools::globals::verbose ) cout << "... ... " << atom_id << " " << atom_type << " " << atom_charge << endl;
                    getline(_input_file, _line);
                    boost::trim( _line );
                    boost::algorithm::split( _row , _line, boost::is_any_of("\t "), boost::algorithm::token_compress_on);  
                    nfields =  _row.size();
                    
                     if ( _orbitals->_has_atoms == false ) {
                         _orbitals->AddAtom( atom_type, 0, 0, 0, atom_charge );
                     } else {
                         QMAtom* pAtom = _orbitals->_atoms.at( atom_id - 1 );
                         pAtom->type = atom_type;
                         pAtom->charge = atom_charge;
                     }
                    
                }
                _orbitals->_has_atoms = true;
        }
        

         /*
         * Coordinates of the final configuration
         * stored in the archive at the end of the file
         */
         std::string::size_type coordinates_pos = _line.find("Test job not archived");
        
        if (coordinates_pos != std::string::npos) {
            LOG(logDEBUG,*_pLog) << "Getting the coordinates" << flush;
            _has_coordinates = true;
            string archive;
            while ( _line.size() != 0 ) {
                getline(_input_file, _line);
                boost::trim(_line);
                archive += _line;                
            }
            
            std::list<std::string> stringList;
            vector<string> results;
            boost::iter_split( stringList, archive, boost::first_finder("\\\\") );
             
            list<string>::iterator coord_block = stringList.begin();
            advance(coord_block, 3);
            
            vector<string> atom_block;
            boost::algorithm::split(atom_block, *coord_block, boost::is_any_of("\\"), boost::algorithm::token_compress_on);
            
            vector<string>::iterator atom_block_it;
            int aindex = 0;
            
            for(atom_block_it =  ++atom_block.begin(); atom_block_it != atom_block.end(); ++atom_block_it) {
                vector<string> atom;
                
                boost::algorithm::split(atom, *atom_block_it, boost::is_any_of(","), boost::algorithm::token_compress_on);
                string _atom_type = atom.front() ; 
                
                vector<string>::iterator it_atom;
                it_atom = atom.end();
                double _z =  boost::lexical_cast<double>( *(--it_atom) );
                double _y =  boost::lexical_cast<double>( *(--it_atom) );
                double _x =  boost::lexical_cast<double>( *(--it_atom) );
                
                if ( _orbitals->_has_atoms == false ) {
                        _orbitals->AddAtom( _atom_type, _x, _y, _z );
                } else {
                         QMAtom* pAtom = _orbitals->_atoms.at( aindex );
                         pAtom->type = _atom_type;
                         pAtom->x = _x;
                         pAtom->y = _y;
                         pAtom->z = _z;
                         aindex++;
                }
                
            }
            
            // get the QM energy out
            advance(coord_block, 1);
            vector<string> block;
            vector<string> energy;
            boost::algorithm::split(block, *coord_block, boost::is_any_of("\\"), boost::algorithm::token_compress_on);
            boost::algorithm::split(energy, block[1], boost::is_any_of("="), boost::algorithm::token_compress_on);
            _orbitals->_qm_energy = _conv_Hrt_eV * boost::lexical_cast<double> ( energy[1] );
            
            LOG(logDEBUG, *_pLog) << "QM energy " << _orbitals->_qm_energy <<  flush;
            _has_qm_energy = true;
            _orbitals->_has_atoms = true;
            _orbitals->_has_qm_energy = true;

        }

         /*
         * Self-energy of external charges
         */
         std::string::size_type self_energy_pos = _line.find("Self energy of the charges");
        
        if (self_energy_pos != std::string::npos) {
            LOG(logDEBUG,*_pLog) << "Getting the self energy\n";  
            vector<string> block;
            vector<string> energy;
            boost::algorithm::split(block, _line, boost::is_any_of("="), boost::algorithm::token_compress_on);
            boost::algorithm::split(energy, block[1], boost::is_any_of("\t "), boost::algorithm::token_compress_on);
            
            _orbitals->_has_self_energy = true;
            _orbitals->_self_energy = _conv_Hrt_eV * boost::lexical_cast<double> ( energy[1] );
            
            LOG(logDEBUG, *_pLog) << "Self energy " << _orbitals->_self_energy <<  flush;

        }
        
        // check if all information has been accumulated and quit 
        if ( _has_number_of_electrons && 
             _has_basis_set_size && 
             _has_occupied_levels && 
             _has_unoccupied_levels &&
             _has_overlap_matrix &&
             _has_charges && 
             _has_self_energy
           ) break;
        
    } // end of reading the file line-by-line
   
    LOG(logDEBUG,*_pLog) << "Done parsing" << flush;
    return true;
}

string Gaussian::FortranFormat( const double &number ) {
    stringstream _ssnumber;
    if ( number >= 0) _ssnumber << " ";
    _ssnumber <<  setiosflags(ios::fixed) << setprecision(8) << std::scientific << number;
    std::string _snumber = _ssnumber.str(); 
    boost::replace_first(_snumber, "e", "D");
    return _snumber;
}
        



}}
