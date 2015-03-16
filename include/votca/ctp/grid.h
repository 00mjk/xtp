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

#ifndef __CTP_GRID__H
#define	__CTP_GRID__H


#include <votca/ctp/elements.h>
#include <string>
#include <map>
#include <vector>
#include <votca/tools/property.h>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/multi_array.hpp>
#include <votca/ctp/qmatom.h>
#include <votca/ctp/logger.h>
#include <votca/ctp/apolarsite.h>
/**
* \brief Takes a list of atoms, and creates different grids for it. Right now only CHELPG grid.
*
* 
* 
*/
using namespace std;
using namespace votca::tools;


namespace votca { namespace ctp {
    namespace ub = boost::numeric::ublas;
    
  class Grid{
    public:
        
        
        Grid( bool createpolarsites, bool useVdWcutoff, bool useVdWcutoff_inside)
            :_cutoff(3),_gridspacing(0.3),_cutoff_inside(1.5),_shift_cutoff(0.0),_shift_cutoff_inside(0.0),
             _useVdWcutoff(useVdWcutoff),_useVdWcutoff_inside(useVdWcutoff_inside),_cubegrid(false),_padding(3.0),
             _createpolarsites(createpolarsites), _sites_seg(NULL) {};
           
        
        Grid()
            :_cutoff(3),_gridspacing(0.3),_cutoff_inside(1.5),_shift_cutoff(0.0),_shift_cutoff_inside(0.0),
             _useVdWcutoff(false),_useVdWcutoff_inside(false),_cubegrid(false),_padding(3.0),
             _createpolarsites(false), _sites_seg(NULL) {};
           
        
        ~Grid() {};
        
        std::vector< ub::vector<double> > &getGrid() {return _gridpoints;}
        std::vector< APolarSite* > &Sites() {return _gridsites;}
        std::vector< APolarSite*>* getSites() {return &_gridsites;} 
        PolarSeg* getSeg(){return _sites_seg;}

        
        void setCutoffs(double cutoff, double cutoff_inside){_cutoff=cutoff;_cutoff_inside=cutoff_inside;}
        void setCutoffshifts(double shift_cutoff, double shift_cutoff_inside){_shift_cutoff=shift_cutoff;_shift_cutoff_inside=shift_cutoff_inside;}
        void setSpacing(double spacing){_gridspacing=spacing;}
        void setPadding(double padding){_padding=padding;}
        void setCubegrid(bool cubegrid){_cubegrid=cubegrid;_createpolarsites=true;}
        void setAtomlist(vector< QMAtom* >* Atomlist){_atomlist=Atomlist;}
        int  getTotalSize(){return _gridpoints.size();}
        
        int getsize(){
            int size=0.0;
            if(_cubegrid){size=_gridsites.size();}
            else{size=_gridpoints.size();}
            return size; 
        }

        void printGridtofile(const char* _filename)
        
        void readgridfromCubeFile(string filename, bool ignore_zeros)
       
        void printgridtoCubefile(string filename)
        
        void setupradialgrid(int depth)
        
        void setupgrid()
       
        void setupCHELPgrid(){
            //_padding=2.8; // Additional distance from molecule to set up grid according to CHELPG paper [Journal of Computational Chemistry 11, 361, 1990]
            _gridspacing=0.3; // Grid spacing according to same paper 
            _cutoff=2.8;
            _useVdWcutoff_inside=true;
            _shift_cutoff_inside=0.0;
            _useVdWcutoff=false;
            setupgrid();
        }
        
      
  private:
      std::vector< ub::vector<double> > _gridpoints;
      std::vector< APolarSite* > _gridsites;
      std::vector< APolarSite* > _all_gridsites;
      PolarSeg *_sites_seg;
      const vector< QMAtom* >* _atomlist;
      double _gridspacing;
      double _cutoff;
      double _cutoff_inside;
      double _shift_cutoff_inside;
      double _shift_cutoff;
      double _padding;
      bool   _createpolarsites;
      bool   _useVdWcutoff;
      bool   _useVdWcutoff_inside;
      bool   _cubegrid;
      vec _upperbound;
      vec _lowerbound;
 
    };   
    
 
    
}}

#endif	/* GRID_H */