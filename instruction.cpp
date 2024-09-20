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
#include "instruction.hpp"

using namespace std;

template <typename T>
string to_string(const T& object){
    ostringstream ss;
    ss << object;
    return ss.str();
}

Instruction::Instruction(){
    assert(false); //reminder to self, don't make one of these
}

Instruction::Instruction(uint8_t ty, size_t l, size_t val1, size_t val2){
    type = ty;
    layer = l;
    value1 = val1;
    value2 = val2;
}

Instruction::Instruction(const Instruction& in){
    type = in.type;
    layer = in.layer;
    value1 = in.value1;
    value2 = in.value2;
}

Instruction& Instruction::operator=(Instruction in) noexcept{
    type = in.type;
    layer = in.layer;
    value1 = in.value1;
    value2 = in.value2;
    return *this;
}

size_t Instruction::getType(){
    return type;
}

size_t Instruction::getLayer(){
    return layer;
}

size_t Instruction::getValue1(){
    return value1;
}

size_t Instruction::getValue2(){
    return value2;
}

Instruction::~Instruction(){
    //nothing to do
}

ostream& Instruction::print(ostream& out) const{
    out << type << " " << layer << " " << value1 << " " << value2;
    return out;
}