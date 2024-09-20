#include <string>
#include <vector>
// #include "symbextree.hpp"
#include "symformula.hpp"
#include "leaflist.hpp"
#include "leaf.hpp"
#include "instruction.hpp"
#include "abstract.hpp"
#include "bigsizet.hpp"
#include "helpers.hpp"
#include "z3++.h"
#ifndef NNETQ_HPP
#define NNETQ_HPP

using namespace std;

template <typename T>
class NnetQ{
    private:
        size_t numLayers;
        size_t* layerSizes;
        size_t inputSize;
        size_t outputSize;
        // T* mins;
        // T* maxes;
        // T* means;
        // T* ranges;
        vector<T>** weights;
        T** biases;
        int quant_len;
        int dec_bits;
        /*static */z3::context c;
        //parameters, possibly decided by user
        size_t VERBOSE_MODE = 0;
        size_t MODEL_COUNTER = 0;
        size_t MODEL_FIX_THRESHOLD = 10000; //placeholder value, should never be reached. Will be replaced by correct value once first z3 timeout happens
        size_t MODEL_FIX_LIMIT = 5;
        size_t NUM_EXPR_IN_SUBSET = 10000; //placeholder value, should never be reached. Will be replaced by correct value once first z3 timeout happens
        size_t SOLVEINPARTS_LIMIT = 0;
        size_t TIME_LIMIT = 0;
        size_t Z3_LEAF_COUNT_LIMIT = 0;
        size_t Z3_LEAF_TIME_LIMIT = 0;
        double TIME_LIMIT_MODIFIER = 1;
        size_t PROFILE_TIME_LIMIT = 0;
        float PROFILE_THRESHOLD = 0.05;
        //paremeters to decide to skip some/all optimizations
        bool SKIP_ABSTRACT = 0;
        bool SKIP_MODEL_GEN = 0; // if this is 1, two below have to be 1 as well
        bool SKIP_MODEL_FIX = 0;
        bool SKIP_SOLVEINPARTS = 0;
        //variables to tally effect of smt optimizations
        int z3_avoided_unsat; // number of z3 computations that can be entirely avoided by abstract interpretation, unsat result
        int z3_avoided_alwayssat; // number of z3 computations that can be entirely avoided because nothing was added
        int z3_simplified; // number of z3 formulas that still needed to be run but with only 1 of 2 possible added clauses
        int z3_avoided_model; // number of z3 computations that can be entirely avoided because the model worked
        int getfixes_unsat; // number of z3 computations avoided because getFixes found unsat with just last clause and bounds
        int fixedmodel_sat; // number of cases where getPossibleFixes returned a model that declared the formula SAT
        int solveinparts_unsat; // number of z3 computations avoided because solveInParts found unsat
        int solveinparts_sat; // number of z3 computations avoided because solveInParts found a model
        int didnt_change_ok; //number of z3 computations that didn't change and were below solveInParts threshold
        int didnt_change_bad; // number of z3 computations that didn't change because solveInParts (or other optimization) failed
    public:
        NnetQ();
        NnetQ(string filename, int ql, int db);
        NnetQ(const NnetQ<T>& n);
        NnetQ<T>& operator=(NnetQ<T> n) noexcept;
        vector<T> networkComputation(vector<T> *layerWeights, T *layerBiases, vector<T> inputNorm, size_t layerSize);
        T* evaluate(T* input);
        void evaluateSymbolic(string knownboundsfilename, string constraintsfilename,size_t verbosemode,bool skip_abstract,bool skip_modelgen,bool skip_modelfix,bool skip_solveinparts,size_t modelfixthreshold,size_t modelfixlimit,size_t solveinpartsthreshold,size_t solveinpartslimit,size_t modelcounter, size_t timelimit, size_t z3leafcountlimit, size_t z3leaftimelimit, double timelimitmodifier, size_t profiletimelimit, float profilethreshold);
        int runInstructions(Leaf<T> leaf, queue<Instruction> instructions, z3::expr outputconstraints);
        size_t getInputSize();
        size_t getOutputSize();
        tuple<bool, vector<z3::model>> getPossibleFixes(z3::model model, z3::expr newexpression, z3::expr bounds);
        tuple<bool, vector<z3::model>> solveInParts(vector<z3::expr> pc, z3::expr bounds, z3::expr newexpr);
        tuple<bool, size_t> checkAlreadyTried(vector<string> alreadyTried, vector<size_t> option, string optionstring);
        vector<size_t> getNotYetTried(vector<string> alreadyTried, size_t max);
        tuple<vector<T>, vector<T>, z3::expr, z3::expr, BigSizeT> parseConstraints(vector<string> knownboundslines, vector<string> lines);
        string proveroImpl(string knownboundsfilename, string constraintsfilename,float theta,float eta, float delta, size_t timelimit);
        tuple<float,float> createInterval(float theta, float theta_one, float theta_two, float eta, bool left);
        string proveroTester(float theta_one, float theta_two, float delta, long int starttime, size_t timelimit, vector<size_t> bases, vector<T> inputmins, size_t maxcount);
        ~NnetQ<T>();
        ostream& print(ostream& out) const;
};

template <typename T>
inline ostream & operator<<(ostream& str, const NnetQ<T>& nnet){
    return nnet.print(str);
}

#endif