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
#include "leaf.hpp"

using namespace std;

template class Leaf<float>;
template class Leaf<int8_t>;
template class Leaf<int16_t>;
template class Leaf<int32_t>;

template <typename Ty>
string to_string(const Ty& object){
    ostringstream ss;
    ss << object;
    return ss.str();
}

template <typename T>
Leaf<T>::Leaf(unordered_map<string, z3::expr> inputvars, unordered_map<string, Abstract> abstractinputvars, vector<z3::expr> pathconstraints, z3::expr varbounds, size_t clausecount) : pc(pathconstraints), bounds(varbounds){
    vars = inputvars;
    abstractvars = abstractinputvars;
    // pc = pathconstraints;
    det = false;
    hasModel = false;
    clauseCount = clausecount;
}

template <typename T>
Leaf<T>::Leaf(unordered_map<string, z3::expr> inputvars, unordered_map<string, Abstract> abstractinputvars, vector<z3::expr> pathconstraints, z3::expr varbounds, size_t clausecount, z3::model inputmodel) : pc(pathconstraints), bounds(varbounds){
    vars = inputvars;
    abstractvars = abstractinputvars;
    // pc = pathconstraints;
    det = false;
    hasModel = true;
    model = new z3::model(inputmodel);
    // model = inputmodel;
    clauseCount = clausecount;
}

template <typename T>
Leaf<T>::Leaf(const Leaf& in) : pc(in.pc), bounds(in.bounds){
    vars = in.vars;
    abstractvars = in.abstractvars;
    // pc = in.pc;
    det = in.det;
    hasModel = in.hasModel;
    if(hasModel){
        model = new z3::model(*in.model);
    }
    clauseCount = in.clauseCount;
}

template <typename T>
Leaf<T>& Leaf<T>::operator=(Leaf<T> in) noexcept{
    vars = in.vars;
    abstractvars = in.abstractvars;
    pc = in.pc;
    bounds = in.bounds;
    det = in.det;
    hasModel = in.hasModel;
    if(hasModel){
        model = new z3::model(*in.model);
    }
    clauseCount = in.clauseCount;
    return *this;
}

template <typename T>
vector<z3::expr> Leaf<T>::getPC(){
    return pc;
}

template <typename T>
z3::expr Leaf<T>::getBounds(){
    return bounds;
}

template <typename T>
unordered_map<string, z3::expr> Leaf<T>::getVars(){
    return vars;
}

template <typename T>
void Leaf<T>::setVars(unordered_map<string, z3::expr> newvars){
    vars = newvars;
}

template <typename T>
unordered_map<string, Abstract> Leaf<T>::getAbstractVars(){
    return abstractvars;
}

template <typename T>
void Leaf<T>::setAbstractVars(unordered_map<string, Abstract> newvars){
    abstractvars = newvars;
}

template <typename T>
bool Leaf<T>::isDet(){
    return det;
}

template <typename T>
bool Leaf<T>::getHasModel(){
    return hasModel;
}

template <typename T>
z3::model Leaf<T>::getModel(){
    if(hasModel){
        return *model;
    }
    else{
        cerr << "cannot get non-existent model" << endl;
        exit(1);
    }
}

template <typename T>
size_t Leaf<T>::getClauseCount(){
    return clauseCount;
}

template <typename T>
Leaf<T>::~Leaf(){
    if(hasModel){
        delete model;
    }
}

template <typename T>
ostream& Leaf<T>::print(ostream& out) const{
    // out << pc << "\n";
    for(size_t i = 0; i < pc.size(); ++i){
        out << pc[i] << "\n";
    }
    for (auto const &pair: vars) {
        out << pair.first << ": " << to_string(pair.second) << ", ";
    }
    out << "\n";
    for (auto const &pair: abstractvars) {
        out << pair.first << ": " << to_string(pair.second) << ", ";
    }
    out << "\n";
    if(det){
        out << "Deterministic";
    }
    return out;
}