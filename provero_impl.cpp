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
        cout << "Usage: ./provero_impl <networkfile> <knownboundsfile> <constraintsfile> <fixedpointlength> <numdecimalbits> <eta> <delta> <timeout (s)>" << endl;
        return 0;
    }
    string knownboundsfilename = argv[2];
    string constraintsfilename = argv[3];
    int quant_len = stoi(argv[4]);
    int dec_bits = stoi(argv[5]);
    float eta = stof(argv[6]);
    float delta = stof(argv[7]);
    size_t timelimit = (size_t)stoi(argv[8]); //also default unlimited
    // double timelimitmodifier = 1; // assume seconds
    if(quant_len == 8){
        NnetQ<int8_t> nnet = NnetQ<int8_t>(filename, quant_len, dec_bits);
        // cout << nnet << endl;
        // string res = nnet.proveroImpl(knownboundsfilename, constraintsfilename,0.62,eta, delta, timelimit);
        // cout << res << endl;
        // return 0;
        //TODO: instead of calling once, call with binary search loop starting at theta=0.5. 
        //let each run of proveroimpl call its methods and print its outputs
        // but at the end of this print the high and low bounds on the robustness
        //also adjust timelimit each time based on how much time has elapsed since start---if we've already used 25 minutes, timelimit should be 5 not 30 minutes
        long int overall_starttime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        float lowbound = 0;
        float highbound = 1;
        while(duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - overall_starttime < (long int) timelimit){
            if(highbound - lowbound <= eta){
                cout << "Distance less than eta" << endl;
                cout << "Lowbound: " << lowbound << endl;
                cout << "Highbound: " << highbound << endl;
                long int timeelapsed = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - overall_starttime;
                cout << "Time total: " << timeelapsed << endl;
                break;
            }
            float theta = (highbound + lowbound)/2;
            cout << "Trying theta " << theta << endl;
            size_t newtimelimit = timelimit - (size_t)(duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - overall_starttime);
            string res = nnet.proveroImpl(knownboundsfilename, constraintsfilename,theta,eta, delta, newtimelimit);
            if(res == "Yes"){
                //move highbound to be equal to theta
                highbound = theta;
            }
            else if(res == "No"){
                lowbound = theta;
            }
            else{
                //got either "None" or "Timeout"
                cout << "non yes or no result" << endl;
                cout << "Lowbound: " << lowbound << endl;
                cout << "Highbound: " << highbound << endl;
                long int timeelapsed = duration_cast<seconds>(system_clock::now().time_since_epoch()).count() - overall_starttime;
                cout << "Time total: " << timeelapsed << endl;
                break;
            }
        }
        
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
