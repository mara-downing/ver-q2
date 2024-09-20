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
#include <assert.h>
#include <queue>
#include <bitset>
#include <math.h>
#include <typeinfo>
#include <algorithm>
#include <chrono>
#include <sys/time.h>
#include <ctime>
#include "nnet_q.hpp"

using namespace std;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

template class NnetQ<int8_t>;
template class NnetQ<int16_t>;
template class NnetQ<int32_t>;

static BigSizeT finalmodelcount = 0;
static BigSizeT profilecorrectcount = 0;
static BigSizeT profileincorrectcount = 0;
static string constraintkey = "";
static size_t modelcountcount = 0;
static long int starttime = 0;
static bool exact = 1;
static int outputclass = -1;

template <typename T>
T floatToFixedPoint(float f, int qlen, int dec){
    if(f*pow(2, dec) <= -1*pow(2, qlen-1)){
        return (T)(-1*pow(2, qlen-1));
    }
    else if(f*pow(2, dec) >= pow(2, qlen-1)-1){
        return (T)pow(2, qlen-1)-1;
    }
    else{
        return (T)(round(f * (1 << dec)));
    }
}

template <typename T>
vector<T> dotProduct(vector<T>* layerWeights, vector<T> inputNorm, size_t layerSize){
    //todo: write here
    vector<T> output;
    for(size_t i = 0; i < layerSize; ++i){
        assert(layerWeights[i].size() == inputNorm.size());
        T dotprod = 0;
        for(size_t j = 0; j < inputNorm.size(); ++j){
            dotprod += layerWeights[i][j]*inputNorm[j];
        }
        output.push_back(dotprod);
    }
    return output;
}

template <typename T>
vector<T> addBiases(T* layerBiases, vector<T> inputNorm, size_t layerSize){
    //todo: write here
    assert(layerSize == inputNorm.size());
    vector<T> output;
    for(size_t i = 0; i < layerSize; ++i){
        output.push_back(layerBiases[i] + inputNorm[i]);
    }
    return output;
}

template <typename T>
vector<T> maxWithZero(vector<T> inputNorm){
    //todo: write here
    for(size_t i = 0; i < inputNorm.size(); ++i){
        if(inputNorm[i] < 0){
            inputNorm[i] = 0;
        }
    }
    return inputNorm;
}

string z3ToBarvinok(z3::expr e){
    string result = "(";
    if(e.is_numeral()){
        // cout << "number: " << e << endl;
        string temp = e.to_string();
        result = "";
        if(temp[0] == '('){
            for(size_t i = 0; i < temp.length(); ++i){
                if(temp[i] != '(' && temp[i] != ')' && temp[i] != ' '){
                    result += temp[i];
                }
            }
        }
        else{
            result = temp;
        }
    }
    else if(e.is_const()){
        // cout << "var: " << e << endl;
        result = e.to_string();
    }
    else if(e.is_app()){
        if(e.is_and()){
            for(size_t i = 0; i < e.num_args(); ++i){
                result += z3ToBarvinok(e.arg(i));
                if(i != e.num_args()-1){
                    result += " and ";
                }
            }
        }
        else if(e.is_or()){
            for(size_t i = 0; i < e.num_args(); ++i){
                result += z3ToBarvinok(e.arg(i));
                if(i != e.num_args()-1){
                    result += " or ";
                }
            }
        }
        else if(e.decl().name().str() == "<"){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "<" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == ">"){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + ">" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == "<="){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "<=" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == ">="){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + ">=" + z3ToBarvinok(e.arg(1));
        }
        // else if(e.decl().name().str() == "div"){
        //     assert(e.num_args() == 2);
        //     result += z3ToBarvinok(e.arg(0)) + "/" + z3ToBarvinok(e.arg(1));
        // }
        else if(e.decl().name().str() == "+"){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "+" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == "-"){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "-" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == "*"){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "*" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == "="){
            assert(e.num_args() == 2);
            result += z3ToBarvinok(e.arg(0)) + "=" + z3ToBarvinok(e.arg(1));
        }
        else if(e.decl().name().str() == "not"){
            assert(e.num_args() == 1);
            result += "not " + z3ToBarvinok(e.arg(0));
        }
        else{
            cout << "application" << endl;
            cout << e.decl() << endl;
            cout << e.decl().name().str() << endl;
            cout << e.num_args() << endl;
            cout << "not yet implemented" << endl;
        }
        result += ")";
    }
    else{
        cout << "no idea: " << e << endl;
    }
    // cout << result << endl;
    return result;
}

vector<z3::expr> getClauses(z3::expr e){
    vector<z3::expr> clauses;
    if(e.is_app()){
        if(e.is_and()){
            for(size_t i = 0; i < e.num_args(); ++i){
                vector<z3::expr> newclauses = getClauses(e.arg(i));
                for(size_t j = 0; j < newclauses.size(); ++j){
                    clauses.push_back(newclauses[j]);
                }
            }
        }
        else{
            clauses.push_back(e);
        }
    }
    else{
        cout << "Something weird: " << e << endl;
    }
    return clauses;
}

vector<int> getCoefficients(vector<string> varnamevector, z3::expr e){
    //by now e has to be a mathematical expression
    vector<int> results;
    for(size_t i = 0; i < varnamevector.size(); ++i){
        results.push_back(0);
    }
    results.push_back(0);
    if(e.is_numeral()){
        string temp = e.to_string();
        string result = "";
        if(temp[0] == '('){
            for(size_t i = 0; i < temp.length(); ++i){
                if(temp[i] != '(' && temp[i] != ')' && temp[i] != ' '){
                    result += temp[i];
                }
            }
        }
        else{
            result = temp;
        }
        results[(int)varnamevector.size()] = stoi(result);
    }
    else if(e.is_const()){
        string result = e.to_string();
        for(size_t i = 0; i < varnamevector.size(); ++i){
            if(result == varnamevector[i]){
                results[i] = 1;
                break;
            }
        }
    }
    else if(e.is_app()){
        if(e.decl().name().str() == "*"){
            assert(e.num_args() == 2);
            if(e.arg(0).is_numeral() && e.arg(1).is_numeral()){
                string temp1 = e.arg(0).to_string();
                string result1 = "";
                if(temp1[0] == '('){
                    for(size_t i = 0; i < temp1.length(); ++i){
                        if(temp1[i] != '(' && temp1[i] != ')' && temp1[i] != ' '){
                            result1 += temp1[i];
                        }
                    }
                }
                else{
                    result1 = temp1;
                }
                string temp2 = e.arg(0).to_string();
                string result2 = "";
                if(temp2[0] == '('){
                    for(size_t i = 0; i < temp2.length(); ++i){
                        if(temp2[i] != '(' && temp2[i] != ')' && temp2[i] != ' '){
                            result2+= temp2[i];
                        }
                    }
                }
                else{
                    result2 = temp2;
                }
                results[(int)varnamevector.size()] = stoi(result1) * stoi(result2);
            }
            else if(e.arg(0).is_const() && e.arg(1).is_numeral()){
                string temp = e.arg(1).to_string();
                string result = "";
                if(temp[0] == '('){
                    for(size_t i = 0; i < temp.length(); ++i){
                        if(temp[i] != '(' && temp[i] != ')' && temp[i] != ' '){
                            result += temp[i];
                        }
                    }
                }
                else{
                    result = temp;
                }
                string result2 = e.arg(0).to_string();
                for(size_t i = 0; i < varnamevector.size(); ++i){
                    if(result2 == varnamevector[i]){
                        results[i] = stoi(result);
                        break;
                    }
                }
            }
            else if(e.arg(0).is_numeral() && e.arg(1).is_const()){
                string temp = e.arg(0).to_string();
                string result = "";
                if(temp[0] == '('){
                    for(size_t i = 0; i < temp.length(); ++i){
                        if(temp[i] != '(' && temp[i] != ')' && temp[i] != ' '){
                            result += temp[i];
                        }
                    }
                }
                else{
                    result = temp;
                }
                string result2 = e.arg(1).to_string();
                for(size_t i = 0; i < varnamevector.size(); ++i){
                    if(result2 == varnamevector[i]){
                        results[i] = stoi(result);
                        break;
                    }
                }
            }
            else{
                cerr << "Non-linear constraint: " << e << endl;
                exit(1);
            }
        }
        else if(e.decl().name().str() == "+"){
            assert(e.num_args() == 2);
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = left[i] + right[i];
            }
        }
        else if(e.decl().name().str() == "-"){
            assert(e.num_args() == 2);
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = left[i] - right[i];
            }
        }
        else{
            cerr << "Not a valid application at this point: " << e << endl;
            exit(1);
        }
    }
    else{
        cerr << "No idea what this is: " << e << endl;
        exit(1);
    }
    return results;
}

tuple<string, bool> convertClause(vector<string> varnamevector, z3::expr e){
    vector<int> results;
    for(size_t i = 0; i < varnamevector.size(); ++i){
        results.push_back(0);
    }
    results.push_back(0);
    if(e.is_app()){
        if(e.is_and()){
            cerr << "Should not have ands left at this stage" << endl;
            exit(1);
        }
        else if(e.is_or()){
            cerr << "Or expressions not allowed in Latte" << endl;
            exit(1);
        }
        else if(e.decl().name().str() == "not"){
            cerr << "!= not allowed in Latte" << endl;
            exit(1);
        }
        else if(e.decl().name().str() == "<"){
            //right hand side stays same, left hand side gets negated, subtract one from rhs
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = right[i] - left[i];
            }
            results[varnamevector.size()] -= 1;
            string clauseline = "";
            clauseline += to_string(results[varnamevector.size()]);
            for(size_t i = 0; i < results.size()-1; ++i){
                clauseline += " " + to_string(results[i]);
            }
            return tuple<string, bool>(clauseline, 0);
        }
        else if(e.decl().name().str() == ">"){
            //right hand side gets negated, left hand side stays same sign, add one to rhs before negation
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = left[i] - right[i];
            }
            results[varnamevector.size()] -= 1;
            string clauseline = "";
            clauseline += to_string(results[varnamevector.size()]);
            for(size_t i = 0; i < results.size()-1; ++i){
                clauseline += " " + to_string(results[i]);
            }
            return tuple<string, bool>(clauseline, 0);
        }
        else if(e.decl().name().str() == "<="){
            //right hand side stays same, left hand side gets negated
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = right[i] - left[i];
            }
            string clauseline = "";
            clauseline += to_string(results[varnamevector.size()]);
            for(size_t i = 0; i < results.size()-1; ++i){
                clauseline += " " + to_string(results[i]);
            }
            return tuple<string, bool>(clauseline, 0);
        }
        else if(e.decl().name().str() == ">="){
            //right hand side gets negated, left hand side stays same sign
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = left[i] - right[i];
            }
            string clauseline = "";
            clauseline += to_string(results[varnamevector.size()]);
            for(size_t i = 0; i < results.size()-1; ++i){
                clauseline += " " + to_string(results[i]);
            }
            return tuple<string, bool>(clauseline, 0);
        }
        else if(e.decl().name().str() == "="){
            //right hand side stays same, left hand side gets negated, bool is 1
            vector<int> left = getCoefficients(varnamevector, e.arg(0));
            vector<int> right = getCoefficients(varnamevector, e.arg(1));
            for(size_t i = 0; i < results.size(); ++i){
                results[i] = right[i] - left[i];
            }
            string clauseline = "";
            clauseline += to_string(results[varnamevector.size()]);
            for(size_t i = 0; i < results.size()-1; ++i){
                clauseline += " " + to_string(results[i]);
            }
            return tuple<string, bool>(clauseline, 1);
        }
    }
    else{
        cerr << "No idea what this is: " << e << endl;
        exit(1);
    }
    return tuple<string, bool>("error", 0);
}

string z3ToLatte(string varnames, z3::expr e){
    vector<z3::expr> clauses = getClauses(e);
    vector<string> varnamevector;
    string varname = "";
    for(size_t i = 0; i < varnames.length(); ++i){
        if(varnames[i] != ','){
            varname += varnames[i];
        }
        else{
            varnamevector.push_back(varname);
            varname = "";
        }
    }
    varnamevector.push_back(varname);
    string result = to_string(clauses.size()) + " " + to_string(varnamevector.size()+1) + "\n";
    vector<size_t> linearities;
    for(size_t i = 0; i < clauses.size(); ++i){
        tuple<string, bool> tempresult = convertClause(varnamevector, clauses[i]);
        result += get<0>(tempresult) + "\n";
        if(get<1>(tempresult)){
            linearities.push_back(i+1);
        }
    }
    if(linearities.size() != 0){
        result += "linearity " + to_string(linearities.size());
        for(size_t i = 0; i < linearities.size(); ++i){
            result += " " + to_string(linearities[i]);
        }
        result += "\n";
    }
    return result;
}

vector<string> getConsts(z3::expr e){
    vector<string> result;
    if(e.is_numeral()){
        //do nothing
    }
    else if(e.is_const()){
        // cout << "var: " << e << endl;
        if(e.to_string()[0] == 'c'){
            bool alreadypresent = 0;
            for(size_t i = 0; i < result.size(); ++i){
                if(e.to_string() == result[i]){
                    alreadypresent = 1;
                    break;
                }
            }
            if(!alreadypresent){
                result.push_back(e.to_string());
            }
        }
    }
    else if(e.is_app()){
        for(size_t i = 0; i < e.num_args(); ++i){
            vector<string> temp = getConsts(e.arg(i));
            for(size_t j = 0; j < temp.size(); ++j){
                bool alreadypresent = 0;
                for(size_t i = 0; i < result.size(); ++i){
                    if(temp[j] == result[i]){
                        alreadypresent = 1;
                        break;
                    }
                }
                if(!alreadypresent){
                    result.push_back(temp[j]);
                }
            }
        }
        
    }
    else{
        cout << "no idea: " << e << endl;
    }
    // cout << result << endl;
    return result;
}

template <typename T>
NnetQ<T>::NnetQ(){
    Z3_set_ast_print_mode(c,Z3_PRINT_SMTLIB2_COMPLIANT);
    numLayers = 0;
    inputSize = 0;
    outputSize = 0;
    z3_avoided_unsat = 0;
    z3_avoided_alwayssat = 0;
    z3_simplified = 0;
    // didnt_change = 0;
    z3_avoided_model = 0;
    getfixes_unsat = 0;
    fixedmodel_sat = 0;
    solveinparts_sat = 0;
    solveinparts_unsat = 0;
    didnt_change_ok = 0;
    didnt_change_bad = 0;
}

template <typename T>
NnetQ<T>::NnetQ(string filename, int ql, int db){
    Z3_set_ast_print_mode(c,Z3_PRINT_SMTLIB2_COMPLIANT);
    quant_len = ql;
    dec_bits = db;
    ifstream t(filename);
    ostringstream contents;
    contents << t.rdbuf();
    string nnet = contents.str();
    size_t start = 0;
    vector<string> lines;
    for(size_t i = 0; i < nnet.length(); ++i){
        if(nnet[i] == '\n'){
            lines.push_back(nnet.substr(start, i-start));
            start = i + 1;
        }
    }
    while(1){
        if(lines[0].length() >= 2 && lines[0].substr(0,2) == "//"){
            lines.erase(lines.begin());
        }
        else{
            break;
        }
    }
    //line 1: numLayers, inputSize, outputSize, maxLayerSize (ignored for now)
    string num = "";
    start = 0;
    size_t count = 0;
    for(size_t i = 0; i < lines[0].size(); ++i){
        if(lines[0][i] == ','){
            size_t res = int(stoi(num));
            if(count == 0){
                numLayers = res;
            }
            else if(count == 1){
                inputSize = res;
            }
            else if(count == 2){
                outputSize = res;
            }
            else{
                break;
            }
            num = "";
            count++;
        }
        else{
            num += string(1,lines[0][i]);
        }
    }
    //read in layer sizes line
    layerSizes = new size_t[numLayers + 1];
    start = 0;
    count = 0;
    num = "";
    for(size_t i = 0; i < lines[1].size(); ++i){
        if(lines[1][i] == ','){
            size_t res = int(stoi(num));
            layerSizes[count] = res;
            num = "";
            count++;
        }
        else{
            num += string(1,lines[1][i]);
        }
    }
    weights = new vector<T>*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        weights[i] = new vector<T>[layerSizes[i+1]];
    }
    biases = new T*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        biases[i] = new T[layerSizes[i+1]];
    }
    size_t wstart = 3;
    size_t bstart = 0;
    for(size_t i = 0; i < numLayers; ++i){
        //need to read layerSizes[i+1] lines into weights, and then same number into biases
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            num = "";
            count = 0;
            for(size_t k = 0; i < lines[wstart + j].size(); ++k){
                if(lines[wstart + j][k] == ','){
                    T res = floatToFixedPoint<T>(stof(num), quant_len, dec_bits);
                    weights[i][j].push_back(res);
                    num = "";
                    ++count;
                    if(count >= layerSizes[i]){
                        break;
                    }
                }
                else{
                    num += string(1,lines[wstart + j][k]);
                }
            }
        }
        bstart = wstart + layerSizes[i+1];
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            num = "";
            count = 0;
            for(size_t k = 0; i < lines[bstart + j].size(); ++k){
                if(lines[bstart + j][k] == ','){
                    T res = floatToFixedPoint<T>(stof(num), quant_len, dec_bits);
                    biases[i][j] = res;
                    num = "";
                    break;
                }
                else{
                    num += string(1,lines[bstart + j][k]);
                }
            }
        }
        wstart = bstart + layerSizes[i+1];
    }
    z3_avoided_unsat = 0;
    z3_avoided_alwayssat = 0;
    z3_simplified = 0;
    z3_avoided_model = 0;
    getfixes_unsat = 0;
    fixedmodel_sat = 0;
    solveinparts_sat = 0;
    solveinparts_unsat = 0;
    didnt_change_ok = 0;
    didnt_change_bad = 0;
}

template <typename T>
NnetQ<T>::NnetQ(const NnetQ& n){
    numLayers = n.numLayers;
    inputSize = n.inputSize;
    outputSize = n.outputSize;
    layerSizes = new size_t[numLayers + 1];
    for(size_t i = 0; i < numLayers+1; ++i){
        layerSizes[i] = n.layerSizes[i];
    }
    // mins = new T[inputSize];
    // for(size_t i = 0; i < inputSize; ++i){
    //     mins[i] = n.mins[i];
    // }
    // maxes = new T[inputSize];
    // for(size_t i = 0; i < inputSize; ++i){
    //     maxes[i] = n.maxes[i];
    // }
    // means = new T[inputSize+1];
    // for(size_t i = 0; i < inputSize+1; ++i){
    //     means[i] = n.means[i];
    // }
    // ranges = new T[inputSize+1];
    // for(size_t i = 0; i < inputSize+1; ++i){
    //     ranges[i] = n.ranges[i];
    // }
    weights = new vector<T>*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        weights[i] = new vector<T>[layerSizes[i+1]];
    }
    for(size_t i = 0; i < numLayers; ++i){
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            weights[i][j] = n.weights[i][j];
        }
    }
    biases = new T*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        biases[i] = new T[layerSizes[i+1]];
    }
    for(size_t i = 0; i < numLayers; ++i){
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            biases[i][j] = n.biases[i][j];
        }
    }
    z3_avoided_unsat = n.z3_avoided_unsat;
    z3_avoided_alwayssat = n.z3_avoided_alwayssat;
    z3_simplified = n.z3_simplified;
    z3_avoided_model = n.z3_avoided_model;
    getfixes_unsat = n.getfixes_unsat;
    fixedmodel_sat = n.fixedmodel_sat;
    solveinparts_sat = n.solveinparts_sat;
    solveinparts_unsat = n.solveinparts_unsat;
    didnt_change_ok = n.didnt_change_ok;
    didnt_change_bad = n.didnt_change_bad;
}

template <typename T>
NnetQ<T>& NnetQ<T>::operator=(NnetQ<T> n) noexcept{
    numLayers = n.numLayers;
    inputSize = n.inputSize;
    outputSize = n.outputSize;
    layerSizes = new size_t[numLayers + 1];
    for(size_t i = 0; i < numLayers+1; ++i){
        layerSizes[i] = n.layerSizes[i];
    }
    // mins = new T[inputSize];
    // for(size_t i = 0; i < inputSize; ++i){
    //     mins[i] = n.mins[i];
    // }
    // maxes = new T[inputSize];
    // for(size_t i = 0; i < inputSize; ++i){
    //     maxes[i] = n.maxes[i];
    // }
    // means = new T[inputSize+1];
    // for(size_t i = 0; i < inputSize+1; ++i){
    //     means[i] = n.means[i];
    // }
    // ranges = new T[inputSize+1];
    // for(size_t i = 0; i < inputSize+1; ++i){
    //     ranges[i] = n.ranges[i];
    // }
    weights = new vector<T>*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        weights[i] = new vector<T>[layerSizes[i+1]];
    }
    for(size_t i = 0; i < numLayers; ++i){
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            weights[i][j] = n.weights[i][j];
        }
    }
    biases = new T*[numLayers];
    for(size_t i = 0; i < numLayers; ++i){
        biases[i] = new T[layerSizes[i+1]];
    }
    for(size_t i = 0; i < numLayers; ++i){
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            biases[i][j] = n.biases[i][j];
        }
    }
    z3_avoided_unsat = n.z3_avoided_unsat;
    z3_avoided_alwayssat = n.z3_avoided_alwayssat;
    z3_simplified = n.z3_simplified;
    z3_avoided_model = n.z3_avoided_model;
    getfixes_unsat = n.getfixes_unsat;
    fixedmodel_sat = n.fixedmodel_sat;
    solveinparts_sat = n.solveinparts_sat;
    solveinparts_unsat = n.solveinparts_unsat;
    didnt_change_ok = n.didnt_change_ok;
    didnt_change_bad = n.didnt_change_bad;
    return *this;
}

template <typename T>
vector<T> NnetQ<T>::networkComputation(vector<T> *layerWeights, T *layerBiases, vector<T> inputNorm, size_t layerSize){
    int max = pow(2, (quant_len - 1)) - 1;
    int min = -(pow(2, (quant_len - 1)));

    vector<T> output;

    if (quant_len == 8){
        for (size_t i = 0; i < layerSize; ++i){
            // dotprod
            assert(layerWeights[i].size() == inputNorm.size());
            int32_t dotprod = 0;
            for (size_t j = 0; j < inputNorm.size(); ++j){
                dotprod += (int32_t)layerWeights[i][j] * (int32_t)inputNorm[j];
            }

            // biases
            // converting biases to correct number of bits and add to dot product
            int32_t newLayerBias = (int32_t)layerBiases[i];
            newLayerBias *= pow(2, dec_bits);
            int32_t result = dotprod + newLayerBias;

            // checking if result is in bounds min <= x <= max

            T casted_result = 0;
            if (result > max * (int32_t)pow(2, dec_bits)){
                result = max;
                // chop off first three quarters of int by casting to type T
                casted_result = (T)result;
            }
            else if (result < min * (int32_t)pow(2, dec_bits)){
                result = min;
                // chop off first three quarters of int by casting to type T
                casted_result = (T)result;
            }
            else{
                // Note: % mod operator will return a negative number, ex. -3%2 = -1
                // Note: >> operator is arithmetic, ie it will preserve negative, ex. 6>>2 = 2, -6>>2 = -2
                if (result > 0 && ((result & ((int32_t)pow(2, dec_bits)-1)) >= (int32_t)pow(2, dec_bits - 1))){
                    // round up: remove dec_bits then add 1 (must beware of overflow/carry-out)
                    result = result >> dec_bits;
                    result += 1;
                    if (result < 0)
                    {
                        result = max;
                    }
                    casted_result = (T)result;
                }
                else if (result > 0 && ((result & ((int32_t)pow(2, dec_bits)-1)) < (int32_t)pow(2, dec_bits - 1))){
                    // round down: remove dec_bits then cast
                    result = result >> dec_bits;
                    casted_result = (T)result;
                }
                else if (result < 0 && ((result & ((int32_t)pow(2, dec_bits)-1)) >= (int32_t)pow(2, dec_bits - 1))){
                    // round up: remove dec_bits then add 1 (must beware of overflow/carry-out)
                    result = result >> dec_bits;
                    result += 1;
                    casted_result = (T)result;
                }
                else if (result < 0 && ((result & ((int32_t)pow(2, dec_bits)-1)) < (int32_t)pow(2, dec_bits - 1))){
                    // round down: remove dec_bits then cast
                    result = result >> dec_bits;
                    casted_result = (T)result;
                }
                else{
                    // result = 0
                    casted_result = (T)result;
                }
                // can be further simplified, combine 2nd and 4th else if statements as well
                // as else statement into one else statement
            }

            // output.push_back(layerBiases[i] + inputNorm[i]);
            output.push_back(casted_result);
        }
    }
    else if (quant_len == 16){
        for (size_t i = 0; i < layerSize; ++i){
            // dotprod
            assert(layerWeights[i].size() == inputNorm.size());
            int64_t dotprod = 0;
            for (size_t j = 0; j < inputNorm.size(); ++j){
                dotprod += layerWeights[i][j] * inputNorm[j];
            }

            // biases
            // converting biases to correct number of bits and add to dot product
            int64_t newLayerBias = (int64_t)layerBiases[i];
            newLayerBias *= pow(2, dec_bits);
            int64_t result = dotprod + newLayerBias;

            // checking if result is in bounds min <= x <= max

            T casted_result = 0;
            if (result > max * pow(2, dec_bits)){
                result = max;
                // chop off first three quarters of int by casting to type T
                casted_result = (T)result;
            }
            else if (result < min * pow(2, dec_bits)){
                result = min;
                // chop off first three quarters of int by casting to type T
                casted_result = (T)result;
            }
            else{
                // Note: % mod operator will return a negative number, ex. -3%2 = -1
                // Note: >> operator is arithmetic, ie it will preserve negative, ex. 6>>2 = 2, -6>>2 = -2
                if (result > 0 && ((result & ((int64_t)pow(2, dec_bits)-1)) >= (int64_t)pow(2, dec_bits - 1))){
                    // round up: remove dec_bits then add 1 (must beware of overflow/carry-out)
                    result = result >> dec_bits;
                    result += 1;
                    if (result < 0){
                        result = max;
                    }
                    casted_result = (T)result;
                }
                else if (result > 0 && ((result & ((int64_t)pow(2, dec_bits)-1)) < (int64_t)pow(2, dec_bits - 1))){
                    // round down: remove dec_bits then cast
                    result = result >> dec_bits;
                    casted_result = (T)result;
                }
                else if (result < 0 && ((result & ((int64_t)pow(2, dec_bits)-1)) >= (int64_t)pow(2, dec_bits - 1))){
                    // round up: remove dec_bits then add 1 (must beware of overflow/carry-out)
                    result = result >> dec_bits;
                    result += 1;
                    casted_result = (T)result;
                }
                else if (result < 0 && ((result & ((int64_t)pow(2, dec_bits)-1)) < (int64_t)pow(2, dec_bits - 1))){
                    // round down: remove dec_bits then cast
                    result = result >> dec_bits;
                    casted_result = (T)result;
                }
                else{
                    // result = 0
                    casted_result = (T)result;
                }
                // can be further simplified, combine 2nd and 4th else if statements as well
                // as else statement into one else statement
            }
            output.push_back(casted_result);
        }
    }
    else{
        cerr << "Casting for quant_len > 16 not currently implemented" << endl;
        exit(1);
    }

    return output;
}

template <typename T>
T *NnetQ<T>::evaluate(T *input){
    vector<T> inputNorm;
    for (size_t i = 0; i < inputSize; ++i){
        inputNorm.push_back(input[i]);
    }
    for (size_t i = 0; i < numLayers - 1; ++i){
        // inputNorm = dotProduct(weights[i], inputNorm, layerSizes[i+1]);
        // inputNorm = addBiases(biases[i], inputNorm, layerSizes[i+1]);
        inputNorm = networkComputation(weights[i], biases[i], inputNorm, layerSizes[i + 1]);
        inputNorm = maxWithZero(inputNorm); // ReLU
    }
    inputNorm = networkComputation(weights[numLayers - 1], biases[numLayers - 1], inputNorm, layerSizes[numLayers]);
    T *output = new T[outputSize];
    for (size_t i = 0; i < outputSize; ++i){
        output[i] = inputNorm[i];
    }
    return output;
}

template <typename T>
void NnetQ<T>::evaluateSymbolic(string knownboundsfilename, string constraintsfilename,size_t verbosemode,bool skip_abstract,bool skip_modelgen,bool skip_modelfix,bool skip_solveinparts,size_t modelfixthreshold,size_t modelfixlimit,size_t solveinpartsthreshold,size_t solveinpartslimit,size_t modelcounter, size_t timelimit, size_t z3leafcountlimit, size_t z3leaftimelimit, double timelimitmodifier, size_t profiletimelimit, float profilethreshold){
    long int sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    // cout << "start of eval: " << sec_since_epoch << endl;
    auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    starttime = sec_since_epoch;
    VERBOSE_MODE = verbosemode;
    MODEL_FIX_THRESHOLD = modelfixthreshold;
    MODEL_FIX_LIMIT = modelfixlimit;
    NUM_EXPR_IN_SUBSET = solveinpartsthreshold;
    SOLVEINPARTS_LIMIT = solveinpartslimit;
    MODEL_COUNTER = modelcounter;
    SKIP_ABSTRACT = skip_abstract;
    SKIP_MODEL_GEN = skip_modelgen;
    if(skip_modelgen){
        SKIP_MODEL_FIX = 1;
        SKIP_SOLVEINPARTS = 1;
    }
    else{
        SKIP_MODEL_FIX = skip_modelfix;
        SKIP_SOLVEINPARTS = skip_solveinparts;
    }
    TIME_LIMIT = timelimit;
    TIME_LIMIT_MODIFIER = timelimitmodifier;
    Z3_LEAF_COUNT_LIMIT = z3leafcountlimit;
    Z3_LEAF_TIME_LIMIT = z3leaftimelimit;
    PROFILE_TIME_LIMIT = profiletimelimit;
    PROFILE_THRESHOLD = profilethreshold;
    ifstream kbt(knownboundsfilename);
    ostringstream kbcontents;
    kbcontents << kbt.rdbuf();
    string knownboundsstring = kbcontents.str();
    string knownbound = "";
    vector<string> knownboundslines;
    size_t start = 0;
    for(size_t i = 0; i < knownboundsstring.length(); ++i){
        if(knownboundsstring[i] == '\n'){
            knownboundslines.push_back(knownboundsstring.substr(start, i-start));
            start = i + 1;
        }
    }
    ifstream t(constraintsfilename);
    ostringstream contents;
    contents << t.rdbuf();
    string constraintstring = contents.str();
    string constraint = "";
    vector<string> lines;
    start = 0;
    for(size_t i = 0; i < constraintstring.length(); ++i){
        if(constraintstring[i] == '\n'){
            lines.push_back(constraintstring.substr(start, i-start));
            start = i + 1;
        }
    }
    for(size_t i = 0; i < numLayers+1; ++i){
        constraintkey += to_string(layerSizes[i]) + "_";
    }
    for(size_t i = 0; i < constraintsfilename.length(); ++i){
        if(constraintsfilename[i] != '.' && constraintsfilename[i] != '/'){
            constraintkey += string(1,constraintsfilename[i]);
        }
        else{
            constraintkey += "_";
        }
    }
    tuple<vector<T>, vector<T>, z3::expr, z3::expr, BigSizeT> constraintinfo = parseConstraints(knownboundslines, lines);
    vector<T> inputmins = get<0>(constraintinfo);
    vector<T> inputmaxes = get<1>(constraintinfo);
    z3::expr inputconstraints = get<2>(constraintinfo);
    z3::expr outputconstraints = get<3>(constraintinfo);
    BigSizeT kbmc = get<4>(constraintinfo);
    cout << "Model Count of Known Bounds: " << kbmc << endl;
    unordered_map<string, z3::expr> vars;
    unordered_map<string, Abstract> abstractvars;
    vector<z3::expr> emptyvec;
    for(size_t i = 0; i < inputSize; ++i){
        vars.insert(pair<const string, z3::expr>("x"+to_string(i), c.int_const(("sym_x"+to_string(i)).c_str())));
        if(inputmins[i] > 0){
            abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(3)));
        }
        else if(inputmins[i] == 0){
            if(inputmaxes[i] == 0){
                abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(2)));
            }
            else{
                abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(5)));
            }
        }
        else if(inputmaxes[i] < 0){
            abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(1)));
        }
        else if(inputmaxes[i] == 0){
            abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(4)));
        }
        else{
            abstractvars.insert(pair<const string, Abstract>("x"+to_string(i), Abstract(7)));
        }
    }
    Leaflist<T> leaflist;
    z3::expr between_bounds = c.int_const(("sym_x"+to_string(0)).c_str()) >= c.int_val(inputmins[0]) && c.int_const(("sym_x"+to_string(0)).c_str()) <= c.int_val(inputmaxes[0]);
    for(size_t i = 1; i < inputSize; ++i){
        between_bounds = between_bounds && c.int_const(("sym_x"+to_string(i)).c_str()) >= c.int_val(inputmins[i]) && c.int_const(("sym_x"+to_string(i)).c_str()) <= c.int_val(inputmaxes[i]);
    }
    // cout << between_bounds << endl;
    between_bounds = between_bounds && inputconstraints; //they should be pretty simple anyway, just throwing them in with the bounds
    //get model count of input constraints (including bounds)
    ofstream abcfile;
    abcfile.open("formula.smt2");
    abcfile << "(set-logic QF_LIA)" << endl;
    string countstring = "--count-variable ";
    for(size_t i = 0; i < inputSize; ++i){
        abcfile << "(declare-fun sym_x" << i << " () Int)" << endl;
        countstring += "sym_x"+to_string(i);
        if(i != inputSize-1){
            countstring += ",";
        }
    }
    abcfile << "(assert " << between_bounds << ")" << endl;
    abcfile << "(check-sat)" << endl;
    abcfile.close();
    string countbound = "-bi ";
    if(quant_len == 8){ 
        countbound += "8";
    }
    else if(quant_len == 16){
        countbound += "16";
    }
    else{
        countbound += "32";
    }
    const string command = "abc -i formula.smt2 --count-tuple " + countbound + " " + countstring + " -v 0 >abcoutput.txt 2>abcoutput.txt";
    // cout << command << endl;
    system(command.c_str());
    ifstream abcresfile("abcoutput.txt");
    ostringstream abcrescontents;
    abcrescontents << abcresfile.rdbuf();
    string abcres = abcrescontents.str();
    size_t found = abcres.find("Aborted");
    if(found != string::npos){
        cerr << "Model count not able to be completed by ABC" << endl;
        exit(1);
    }
    size_t tuple_line = 0;
    for(size_t i = 0; i < abcres.length(); ++i){
        if(abcres[i] == 'T' && i < abcres.length() - 5){
            if(abcres[i+1] == 'U' && abcres[i+2] == 'P' && abcres[i+3] == 'L' && abcres[i+4] == 'E'){
                tuple_line = i+5;
                break;
            }
        }
    }
    string abcmodelcount = "";
    for(size_t i = tuple_line; i < abcres.length(); ++i){
        if(abcres[i] == 'c' && i < abcres.length() - 5){
            if(abcres[i+1] == 'o' && abcres[i+2] == 'u' && abcres[i+3] == 'n' && abcres[i+4] == 't'){
                for(size_t j = i + 7; j < abcres.length(); ++j){
                    if(isdigit(abcres[j])){
                        abcmodelcount += string(1,abcres[j]);
                    }
                    else{
                        break;
                    }
                }
                break;
            }
        }
    }
    BigSizeT abcmc(abcmodelcount);
    cout << "Model count of all input constraints: " << abcmc << endl;
    //done with abc (for now)
    leaflist.addLeaf(Leaf<T>(vars, abstractvars, emptyvec, between_bounds, 0));
    for(size_t j = 0; j < leaflist.getFreeLeavesSize(); ++j){
        vars = leaflist.getFreeLeaves()[j]->getVars();
        abstractvars = leaflist.getFreeLeaves()[j]->getAbstractVars();
        for(size_t i = 0; i < inputSize; ++i){
            vars.insert(pair<const string, z3::expr>(("in"+to_string(i)).c_str(), vars.at("x" + to_string(i))));
            abstractvars.insert(pair<const string, Abstract>("in"+to_string(i), abstractvars.at("x" + to_string(i))));
        }
        leaflist.setFreeLeafVars(j,vars);
        leaflist.setAbstractFreeLeafVars(j,abstractvars);
    }
    queue<Instruction> instructions;
    for(size_t i = 0; i < numLayers-1; ++i){
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            instructions.push(Instruction(0,i,layerSizes[i],j));
        }
        instructions.push(Instruction(1,i,layerSizes[i+1], 0));
    }
    for(size_t i = 0; i < outputSize; ++i){
        instructions.push(Instruction(2,numLayers-1,layerSizes[numLayers-1],i));
    }
    instructions.push(Instruction(3,numLayers-1,outputSize,0));
    // Helpers::printQueue(instructions);
    finalmodelcount = 0;
    if(MODEL_COUNTER == 5 || MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
        bool profilechosebf = 0;
        // cout << "in brute force section" << endl;
        if(TIME_LIMIT == 0){
            assert(abcmc < BigSizeT(10000000)); //quick check to make sure we don't start a multi-year program running
        }
        vector<size_t> inputranges;
        for(size_t i = 0; i < inputSize; ++i){
            inputranges.push_back(inputmaxes[i] - inputmins[i] + 1);
        }
        vector<size_t> bases;
        size_t total = 1;
        for(int i = inputSize - 1; i >= 0; --i){
            bases.insert(bases.begin(), total);
            total *= inputranges[i];
        }
        size_t maxcount = total;
        // for(size_t i = 0; i < inputSize; ++i){
        //     cout << (int)inputmins[i] << " " << (int)inputmaxes[i] << endl;
        // }
        //check if inputconstraints is a tautology:
        bool inputtautology = 0;
        z3::solver stemp(c);
        stemp.add(!inputconstraints);
        if(stemp.check() == z3::unsat){
            inputtautology = 1;
        }
        if((MODEL_COUNTER == 6 || MODEL_COUNTER == 7) && (outputclass == -1 || !inputtautology)){
            cout << outputclass << endl;
            cout << inputtautology << endl;
            cout << outputconstraints << endl;
            cerr << "cannot do concrete-enumerate-fast with complex input constraints" << endl;
            exit(1);
        }
        size_t count = 0;
        vector<size_t> random_tested;
        srand(time(0));
        //auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        while(TIME_LIMIT == 0 || duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - millisec_since_epoch < 1000 * TIME_LIMIT * TIME_LIMIT_MODIFIER){
            if(count >= maxcount){
                break;
            }
            size_t tempcount;
            if(TIME_LIMIT != 0){
                // want to choose random rather than use the next in counting
                while(1){
                    //srand(time(0));
                    tempcount = rand() % maxcount;
                    if (!binary_search(random_tested.begin(), random_tested.end(), tempcount)) {
                        //insert into vector
                        if(random_tested.size() == 0){
                            random_tested.push_back(tempcount);
                        }
                        else{
                            for(size_t i = 0; i < random_tested.size(); ++i){
                                if(random_tested.at(i) > tempcount){
                                    random_tested.insert(random_tested.begin() + i, tempcount);
                                    break;
                                }
                                else if(i == random_tested.size() - 1){
                                    random_tested.push_back(tempcount);
                                    break;
                                }
                            }
                        }
                        //end loop (loop was to find unused)
                        break;
                    }
                }
            }
            else{
                tempcount = count;
            }
            size_t basecount = 0;
            size_t* indices = new size_t[inputSize];
            for(size_t i = 0; i < bases.size(); ++i){
                while(tempcount >= bases[i]){
                    tempcount = tempcount - bases[i];
                    basecount += 1;
                }
                indices[i] = basecount;
                basecount = 0;
            }
            T* instance = new T[inputSize];
            for(size_t i = 0; i < inputSize; ++i){
                instance[i] = (T)indices[i] + inputmins[i];
            }
            if(MODEL_COUNTER == 5){
                z3::expr checkinstance = inputconstraints;
                for(size_t i = 0; i < inputSize; ++i){
                    checkinstance = checkinstance && c.int_const(("sym_x"+to_string(i)).c_str()) == c.int_val((int)instance[i]);
                }
                z3::solver s(c);
                z3::solver o(c);
                s.add(checkinstance);
                z3::expr checkoutputs = outputconstraints;
                // T* outputs = this->evaluate(instance);
                switch(s.check()){
                    case z3::unsat:
                        break;
                    case z3::sat:
                        {
                            //at this point, instance constains one full element to test with the neural network
                            //TODO: put code to call evaluate() on instance here, then code to check if the result satisfies outputconstraints
                            //if it satisfies outputconstraints, add 1 to finalmodelcount
                            T* outputs = this->evaluate(instance);
                            for(size_t i = 0; i < outputSize; ++i){
                                checkoutputs = checkoutputs && (c.int_const(("y"+to_string(i)).c_str()) == c.int_val((int)outputs[i]));
                            }
                            o.add(checkoutputs);
                            if(o.check() == z3::sat){
                                finalmodelcount = finalmodelcount + 1;
                            }
                            /** 
                            for(size_t i = 0; i < inputSize; ++i){ //this code (through cout << endl;) just prints the instance
                                cout << (int)instance[i] << ",";
                            }
                            cout << endl;
                            **/
                            delete[] outputs;
                            break;
                        }
                    case z3::unknown:
                        cerr << "Should not be unknown" << endl;
                        exit(1);
                        break;
                }
            }
            else if(MODEL_COUNTER == 6){
                T* outputs = this->evaluate(instance);
                //cannot get here without inputtautology, so any generated instance is correct.
                //check if outputclass is the greatest of the outputs
                bool outcome = 1;
                for(size_t i = 0; i < outputSize; ++i){
                    if(outputclass != (int)i && outputs[outputclass] <= outputs[i]){
                        outcome = 0;
                    }
                }
                if(TIME_LIMIT == 0){
                    if(outcome){
                        finalmodelcount = finalmodelcount + 1;
                    }
                }
                else{
                    if(!outcome){
                        finalmodelcount = finalmodelcount + 1;
                    }
                }
                delete[] outputs;
            }
            else{
                //profiling first and then deciding
                assert(TIME_LIMIT != 0);
                assert(PROFILE_TIME_LIMIT != 0);
                // check if time passed since millisec_since_epoch is less than profiletimelimit before each check
                if((duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - millisec_since_epoch) < 1000.0 * PROFILE_TIME_LIMIT){
                    T* outputs = this->evaluate(instance);
                    bool outcome = 1;
                    for(size_t i = 0; i < outputSize; ++i){
                        if(outputclass != (int)i && outputs[outputclass] <= outputs[i]){
                            outcome = 0;
                        }
                    }
                    //if outcome is 0, was misclassified
                    if(outcome){
                        //add one to correctly classified count
                        profilecorrectcount = profilecorrectcount + 1;
                    }
                    if(!outcome){
                        //add one to incorrectly classified count
                        profileincorrectcount = profileincorrectcount + 1;
                    }
                    delete[] outputs;
                }
                // if it is past, decide model count method based on threshold--- stay here but act like MODEL_COUNTER == 6 if above threshold, otherwise set to 3 and add code to transfer over to symbolic
                else if(!profilechosebf){
                    //first time over time limit: check threshold
                    if(profileincorrectcount / (profileincorrectcount + profilecorrectcount) > PROFILE_THRESHOLD){
                        profilechosebf = 1;
                    }
                    else{
                        delete[] instance;
                        delete[] indices;
                        MODEL_COUNTER = 3;
                        //print profile info before switching: X out of Y tested in profiletimelimit seconds were adversaries
                        cout << profileincorrectcount << " out of " << (profileincorrectcount + profilecorrectcount) << " tested in " << profiletimelimit << " seconds were adversaries" << endl;
                        goto symboliccount;
                    }
                }
                else{
                    T* outputs = this->evaluate(instance);
                    bool outcome = 1;
                    for(size_t i = 0; i < outputSize; ++i){
                        if(outputclass != (int)i && outputs[outputclass] <= outputs[i]){
                            outcome = 0;
                        }
                    }
                    //if outcome is 0, was misclassified
                    if(outcome){
                        //add one to correctly classified count
                        profilecorrectcount = profilecorrectcount + 1;
                    }
                    if(!outcome){
                        //add one to incorrectly classified count
                        profileincorrectcount = profileincorrectcount + 1;
                    }
                    delete[] outputs;
                }
            }
            delete[] instance;
            delete[] indices;
            ++count;
        }
    }
    else{
        symboliccount: // used to jump here from profile then choose approach, everything necessary has already been constructed
        for(size_t i = 0; i < leaflist.getFreeLeavesSize(); ++i){
            Leaf<T> leaf = *(leaflist.getFreeLeaves()[i]);
            this->runInstructions(leaf, instructions, outputconstraints);
        }
    }
    if(MODEL_COUNTER != 3 && (MODEL_COUNTER != 6 || TIME_LIMIT == 0) && MODEL_COUNTER != 7){
        cout << "Final Model Count: " << finalmodelcount << endl;
        if(exact){
            cout << "Exact Solution" << endl;
        }
        else{
            cout << "Sound Lower Bound" << endl;
        }
    }
    else if(MODEL_COUNTER == 7){
        // present estimated percent robust, percent tested of total, and any other statistical things I can come up with.
        // also raw count of adversaries discovered -- max possible robustness
        cout << "Estimated percent robust: " << profilecorrectcount / (profileincorrectcount + profilecorrectcount) << endl;
        cout << "Tested " << (profileincorrectcount + profilecorrectcount) << " total inputs" << endl;
        cout << "Sound upper bound on robustness: " << abcmc - profileincorrectcount << endl;
    }
    else{
        finalmodelcount = abcmc - finalmodelcount;
        cout << "Final Model Count: " << finalmodelcount << endl;
        if(exact){
            cout << "Exact Solution" << endl;
        }
        else{
            cout << "Sound Upper Bound" << endl;
        }
    }
    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    // cout << "end of eval: " << millisec_since_epoch << endl;
}

template <typename T>
int NnetQ<T>::runInstructions(Leaf<T> leaf, queue<Instruction> instructions, z3::expr outputconstraints){
    long int sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    if(TIME_LIMIT != 0 && (long int)TIME_LIMIT < sec_since_epoch - starttime){
        //have to find a way to return without killing program outright
        exact = 0;
        return 1;
    }
    Instruction instr = instructions.front();
    instructions.pop();
    string newconstname = "c" + to_string(instr.getType()) + to_string(instr.getLayer()) + to_string(instr.getValue1()) + to_string(instr.getValue2());
    if(instr.getType() == 0){
        unordered_map<string, z3::expr> vars = leaf.getVars();
        unordered_map<string, Abstract> abstractvars = leaf.getAbstractVars();
        size_t i = instr.getValue2();
        z3::expr dotprod = c.int_val(0);//filler val until actual things happen
        Abstract abstractdotprod(2);
        if(weights[instr.getLayer()][i][0] == 0){
            dotprod = c.int_val(0);
            abstractdotprod = Abstract(2);
        }
        else{
            dotprod = vars.at("in0") * c.int_val(weights[instr.getLayer()][i][0]);
            Abstract abstractweight(0);
            if(weights[instr.getLayer()][i][0] < 0){
                abstractweight = Abstract(1);
            }
            else{
                abstractweight = Abstract(3);
            }
            abstractdotprod = Abstract(2, abstractvars.at("in0"), abstractweight); 
        }
        for(size_t j = 1; j < instr.getValue1(); ++j){
            if(weights[instr.getLayer()][i][j] != 0){
                if(vars.at("in"+to_string(j)).is_numeral() && vars.at("in"+to_string(j)).get_numeral_int64() == 0){
                    //do nothing
                }
                else{
                    dotprod = dotprod + vars.at("in"+to_string(j)) * c.int_val(weights[instr.getLayer()][i][j]);
                    Abstract abstractweight(0);
                    if(weights[instr.getLayer()][i][j] < 0){
                        abstractweight = Abstract(1);
                    }
                    else{
                        abstractweight = Abstract(3);
                    }
                    abstractdotprod = Abstract(0,abstractdotprod, Abstract(2, abstractvars.at("in"+to_string(j)), abstractweight));
                }
            }
        }
        z3::expr new_in = dotprod + c.int_val(biases[instr.getLayer()][i]*(int)pow(2,dec_bits));//) / c.int_val((int)pow(2,dec_bits));
        Abstract abstractbias(0);
        if(biases[instr.getLayer()][i] < 0){
            abstractbias = Abstract(1);
        }
        else if(biases[instr.getLayer()][i] == 0){
            abstractbias = Abstract(2);
        }
        else{
            abstractbias = Abstract(3);
        }
        Abstract new_abstract_in(3,Abstract(0,abstractdotprod,abstractbias), Abstract(3));
        unordered_map<string, z3::expr> leqzerovars = vars;
        leqzerovars.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_val(0)));
        unordered_map<string, Abstract> a_leqzerovars = abstractvars;
        a_leqzerovars.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(2)));
        Abstract a_leqzeropc;
        if(!SKIP_ABSTRACT){
            a_leqzeropc = Abstract(6,new_abstract_in, Abstract(2));
        }
        if(a_leqzeropc.getIsSat()){
            if(a_leqzeropc.getSatLevel() == 0){
                ++z3_avoided_alwayssat;
                if(leaf.getHasModel()){
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
            }
            else{
                z3::expr leqzeropc = new_in < c.int_val((int)pow(2,dec_bits)/2);//new_in <= c.int_val(0);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    leqzeropc = leqzeropc && pc[e];
                }
                pc.push_back(new_in < c.int_val((int)pow(2,dec_bits)/2));
                z3::expr leqzeropc_bounded = leqzeropc && leaf.getBounds();
                // cout << leaf.getClauseCount() << endl;
                if(leaf.getHasModel() && leaf.getModel().eval(new_in < c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in < c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(leqzeropc).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in < c.int_val((int)pow(2,dec_bits)/2));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(leqzeropc_bounded);
                            auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                            // cout << "start of z3 check: " << millisec_since_epoch << endl;
                            switch(s.check()){
                                case z3::unsat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    break;
                                case z3::sat:
                                {
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    if(!SKIP_MODEL_GEN){
                                        z3::model m = s.get_model();
                                        int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, m), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                }
                                case z3::unknown:
                                    auto millisec_since_epoch2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch2 << endl;
                                    if(millisec_since_epoch2 - millisec_since_epoch > 5000 && NUM_EXPR_IN_SUBSET == 10000){
                                        if(MODEL_FIX_THRESHOLD == 10000 || NUM_EXPR_IN_SUBSET == 10000){
                                            cout << "Found first timeout, setting threshold to " << pc.size()-2 << endl;
                                        }
                                        if(NUM_EXPR_IN_SUBSET == 10000){
                                            NUM_EXPR_IN_SUBSET = pc.size()-2;
                                        }
                                        if(MODEL_FIX_THRESHOLD == 10000){
                                            MODEL_FIX_THRESHOLD = pc.size()-2;
                                        }
                                    }
                                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else{
            ++z3_avoided_unsat;
        }
        unordered_map<string, z3::expr> gtzerovars_ltmax = vars;
        gtzerovars_ltmax.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_const(newconstname.c_str())));//new_in));
        unordered_map<string, Abstract> a_gtzerovars_ltmax = abstractvars;
        a_gtzerovars_ltmax.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(3)));
        // Abstract a_gtzeropc_ltmax1(5,new_abstract_in, Abstract(2));
        // Abstract a_gtzeropc_ltmax2(4,new_abstract_in, Abstract(3));
        Abstract a_gtzeropc_ltmax1;
        Abstract a_gtzeropc_ltmax2;
        if(!SKIP_ABSTRACT){
            a_gtzeropc_ltmax1 = Abstract(5,new_abstract_in, Abstract(2));
            a_gtzeropc_ltmax2 = Abstract(4,new_abstract_in, Abstract(3));
        }
        if((!a_gtzeropc_ltmax1.getIsSat() || !a_gtzeropc_ltmax2.getIsSat())){
            ++z3_avoided_unsat;
        }
        else{
            if(a_gtzeropc_ltmax1.getSatLevel() == 0){
                if(a_gtzeropc_ltmax2.getSatLevel() == 0){
                    ++z3_avoided_alwayssat;
                    vector<z3::expr> pc = leaf.getPC();
                    pc.push_back(new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                    if(leaf.getHasModel()){
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                    else{
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                }
                else{
                    ++z3_simplified;
                    z3::expr gtzeropc_ltmax = new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in < c.int_val((int)pow(2,quant_len-1)-1);
                    vector<z3::expr> pc = leaf.getPC();
                    for(size_t e = 0; e < pc.size(); ++e){
                        gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                    }
                    pc.push_back(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                    z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                    if(leaf.getHasModel() && leaf.getModel().eval(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                        ++z3_avoided_model;
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                    else{
                        bool foundfix_orunsat = 0;
                        if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                            tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                            if(!(get<0>(fixesTuple))){
                                ++getfixes_unsat;
                                foundfix_orunsat = 1;
                            }
                            for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                                if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                    ++fixedmodel_sat;
                                    foundfix_orunsat = 1;
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                                }
                            }
                        }
                        if(!foundfix_orunsat){
                            bool foundmodel_orunsat = 0;
                            if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                                tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                                if(!get<0>(thing)){
                                    foundmodel_orunsat = 1;
                                    ++solveinparts_unsat;
                                }
                                if(get<0>(thing) && get<1>(thing).size() > 0){
                                    foundmodel_orunsat = 1;
                                    ++solveinparts_sat;
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                }
                            }
                            if(!foundmodel_orunsat){
                                z3::solver s(c);
                                z3::params p(c);
                                p.set(":timeout", 5000u);
                                s.set(p);
                                s.add(gtzeropc_ltmax_bounded);
                                auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                // cout << "start of z3 check: " << millisec_since_epoch << endl;
                                switch(s.check()){
                                    case z3::unsat:
                                        // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                        // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                        break;
                                    case z3::sat:
                                        // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                        // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                        if(!SKIP_MODEL_GEN){
                                            int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                            if(signal == 1){
                                                return 1;
                                            }
                                        }
                                        else{
                                            int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                            if(signal == 1){
                                                return 1;
                                            }
                                        }
                                        break;
                                    case z3::unknown:
                                        auto millisec_since_epoch2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                        // cout << "end of z3 check: " << millisec_since_epoch2 << endl;
                                        if(millisec_since_epoch2 - millisec_since_epoch > 5000 && NUM_EXPR_IN_SUBSET == 10000){
                                            if(MODEL_FIX_THRESHOLD == 10000 || NUM_EXPR_IN_SUBSET == 10000){
                                                cout << "Found first timeout, setting threshold to " << pc.size()-2 << endl;
                                            }
                                            if(NUM_EXPR_IN_SUBSET == 10000){
                                                NUM_EXPR_IN_SUBSET = pc.size()-2;
                                            }
                                            if(MODEL_FIX_THRESHOLD == 10000){
                                                MODEL_FIX_THRESHOLD = pc.size()-2;
                                            }
                                        }
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            else if(a_gtzeropc_ltmax2.getSatLevel() == 0){
                ++z3_simplified;
                z3::expr gtzeropc_ltmax = new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in > c.int_val(0);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                }
                pc.push_back(new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_ltmax_bounded);
                            auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                            // cout << "start of z3 check: " << millisec_since_epoch << endl;
                            switch(s.check()){
                                case z3::unsat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    break;
                                case z3::sat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    auto millisec_since_epoch2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch2 << endl;
                                    if(millisec_since_epoch2 - millisec_since_epoch > 5000){
                                        if(MODEL_FIX_THRESHOLD == 10000 || NUM_EXPR_IN_SUBSET == 10000){
                                            cout << "Found first timeout, setting threshold to " << pc.size()-2 << endl;
                                        }
                                        if(NUM_EXPR_IN_SUBSET == 10000){
                                            NUM_EXPR_IN_SUBSET = pc.size()-2;
                                        }
                                        if(MODEL_FIX_THRESHOLD == 10000){
                                            MODEL_FIX_THRESHOLD = pc.size()-2;
                                        }
                                    }
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
            else{
                z3::expr gtzeropc_ltmax = new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in > c.int_val(0) && new_in < c.int_val((int)pow(2,quant_len-1)-1);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                }
                pc.push_back(new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in >= c.int_val((int)pow(2,dec_bits)/2) && new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_ltmax_bounded);
                            auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                            // cout << "start of z3 check: " << millisec_since_epoch << endl;
                            switch(s.check()){
                                case z3::unsat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    break;
                                case z3::sat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    auto millisec_since_epoch2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch2 << endl;
                                    if(millisec_since_epoch2 - millisec_since_epoch > 5000 && NUM_EXPR_IN_SUBSET == 10000){
                                        if(MODEL_FIX_THRESHOLD == 10000 || NUM_EXPR_IN_SUBSET == 10000){
                                            cout << "Found first timeout, setting threshold to " << pc.size()-2 << endl;
                                        }
                                        if(NUM_EXPR_IN_SUBSET == 10000){
                                            NUM_EXPR_IN_SUBSET = pc.size()-2;
                                        }
                                        if(MODEL_FIX_THRESHOLD == 10000){
                                            MODEL_FIX_THRESHOLD = pc.size()-2;
                                        }
                                    }
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        unordered_map<string, z3::expr> gtzerovars_gtmax = vars;
        gtzerovars_gtmax.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_val((int)pow(2,quant_len-1)-1)));
        unordered_map<string, Abstract> a_gtzerovars_gtmax = abstractvars;
        a_gtzerovars_gtmax.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(3)));
        // Abstract a_gtzeropc_gtmax(5,new_abstract_in, Abstract(3));
        Abstract a_gtzeropc_gtmax;
        if(!SKIP_ABSTRACT){
            a_gtzeropc_gtmax = Abstract(5,new_abstract_in, Abstract(3));
        }
        if(a_gtzeropc_gtmax.getIsSat()){
            if(a_gtzeropc_gtmax.getSatLevel() == 0){
                ++z3_avoided_alwayssat;
                if(leaf.getHasModel()){
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
            }
            else{
                z3::expr gtzeropc_gtmax = new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2));//new_in >= c.int_val((int)pow(2,quant_len-1)-1);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_gtmax = gtzeropc_gtmax && pc[e];
                }
                pc.push_back(new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)));
                z3::expr gtzeropc_gtmax_bounded = gtzeropc_gtmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2))).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_gtmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_gtmax_bounded);
                            auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                            // cout << "start of z3 check: " << millisec_since_epoch << endl;
                            switch(s.check()){
                                case z3::unsat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    break;
                                case z3::sat:
                                    // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch << endl;
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    auto millisec_since_epoch2 = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                                    // cout << "end of z3 check: " << millisec_since_epoch2 << endl;
                                    if(millisec_since_epoch2 - millisec_since_epoch > 5000 && NUM_EXPR_IN_SUBSET == 10000){
                                        if(MODEL_FIX_THRESHOLD == 10000 || NUM_EXPR_IN_SUBSET == 10000){
                                            cout << "Found first timeout, setting threshold to " << pc.size()-2 << endl;
                                        }
                                        if(NUM_EXPR_IN_SUBSET == 10000){
                                            NUM_EXPR_IN_SUBSET = pc.size()-2;
                                        }
                                        if(MODEL_FIX_THRESHOLD == 10000){
                                            MODEL_FIX_THRESHOLD = pc.size()-2;
                                        }
                                    }
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else{
            ++z3_avoided_unsat;
        }
        return 0;
    }
    else if(instr.getType() == 1){
        unordered_map<string, z3::expr> vars = leaf.getVars();
        unordered_map<string, Abstract> abstractvars = leaf.getAbstractVars();
        for(size_t i = 0; i < instr.getValue1(); ++i){
            vars.erase("in"+to_string(i));
            vars.insert(pair<const string, z3::expr>(("in"+to_string(i)).c_str(), vars.at("new_in"+to_string(i))));
            vars.erase("new_in"+to_string(i));
            abstractvars.erase("in"+to_string(i));
            abstractvars.insert(pair<const string, Abstract>(("in"+to_string(i)).c_str(), abstractvars.at("new_in"+to_string(i))));
            abstractvars.erase("new_in"+to_string(i));
        }
        leaf.setVars(vars);
        leaf.setAbstractVars(abstractvars);
        int signal = this->runInstructions(leaf, instructions, outputconstraints);
        if(signal == 1){
            return 1;
        }
        return 0;
    }
    else if(instr.getType() == 2){
        unordered_map<string, z3::expr> vars = leaf.getVars();
        unordered_map<string, Abstract> abstractvars = leaf.getAbstractVars();
        size_t i = instr.getValue2();
        z3::expr dotprod = c.int_val(0);//filler val until actual things happen
        Abstract abstractdotprod(2);
        if(weights[instr.getLayer()][i][0] == 0){
            dotprod = c.int_val(0);
            abstractdotprod = Abstract(2);
        }
        else{
            dotprod = vars.at("in0") * c.int_val(weights[instr.getLayer()][i][0]);
            Abstract abstractweight(0);
            if(weights[instr.getLayer()][i][0] < 0){
                abstractweight = Abstract(1);
            }
            else{
                abstractweight = Abstract(3);
            }
            abstractdotprod = Abstract(2, abstractvars.at("in0"), abstractweight); 
        }
        for(size_t j = 1; j < instr.getValue1(); ++j){
            if(weights[instr.getLayer()][i][j] != 0){
                if(vars.at("in"+to_string(j)).is_numeral() && vars.at("in"+to_string(j)).get_numeral_int64() == 0){
                    //do nothing
                }
                else{
                    dotprod = dotprod + vars.at("in"+to_string(j)) * c.int_val(weights[instr.getLayer()][i][j]);
                    Abstract abstractweight(0);
                    if(weights[instr.getLayer()][i][j] < 0){
                        abstractweight = Abstract(1);
                    }
                    else{
                        abstractweight = Abstract(3);
                    }
                    abstractdotprod = Abstract(0,abstractdotprod, Abstract(2, abstractvars.at("in"+to_string(j)), abstractweight));
                }
            }
        }
        z3::expr new_in = dotprod + c.int_val(biases[instr.getLayer()][i]*(int)pow(2,dec_bits));//) / c.int_val((int)pow(2,dec_bits));
        Abstract abstractbias(0);
        if(biases[instr.getLayer()][i] < 0){
            abstractbias = Abstract(1);
        }
        else if(biases[instr.getLayer()][i] == 0){
            abstractbias = Abstract(2);
        }
        else{
            abstractbias = Abstract(3);
        }
        Abstract new_abstract_in(3,Abstract(0,abstractdotprod,abstractbias), Abstract(3));
        unordered_map<string, z3::expr> leqzerovars = vars;
        leqzerovars.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_val(-1*(int)pow(2,quant_len-1))));
        unordered_map<string, Abstract> a_leqzerovars = abstractvars;
        a_leqzerovars.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(1)));
        //Abstract a_leqzeropc(6,new_abstract_in, Abstract(1));
        Abstract a_leqzeropc;
        if(!SKIP_ABSTRACT){
            a_leqzeropc = Abstract(6,new_abstract_in, Abstract(1));
        }
        if(a_leqzeropc.getIsSat()){
            if(a_leqzeropc.getSatLevel() == 0){
                ++z3_avoided_alwayssat;
                if(leaf.getHasModel()){
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
            }
            else{
                z3::expr leqzeropc = new_in < c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits) + ((int)pow(2,dec_bits)/2)));//new_in <= c.int_val(-1*(int)pow(2,quant_len-1));
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    leqzeropc = leqzeropc && pc[e];
                }
                pc.push_back(new_in < c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits) + ((int)pow(2,dec_bits)/2))));
                z3::expr leqzeropc_bounded = leqzeropc && leaf.getBounds();
                // cout << leaf.getClauseCount() << endl;
                if(leaf.getHasModel() && leaf.getModel().eval(new_in < c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits) + ((int)pow(2,dec_bits)/2)))).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in < c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits) + ((int)pow(2,dec_bits)/2))), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(leqzeropc).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in < c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits) + ((int)pow(2,dec_bits)/2))));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(leqzeropc_bounded);
                            switch(s.check()){
                                case z3::unsat:
                                    break;
                                case z3::sat:
                                {
                                    if(!SKIP_MODEL_GEN){
                                        z3::model m = s.get_model();
                                        int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1, m), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                }
                                case z3::unknown:
                                    int signal = this->runInstructions(Leaf<T>(leqzerovars, a_leqzerovars, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else{
            ++z3_avoided_unsat;
        }
        unordered_map<string, z3::expr> gtzerovars_ltmax = vars;
        gtzerovars_ltmax.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_const(newconstname.c_str())));
        unordered_map<string, Abstract> a_gtzerovars_ltmax = abstractvars;
        a_gtzerovars_ltmax.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(7)));
        // Abstract a_gtzeropc_ltmax1(5,new_abstract_in, Abstract(1));
        // Abstract a_gtzeropc_ltmax2(4,new_abstract_in, Abstract(7));
        Abstract a_gtzeropc_ltmax1;
        Abstract a_gtzeropc_ltmax2;
        if(!SKIP_ABSTRACT){
            a_gtzeropc_ltmax1 = Abstract(5,new_abstract_in, Abstract(1));
            a_gtzeropc_ltmax2 = Abstract(4,new_abstract_in, Abstract(7));
        }
        if((!a_gtzeropc_ltmax1.getIsSat() || !a_gtzeropc_ltmax2.getIsSat())){
            ++z3_avoided_unsat;
        }
        else{
            if(a_gtzeropc_ltmax1.getSatLevel() == 0){
                if(a_gtzeropc_ltmax2.getSatLevel() == 0){
                    ++z3_avoided_alwayssat;
                    vector<z3::expr> pc = leaf.getPC();
                    pc.push_back(new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                    if(leaf.getHasModel()){
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                    else{
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                }
                else{
                    ++z3_simplified;
                    z3::expr gtzeropc_ltmax = new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in < c.int_val((int)pow(2,quant_len-1)-1);
                    vector<z3::expr> pc = leaf.getPC();
                    for(size_t e = 0; e < pc.size(); ++e){
                        gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                    }
                    pc.push_back(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                    z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                    if(leaf.getHasModel() && leaf.getModel().eval(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                        ++z3_avoided_model;
                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                        if(signal == 1){
                            return 1;
                        }
                    }
                    else{
                        bool foundfix_orunsat = 0;
                        if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                            tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                            if(!(get<0>(fixesTuple))){
                                ++getfixes_unsat;
                                foundfix_orunsat = 1;
                            }
                            for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                                if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                    ++fixedmodel_sat;
                                    foundfix_orunsat = 1;
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                                }
                            }
                        }
                        if(!foundfix_orunsat){
                            bool foundmodel_orunsat = 0;
                            if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                                tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                                if(!get<0>(thing)){
                                    foundmodel_orunsat = 1;
                                    ++solveinparts_unsat;
                                }
                                if(get<0>(thing) && get<1>(thing).size() > 0){
                                    foundmodel_orunsat = 1;
                                    ++solveinparts_sat;
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                }
                            }
                            if(!foundmodel_orunsat){
                                z3::solver s(c);
                                z3::params p(c);
                                p.set(":timeout", 5000u);
                                s.set(p);
                                s.add(gtzeropc_ltmax_bounded);
                                switch(s.check()){
                                    case z3::unsat:
                                        break;
                                    case z3::sat:
                                        if(!SKIP_MODEL_GEN){
                                            int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                            if(signal == 1){
                                                return 1;
                                            }
                                        }
                                        else{
                                            int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                            if(signal == 1){
                                                return 1;
                                            }
                                        }
                                        break;
                                    case z3::unknown:
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                        break;
                                }
                            }
                        }
                    }
                }
            }
            else if(a_gtzeropc_ltmax2.getSatLevel() == 0){
                ++z3_simplified;
                z3::expr gtzeropc_ltmax = new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in > c.int_val(-1*(int)pow(2,quant_len-1));
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                }
                pc.push_back(new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_ltmax_bounded);
                            switch(s.check()){
                                case z3::unsat:
                                    break;
                                case z3::sat:
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
            else{
                z3::expr gtzeropc_ltmax = new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2);//new_in > c.int_val(-1*(int)pow(2,quant_len-1)) && new_in < c.int_val((int)pow(2,quant_len-1)-1);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_ltmax = gtzeropc_ltmax && pc[e];
                }
                pc.push_back(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                z3::expr gtzeropc_ltmax_bounded = gtzeropc_ltmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2)).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_ltmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in < c.int_val((((int)pow(2,quant_len-1)-1)*(int)pow(2,dec_bits)) - ((int)pow(2,dec_bits)/2)) && new_in >= c.int_val((-1*(int)pow(2,quant_len-1)*(int)pow(2,dec_bits)) + ((int)pow(2,dec_bits)/2)) && new_in < c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) + c.int_val((int)pow(2,dec_bits)/2) && new_in >= c.int_val((int)pow(2,dec_bits)) * c.int_const(newconstname.c_str()) - c.int_val((int)pow(2,dec_bits)/2));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_ltmax_bounded);
                            switch(s.check()){
                                case z3::unsat:
                                    break;
                                case z3::sat:
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_ltmax, a_gtzerovars_ltmax, pc, leaf.getBounds(), leaf.getClauseCount()+2), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        unordered_map<string, z3::expr> gtzerovars_gtmax = vars;
        gtzerovars_gtmax.insert(pair<const string, z3::expr>(("new_in"+to_string(instr.getValue2())).c_str(), c.int_val((int)pow(2,quant_len-1)-1)));
        unordered_map<string, Abstract> a_gtzerovars_gtmax = abstractvars;
        a_gtzerovars_gtmax.insert(pair<const string, Abstract>("new_in"+to_string(instr.getValue2()), Abstract(3)));
        // Abstract a_gtzeropc_gtmax(5,new_abstract_in, Abstract(3));
        Abstract a_gtzeropc_gtmax;
        if(!SKIP_ABSTRACT){
            a_gtzeropc_gtmax = Abstract(5,new_abstract_in, Abstract(3));
        }
        if(a_gtzeropc_gtmax.getIsSat()){
            if(a_gtzeropc_gtmax.getSatLevel() == 0){
                ++z3_avoided_alwayssat;
                if(leaf.getHasModel()){
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount(), leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, leaf.getPC(), leaf.getBounds(), leaf.getClauseCount()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
            }
            else{
                z3::expr gtzeropc_gtmax = new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2));//new_in >= c.int_val((int)pow(2,quant_len-1)-1);
                vector<z3::expr> pc = leaf.getPC();
                for(size_t e = 0; e < pc.size(); ++e){
                    gtzeropc_gtmax = gtzeropc_gtmax && pc[e];
                }
                pc.push_back(new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)));
                z3::expr gtzeropc_gtmax_bounded = gtzeropc_gtmax && leaf.getBounds();
                if(leaf.getHasModel() && leaf.getModel().eval(new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2))).is_true()){
                    ++z3_avoided_model;
                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, leaf.getModel()), instructions, outputconstraints);
                    if(signal == 1){
                        return 1;
                    }
                }
                else{
                    bool foundfix_orunsat = 0;
                    if(!SKIP_MODEL_FIX && leaf.getHasModel() && pc.size() > MODEL_FIX_THRESHOLD){
                        tuple<bool, vector<z3::model>> fixesTuple = getPossibleFixes(leaf.getModel(), new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)), leaf.getBounds());
                        if(!(get<0>(fixesTuple))){
                            ++getfixes_unsat;
                            foundfix_orunsat = 1;
                        }
                        for(size_t ii = 0; ii < get<1>(fixesTuple).size(); ++ii){
                            if(get<1>(fixesTuple)[ii].eval(gtzeropc_gtmax).is_true()){
                                ++fixedmodel_sat;
                                foundfix_orunsat = 1;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(fixesTuple)[ii]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                                break;
                            }
                        }
                    }
                    if(!foundfix_orunsat){
                        bool foundmodel_orunsat = 0;
                        if(!SKIP_SOLVEINPARTS && pc.size() > NUM_EXPR_IN_SUBSET){
                            tuple<bool, vector<z3::model>> thing = solveInParts(leaf.getPC(), leaf.getBounds(), new_in >= c.int_val(((int)pow(2,dec_bits) * ((int)pow(2,quant_len-1)-1)) - (((int)pow(2,dec_bits))/2)));
                            if(!get<0>(thing)){
                                foundmodel_orunsat = 1;
                                ++solveinparts_unsat;
                            }
                            if(get<0>(thing) && get<1>(thing).size() > 0){
                                foundmodel_orunsat = 1;
                                ++solveinparts_sat;
                                int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, get<1>(thing)[0]), instructions, outputconstraints);
                                if(signal == 1){
                                    return 1;
                                }
                            }
                        }
                        if(!foundmodel_orunsat){
                            if(pc.size() <= NUM_EXPR_IN_SUBSET){
                                ++didnt_change_ok;
                            }
                            else{
                                ++didnt_change_bad;
                            }
                            z3::solver s(c);
                            z3::params p(c);
                            p.set(":timeout", 5000u);
                            s.set(p);
                            s.add(gtzeropc_gtmax_bounded);
                            switch(s.check()){
                                case z3::unsat:
                                    break;
                                case z3::sat:
                                    if(!SKIP_MODEL_GEN){
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1, s.get_model()), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    else{
                                        int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                        if(signal == 1){
                                            return 1;
                                        }
                                    }
                                    break;
                                case z3::unknown:
                                    int signal = this->runInstructions(Leaf<T>(gtzerovars_gtmax, a_gtzerovars_gtmax, pc, leaf.getBounds(), leaf.getClauseCount()+1), instructions, outputconstraints);
                                    if(signal == 1){
                                        return 1;
                                    }
                                    break;
                            }
                        }
                    }
                }
            }
        }
        else{
            ++z3_avoided_unsat;
        }
        return 0;
    }
    else if(instr.getType() == 3){
        // z3::expr new_in = (dotprod + c.int_val(biases[instr.getLayer()][i]*(int)pow(2,dec_bits))) / c.int_val((int)pow(2,dec_bits));
        // for(size_t i = 0; i < instr.getValue1(); ++i){
        //     vars.insert(pair<const string, z3::expr>(("y"+to_string(i)).c_str(), new_in));
        // }
        vector<string> newconsts;
        unordered_map<string, z3::expr> vars = leaf.getVars();
        for(size_t i = 0; i < instr.getValue1(); ++i){
            vars.insert(pair<const string, z3::expr>(("y"+to_string(i)).c_str(), vars.at("new_in"+to_string(i))));
            vars.erase("new_in"+to_string(i));
        }
        unordered_map<string, z3::expr> newvars;
        for (auto const &pair: vars) {
            if((pair.first).substr(0,1) == "x" || (pair.first).substr(0,1) == "y"){
                newvars.insert(pair);
            }
        }
        leaf.setVars(newvars);
        z3::expr yconstraint = c.int_const("y0") == newvars.at("y0");
        for(size_t i = 1; i < outputSize; ++i){
            yconstraint = yconstraint && c.int_const(("y"+to_string(i)).c_str()) == newvars.at("y"+to_string(i));
        }
        // cout << leaf << endl;
        // cout << "leaf" << endl;
        z3::expr totalconstraint = outputconstraints && leaf.getBounds() && yconstraint;
        z3::expr antitotalconstraint = !(outputconstraints) && leaf.getBounds() && yconstraint;
        vector<z3::expr> pc = leaf.getPC();
        for(size_t e = 0; e < pc.size(); ++e){
            totalconstraint = totalconstraint && pc[e];
            antitotalconstraint = antitotalconstraint && pc[e];
        }
        // cout << totalconstraint << endl;
        // cout << antitotalconstraint << endl;
        bool z3foundunsat = 0;
        z3::solver s(c);
        z3::params p(c);
        p.set(":timeout", 5000u);
        s.set(p);
        if(MODEL_COUNTER != 3){
            s.add(totalconstraint);
            switch(s.check()){
                case z3::unsat:
                    z3foundunsat = 1;
                    break;
                case z3::sat:
                    break;
                case z3::unknown:
                    break;
            }
        }
        else{
            s.add(antitotalconstraint);
            switch(s.check()){
                case z3::unsat:
                    z3foundunsat = 1;
                    break;
                case z3::sat:
                    break;
                case z3::unknown:
                    break;
            }
        }
        newconsts = getConsts(totalconstraint);
        //get model count of leaf constraints
        ofstream abcfile;
        //abcfile.open(("formula"+to_string(finalmodelcount)+".smt2").c_str());
        abcfile.open("formula.smt2");
        abcfile << "(set-logic QF_LIA)" << endl;
        string countstring = "--count-variable ";
        for(size_t i = 0; i < inputSize; ++i){
            abcfile << "(declare-fun sym_x" << i << " () Int)" << endl;
            countstring += "sym_x"+to_string(i);
            countstring += ",";
        }
        for(size_t i = 0; i < outputSize; ++i){
            abcfile << "(declare-fun y" << i << " () Int)" << endl;
            countstring += "y"+to_string(i);
            countstring += ",";
        }
        for(size_t i = 0; i < newconsts.size(); ++i){
            abcfile << "(declare-fun " << newconsts[i] << " () Int)" << endl;
            countstring += newconsts[i];
            if(i < newconsts.size() - 1){
                countstring += ",";
            }
        }
        string varnames = countstring.substr(17);
        abcfile << "(assert " << totalconstraint << ")" << endl;
        abcfile << "(check-sat)" << endl;
        abcfile.close();
        string countbound = "-bi ";
        if(quant_len == 8){ 
            countbound += "8";
        }
        else if(quant_len == 16){
            countbound += "16";
        }
        else{
            countbound += "32";
        }
        if(z3foundunsat){
            //do nothing
        }
        else if(MODEL_COUNTER == 0){
            const string command = "abc -i formula.smt2 --count-tuple " + countbound + " " + countstring + " -v 0 >abcoutput.txt 2>abcoutput.txt";
            // cout << command << endl;
            // ofstream profilefile;
            // profilefile.open("timingprofile.txt", ios_base::app);
            // profilefile << "Command: " << command << endl;
            // auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            // profilefile << "Before leaf (ms): " << millisec_since_epoch << endl;
            system(command.c_str());
            // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            // profilefile << "After leaf (ms): " << millisec_since_epoch << endl;
            //system("cat formula.smt2 >> formulasave.smt2");
            ifstream abcresfile("abcoutput.txt");
            ostringstream abcrescontents;
            abcrescontents << abcresfile.rdbuf();
            string abcres = abcrescontents.str();
            size_t found = abcres.find("Aborted");
            if(found != string::npos){
                cerr << "Model count not able to be completed by ABC" << endl;
                exit(1);
            }
            size_t tuple_line = 0;
            for(size_t i = 0; i < abcres.length(); ++i){
                if(abcres[i] == 'T' && i < abcres.length() - 5){
                    if(abcres[i+1] == 'U' && abcres[i+2] == 'P' && abcres[i+3] == 'L' && abcres[i+4] == 'E'){
                        tuple_line = i+5;
                        break;
                    }
                }
            }
            string abcmodelcount = "";
            for(size_t i = tuple_line; i < abcres.length(); ++i){
                if(abcres[i] == 'c' && i < abcres.length() - 5){
                    if(abcres[i+1] == 'o' && abcres[i+2] == 'u' && abcres[i+3] == 'n' && abcres[i+4] == 't'){
                        for(size_t j = i + 7; j < abcres.length(); ++j){
                            if(isdigit(abcres[j])){
                                abcmodelcount += string(1,abcres[j]);
                            }
                            else{
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            BigSizeT abcmc(abcmodelcount);
            if(VERBOSE_MODE > 1){
                cout << "Model count of leaf: " << abcmc << endl;
            }
            // profilefile << "Count: " << abcmc << endl;
            // const string cpcommand = "cp formula.smt2 profileconstraints/" + constraintkey + "_" + to_string(modelcountcount) + ".smt2";
            // system(cpcommand.c_str());
            // profilefile << "Filename: " << constraintkey << "_" << modelcountcount << ".smt2" << endl;
            // profilefile.close();
            //done with abc
            ++modelcountcount;
            finalmodelcount = finalmodelcount + abcmc;
        }
        else if(MODEL_COUNTER == 1){
            ofstream barvfile;
            barvfile.open("barvinok_to_run.txt");
            // cout << "calling z3ToBarvinok" << endl;
            barvfile << "C := { [" << varnames << "] : ";
            barvfile << z3ToBarvinok(totalconstraint);
            barvfile << "}; card C;" << endl;
            barvfile.close();
            system("barv_fileread");
            ifstream barvresfile("barvinok_output.txt");
            ostringstream barvrescontents;
            barvrescontents << barvresfile.rdbuf();
            string barvres = barvrescontents.str();
            string barvmodelcount = barvres.substr(2,barvres.length()-5);
            BigSizeT barvmc(0);
            if(barvmodelcount != ""){
                barvmc = BigSizeT(barvmodelcount);
            }
            if(VERBOSE_MODE > 1){
                cout << "Model count of leaf: " << barvmc << endl;
            }
            finalmodelcount = finalmodelcount + barvmc;
            // cout << "done with z3ToBarvinok" << endl;
        }
        else if(MODEL_COUNTER == 2){
            ofstream lattefile;
            lattefile.open("latte_to_run.txt");
            lattefile << z3ToLatte(varnames, totalconstraint);
            lattefile.close();
            system("/home/tools/latte-integrale-1.7.5/dest/bin/count latte_to_run.txt >latte_output.txt 2>latte_output.txt");
            ifstream latteresfile("numOfLatticePoints");
            ostringstream latterescontents;
            latterescontents << latteresfile.rdbuf();
            string latteres = latterescontents.str();
            string lattemodelcount = latteres.substr(0,latteres.length()-1);
            BigSizeT lattemc(lattemodelcount);
            if(VERBOSE_MODE > 1){
                cout << "Model count of leaf: " << lattemc << endl;
            }
            finalmodelcount = finalmodelcount + lattemc;
        }
        else if(MODEL_COUNTER == 3){
            bool findingsolutions = 1;
            BigSizeT z3mc(0);
            z3::expr temptotalconstraint = antitotalconstraint;
            long int z3loopstart = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            while(findingsolutions){
                long int sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
                if(TIME_LIMIT != 0 && (long int)TIME_LIMIT < sec_since_epoch - starttime){
                    exact = 0;
                    return 1;
                }
                else if((Z3_LEAF_TIME_LIMIT != 0 && (long int)Z3_LEAF_TIME_LIMIT < sec_since_epoch - z3loopstart) || (Z3_LEAF_COUNT_LIMIT != 0 && BigSizeT(Z3_LEAF_COUNT_LIMIT) == z3mc)){
                    exact = 0;
                    break;
                }
                s.reset();
                s.add(temptotalconstraint);
                switch(s.check()){
                    case z3::unsat:
                        findingsolutions = 0;
                        break;
                    case z3::sat:
                        z3mc = z3mc + BigSizeT(1);
                        {
                            z3::model model = s.get_model();
                            unordered_map<string, int> modelVals;
                            int num_consts = model.num_consts();
                            for(int i = 0; i < num_consts; ++i){
                                string sym = model.get_const_decl(i).name().str();
                                if(sym[0] == 's'){
                                    int sol = (int)(model.get_const_interp(model.get_const_decl(i))).get_numeral_int64();
                                    modelVals[sym] = sol;
                                }
                                else if(sym[0] == 'c'){
                                    //not including created constants in model
                                }
                                else if(sym[0] == 'y'){
                                    //also not including output vars in model
                                }
                                else{
                                    cout << "shouldn't happen? " << sym << endl;
                                }
                            }
                            z3::expr temp = c.int_const("sym_x0") == c.int_val(modelVals["sym_x0"]);
                            for(size_t i = 1; i < inputSize; ++i){
                                temp = temp && c.int_const(("sym_x" + to_string(i)).c_str()) == c.int_val(modelVals["sym_x"+to_string(i)]);
                            }
                            temptotalconstraint = temptotalconstraint && !(temp);
                        }
                        break;
                    case z3::unknown:
                        cerr << "unknown, cannot generate model to negate" << endl;
                        findingsolutions = 0;
                        exact = 0;
                        break;
                }
            }
            if(VERBOSE_MODE > 1){
                cout << "Model count of leaf: " << z3mc << endl;
            }
            finalmodelcount = finalmodelcount + z3mc;
        }
        else if(MODEL_COUNTER == 4){
            bool findingsolutions = 1;
            BigSizeT z3mc(0);
            z3::expr temptotalconstraint = totalconstraint;
            long int z3loopstart = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
            while(findingsolutions){
                long int sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
                if(TIME_LIMIT != 0 && (long int)TIME_LIMIT < sec_since_epoch - starttime){
                    exact = 0;
                    return 1;
                }
                else if((Z3_LEAF_TIME_LIMIT != 0 && (long int)Z3_LEAF_TIME_LIMIT < sec_since_epoch - z3loopstart) || (Z3_LEAF_COUNT_LIMIT != 0 && BigSizeT(Z3_LEAF_COUNT_LIMIT) == z3mc)){
                    exact = 0;
                    break;
                }
                s.reset();
                s.add(temptotalconstraint);
                switch(s.check()){
                    case z3::unsat:
                        findingsolutions = 0;
                        break;
                    case z3::sat:
                        z3mc = z3mc + BigSizeT(1);
                        {
                            z3::model model = s.get_model();
                            unordered_map<string, int> modelVals;
                            int num_consts = model.num_consts();
                            for(int i = 0; i < num_consts; ++i){
                                string sym = model.get_const_decl(i).name().str();
                                if(sym[0] == 's'){
                                    int sol = (int)(model.get_const_interp(model.get_const_decl(i))).get_numeral_int64();
                                    modelVals[sym] = sol;
                                }
                                else if(sym[0] == 'c'){
                                    //not including created constants in model
                                }
                                else if(sym[0] == 'y'){
                                    //also not including output vars in model
                                }
                                else{
                                    cout << "shouldn't happen? " << sym << endl;
                                }
                            }
                            z3::expr temp = c.int_const("sym_x0") == c.int_val(modelVals["sym_x0"]);
                            for(size_t i = 1; i < inputSize; ++i){
                                temp = temp && c.int_const(("sym_x" + to_string(i)).c_str()) == c.int_val(modelVals["sym_x"+to_string(i)]);
                            }
                            temptotalconstraint = temptotalconstraint && !(temp);
                        }
                        break;
                    case z3::unknown:
                        cerr << "unknown, cannot generate model to negate" << endl;
                        findingsolutions = 0;
                        exact = 0;
                        break;
                }
            }
            if(VERBOSE_MODE > 1){
                cout << "Model count of leaf: " << z3mc << endl;
            }
            finalmodelcount = finalmodelcount + z3mc;
        }
        else{
            //do nothing
            cout << "Model count of leaf: 0" << endl;
        }
        //finalmodelcount++;
        return 0;
    }
    else{
        assert(false);//should never happen
    }
}

template <typename T>
size_t NnetQ<T>::getInputSize(){
    return inputSize;
}

template <typename T>
size_t NnetQ<T>::getOutputSize(){
    return outputSize;
}

template <typename T>
tuple<bool, vector<z3::model>> NnetQ<T>::getPossibleFixes(z3::model model, z3::expr newexpression, z3::expr bounds){
    vector<z3::model> fixes;
    //want to generate a group of models by setting all but 1 var in newexpression to value from model, then checking SAT on newexpression && bounds
    //if UNSAT continue, otherwise add a model to the vector.
    //end either when out of vars to vary, or have generated MODEL_FIX_LIMIT models.
    //if no models found with this method, generate 1 model at the end with just newexpression && bounds, unvaried
    z3::expr new_bounded = newexpression && bounds;
    bool sat;
    z3::solver s(c);
    z3::params p(c);
    p.set(":timeout", 5000u);
    s.set(p);
    s.add(new_bounded);
    switch(s.check()){
        case z3::unsat:
            sat = 0;
            break;
        case z3::sat:
            sat = 1;
            break;
        case z3::unknown:
            sat = 1;
            break;
    }
    if(!sat){
        return tuple<bool, vector<z3::model>>{0,fixes};
    }
    else{
        unordered_map<string, int> modelVals;
        int num_consts = model.num_consts();
        for(int i = 0; i < num_consts; ++i){
            string sym = model.get_const_decl(i).name().str();
            if(sym[0] == 's'){
                int sol = (int)(model.get_const_interp(model.get_const_decl(i))).get_numeral_int64();
                modelVals[sym] = sol;
            }
            else if(sym[0] == 'c'){
                //not including created constants in model
            }
            else{
                cout << "shouldn't happen? " << sym << endl;
            }
        }
        for(int i = 0; i < num_consts; ++i){
            z3::expr temp = new_bounded;
            // i loops through which symbolic var is excluded each try
            // j loops through all symbolic variables to put values of all but i into solver
            for(int j = 0; j < num_consts; ++j){
                if(i != j){
                    temp = temp && c.int_const(("sym_x" + to_string(j)).c_str()) == c.int_val(modelVals["sym_x"+to_string(j)]);
                }
            }
            bool hasFix;
            s.reset();
            s.add(temp);
            switch(s.check()){
                case z3::unsat:
                    hasFix = 0;
                    break;
                case z3::sat:
                    hasFix = 1;
                    break;
                case z3::unknown:
                    hasFix = 0;
                    break;
            }
            if(hasFix){
                z3::model m = s.get_model();
                fixes.push_back(m);
                if(fixes.size() >= MODEL_FIX_LIMIT){
                    break;
                }
            }
        }
        if(fixes.size() == 0){
            s.reset();
            s.add(new_bounded);
            if(s.check() == z3::sat){
                fixes.push_back(s.get_model());
            }
        }
        return tuple<bool, vector<z3::model>>{1,fixes};
    }
}

template <typename T>
tuple<bool, vector<z3::model>> NnetQ<T>::solveInParts(vector<z3::expr> pc, z3::expr bounds, z3::expr newexpr){
    //size_t age[NUM_EXPR_IN_SUBSET-1]{};
    vector<size_t> age;
    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
        age.push_back(0);
    }
    vector<z3::model> finalmodel;
    vector<string> alreadyTried;
    z3::expr subset = newexpr && bounds;
    string tried = "";
    size_t count = 0;
    size_t maxpermutations = 1;
    for(size_t i = pc.size()-1; i > pc.size()-NUM_EXPR_IN_SUBSET; --i){
        subset = subset && pc[i];
        tried = to_string(i) + "." + tried;
        age[count] = i;
        count++;
        maxpermutations *= (i+1);
    }
    alreadyTried.push_back(tried);
    count = 0;
    size_t temp_solveinpartslimit = SOLVEINPARTS_LIMIT;
    if(SOLVEINPARTS_LIMIT == 0){
        temp_solveinpartslimit = maxpermutations;
    }
    while(count < maxpermutations && count < temp_solveinpartslimit){
        z3::solver s(c);
        z3::params p(c);
        p.set(":timeout", 5000u);
        s.set(p);
        s.add(subset);
        bool res = 0;
        // auto millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        // cout << "start of z3 check (solveinparts): " << millisec_since_epoch << endl;
        switch(s.check()){
            case z3::unsat:
                // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                // cout << "end of z3 check: " << millisec_since_epoch << endl;
                return tuple<bool, vector<z3::model>>{0,finalmodel};
                break;
            case z3::sat:
                // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                // cout << "end of z3 check: " << millisec_since_epoch << endl;
                res = 1;
                break;
            case z3::unknown:
                // millisec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
                // cout << "end of z3 check: " << millisec_since_epoch << endl;
                res = 0;
                break;
        }
        if(res){
            //generate model, try model on all elements of pc. at first failure, replace oldest element (besides newexpr) with failed expr, then loop again
            z3::model m = s.get_model();
            bool modelworks = 1;
            size_t failedclause = 0;
            for(size_t i = 0; i < pc.size(); ++i){
                if(!(m.eval(pc[i]).is_true())){
                    modelworks = 0;
                    failedclause = i;
                    break;
                }
            }
            if(modelworks){
                finalmodel.push_back(m);
                return tuple<bool, vector<z3::model>>{1,finalmodel};
            }
            else{
                //boot element from end of age array, move all others down, add this element to beginning
                for(size_t i = NUM_EXPR_IN_SUBSET - 2; i > 0; --i){
                    age[i] = age[i-1];
                }
                age[0] = failedclause;
                //create new (sorted) tried string from age array, check if it is in alreadyTried
                tried = "";
                //size_t agecopy[NUM_EXPR_IN_SUBSET-1];
                vector<size_t> agecopy;
                for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                    //agecopy[i] = age[i];
                    agecopy.push_back(age[i]);
                }
                //sort(agecopy, agecopy + NUM_EXPR_IN_SUBSET-1);
                sort(agecopy.begin(), agecopy.end());
                for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                    tried = tried + to_string(agecopy[i]) + ".";
                }
                tuple<bool, size_t> checked = checkAlreadyTried(alreadyTried, agecopy, tried);
                //if not, insert in proper place, then create z3::expr subset
                if(!get<0>(checked)){
                    alreadyTried.insert(alreadyTried.begin()+get<1>(checked), tried);
                    subset = newexpr && bounds;
                    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                        subset = subset && pc[age[i]];
                    }
                }
                else{
                    //if it is in alreadyTried, generate not yet tried subset and put into z3::expr subset
                    subset = newexpr && bounds;
                    vector<size_t> nyt = getNotYetTried(alreadyTried, pc.size()-1);
                    if(nyt[0] == pc.size()-1){
                        break;
                    }
                    tried = "";
                    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                        subset = subset && pc[nyt[i]];
                        age[i] = nyt[i];
                        tried = tried + to_string(nyt[i]) + ".";
                    }
                    alreadyTried.insert(alreadyTried.begin()+nyt[nyt.size()-1], tried);
                }
            }
        }
        else{
            //randomly generate new untried subset, loop
            subset = newexpr && bounds;
            vector<size_t> nyt = getNotYetTried(alreadyTried, pc.size()-1);
            if(nyt[0] == pc.size()-1){
                break;
            }
            tried = "";
            for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                subset = subset && pc[nyt[i]];
                age[i] = nyt[i];
                tried = tried + to_string(nyt[i]) + ".";
            }
            alreadyTried.insert(alreadyTried.begin()+nyt[nyt.size()-1], tried);
        }
        ++count;
    }
    return tuple<bool, vector<z3::model>>{1,finalmodel};
}

template <typename T>
tuple<bool, size_t> NnetQ<T>::checkAlreadyTried(vector<string> alreadyTried, vector<size_t> option, string optionstring){
    size_t wheretoinsert = alreadyTried.size();
    bool passedpossiblematch = 0;
    bool foundinalreadytried = 0;
    for(size_t i = 0; i < alreadyTried.size(); ++i){
        if(optionstring == alreadyTried[i]){
            //found it
            foundinalreadytried = 1;
            break;
        }
        else{
            //split up alreadyTried[i] into array of size_t
            //size_t alreadyTriedArray[NUM_EXPR_IN_SUBSET-1]{};
            vector<size_t> alreadyTriedArray;
            for(size_t j = 0; j < NUM_EXPR_IN_SUBSET-1; ++j){
                alreadyTriedArray.push_back(0);
            }
            size_t strcount = 0;
            string num = "";
            for(size_t j = 0; j < alreadyTried[i].size(); ++j){
                if(alreadyTried[i][j] == '.'){
                    size_t result = (size_t)stoi(num);
                    alreadyTriedArray[strcount] = result;
                    num = "";
                    strcount++;
                }
                else{
                    num += string(1,alreadyTried[i][j]);
                }
            }
            for(size_t j = 0; j < NUM_EXPR_IN_SUBSET-1; ++j){
                if(alreadyTriedArray[j] > option[j]){
                    wheretoinsert = i;
                    passedpossiblematch = 1;
                    break;
                }
                else if(alreadyTriedArray[j] == option[j]){
                    continue;
                }
                else{
                    break;
                }
            }
            if(passedpossiblematch){
                break;
            }
        }
    }
    return tuple<bool, size_t>{foundinalreadytried, wheretoinsert};
}

template <typename T>
vector<size_t> NnetQ<T>::getNotYetTried(vector<string> alreadyTried, size_t max){
    //size_t arrayOfMaxes[NUM_EXPR_IN_SUBSET-1]{};
    vector<size_t> arrayOfMaxes;
    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
        //arrayOfMaxes[i] = max - (NUM_EXPR_IN_SUBSET-1) + i + 1;
        arrayOfMaxes.push_back(max - (NUM_EXPR_IN_SUBSET-1) + i + 1);
    }
    //size_t notyettried[NUM_EXPR_IN_SUBSET-1];
    vector<size_t> notyettried;
    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
        //notyettried[i] = i;
        notyettried.push_back(i);
    }
    string nytried = "";
    for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
        nytried = nytried + to_string(notyettried[i]) + ".";
    }
    size_t count = 0;
    while(count <= alreadyTried.size()){
        tuple<bool, size_t> result = checkAlreadyTried(alreadyTried, notyettried, nytried);
        if(!get<0>(result)){
            vector<size_t> ret;
            for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                ret.push_back(notyettried[i]);
            }
            ret.push_back(get<1>(result));
            return ret;
        }
        else{
            //increment notyettried to next available
            bool ismax = 1;
            for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                if(notyettried[i] != arrayOfMaxes[i]){
                    ismax = 0;
                }
            }
            if(ismax){
                notyettried[0] = max; //marking that we ran out of options
                vector<size_t> ret;
                for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                    ret.push_back(notyettried[i]);
                }
                ret.push_back(get<1>(result));
                return ret;
            }
            else{
                //increment here
                bool incrementNext = 0;
                for(size_t i = NUM_EXPR_IN_SUBSET - 2; i > 0; --i){
                    if(notyettried[i] < arrayOfMaxes[i]){
                        notyettried[i] = notyettried[i]+1;
                        break;
                    }
                    else{
                        notyettried[i] = 0;
                        if(i == 1){
                            incrementNext = 1;
                        }
                    }
                }
                if(incrementNext){
                    if(notyettried[0] < arrayOfMaxes[0]){
                        notyettried[0] = notyettried[0]+1;
                    }
                    else{
                        notyettried[0] = max; //mark that we ran out
                        vector<size_t> ret;
                        for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                            ret.push_back(notyettried[i]);
                        }
                        ret.push_back(get<1>(result));
                        return ret;
                    }
                }
                nytried = "";
                for(size_t i = 0; i < NUM_EXPR_IN_SUBSET-1; ++i){
                    nytried = nytried + to_string(notyettried[i]) + ".";
                }
            }
        }
        ++count;
    }
    vector<size_t> ret;
    ret.push_back(max);
    return ret;
}

template <typename T>
tuple<vector<T>, vector<T>, z3::expr, z3::expr, BigSizeT> NnetQ<T>::parseConstraints(vector<string> knownboundslines, vector<string> lines){
    //fill mins and maxes vectors:
    vector<T> inputmins;
    vector<T> inputmaxes;
    z3::expr inputconstraints = c.int_val(0) < c.int_val(1);
    z3::expr outputconstraints = c.int_val(0) < c.int_val(1);
    for(size_t i = 0; i < inputSize; ++i){
        inputmins.push_back(-1*(T)pow(2,quant_len-1));
        inputmaxes.push_back((T)pow(2,quant_len-1)-1);
    }
    vector<bool> inputminschecker;
    vector<bool> inputmaxeschecker;
    for(size_t i = 0; i < inputSize; ++i){
        inputminschecker.push_back(0);
        inputmaxeschecker.push_back(0);
    }
    //parse knownbounds lines: can only have <, <=, >, >=, and can only compare var to constant
    for(size_t i = 0; i < knownboundslines.size(); ++i){
        vector<string> lexed;
        for(size_t j = 0; j < knownboundslines[i].length(); ++j){
            if(knownboundslines[i][j] == 'x' || knownboundslines[i][j] == 'y'){
                //found a variable, collect until non-numeric character found
                string var = "V:";
                var += string(1, knownboundslines[i][j]);
                ++j;
                while(j < knownboundslines[i].length()){
                    if(isdigit(knownboundslines[i][j])){
                        var += string(1, knownboundslines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(var);
            }
            else if(knownboundslines[i][j] == '<' || knownboundslines[i][j] == '>'){
                //check for equals next, optional
                string comp = "C:";
                comp += string(1,knownboundslines[i][j]);
                if(j != knownboundslines[i].length()-1 && knownboundslines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                lexed.push_back(comp);
            }
            else if(knownboundslines[i][j] == '=' || knownboundslines[i][j] == '!'){
                cerr << "Not a valid comparison on line " << i << " of known bounds file" << endl;
                exit(1);
            }
            else if(isdigit(knownboundslines[i][j]) || knownboundslines[i][j] == '.' || knownboundslines[i][j] == '-'){
                //is a number, collect until end of number
                string val = "N:";
                val += string(1, knownboundslines[i][j]);
                ++j;
                while(j < knownboundslines[i].length()){
                    if(isdigit(knownboundslines[i][j]) || knownboundslines[i][j] == '.'){
                        val += string(1, knownboundslines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(val);
            }
            else{
                assert(knownboundslines[i][j] == ' ');
            }
        }
        if(lexed.size() != 0){
            assert(lexed.size() == 3); // make sure there are three elements after lexing (should be a var, a comparison, and a val)
            assert(lexed[1][0] == 'C'); // make sure the middle element is a comparison
            assert(lexed[0][0] == 'V' || lexed[0][0] == 'N'); // make sure the first element is a var or a val
            assert(lexed[2][0] == 'V' || lexed[2][0] == 'N'); // make sure the first element is a var or a val
            if(lexed[0][0] == 'V'){
                if(lexed[2][0] == 'V'){
                    cerr << "Variable comparison not allowed in known bounds" << endl;
                    exit(1);
                }
                else{
                    //for now just determine input/output and put into correct expr
                    if(lexed[0][2] == 'x'){
                        //input expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)inputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                        if(lexed[1] == "C:<"){
                            if(quantizednumber != -1*(T)pow(2,quant_len-1)){
                                inputmaxes[intnum] = quantizednumber-1;
                                inputmaxeschecker[intnum] = 1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:<="){
                            inputmaxes[intnum] = quantizednumber;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else if(lexed[1] == "C:>"){
                            if(quantizednumber != (T)pow(2,quant_len-1)-1){
                                inputmins[intnum] = quantizednumber+1;
                                inputminschecker[intnum] = 1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:>="){
                            inputmins[intnum] = quantizednumber;
                            inputminschecker[intnum] = 1;
                        }
                        else if(lexed[1] == "C:=="){
                            // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                            inputmins[intnum] = quantizednumber;
                            inputminschecker[intnum] = 1;
                            inputmaxes[intnum] = quantizednumber;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                    else{
                        assert(lexed[0][2] == 'y');
                        //output expr
                        cerr << "Known bounds cannot include output variables" << endl;
                        exit(1);
                    }
                }
            }
            else if(lexed[2][0] == 'V'){
                //for now just determine input/output and put into correct expr
                if(lexed[2][2] == 'x'){
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)inputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                    if(lexed[1] == "C:<"){
                        if(quantizednumber != (T)pow(2,quant_len-1)-1){
                            inputmins[intnum] = quantizednumber+1;
                            inputminschecker[intnum] = 1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:<="){
                        inputmins[intnum] = quantizednumber;
                        inputminschecker[intnum] = 1;
                    }
                    else if(lexed[1] == "C:>"){
                        if(quantizednumber != -1*(T)pow(2,quant_len-1)){
                            inputmaxes[intnum] = quantizednumber-1;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:>="){
                        inputmaxes[intnum] = quantizednumber;
                        inputmaxeschecker[intnum] = 1;
                    }
                    else if(lexed[1] == "C:=="){
                        // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        inputmins[intnum] = quantizednumber;
                        inputminschecker[intnum] = 1;
                        inputmaxes[intnum] = quantizednumber;
                        inputmaxeschecker[intnum] = 1;
                    }
                    else{
                        inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                    }
                }
                else{
                    assert(lexed[2][2] == 'y');
                    //output expr
                    cerr << "Known bounds cannot include output variables" << endl;
                    exit(1);
                }
            }
            else{
                cerr << "Cannot compare constants on line " << i << " of known bounds file" << endl;
                exit(1);
            }
        }
    }
    //generate model count for known bounds: will be used as total for percentage calculation
    BigSizeT mc(1);
    for(size_t i = 0; i < inputSize; ++i){
        BigSizeT tempmc(0);
        for(size_t j = 0; j < (size_t)(inputmaxes[i] - inputmins[i] + 1); ++j){
            tempmc = tempmc + mc;
        }
        //mc = mc * (inputmaxes[i] - inputmins[i] + 1);
        mc = tempmc;
    }
    //parse each line, put into correct spot
    for(size_t i = 0; i < lines.size(); ++i){
        vector<string> lexed;
        for(size_t j = 0; j < lines[i].length(); ++j){
            if(lines[i][j] == 'x' || lines[i][j] == 'y'){
                //found a variable, collect until non-numeric character found
                string var = "V:";
                var += string(1, lines[i][j]);
                ++j;
                while(j < lines[i].length()){
                    if(isdigit(lines[i][j])){
                        var += string(1, lines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(var);
            }
            else if(lines[i][j] == '<' || lines[i][j] == '>'){
                //check for equals next, optional
                string comp = "C:";
                comp += string(1,lines[i][j]);
                if(j != lines[i].length()-1 && lines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                lexed.push_back(comp);
            }
            else if(lines[i][j] == '=' || lines[i][j] == '!'){
                //check for equals next, required
                string comp = "C:";
                comp += string(1,lines[i][j]);
                if(j != lines[i].length()-1 && lines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                else{
                    cerr << "Not a valid comparison on line " << i << " of constraints file" << endl;
                    exit(1);
                }
                lexed.push_back(comp);
            }
            else if(isdigit(lines[i][j]) || lines[i][j] == '.' || lines[i][j] == '-'){
                //is a number, collect until end of number
                string val = "N:";
                val += string(1, lines[i][j]);
                ++j;
                while(j < lines[i].length()){
                    if(isdigit(lines[i][j]) || lines[i][j] == '.'){
                        val += string(1, lines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(val);
            }
            else{
                assert(lines[i][j] == ' ');
            }
        }
        if(lexed.size() != 0){
            assert(lexed.size() == 3); // make sure there are three elements after lexing (should be a var, a comparison, and a val (or another var))
            assert(lexed[1][0] == 'C'); // make sure the middle element is a comparison
            assert(lexed[0][0] == 'V' || lexed[0][0] == 'N'); // make sure the first element is a var or a val
            assert(lexed[2][0] == 'V' || lexed[2][0] == 'N'); // make sure the first element is a var or a val
            if(lexed[0][0] == 'V'){
                if(lexed[2][0] == 'V'){
                    if (lexed[0][2] == 'x' && lexed[2][2] == 'x') {
                        // check first x
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)inputSize);
                        num1 = lexed[0].substr(2);
                        if(num1[0] == 'x'){
                            num1 = "sym_" + num1;
                        }
                        // check second x
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)inputSize);
                        num2 = lexed[2].substr(2);
                        if(num2[0] == 'x'){
                            num2 = "sym_" + num2;
                        }

                        if(lexed[1] == "C:<"){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }
                    } else if (lexed[0][2] == 'x' && lexed[2][2] == 'y') {
                        // check x
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)inputSize);
                        num1 = lexed[0].substr(2);
                        if(num1[0] == 'x'){
                            num1 = "sym_" + num1;
                        }
                        // check y
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)outputSize);
                        num2 = lexed[2].substr(2);
                        if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                            cerr << "Cannot do this in output constraints for concrete-enumerate-fast" << endl;
                            exit(1);
                        }

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }

                    } else if (lexed[0][2] == 'y' && lexed[2][2] == 'x') {
                        // check y
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)outputSize);
                        num1 = lexed[0].substr(2);

                        // check x
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)inputSize);
                        num2 = lexed[2].substr(2);
                        if(num2[0] == 'x'){
                            num2 = "sym_" + num1;
                        }
                        if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                            cerr << "Cannot do this in output constraints for concrete-enumerate-fast" << endl;
                            exit(1);
                        }

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }

                    }
                    else if (lexed[0][2] == 'y' && lexed[2][2] == 'y') {
                        // check first y
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)outputSize);
                        num1 = lexed[0].substr(2);

                        // check second y
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)outputSize);
                        num2 = lexed[2].substr(2);

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                assert(outputclass == -1 || outputclass == intnum2);
                                outputclass = intnum2;
                            }
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                cerr << "Cannot use <= in output constraints for concrete-enumerate-fast" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                assert(outputclass == -1 || outputclass == intnum1);
                                outputclass = intnum1;
                            }
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                cerr << "Cannot use >= in output constraints for concrete-enumerate-fast" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                cerr << "Cannot use == in output constraints for concrete-enumerate-fast" << endl;
                                exit(1);
                            }
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                            if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                                cerr << "Cannot use != in output constraints for concrete-enumerate-fast" << endl;
                                exit(1);
                            }
                        }

                    }
                }
                else{
                    //for now just determine input/output and put into correct expr
                    if(lexed[0][2] == 'x'){
                        //input expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)inputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                        if(lexed[1] == "C:<"){
                            if(quantizednumber != -1*(T)pow(2,quant_len-1)){
                                if(inputmaxes[intnum] > quantizednumber-1){
				    inputmaxes[intnum] = quantizednumber-1;
				}
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:<="){
                            if(inputmaxes[intnum] > quantizednumber){
				inputmaxes[intnum] = quantizednumber;
			    }
                        }
                        else if(lexed[1] == "C:>"){
                            if(quantizednumber != (T)pow(2,quant_len-1)-1){
				if(inputmins[intnum] < quantizednumber+1){
                                    inputmins[intnum] = quantizednumber+1;
				}
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:>="){
			    if(inputmins[intnum] < quantizednumber){
                                inputmins[intnum] = quantizednumber;
			    }
                        }
                        else if(lexed[1] == "C:=="){
                            // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                            inputmins[intnum] = quantizednumber;
                            inputmaxes[intnum] = quantizednumber;
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                    else{
                        assert(lexed[0][2] == 'y');
                        if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                            cerr << "Cannot do this in output constraints for concrete-enumerate-fast" << endl;
                            exit(1);
                        }
                        //output expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)outputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) < c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) <= c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) > c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) >= c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                }
            }
            else if(lexed[2][0] == 'V'){
                //for now just determine input/output and put into correct expr
                if(lexed[2][2] == 'x'){
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)inputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                    if(lexed[1] == "C:<"){
                        if(quantizednumber != (T)pow(2,quant_len-1)-1){
                            if(inputmins[intnum] < quantizednumber+1){
				inputmins[intnum] = quantizednumber+1;
			    }
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:<="){
                        if(inputmins[intnum] < quantizednumber){
			    inputmins[intnum] = quantizednumber;
			}
                    }
                    else if(lexed[1] == "C:>"){
                        if(quantizednumber != -1*(T)pow(2,quant_len-1)){
                            if(inputmaxes[intnum] > quantizednumber-1){
				inputmaxes[intnum] = quantizednumber-1;
			    }
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:>="){
                        if(inputmaxes[intnum] > quantizednumber){
			    inputmaxes[intnum] = quantizednumber;
			}
                    }
                    else if(lexed[1] == "C:=="){
                        // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        inputmins[intnum] = quantizednumber;
                        inputmaxes[intnum] = quantizednumber;
                    }
                    else{
                        inputconstraints = inputconstraints && c.int_const(num.c_str()) != c.int_val(quantizednumber);
                    }
                }
                else{
                    assert(lexed[2][2] == 'y');
                    if(MODEL_COUNTER == 6 || MODEL_COUNTER == 7){
                        cerr << "Cannot do this in output constraints for concrete-enumerate-fast" << endl;
                        exit(1);
                    }

                    //output expr
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)outputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    T quantizednumber = floatToFixedPoint<T>(number, quant_len, dec_bits);
                    if(lexed[1] == "C:<"){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) > c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:<="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) >= c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:>"){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) < c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:>="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) <= c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:=="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                    }
                    else{
                        outputconstraints = outputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                    }
                }
            }
            else{
                cerr << "Later we will implement number to number comparison, not super useful though" << endl;
                exit(1);
            }
        }
    }
    //first int vector is mins, second is maxes (set to int_min and int_max if not specified, ordered in same order as x0, x1, . . .)
    //these two are only for inputs, not caring about outputs
    //first z3::expr is input constraints with != or ==
    //second z3::expr is output constraints (all)
    for(size_t i = 0; i < inputSize; ++i){
        if(!inputminschecker[i]){
            cout << "Warning: input var x" << i << " does not have a minimum set, using smallest fixed point as min" << endl;
        }
        if(!inputmaxeschecker[i]){
            cout << "Warning: input var x" << i << " does not have a maximum set, using largest fixed point as max" << endl;
        }
    }
    return tuple<vector<T>, vector<T>, z3::expr, z3::expr, BigSizeT>(inputmins, inputmaxes, inputconstraints, outputconstraints, mc);
}

template <typename T>
string NnetQ<T>::proveroImpl(string knownboundsfilename, string constraintsfilename,float theta,float eta, float delta, size_t timelimit){
    //This function replicates BinPCertify
    //TODO: read in known bounds and constraints to get sampling ready
    long int sec_since_epoch = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    MODEL_COUNTER = 6;//this makes sure outputclass is set
    //
    ifstream kbt(knownboundsfilename);
    ostringstream kbcontents;
    kbcontents << kbt.rdbuf();
    string knownboundsstring = kbcontents.str();
    string knownbound = "";
    vector<string> knownboundslines;
    size_t start = 0;
    for(size_t i = 0; i < knownboundsstring.length(); ++i){
        if(knownboundsstring[i] == '\n'){
            knownboundslines.push_back(knownboundsstring.substr(start, i-start));
            start = i + 1;
        }
    }
    ifstream t(constraintsfilename);
    ostringstream contents;
    contents << t.rdbuf();
    string constraintstring = contents.str();
    vector<string> lines;
    start = 0;
    for(size_t i = 0; i < constraintstring.length(); ++i){
        if(constraintstring[i] == '\n'){
            lines.push_back(constraintstring.substr(start, i-start));
            start = i + 1;
        }
    }
    tuple<vector<T>, vector<T>, z3::expr, z3::expr, BigSizeT> constraintinfo = parseConstraints(knownboundslines, lines);
    vector<T> inputmins = get<0>(constraintinfo);
    vector<T> inputmaxes = get<1>(constraintinfo);
    // z3::expr inputconstraints = get<2>(constraintinfo);
    // z3::expr outputconstraints = get<3>(constraintinfo);
    // BigSizeT kbmc = get<4>(constraintinfo);
    // cout << "Model Count of Known Bounds: " << kbmc << endl;
    if(outputclass == -1){
        cerr << "problem with outputclass" << endl;
        return "error";
    }
    vector<size_t> inputranges;
    for(size_t i = 0; i < inputSize; ++i){
        inputranges.push_back(inputmaxes[i] - inputmins[i] + 1);
    }
    vector<size_t> bases;
    size_t total = 1;
    for(int i = inputSize - 1; i >= 0; --i){
        bases.insert(bases.begin(), total);
        total *= inputranges[i];
    }
    size_t maxcount = total;
    srand(time(0));
    //this point starts the sampling of one case, needs bases, inputmins, maxcount
    //now provero setup variables
    float theta_one_l = 0;
    float theta_two_l = 0;
    float theta_one_r = 0;
    float theta_two_r = 0;
    float n = 3 + max((float)0, log(theta/eta)) + max((float)0, log((1-theta-eta)/eta));
    float delta_min = delta/n;
    while(1){
        tuple<float,float> thetas = this->createInterval(theta, theta_one_l, theta_two_l, eta, 1);
        theta_one_l = get<0>(thetas);
        theta_two_l = get<1>(thetas);
        if(theta_two_l - theta_one_l > eta){
            string ans = this->proveroTester(theta_one_l, theta_two_l, delta_min, sec_since_epoch, timelimit, bases, inputmins, maxcount);
            // cout << "here" << theta_one_l << " " << theta_two_l << " " << ans << endl;
            if(ans == "Yes"){
                cout << "Yes" << endl;
                return "Yes";
            }
        }
        thetas = this->createInterval(theta, theta_one_r, theta_two_r, eta, 0);
        theta_one_r = get<0>(thetas);
        theta_two_r = get<1>(thetas);
        if(theta_two_r - theta_one_r > eta){
            string ans = this->proveroTester(theta_one_r, theta_two_r, delta_min, sec_since_epoch, timelimit, bases, inputmins, maxcount);
            if(ans == "No"){
                cout << "No" << endl;
                return "No";
            }
        }
        if(theta_two_r - theta_one_r <= eta && theta_two_l - theta_one_l <= eta){
            string res = this->proveroTester(theta, theta+eta, delta_min, sec_since_epoch, timelimit, bases, inputmins, maxcount);
            cout << res << endl;
            return res;
        }
        long int currtime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        if(currtime - sec_since_epoch >= (long int)timelimit){
            cout << "Timeout" << endl;
            return "Timeout";
        }
    }
}

template <typename T>
tuple<float,float> NnetQ<T>::createInterval(float theta, float theta_one, float theta_two, float eta, bool left){
    if(theta == 0 && left){
        return tuple<float, float>(theta, theta+eta);
    }
    else if(theta_one == 0 && theta_two == 0 && left){
        return tuple<float, float>(0,theta);
    }
    else if(theta_one == 0 && theta_two == 0 && !left){
        return tuple<float, float>(theta+eta, 1);
    }
    else{
        float alpha = theta_two - theta_one;
        if(left){
            return tuple<float,float>(theta_two - max(eta, alpha/2), theta_two);
        }
        else{
            return tuple<float,float>(theta_one,theta_one + max(eta, alpha/2));
        }
    }
}

template <typename T>
string NnetQ<T>::proveroTester(float theta_one, float theta_two, float delta, long int starttime, size_t timelimit, vector<size_t> bases, vector<T> inputmins, size_t maxcount){
    size_t capN = (size_t) ceil((pow(sqrt(3*theta_one) + sqrt(3*theta_two), 2)/pow(theta_two - theta_one, 2))*log(1/delta));
    float eta_one = (theta_two - theta_one) * pow(1 + sqrt(2*theta_two/(3 * theta_one)), -1);
    float eta_two = theta_two - theta_one - eta_one;
    //take capN samples from region
    float p_hat = 0;
    size_t tempcount;
    for(size_t i = 0; i < capN; ++i){
        //take sample from region, also check timeouts in this loop
        tempcount = rand() % maxcount;
        size_t basecount = 0;
        size_t* indices = new size_t[inputSize];
        for(size_t i = 0; i < bases.size(); ++i){
            while(tempcount >= bases[i]){
                tempcount = tempcount - bases[i];
                basecount += 1;
            }
            indices[i] = basecount;
            basecount = 0;
        }
        T* instance = new T[inputSize];
        for(size_t i = 0; i < inputSize; ++i){
            instance[i] = (T)indices[i] + inputmins[i];
        }
        T* outputs = this->evaluate(instance);
        //check if outputclass is the greatest of the outputs
        bool outcome = 1;
        // cout << "outputclass: " << outputclass << endl;
        for(size_t i = 0; i < outputSize; ++i){
            if(outputclass != (int)i && outputs[outputclass] <= outputs[i]){
                outcome = 0;
            }
        }
        //outcome = 1 means correctly classified, 0 means incorrect
        delete[] outputs;
        delete[] instance;
        delete[] indices;
        if(outcome == 0){
            ++p_hat;
        }
        long int currtime = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
        if(currtime - starttime >= (long int)timelimit){
            cout << "Timeout" << endl;
            return "Timeout";
        }
    }
    //simultaneously compute p_hat
    // cout << p_hat << endl;
    p_hat /= (float)capN;
    // cout << capN << " " << p_hat << endl;
    if(p_hat <= theta_one + eta_one){
        return "Yes";
    }
    else if(p_hat > theta_two - eta_two){
        return "No";
    }
    return "None";
}

template <typename T>
NnetQ<T>::~NnetQ(){
    if(VERBOSE_MODE > 0){
        cout << "z3 avoided by unsat: " << z3_avoided_unsat << endl;
        cout << "z3 avoided by always sat: " << z3_avoided_alwayssat << endl;
        cout << "z3 formula simplified: " << z3_simplified << endl;
        cout << "z3 avoided by model: " << z3_avoided_model << endl;
        cout << "getPossibleFixes discovered unsat: " << getfixes_unsat << endl;
        cout << "getPossibleFixes fixed model: " << fixedmodel_sat << endl;
        cout << "solveInParts discovered unsat: " << solveinparts_unsat << endl;
        cout << "solveInParts found model: " << solveinparts_sat << endl;
        cout << "didn't change (OK) " << didnt_change_ok << endl;
        cout << "didn't change (bad) " << didnt_change_bad << endl;
    }
    delete[] layerSizes;
    // if(!normalized){
    //     delete[] mins;
    //     delete[] maxes;
    //     delete[] means;
    //     delete[] ranges;
    // }
    for(size_t i = 0; i < numLayers; ++i){
        delete[] weights[i];
    }
    delete[] weights;
    for(size_t i = 0; i < numLayers; ++i){
        delete[] biases[i];
    }
    delete[] biases;
}

template <typename T>
ostream& NnetQ<T>::print(ostream& out) const{
    out << "numLayers: " << numLayers << "\n";
    out << "inputSize: " << inputSize << "\n";
    out << "outputSize: " << outputSize << "\n";
    out << "layerSizes: ";
    for(size_t i = 0; i < numLayers + 1; ++i){
        out << layerSizes[i] << " ";
    }
    out << "\n";
    // if(!normalized){
    //     out << "mins: ";
    //     for(size_t i = 0; i < inputSize; ++i){
    //         out << mins[i] << " ";
    //     }
    //     out << "\n";
    //     out << "maxes: ";
    //     for(size_t i = 0; i < inputSize; ++i){
    //         out << maxes[i] << " ";
    //     }
    //     out << "\n";
    //     out << "means: ";
    //     for(size_t i = 0; i < inputSize+1; ++i){
    //         out << means[i] << " ";
    //     }
    //     out << "\n";
    //     out << "ranges: ";
    //     for(size_t i = 0; i < inputSize+1; ++i){
    //         out << ranges[i] << " ";
    //     }
    //     out << "\n";
    // }
    out << "Weights: \n";
    for(size_t i = 0; i < numLayers; ++i){
        out << "Layer " << i+1 << "\n";
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            for(size_t k = 0; k < weights[i][j].size(); ++k){
                if(quant_len == 8){
                    out << (bitset<8>)weights[i][j][k] << ", ";
                }
                else if(quant_len == 16){
                    out << (bitset<16>)weights[i][j][k] << ", ";
                }
                else{ //Don't have to check for invalid option here since it could only get here with 8, 16, or 32
                    out << (bitset<32>)weights[i][j][k] << ", ";
                }
            }
            out << "\n";
        }
    }
    out << "Biases: \n";
    for(size_t i = 0; i < numLayers; ++i){
        out << "Layer " << i+1 << "\n";
        for(size_t j = 0; j < layerSizes[i+1]; ++j){
            if(quant_len == 8){
                out << (bitset<8>)biases[i][j] << ", ";
            }
            else if(quant_len == 16){
                out << (bitset<16>)biases[i][j] << ", ";
            }
            else{
                out << (bitset<32>)biases[i][j] << ", ";
            }
        }
        out << "\n";
    }
    return out;
}
