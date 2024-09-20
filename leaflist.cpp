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
#include "leaflist.hpp"

using namespace std;

template class Leaflist<float>;
template class Leaflist<int8_t>;
template class Leaflist<int16_t>;
template class Leaflist<int32_t>;

template <typename T>
Leaflist<T>::Leaflist(){
    //nothing to do, but exists now
}

template <typename T>
Leaflist<T>::Leaflist(const Leaflist<T>& in){
    for(size_t i = 0; i < freeLeaves.size(); ++i){
        delete freeLeaves[i];
    }
    for(size_t i = 0; i < detLeaves.size(); ++i){
        delete detLeaves[i];
    }
    freeLeaves.clear();
    detLeaves.clear();
    for(size_t i = 0; i < in.freeLeaves.size(); ++i){
        freeLeaves.push_back(new Leaf<T>(*(in.freeLeaves[i])));
    }
    for(size_t i = 0; i < in.detLeaves.size(); ++i){
        detLeaves.push_back(new Leaf<T>(*(in.detLeaves[i])));
    }
}

template <typename T>
Leaflist<T>& Leaflist<T>::operator=(Leaflist<T> in) noexcept{
    for(size_t i = 0; i < freeLeaves.size(); ++i){
        delete freeLeaves[i];
    }
    for(size_t i = 0; i < detLeaves.size(); ++i){
        delete detLeaves[i];
    }
    freeLeaves.clear();
    detLeaves.clear();
    for(size_t i = 0; i < in.freeLeaves.size(); ++i){
        freeLeaves.push_back(new Leaf<T>(*(in.freeLeaves[i])));
    }
    for(size_t i = 0; i < in.detLeaves.size(); ++i){
        detLeaves.push_back(new Leaf<T>(*(in.detLeaves[i])));
    }
    return *this;
}

template <typename T>
vector<Leaf<T>*> Leaflist<T>::getFreeLeaves(){
    return freeLeaves;
}

template <typename T>
vector<Leaf<T>*> Leaflist<T>::getDetLeaves(){
    return detLeaves;
}

template <typename T>
void Leaflist<T>::addLeaf(Leaf<T> l){
    if(l.isDet()){
        Leaf<T>* leaf = new Leaf<T>(l);
        detLeaves.push_back(leaf);
    }
    else{
        Leaf<T>* leaf = new Leaf<T>(l);
        freeLeaves.push_back(leaf);
    }
}

template <typename T>
void Leaflist<T>::setFreeLeafVars(size_t index, unordered_map<string, z3::expr> newvars){
    freeLeaves[index]->setVars(newvars);
}

template <typename T>
void Leaflist<T>::setAbstractFreeLeafVars(size_t index, unordered_map<string, Abstract> newvars){
    freeLeaves[index]->setAbstractVars(newvars);
}

template <typename T>
size_t Leaflist<T>::getFreeLeavesSize(){
    return freeLeaves.size();
}

template <typename T>
Leaflist<T>::~Leaflist(){
    for(size_t i = 0; i < freeLeaves.size(); ++i){
        delete freeLeaves[i];
    }
    for(size_t i = 0; i < detLeaves.size(); ++i){
        delete detLeaves[i];
    }
}

template <typename T>
ostream& Leaflist<T>::print(ostream& out) const{
    for(size_t i = 0; i < detLeaves.size(); ++i){
        out << *detLeaves[i] << "\n\n";
    }
    for(size_t i = 0; i < freeLeaves.size(); ++i){
        out << *freeLeaves[i] << "\n\n";
    }
    return out;
}