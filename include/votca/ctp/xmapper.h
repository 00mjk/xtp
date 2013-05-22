/* 
 * Copyright 2009-2011 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __XMAPPER__H
#define	__XMAPPER__H

#include <votca/ctp/topology.h>
#include <votca/ctp/xjob.h>
#include <votca/ctp/apolarsite.h>

// TODO Change maps to _alloc_xmlfile_fragsegmol_***
// TODO Confirm thread safety
// TODO Add "const" keyword

namespace votca { namespace ctp {
    
class XMpsMap
{

public:        

    XMpsMap() : _alloc_table("no_alloc") {};
   ~XMpsMap() {};

    // User interface:
    void GenerateMap(string xml_file, string alloc_table, Topology *top, vector<XJob*> &xjobs);
    void EquipWithPolSites(Topology *top);
    
    // Adapt to XJob
    vector<APolarSite*> MapPolSitesToSeg(const vector<APolarSite*> &pols_n, Segment *seg);    
    vector<APolarSite*> GetRawPolSitesJob(const string &mpsfile) { return _mpsFile_pSites_job[mpsfile]; }
    void Gen_QM_MM1_MM2(Topology *top, XJob *job, double co1, double co2);
    
    // Called by GenerateMap(...)
    void CollectMapFromXML(string xml_file);
    void CollectSegMpsAlloc(string alloc_table, Topology *top);
    void CollectSitesFromMps(vector<XJob*> &xjobs);
    
    
private:

    string _alloc_table;
    
    // Maps retrieved from XML mapping files
    map<string, bool>                   _map2md;
    map<string, vector<int> >           _alloc_frag_mpoleIdx;
    map<string, vector<string> >        _alloc_frag_mpoleName;
    map<string, vector<int> >           _alloc_frag_trihedron;
    map<string, vector<double> >        _alloc_frag_weights;
    map<string, vector<int> >           _alloc_frag_isVirtualMp;

    // Allocation of mps-files to segments, state-resolved
    map<int,string>                 _segId_mpsFile_n;
    map<int,string>                 _segId_mpsFile_e;
    map<int,string>                 _segId_mpsFile_h;

    // Raw polar sites collected from mps-files
    map<string,vector<APolarSite*> > _mpsFile_pSites;
    map<string,vector<APolarSite*> > _mpsFile_pSites_job;
};
    
    
    
    
}}


#endif