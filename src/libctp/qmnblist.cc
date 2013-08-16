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


#include <votca/ctp/qmnblist.h>
#include <votca/tools/globals.h>
#include <votca/ctp/topology.h>

namespace votca { namespace ctp {

QMPair *QMNBList::Add(Segment* seg1, Segment* seg2) {

    if (this->FindPair(seg1, seg2) != NULL) {
        throw std::runtime_error("Critical bug: pair already exists");
    }

    // POTENTIAL BUGS : +1 added to start from 1;
    int id = this->size()+1;

    QMPair *pair = new QMPair(id, seg1, seg2);

    this->AddPair(pair);

    return pair;
    
}




void QMNBList::PrintInfo(FILE *out) {

    QMNBList::iterator nit;

    for (nit = this->begin();
         nit != this->end();
         nit++) {

        QMPair *pair = *nit;

        int ghost;
        if (pair->HasGhost()) { ghost = 1; }
        else { ghost = 0; }

        fprintf(out, "PairID %5d  | Seg1 %4d Seg2 %4d dR %2.4f Ghost? %1d | lOuter %1.4f J %2.4f r12 %2.4f r21 %2.4f \n",
                pair->getId(),
                pair->first->getId(),
                pair->second->getId(),
                pair->Dist(),
                ghost,
                0.0, // pair->getLambdaO(),
                0.0, // pair->calcJeff2(),
                0.0, // pair->getRate12(),
                0.0 ); // pair->getRate21() );
    }
}


void QMNBList::GenerateSuperExchange() {

    QMNBList bridged_nblist;
    Property bridges_summary;
    Property *_bridges = &bridges_summary.add("bridges","");
  
    // loop over all donor/acceptor pair types
    for (std::list<SuperExchangeType*>::iterator itDA = _superexchange.begin(); itDA != _superexchange.end(); itDA++) {

        cout << endl << " ... ... Processing " << (*itDA)->donor << ":" << (*itDA)->acceptor << " superexchange pairs" << endl;
        int _bridged_pairs = 0;
        int bridged_molecules = 0;

        // vector of neighboring segments of the donor/acceptor type
        map< int, Segment*> _ns;

        // loop over all segments in the topology
        for (vector< Segment* >::iterator segit = _top->Segments().begin(); segit != _top->Segments().end(); segit++) {

            // check if this is a bridge
            Segment* segment = *segit;
            string name = segment->getName();

            if ((*itDA)->isOfBridge(name)) {
                QMNBList::partners *_partners = FindPartners(segment);

                // loop over all partners of a segment
                QMNBList::partners::iterator itp;
                if (_partners != NULL) {
                    map< int, Segment*> _neighbors;
                    for (itp = _partners->begin(); itp != _partners->end(); itp++) {
                        Segment *nb = itp->first;
                        QMPair *pair = itp->second;
                        // check if the neighbor is of a donor or acceptor type
                        if ((*itDA)->isOfDonorAcceptor(nb->getName())) _neighbors[ nb->getId() ] = nb;
                    } // end of the loop of all partners of a segment

                     // create new pairs
                    if (_neighbors.size() > 1) {
                        for (map<int, Segment*>::iterator it1 = _neighbors.begin(); it1 != _neighbors.end(); it1++) {
                            map<int, Segment*>::iterator it_diag = it1;
                            for (map<int, Segment*>::iterator it2 = ++it_diag; it2 != _neighbors.end(); it2++) {
                                
                                QMPair *pair = bridged_nblist.FindPair(it1->second, it2->second);
                                
                                if (pair == NULL ) { 
                                    QMPair* _pair = bridged_nblist.Add(it1->second, it2->second);
                                    _pair->SetType( QMPair::_superExchange );
                                    _pair->AddBridgingSegment( segment );
                                    
                                    string nameA = it1->second->getName();
                                    string nameB = it2->second->getName();
                                    
                                    Property *_pair_property = &_bridges->add("pair","");
                                    
                                    _pair_property->setAttribute("idA", it1->second->getId());
                                    _pair_property->setAttribute("idB", it2->second->getId());
                                    
                                    Property *_bridge_property = &_pair_property->add("bridge","");
                                    _bridge_property->setAttribute("id", segment->getId());
                                    
                                    _bridged_pairs++;
                                } else { // pair of superexchange type is already there 
                                    pair->AddBridgingSegment( segment );
                                }
                            }
                        }
                    } // 
                } // end of the check of zero partners
            } // end of if this is a bridged pair
        } // end of the loop of all segments
        
        cout << " ... ... Added " << _bridged_pairs << " bridged pairs" << endl;    
        
    } // end of the loop over donor/acceptor types

    
    cout << bridges_summary;
    
    if (votca::tools::globals::verbose) {
        cout << "Bridged Pairs \n [idA:idB] com distance" << endl;
        for (QMNBList::iterator ipair = bridged_nblist.begin(); ipair != bridged_nblist.end(); ++ipair) {
                QMPair *pair = *ipair;
                Segment* segment1 = pair->Seg1PbCopy();
                Segment* segment2 = pair->Seg2PbCopy();
                cout << " [" << segment1->getId() << ":" << segment2->getId()<< "] " << pair->Dist()<< " bridges: " << (pair->getBridgingSegments()).size() << " | " << flush;
                vector<Segment*> bsegments = pair->getBridgingSegments();
        
                for ( vector<Segment*>::iterator itb = bsegments.begin(); itb != bsegments.end(); itb++ ) {
                    cout << (*itb)->getId() << " " ;
                }        
                
                cout << endl;
        }
    }

    return;
}

}}
