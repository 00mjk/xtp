#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <votca/csg/cgengine.h>
#include <votca/csg/version.h>
#include <stdexcept>
#include "projobserver.h"
#include "qmtopology.h"

using namespace std;

void help_text(void)
{
    votca::csg::HelpTextHeader("proJ");
    cout << "Print out .pdb file with pairs of molecules\n\n";
}

int main(int argc, char** argv)
{    
    int write_every=0;
    // we have one observer
    ProJObserver observer;
    // The CGEngine does the work
    CGEngine cg_engine;
    QMTopology qmtopol;

    observer.setQMTopology(qmtopol);
    
    namespace po=boost::program_options;
    
    try {

        // add our observer that it gets called to analyze frames
        cg_engine.AddObserver((CGObserver*)&observer);
    
        // let cg_engine add some program options
        cg_engine.Initialize();
   
        cg_engine.AddProgramOptions()
            ("listcharges,l", po::value<string>(), "  Crg unit definitions");
        cg_engine.AddProgramOptions()
            ("cutoff,c", po::value<double>()-> default_value(1.0), "  CutOff for nearest neighbours");
        cg_engine.AddProgramOptions()
            ("nnnames,n", po::value<string>()-> default_value("*"), "  List of strings that the concatenation of the two molnames must match to be printed");
        cg_engine.ParseCommandLine(argc, argv);

        po::variables_map &vm
            = cg_engine.OptionsMap();
    
        // does the user want help?
        if (vm.count("help")) {
            help_text();
            cout << cg_engine.OptionsDesc() << endl;
            return 0;
        }
    
       
        qmtopol.LoadListCharges(vm["listcharges"].as<string>());
        observer.setCutoff(vm["cutoff"].as<double>());
        observer.setNNnames(vm["nnnames"].as<string>());
        // try to run the cg process, go through the frames, etc...
        cg_engine.Run();
    }
    // did an error occour?
    catch(std::exception &error) {
        cerr << "An error occoured!" << endl << error.what() << endl;
        return -1;
    }
    return 0;
}


