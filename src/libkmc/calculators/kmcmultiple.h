/*
 * Copyright 2009-2011 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *vector<Node*> 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __VOTCA_KMC_MULTIPLE_H
#define	__VOTCA_KMC_MULTIPLE_H

// #include <votca/kmc/vssmgroup.h>
#include <vector>
// #include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <votca/tools/vec.h>
#include <votca/tools/statement.h>
#include <votca/tools/database.h>
#include <votca/tools/tokenizer.h>
#include <votca/tools/globals.h>
#include <votca/tools/random2.h>
// #include "node.h"

using namespace std;


namespace votca { namespace kmc {

static int verbose = 0; // 0=minimal output, 1=verbose output
typedef votca::tools::vec myvec;

struct Event
{
    // Event() : dr(3) {}
    int destination;
    double rate;
    votca::tools::vec dr;
};


class Node
{
    public:
        int id;
        int occupied;
        int injectable;
        double escaperate;
        double occupationtime;
        myvec position;
        vector<Event> event;
    
        double EscapeRate();
        // double CurrentEscapeRate(Node node2);
        // void Jump(Node node2);
        void AddEvent(int seg2, double rate12, myvec dr);
        void InitEscapeRate();
};


void Node::AddEvent(int seg2, double rate12, myvec dr)
{
    Event newEvent;
    newEvent.destination = seg2;
    newEvent.rate = rate12;
    newEvent.dr = dr;
    this->event.push_back(newEvent);
}


void Node::InitEscapeRate()
{
    double newEscapeRate = 0;
    for(unsigned int i=0; i<this->event.size();i++)
    {
        newEscapeRate += this->event[i].rate;
    }
    this->escaperate = newEscapeRate;
    // cout << "Escape rate for segment " << this->id << " was set to " << newEscapeRate << endl;
}


double Node::EscapeRate()
{
    return escaperate;
}


class Chargecarrier
{
    public:
        int position;
        int id;
        Node *node;
        myvec dr_travelled;
};


void progressbar(double fraction)
{
    int totalbars = 50;
    std::cout << "\r";
    for(double bars=0; bars<double(totalbars); bars++)
    {
        if(bars<=fraction*double(totalbars))
        {
            std::cout << "|";
        }
        else
        {
            std::cout << "-";
        }
    }
    std::cout << "  " << int(fraction*1000)/10. <<" %   ";
    std::cout << std::flush;
    if(fraction*100 == 100)
    {
        std::cout << std::endl;
    }
}


int OMPinfo() 
{
    int nthreads=1, tid=0, procs, inpar=0;
    printf("\n||\n|| openMP PARALLEL COMPUTATION STATUS:\n");
    #pragma omp parallel private(tid)
    {
        # ifdef _OPENMP
        tid = omp_get_thread_num();
        if (tid == 0) 
        {
            procs = omp_get_num_procs();
            nthreads = omp_get_num_threads();
            inpar = omp_in_parallel();
            printf("|| Number of processors = %d\n", procs);
            printf("|| Number of threads = %d\n", nthreads);
    
        }
        # endif
        if (tid == 0) 
        {
            printf("|| In parallel? = %d\n||\n", inpar);
        }
    }
    return nthreads;
}


class KMCMultiple : public KMCCalculator 
{
public:
    KMCMultiple() {};
   ~KMCMultiple() {};

    void Initialize(const char *filename, Property *options );
    bool EvaluateFrame();

protected:
	    vector<Node*>  LoadGraph();
            vector<double> RunVSSM(vector<Node*> node, double runtime, unsigned int numberofcharges, votca::tools::Random2 *RandomVariable);
            void WriteOcc(vector<double> occP, vector<Node*> node);

            // map<int , node_t *> _nodes_lookup;
            // vector<node_t *> _nodes;
            // map<int , node_t *> _injection_lookup;
            // vector<node_t *> _injection;
            string _injection_name;
            double _runtime;
            // double _dt;
            int _seed;
            int _numberofcharges;
            int _allowparallel;
            double _fieldX;
            double _fieldY;
            double _fieldZ;
            double _outputtime;
            string _trajectoryfile;
            string _filename; // HACK
            
};


void KMCMultiple::Initialize(const char *filename, Property *options )
{
    	if (options->exists("options.kmcmultiple.runtime")) {
	    _runtime = options->get("options.kmcmultiple.runtime").as<double>();
	}
	else {
	    throw runtime_error("Error in kmcmultiple: total run time is not provided");
        }

//    	if (options->exists("options.kmcmultiple.outputtime")) {
//	    _dt = options->get("options.kmcmultiple.outputtime").as<double>();
//	}
//	else {
//	    throw runtime_error("Error in kmcmultiple: output frequency is not provided");
//        }

    	if (options->exists("options.kmcmultiple.seed")) {
	    _seed = options->get("options.kmcmultiple.seed").as<int>();
	}
	else {
	    throw runtime_error("Error in kmcmultiple: seed is not provided");
        }
        
    	if (options->exists("options.kmcmultiple.numberofcharges")) {
	    _numberofcharges = options->get("options.kmcmultiple.numberofcharges").as<int>();
	}
	else {
	    throw runtime_error("Error in kmcmultiple: number of charges is not provided");
        }

        if (options->exists("options.kmcmultiple.injection")) {
	    _injection_name = options->get("options.kmcmultiple.injection").as<string>();
	}
        else {
	    throw runtime_error("Error in kmcmultiple: injection pattern is not provided");
        }

        if (options->exists("options.kmcmultiple.allowparallel")) {
	    _allowparallel = options->get("options.kmcmultiple.allowparallel").as<int>();
	}
        else {
	    cout << "WARNING in kmcmultiple: You did not specify if parallel computation is allowed. It will be disabled." << endl;
            _allowparallel = 0;
        }
        if (options->exists("options.kmcmultiple.fieldX")) {
	    _fieldX = options->get("options.kmcmultiple.fieldX").as<double>();
	}
        else {
            _fieldX = 0;
        }
        if (options->exists("options.kmcmultiple.fieldY")) {
	    _fieldY = options->get("options.kmcmultiple.fieldY").as<double>();
	}
        else {
            _fieldY = 0;
        }
        if (options->exists("options.kmcmultiple.fieldZ")) {
	    _fieldZ = options->get("options.kmcmultiple.fieldZ").as<double>();
	}
        else {
            _fieldZ = 0;
        }
        if (options->exists("options.kmcmultiple.outputtime")) {
	    _outputtime = options->get("options.kmcmultiple.outputtime").as<double>();
	}
        else {
            _outputtime = 0;
        }
        if (options->exists("options.kmcmultiple.trajectoryfile")) {
	    _trajectoryfile = options->get("options.kmcmultiple.trajectoryfile").as<string>();
	}
        else {
            _trajectoryfile = "trajectory.csv";
        }

        _filename = filename;

       //cout << _seed << endl;
       //srand(_seed);
       //votca::tools::Random::init(rand(), rand(), rand(), rand());
}

vector<Node*> KMCMultiple::LoadGraph()
{
    string injectionpattern = "*";
    
    vector<Node*> node;
    
    // Load nodes
    votca::tools::Database db;
    db.Open( _filename );
    if(verbose >= 1) {cout << "LOADING GRAPH" << endl << "database file: " << _filename << endl; }
    votca::tools::Statement *stmt = db.Prepare("SELECT id-1, name, posX, posY, posZ FROM segments;");

    int i=0;
    while (stmt->Step() != SQLITE_DONE)
    {
        Node *newNode = new Node();
        node.push_back(newNode);

        int newid = stmt->Column<int>(0);
        string name = stmt->Column<string>(1);
        node[i]->id = newid;
        myvec nodeposition = myvec(stmt->Column<double>(2), stmt->Column<double>(3), stmt->Column<double>(4));
        node[i]->position = nodeposition;
        if (votca::tools::wildcmp(injectionpattern.c_str(), name.c_str()))
        {
            node[i]->injectable = 1;
        }
        else
        {
            node[i]->injectable = 0;
        }
        i++;
    }
    delete stmt;
    if(verbose >= 1) { cout << "segments: " << node.size() << endl; }
    
    // Load pairs and rates
    int numberofpairs = 0;
    stmt = db.Prepare("SELECT seg1-1 AS 'segment1', seg2-1 AS 'segment2', rate12e AS 'rate', drX, drY, drZ FROM pairs UNION SELECT seg2-1 AS 'segment1', seg1-1 AS 'segment2', rate21e AS 'rate', -drX AS 'drX', -drY AS 'drY', -drZ AS 'drZ' FROM pairs ORDER BY segment1;");
    while (stmt->Step() != SQLITE_DONE)
    {
        int seg1 = stmt->Column<int>(0);
        int seg2 = stmt->Column<int>(1);
        double rate12 = stmt->Column<double>(2);
        myvec dr = myvec(stmt->Column<double>(3), stmt->Column<double>(4), stmt->Column<double>(5));
        node[seg1]->AddEvent(seg2,rate12,dr);
        numberofpairs ++;
    }    
    delete stmt;

    if(verbose >= 1) { cout << "pairs: " << numberofpairs/2 << endl; }
    
    // Calculate initial escape rates
    for(unsigned int i=0; i<node.size(); i++)
    {
        node[i]->InitEscapeRate();
    }
    return node;
}

void ResetForbidden(vector<int> &forbiddenid)
{
    forbiddenid.clear();
}

void AddForbidden(int id, vector<int> &forbiddenid)
{
    forbiddenid.push_back(id);
}

int Forbidden(int id, vector<int> forbiddenlist)
{
    // cout << "forbidden list has " << forbiddenlist.size() << " entries" << endl;
    int forbidden = 0;
    for (unsigned int i=0; i< forbiddenlist.size(); i++)
    {
        if(id == forbiddenlist[i])
        {
            forbidden = 1;
            // cout << "ID " << id << " has been found as element " << i << " (" << forbiddenlist[i]+1<< ") in the forbidden list." << endl;
            break;
        }
    }
    return forbidden;
}

int Surrounded(Node* node, vector<int> forbiddendests)
{
    //if(node->event.size() == forbiddendests.size())
    //{
    //    return 1;
    //}
    //else
    //{
    //    return 0;
    //}
    int surrounded = 1;
    for(unsigned int i=0; i<node->event.size(); i++)
    {
        int thisevent_possible = 1;
        for(unsigned int j=0; j<forbiddendests.size(); j++)
        {
            if(node->event[i].destination == forbiddendests[j])
            {
                thisevent_possible = 0;
                break;
            }
        }
        if(thisevent_possible == 1)
        {
            surrounded = 0;
            // cout << "NOT SURROUNDED, POSSIBLE EVENT: " << node->id+1 << "->" << node->event[i].destination+1 << endl;
            break;
        }
    }
    return surrounded;
}

void printtime(int seconds_t)
{
    int seconds = seconds_t;
    int minutes = 0;
    int hours = 0;
    while(seconds / 60 >= 1)
    {
        seconds -= 60;
        minutes +=  1;
    }
    while(minutes / 60 >= 1)
    {
        minutes -= 60;
        hours +=  1;
    }
    char buffer [50];
    int n = sprintf(buffer, "%d:%02d:%02d",hours,minutes,seconds);
    printf("%s",buffer,n);
}

vector<double> KMCMultiple::RunVSSM(vector<Node*> node, double runtime, unsigned int numberofcharges, votca::tools::Random2 *RandomVariable)
{
    int tid = 0;
    # ifdef _OPENMP
    tid = omp_get_thread_num();
    # endif

    int realtime_start = time(NULL);
    if(tid == 0)
    {
        cout << endl << "Algorithm: VSSM for Multiple Charges" << endl;
        cout << "number of charges: " << numberofcharges << endl;
        cout << "number of nodes: " << node.size() << endl;
    }
    
    if(numberofcharges > node.size())
    {
        throw runtime_error("ERROR in kmcmultiple: specified number of charges is greater than the number of nodes. This conflicts with single occupation.");
    }
    
    fstream traj;
    char trajfile[100];
    strcpy(trajfile, _trajectoryfile.c_str());
    cout << "Writing trajectory to " << trajfile << "." << endl; 
    traj.open (trajfile, fstream::out);
    if(_outputtime != 0)
    {   
        for(unsigned int i=0; i<numberofcharges; i++)
        {
            traj << "'carrier" << i+1 << "_x'\t";    
            traj << "'carrier" << i+1 << "_y'\t";    
            traj << "'carrier" << i+1 << "_z";    
            if(i<numberofcharges-1)
            {
                traj << "'\t";
            }
        }
        traj << endl;
        
    }
    double outputfrequency = runtime/100;
    vector<double> occP(node.size(),0.);

    // Injection
    vector< Chargecarrier* > carrier;
    vector<myvec> startposition(numberofcharges,myvec(0,0,0));
    for (unsigned int i = 0; i < numberofcharges; i++)
    {
        Chargecarrier *newCharge = new Chargecarrier;      
        newCharge->id = i;
        newCharge->node = node[RandomVariable->rand_uniform_int(node.size())];
        while(newCharge->node->occupied == 1 || newCharge->node->injectable != 1)
        {   // maybe already occupied? or maybe not injectable?
            newCharge->node = node[RandomVariable->rand_uniform_int(node.size())];
        }
        newCharge->node->occupied = 1;
        newCharge->dr_travelled = myvec(0,0,0);
        startposition[i] = newCharge->node->position;
        if(tid == 0) {cout << "starting position for charge " << i+1 << ": segment " << newCharge->node->id+1 << endl; }
        carrier.push_back(newCharge);
    }
    

    double simtime = 0.;
    unsigned long step = 0;
    double nextoutput = outputfrequency;
    double nexttrajoutput = _outputtime;
    
    progressbar(0.);
    vector<int> forbiddennodes;
    vector<int> forbiddendests;
    while(simtime < runtime)
    {
        double cumulated_rate = 0;
        for(unsigned int i=0; i<numberofcharges; i++)
        {
            cumulated_rate += carrier[i]->node->EscapeRate();
        }
        if(cumulated_rate == 0)
        {   // this should not happen: no possible jumps defined for a node
            throw runtime_error("ERROR in kmcmultiple: Incorrect rates in the database file. All the escape rates for the current setting are 0.");
        }
        // go forward in time
        double dt = 0;
        double rand_u = 1-RandomVariable->rand_uniform();
        while(rand_u == 0)
        {
            cout << "WARNING: encountered 0 as a random variable! New try." << endl;
            rand_u = 1-RandomVariable->rand_uniform();
        }
        dt = -1 / cumulated_rate * log(rand_u);
        simtime += dt;
        if(verbose >= 1 && tid == 0) {cout << "simtime += " << dt << endl << endl;}
        step += 1;
        
        for(unsigned int i=0; i<numberofcharges; i++)
        {
            carrier[i]->node->occupationtime += dt;
        }

        //if(verbose >=1 && tid == 0)
        //{
        //    cout << "positions:" << endl;
        //    for(unsigned int i=0; i<numberofcharges; i++)
        //    {
        //        cout << i+1 << " -->  seg " << carrier[i]->node->id+1 << "  ("<< carrier[i]->node->position.getX()*1E9 << "," << carrier[i]->node->position.getY()*1E9 << "," << carrier[i]->node->position.getZ()*1E9 << ")"<<endl;
        //    }
        //    cout << endl;
        //}

        
        ResetForbidden(forbiddennodes);
        int level1step = 0;
        while(level1step == 0)
        // LEVEL 1
        {
            
            // determine which electron will escape
            Node* do_oldnode;
            Node* do_newnode;
            Chargecarrier* do_affectedcarrier;
            
            double u = 1 - RandomVariable->rand_uniform();
            for(unsigned int i=0; i<numberofcharges; i++)
            {
                u -= carrier[i]->node->EscapeRate()/cumulated_rate;
                if(u <= 0)
                {
                    do_oldnode = carrier[i]->node;
                    do_affectedcarrier = carrier[i];
                    break;
                }
               do_oldnode = carrier[i]->node;
               do_affectedcarrier = carrier[i];
            }
                
            double maxprob = 0.;
            double newprob = 0.;
            myvec dr;
            if(verbose >= 1 && tid == 0) {cout << "Charge number " << do_affectedcarrier->id+1 << " which is sitting on segment " << do_oldnode->id+1 << " will escape!" << endl ;}
            if(Forbidden(do_oldnode->id, forbiddennodes) == 1) {continue;}
            
            // determine where it will jump to
            ResetForbidden(forbiddendests);
            while(true)
            {
            // LEVEL 2
                if(verbose >= 1 && tid == 0) {cout << "There are " << do_oldnode->event.size() << " possible jumps for this charge:"; }

                do_newnode = NULL;
                u = 1 - RandomVariable->rand_uniform();
                for(unsigned int j=0; j<do_oldnode->event.size(); j++)
                {
                    // BEGIN OBSOLETE CODE
                    // THIS YIELDS WRONG STATISTICS AND HAS THUS BEEN REPLACED BY THE CHECK AFTER SELECTION
                    //if(Forbidden(do_oldnode->event[j].destination, forbiddendests) == 1)
                    //{   // directly skip forbidden events
                    //    if(verbose >= 1 && tid == 0) { cout << " [" << do_oldnode->event[j].destination+1 << " FORBIDDEN]" ; }                         
                    //    continue;
                    //}
                    // END OBSOLETE CODE
                    if(verbose >= 1 && tid == 0) { cout << " " << do_oldnode->event[j].destination+1 ; }
                    u -= do_oldnode->event[j].rate/do_oldnode->EscapeRate();
                    if(u <= 0)
                    {
                        do_newnode = node[do_oldnode->event[j].destination];
                        dr = do_oldnode->event[j].dr;
                        break;
                    }
                    do_newnode = node[do_oldnode->event[j].destination];
                    dr = do_oldnode->event[j].dr;
                }

                if(do_newnode == NULL)
                {
                    if(verbose >= 1 && tid == 0) {cout << endl << "Node " << do_oldnode->id+1  << " is SURROUNDED by forbidden destinations and zero rates. Adding it to the list of forbidden nodes. After that: selection of a new escape node." << endl; }
                    AddForbidden(do_oldnode->id, forbiddennodes);
                    int nothing=0;
                    break; // select new escape node (ends level 2 but without setting level1step to 1)
                }
                if(verbose >= 1 && tid == 0) {cout << endl << "Selected jump: " << do_newnode->id+1 << endl; }
                
                // check after the event if this was allowed
                if(Forbidden(do_newnode->id, forbiddendests) == 1)
                {
                    if(verbose >= 1 && tid == 0) {cout << "Node " << do_oldnode->id+1  << " is FORBIDDEN. Now selection new hopping destination." << endl; }
                    continue;
                }

                // if the new segment is unoccupied: jump; if not: nothing??
                if(do_newnode->occupied == 1)
                {
                    if(Surrounded(do_oldnode, forbiddendests) == 1)
                    {
                        if(verbose >= 1 && tid == 0) {cout << "Node " << do_oldnode->id+1  << " is SURROUNDED by forbidden destinations. Adding it to the list of forbidden nodes. After that: selection of a new escape node." << endl; }
                        AddForbidden(do_oldnode->id, forbiddennodes);
                        break; // select new escape node (ends level 2 but without setting level1step to 1)
                    }
                    if(verbose >= 1 && tid == 0) {cout << "Selected segment: " << do_newnode->id+1 << " is already OCCUPIED. Added to forbidden list." << endl << endl;}
                    AddForbidden(do_newnode->id, forbiddendests);
                    if(verbose >= 1 && tid == 0) {cout << "Now choosing different hopping destination." << endl; }
                    continue; // select new destination
                }
                else
                {
                    do_newnode->occupied = 1;
                    do_oldnode->occupied = 0;
                    do_affectedcarrier->node = do_newnode;
                    do_affectedcarrier->dr_travelled += dr;
                    level1step = 1;
                    if(verbose >= 1 && tid == 0) {cout << "Charge has jumped to segment: " << do_newnode->id+1 << "." << endl;}
                    // cout << "old node: " << do_oldnode->id+1 << ", occupation: " << do_oldnode->occupied << endl;
                    // cout << "new node: " << do_newnode->id+1 << ", occupation: " << do_newnode->occupied << endl;
                    break; // this ends LEVEL 2 , so that the time is updated and the next MC step started
                }

                if(verbose >= 1 && tid == 0) {cout << "." << endl;}
            // END LEVEL 2
            }
        // END LEVEL 1
        }    
        
        if(_outputtime != 0 && simtime > nexttrajoutput && tid ==0)       
        {
            nexttrajoutput = simtime + _outputtime;
            for(unsigned int i=0; i<numberofcharges; i++) 
            {
                traj << carrier[i]->dr_travelled.getX() << "\t";
                traj << carrier[i]->dr_travelled.getY() << "\t";
                traj << carrier[i]->dr_travelled.getZ();
                //traj << startposition[i].getX() + carrier[i]->dr_travelled.getX() << "\t";
                //traj << startposition[i].getY() + carrier[i]->dr_travelled.getY() << "\t";
                //traj << startposition[i].getZ() + carrier[i]->dr_travelled.getZ();
                if (i<numberofcharges-1) 
                {
                    traj << "\t";
                }
                else
                {
                    traj << endl;
                }
            }
            
        }
        if(simtime > nextoutput && tid == 0)
        {
            nextoutput = simtime + outputfrequency;
            progressbar(simtime/runtime);
            cout << " remaining: ";
            printtime(int((runtime/simtime-1) * (int(time(NULL)) - realtime_start))); 
        }
        // cout << "step " << step << endl;
    }
    progressbar(1.);
    
    if(_outputtime != 0)
    {   
        traj.close();
    }


    
    // calculate occupation probabilities from occupation times    
    for(unsigned int j=0; j<node.size(); j++)
    {   
        occP[j] = node[j]->occupationtime / simtime;
    }
    

    if (tid == 0)
    {
        cout << endl << "finished KMC simulation after " << step << " steps." << endl;
        cout << "runtime: ";
        printtime(time(NULL) - realtime_start); 
        myvec dr_travelled = myvec (0,0,0);
        cout << endl << "Average velocities (m/s): " << endl;
        for(unsigned int i=0; i<numberofcharges; i++)
        {
            //cout << std::scientific << "    charge " << i+1 << ": " << carrier[i]->dr_travelled/simtime*1e-9 << endl;
            cout << std::scientific << "    charge " << i+1 << ": " << carrier[i]->dr_travelled/simtime << endl;
            dr_travelled += carrier[i]->dr_travelled;
        }
        dr_travelled /= numberofcharges;
        //cout << std::scientific << "  Overall average velocity (m/s): " << dr_travelled/simtime*1e-9 << endl;
        cout << std::scientific << "  Overall average velocity (m/s): " << dr_travelled/simtime << endl;

        cout << endl << "Distances travelled (m): " << endl;
        for(unsigned int i=0; i<numberofcharges; i++)
        {
            cout << std::scientific << "    charge " << i+1 << ": " << carrier[i]->dr_travelled << endl;
        }
        
        // calculate mobilities
        double absolute_field = sqrt(_fieldX*_fieldX + _fieldY*_fieldY + _fieldZ*_fieldZ);
        double average_mobilityZ = 0;
        if (absolute_field != 0)
        {
            cout << endl << "Mobilities (cm^2/Vs): " << endl;
            for(unsigned int i=0; i<numberofcharges; i++)
            {
                //myvec velocity = carrier[i]->dr_travelled/simtime*1e-9;
                myvec velocity = carrier[i]->dr_travelled/simtime;
                double absolute_velocity = sqrt(velocity.x()*velocity.x() + velocity.y()*velocity.y() + velocity.z()*velocity.z());
                //cout << std::scientific << "    charge " << i+1 << ": mu=" << absolute_velocity/absolute_field*1E4 << endl;
                cout << std::scientific << "    charge " << i+1 << ": muZ=" << velocity.z()/_fieldZ*1E4 << endl;
                average_mobilityZ += velocity.z()/_fieldZ*1E4;
            }
            average_mobilityZ /= numberofcharges;
            cout << std::scientific << "  Overall average z-mobility <muZ>=" << average_mobilityZ << endl;
        }
        cout << endl;

    
    }
    return occP;
}


void KMCMultiple::WriteOcc(vector<double> occP, vector<Node*> node)
{
    votca::tools::Database db;
    cout << "Opening for writing " << _filename << endl;
	db.Open(_filename);
	db.Exec("BEGIN;");
	votca::tools::Statement *stmt = db.Prepare("UPDATE segments SET occPe = ? WHERE id = ?;");  // electron occ. prob., check (think about) this
	for(unsigned int i=0; i<node.size(); ++i)
        {
	    stmt->Reset();
	    stmt->Bind(1, occP[i]);
	    stmt->Bind(2, node[i]->id+1);
	    stmt->Step();
	}
	db.Exec("END;");
	delete stmt;
}

bool KMCMultiple::EvaluateFrame()
{
    std::cout << "-----------------------------------" << std::endl;      
    std::cout << "      KMC FOR MULTIPLE CHARGES" << std::endl;
    std::cout << "-----------------------------------" << std::endl << std::endl;      
 
    unsigned int numberofthreads = 1;
    if(_allowparallel == 1)
    {
        numberofthreads = OMPinfo();
    }
    
    // Initialise random number generator
    // each thread i in a parallel computation needs is own set RandomVariable[i]
    if(verbose >= 1) { cout << endl << "Initialising random number generator" << endl; }
    srand(_seed); // srand expects any integer in order to initialise the random number generator
    vector< votca::tools::Random2* > RandomVariable;
    for (unsigned int i = 0; i < numberofthreads; i++)
    {
        votca::tools::Random2 *newRandomVariable = new votca::tools::Random2();      
        RandomVariable.push_back(newRandomVariable);
        RandomVariable[i]->init(rand(), rand(), rand(), rand());
    }
    
    // VSSM KMC algorithm
    //cout << endl << "KMC SIMULATION" << endl;
    cout << "number of threads: " << numberofthreads << endl;

    vector<Node*> node;
    node = KMCMultiple::LoadGraph();
    vector<double> occP(node.size(),0.);
    vector< vector< double > > occPOneRun ( numberofthreads, vector<double> ( node.size(), 0. ) );

    # ifdef _OPENMP
    (void) omp_set_num_threads(numberofthreads);
    # endif
    #pragma omp parallel private(node)
    {
        node = KMCMultiple::LoadGraph();
        int thread = 0;
        # ifdef _OPENMP
        thread = omp_get_thread_num();
        # endif
        occPOneRun[thread] = KMCMultiple::RunVSSM(node, _runtime/numberofthreads, _numberofcharges, RandomVariable[thread]);
    }

    
    // get mean of multiple runs
    for(unsigned int j=0; j<occP.size();j++) 
    {
        for(unsigned int thread=0; thread<numberofthreads; thread++)
        {
            occP[j] += occPOneRun[thread][j];
        }
        occP[j] /= numberofthreads;
    }
    
    // output occupation probabilites
    for(unsigned int thread=0; thread<numberofthreads; thread++)
    {
        for(unsigned int j=0; j<occPOneRun[thread].size();j++) 
        {
            if(occPOneRun[thread][j] > 0)
            {
                // cout << "[thread " << thread+1 << "] "<<"occupation probability " << node[j]->id+1 << ": " << occPOneRun[thread][j] << endl;
            }
        }
    }
    // output occupation probabilites
    cout << endl;
    for(unsigned int j=0; j<occP.size();j++) 
    {
        if(occP[j] > 0)
        {
            // cout << "occupation probability " << node[j]->id+1 << ": " << occP[j] << endl;
        }
    }
    
    // write occupation probabilites
    KMCMultiple::WriteOcc(occP, node);
    
    return true;
}


}}


#endif	/* __VOTCA_KMC_MULTIPLE_H */
