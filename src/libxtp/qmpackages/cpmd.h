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

#ifndef __VOTCA_XTP_CPMD_H
#define	__VOTCA_XTP_CPMD_H


#include <votca/xtp/apolarsite.h>
#include <votca/xtp/qmpackage.h>

#include <string> 



namespace votca { namespace xtp {
/**
    \brief Wrapper for the CPMD program
 
    The Cpmd class executes the CPMD package 
    and extracts information from its log and io files
    
*/
class Cpmd : public QMPackage
{
public:   

   std::string getPackageName() { return "gaussian"; }

   void Initialize( Property *options );

   /* Writes Gaussian input file with coordinates of segments
    * and a guess for the dimer (if requested) constructed from the
    * monomer orbitals
    */
   bool WriteInputFile( std::vector< Segment* > segments, Orbitals* orbitals_guess = NULL);
   
   bool WriteShellScript();

   bool Run();

   void CleanUp();
   
   bool CheckLogFile();

   bool ParseLogFile( Orbitals* _orbitals );

   bool ParseOrbitalsFile( Orbitals* _orbitals );
   
   bool ConvertToGW( Orbitals* _orbitals );
      
   std::string getScratchDir( ) { return _scratch_dir; }
   
   bool loadMatrices(void);
   
private:  

    std::string                              _shell_file_name;
    std::string                              _chk_file_name;
    std::string                              _scratch_dir;
    std::string                              _input_vxc_file_name;    
    std::string                              _cleanup;
    std::string                              _vdWfooter;
    
    
    bool _rsrt;             //have data from previous run of CPMD we want to reuse
    bool _optWF;            //optimize wavefunction
    bool _elpot;            //calculate and output electrostatic potential (needs conversion with cpmd2cube.x)
    bool _projectWF;        //project wavefunction onto localized atomic orbitals
    bool _popAnalysis;      //do population analysis, required to extract WF coefficients in atomic basis
    bool _getMat;           //get density and overlap matrices, requires _popAnalysis and _projectWF
    double _pwCutoff;       //plane wave cutoff (in Ry)
    double _convCutoff;     //cutoff for MO convergence
    int _symmetry;          //symmetry number (0=isolated, 1=simple cubic, 2=FCC, 3=BCC, ... check CPMD manual under "SYMETRY")
    std::string _cell;      //cell dimensions, check CPMD manual under "CELL"
    std::string _functional;//BLYP, B3LYP, HCTH, LDE, etc.
    std::string _rsrt_kwds; //what parts to reuse from previous run
    std::string _pplib_path;//full path to the pseudopotential library of CPMD
    
    BasisSet _bs;
    //std::vector<std::string> _ppElementNames;       //element names
    //std::vector<std::string> _ppFileNames;          //pseudopotential file names
    std::map<std::string,std::string> _ppFileNames;   //pseudopotential file names indexed by element name
    std::map<std::string,int> _nAtomsOfElement;       //number of atoms of element indexed by element name
    //std::vector<int> _element_Lmax;               //max L of each element
    
    
    

    int NumberOfElectrons( std::string _line ); 
    int BasisSetSize( std::string _line ); 
    int EnergiesFromLog( std::string _line, std::ifstream inputfile ); 
    std::string FortranFormat( const double &number );
    int NumbfQC( std::string _shell_type);
    int NumbfGW( std::string _shell_type);
    int NumbfQC_cart( std::string _shell_type);
    void WriteBasisSet(std::vector<Segment* > segments, ofstream &_com_file);

    
    
};


}}

#endif	/* __VOTCA_XTP_CPMD_H */

