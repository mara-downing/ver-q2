#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <streambuf>
#include <unistd.h>
#include <chrono>
#include <sys/time.h>
#include <ctime>
#include <queue>
#include <string.h>
#include "nnet_f.hpp"
#include "nnet_q.hpp"
// #include "symbextree.hpp"
#include "symformula.hpp"
#include "leaflist.hpp"
#include "instruction.hpp"
#include "bigsizet.hpp"
#include "z3++.h"

using namespace std;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

int main(int argc, char *argv[]){
    // ofstream profilefile;
    // profilefile.open("timingprofile.txt", ios_base::app);
    // profilefile << argv[1] << endl;
    // auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    // profilefile << "Start: " << millisec_since_epoch << endl;
    // profilefile.close();
    string filename = argv[1];
    if(filename == "-h"){
        cout << "Usage: ./main <networkfile> <knownboundsfile> <constraintsfile> <fixedpointlength> <numdecimalbits> [options]" << endl;
        cout << "Options:" << endl;
        cout << "  -v <int>                             : Set verbose mode" << endl;
        cout << "     0 : Default, only prints model counts" << endl;
        cout << "     1 : Prints statistics on how many times each SMT optimization avoided or improved a call to z3" << endl;
        cout << "     2 : Prints model count per leaf during execution" << endl;
        cout << "  --skip-abstract                      : Do not use abstract symbolic execution to speed up SMT solving" << endl;
        cout << "  --skip-model-gen                     : Do not generate satisfying models to check future path constraints; will also skip model fixing and solve in parts" << endl;
        cout << "  --skip-model-fix                     : Do not attempt to fix models that do not satisfy new constraint" << endl;
        cout << "  --skip-solveinparts                  : Do not split large path constraints into subsets to solve" << endl;
        cout << "  --set-fix-threshold <int>            : Set threshold of clauses above which model fixing will be used; default wait until first z3 timeout, set to number of clauses that caused timeout" << endl;
        cout << "  --set-fix-limit <int>                : Set maximum number of fixed models to look for per clause; default 5" << endl;
        cout << "  --set-solveinparts-threshold <int>   : Set threshold of clauses above which constraint will be split into subsets; default wait until first z3 timeout, set to number of clauses that caused timeout" << endl;
        cout << "  --set-solveinparts-limit <int>       : Set maximum number of subsets that can be checked once threshold is reached; default all available" << endl;
        cout << "  --set-model-counter <name>           : Set model counter to be used for leaf counts; options are abc, barvinok, latte, z3, z3up, or concrete-enumerate; default is z3" << endl;
        cout << "  --time-limit <int>                   : Set time limit for entire program (seconds). Will not stop immediately at time limit, but will stop at soonest time check" << endl;
        cout << "                                         If program ends early due to this setting, final count will be reported as a sound lower or upper bound, depending on counting strategy" << endl;
        cout << "  --z3-leaf-count-limit <int>          : Set limit for number of models counted per leaf with z3 or z3up counting method, will not affect other methods" << endl;
        cout << "                                         If program ends early due to this setting, final count will be reported as a sound lower or upper bound, depending on counting strategy" << endl;
        cout << "  --z3-leaf-time-limit <int>           : Set time limit for each leaf count with z3 or z3up counting method, will not affect other methods" << endl;
        cout << "                                         If program ends early due to this setting, final count will be reported as a sound lower or upper bound, depending on counting strategy" << endl;
        return 0;
    }
    string knownboundsfilename = argv[2];
    string constraintsfilename = argv[3];
    int quant_len = stoi(argv[4]);
    int dec_bits = stoi(argv[5]);
    //parse optional flags here
    vector<string> args;
    for(size_t i = 6; i < (size_t)argc; ++i){
        args.push_back(argv[i]);
    }
    bool skip_abstract = 0;
    bool skip_modelgen = 0;
    bool skip_modelfix = 0;
    bool skip_solveinparts = 0;
    size_t modelfixthreshold = 10000;
    size_t modelfixlimit = 5;
    size_t solveinpartsthreshold = 10000;
    size_t solveinpartslimit = 0; // remember to check if this is 0 before setting it in code
    size_t verbosemode = 0;
    size_t modelcounter = 3;
    size_t z3leafcountlimit = 0; //default can have unlimited
    size_t z3leaftimelimit = 0; //default can have unlimited
    size_t timelimit = 0; //also default unlimited
    double timelimitmodifier = 1; // assume seconds
    size_t profiletimelimit = 0;
    float profilethreshold = 0.05;
    for(size_t i = 0; i < args.size(); ++i){
        if(args[i] == "-v"){
            assert(i != args.size() - 1);
            verbosemode = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--skip-abstract"){
            skip_abstract = 1;
        }
        else if(args[i] == "--skip-model-gen"){
            skip_modelgen = 1;
        }
        else if(args[i] == "--skip-model-fix"){
            skip_modelfix = 1;
        }
        else if(args[i] == "--skip-solveinparts"){
            skip_solveinparts = 1;
        }
        else if(args[i] == "--set-fix-threshold"){
            assert(i != args.size() - 1);
            modelfixthreshold = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--set-fix-limit"){
            assert(i != args.size() - 1);
            modelfixlimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--set-solveinparts-threshold"){
            assert(i != args.size() - 1);
            solveinpartsthreshold = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--set-solveinparts-limit"){
            assert(i != args.size() - 1);
            solveinpartslimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--set-model-counter"){
            assert(i != args.size() - 1);
            if(args[i+1] == "abc"){
                modelcounter = 0;
            }
            else if(args[i+1] == "barvinok"){
                modelcounter = 1;
            }
            else if(args[i+1] == "latte"){
                modelcounter = 2;
            }
            else if(args[i+1] == "z3"){
                modelcounter = 3;
            }
            else if(args[i+1] == "z3up"){ //z3, but count up. default is inverse count
                modelcounter = 4;
            }
            else if(args[i+1] == "concrete-enumerate"){
                modelcounter = 5;
            }
            else if(args[i+1] == "concrete-enumerate-fast"){
                modelcounter = 6;
            }
            else if(args[i+1] == "profile-then-choose"){
                modelcounter = 7;
            }
            else{
                cerr << "Not a valid model counter" << endl;
                exit(1);
            }
            ++i;
        }
        else if(args[i] == "--z3-leaf-count-limit"){
            assert(i != args.size() - 1);
            //set limit on count per leaf in z3
            z3leafcountlimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--z3-leaf-time-limit"){
            assert(i != args.size() - 1);
            //set limit on time per leaf in z3
            z3leaftimelimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--time-limit"){
            assert(i != args.size() - 1);
            //set time limit for entire program, add in some checks during instruction running to kill program if time is over limit
            //expects time limit in seconds
            timelimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--time-limit-ms"){
            assert(i != args.size() - 1);
            //set time limit for entire program, add in some checks during instruction running to kill program if time is over limit
            //expects time limit in seconds
            timelimit = (size_t)stoi(args[i+1]);
            timelimitmodifier = 0.001;
            ++i;
        }
        else if(args[i] == "--profile-time-limit"){
            assert(i != args.size() - 1);
            //set time limit for entire program, add in some checks during instruction running to kill program if time is over limit
            //expects time limit in seconds
            profiletimelimit = (size_t)stoi(args[i+1]);
            ++i;
        }
        else if(args[i] == "--profile-threshold"){
            assert(i != args.size() - 1);
            //set profile threshold as a float
            profilethreshold = (float)stod(args[i+1]);
            ++i;
        }
        else{
            cerr << "Unidentified flag: " << args[i] << endl;
            exit(1);
        }
    }
    if(quant_len == 8){
        NnetQ<int8_t> nnet = NnetQ<int8_t>(filename, quant_len, dec_bits);
        // cout << nnet << endl;
        nnet.evaluateSymbolic(knownboundsfilename, constraintsfilename,verbosemode,skip_abstract,skip_modelgen,skip_modelfix,skip_solveinparts,modelfixthreshold,modelfixlimit,solveinpartsthreshold,solveinpartslimit,modelcounter,timelimit,z3leafcountlimit,z3leaftimelimit,timelimitmodifier,profiletimelimit, profilethreshold);
    }
    else if(quant_len == 16){
        NnetQ<int16_t> nnet = NnetQ<int16_t>(filename, quant_len, dec_bits);
        // cout << nnet << endl;
        nnet.evaluateSymbolic(knownboundsfilename, constraintsfilename,verbosemode,skip_abstract,skip_modelgen,skip_modelfix,skip_solveinparts,modelfixthreshold,modelfixlimit,solveinpartsthreshold,solveinpartslimit,modelcounter,timelimit,z3leafcountlimit,z3leaftimelimit,timelimitmodifier,profiletimelimit, profilethreshold);
    }
    else if(quant_len == 32){
        NnetQ<int32_t> nnet = NnetQ<int32_t>(filename, quant_len, dec_bits);
        // cout << nnet << endl;
        nnet.evaluateSymbolic(knownboundsfilename, constraintsfilename,verbosemode,skip_abstract,skip_modelgen,skip_modelfix,skip_solveinparts,modelfixthreshold,modelfixlimit,solveinpartsthreshold,solveinpartslimit,modelcounter,timelimit,z3leafcountlimit,z3leaftimelimit,timelimitmodifier,profiletimelimit, profilethreshold);
    }
    else{
        cerr << quant_len << " is not a valid quantized length" << endl;
        exit(1);
    }
    // profilefile.open("timingprofile.txt", ios_base::app);
    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    // profilefile << "End: " << millisec_since_epoch << endl;
    // profilefile.close();
    return 0;
}
