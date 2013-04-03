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

#include "votca/ctp/gaussian.h"
#include "votca/ctp/segment.h"
#include <votca/tools/globals.h>
#include <boost/algorithm/string.hpp>
#include <stdio.h>
#include <iomanip>
#include <sys/stat.h>

using namespace std;

namespace votca { namespace ctp {

Gaussian::Gaussian( tools::Property *opt ) { 
    
    string key = "package";

    string _name = opt->get(key+".name").as<string> ();
    
    if ( _name != "gaussian" ) {
        cerr << "Tried to use " << _name << " package. ";
        throw std::runtime_error( "Package is not supported.");
    }
    
    _executable =       opt->get(key + ".executable").as<string> ();
    _charge =           opt->get(key + ".charge").as<int> ();
    _spin =             opt->get(key + ".spin").as<int> ();
    _options =          opt->get(key + ".options").as<string> ();
    _memory =           opt->get(key + ".memory").as<string> ();
    _threads =          opt->get(key + ".threads").as<int> ();
    _chk_file_name  =   opt->get(key + ".checkpoint").as<string> ();
    _scratch_dir =      opt->get(key + ".scratch").as<string> ();
    _cleanup =          opt->get(key + ".cleanup").as<string> ();
        
};   
    
Gaussian::~Gaussian() { 
}  

/**
 * Prepares the com file from a vector of segments
 */
bool Gaussian::WriteInputFile( Segment *seg ) {

    vector< Atom* > ::iterator ait;

    int qmatoms = 0;

    vector< Atom* > _atoms;
    _atoms  = seg-> Atoms();

    ofstream _com_file;
    
    string _com_file_name_full = _run_dir + "/" + _com_file_name;
    
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
        _com_file <<  "  " << _options << endl ;
    }
    
    _com_file << endl << seg->getName() << endl << endl;
    _com_file << setw(2) << _charge << setw(2) << _spin << endl;
    
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
    _shell_file << _executable << " " << _com_file_name << endl;    
    _shell_file.close();
}

/**
 * Runs the Gaussian job
 */
bool Gaussian::Run()
{

    if (system(NULL)) {
        // if scratch is provided, run the shell script; 
        // otherwise run gaussian directly and rely on global variables 
        string _command;
        if ( _scratch_dir.size() != 0 ) {
            _command  = "cd " + _run_dir + "; tcsh " + _shell_file_name;
        }
        else {
            _command  = "cd " + _run_dir + "; " + _executable + " " + _com_file_name;
        }
        
        int i = system ( _command.c_str() );
    }
    else {
        cerr << "The job " << _com_file_name << " failed to complete" << endl; 
        exit (EXIT_FAILURE);
    }

}

/**
 * Cleans up after the Gaussian job
 */
void Gaussian::CleanUp( string ID ) {
    
    // cleaning up the generated files
    if ( _cleanup.size() != 0 ) {
        Tokenizer tok_cleanup(_cleanup, " \t\n");
        vector <string> _cleanup_info;
        tok_cleanup.ToVector(_cleanup_info);
        
        vector<string> ::iterator it;
        
        for (it = _cleanup_info.begin(); it != _cleanup_info.end(); ++it) {
            if ( *it == "xyz" || *it == "com" || *it == "log" ) { 
                string file_name = _run_dir + "/mol_" + ID + "." + *it;
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

    std::ifstream _input_file( _orb_file_name.c_str() );
    if (_input_file.fail()) {
        cerr << endl << "File " << _orb_file_name << " with molecular orbitals is not found " << endl;
        return 1;
    } else {
        cout << endl << "... ... Reading molecular orbitals from " << _orb_file_name << endl;
    }

    // number of coefficients per line is  in the first line of the file (5D15.8)
    getline(_input_file, _line);
    std::vector<string> strs;
    boost::algorithm::split(strs, _line, boost::is_any_of("(D)"));
    //cout << strs.at(1) << endl;
    int nrecords_in_line = boost::lexical_cast<int>(strs.at(1));
    string format = strs.at(2);

    //cout << endl << "Orbital file " << filename << " has " 
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
    cout << "... ... Energy levels: " << _levels << endl;

    std::map< int, vector<double> >::iterator iter = _coefficients.begin();
    _basis_size = iter->second.size();

    for (iter = _coefficients.begin()++; iter != _coefficients.end(); iter++) {
        if (iter->second.size() != _basis_size) {
            cerr << "Error reading " << _orb_file_name << ". Basis set size change from level to level.";

        }
    }
    cout << "... ... Basis set size: " << _basis_size << endl;

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
   
     
   cout << "... ... Done reading orbital files from " << _orb_file_name << endl;

   return 0;
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

    int _occupied_levels = 0;
    int _unoccupied_levels = 0;
    int _number_of_electrons = 0;
    int _basis_set_size = 0;
    
    cout << endl << "... ... Parsing " << _log_file_name << endl;

    ifstream _input_file(_log_file_name.c_str());
    if (_input_file.fail()) {
        throw std::runtime_error("LOG file is not found.");
        return 1;
    };

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
            cout << "... ... Alpha electrons: " << boost::lexical_cast<int>(results.front()) << endl;
            _orbitals->_number_of_electrons = boost::lexical_cast<int>(results.front()) ;
            _orbitals->_has_number_of_electrons = true;
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
            cout << "... ... Basis functions: " << _basis_set_size << endl;
        }

        /*
         * energies of occupied/unoccupied levels
         * Alpha  occ.(virt.) eigenvalues -- e1 e2 e3 e4 e5
         */
        std::string::size_type eigenvalues_pos = _line.find("Alpha");
        if (eigenvalues_pos != std::string::npos) {
            
            std::list<std::string> stringList;
            int _unoccupied_levels = 0;
            int _occupied_levels = 0;

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
                    cout << "... ... Occupied levels: " << _occupied_levels << endl;
                    cout << "... ... Unoccupied levels: " << _unoccupied_levels << endl;
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
            cout << "... ... Read the overlap matrix" << endl;
        } // end of the if "Overlap" found   

        
        // check if all information has been accumulated
        if ( _has_number_of_electrons && 
             _has_basis_set_size && 
             _has_occupied_levels && 
             _has_unoccupied_levels &&
             _has_overlap_matrix
           ) break;
        
    } // end of reading the file line-by-line
    cout << "... ... Done parsing " << _log_file_name << endl;
}

}}
