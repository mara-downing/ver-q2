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
#include <type_traits>
#include <bitset>
#include "symformula.hpp"

using namespace std;

template class Symformula<float>;
template class Symformula<int8_t>;
template class Symformula<int16_t>;
template class Symformula<int32_t>;

template <typename T>
Symformula<T>::Symformula(){
    isempty = true;
    type = 0;
    value = 0;
}

template <typename T>
Symformula<T>::Symformula(uint8_t ty, T val){
    assert(ty == 0);
    isempty = false;
    type = ty;
    value = val;
}

template <typename T>
Symformula<T>::Symformula(uint8_t ty, string val){
    assert(ty == 1);
    isempty = false;
    type = ty;
    symvalue = val;
}

template <typename T>
Symformula<T>::Symformula(uint8_t ty, T val, vector<Symformula> operandslist){
    assert(ty == 2);
    isempty = false;
    type = ty;
    value = val;
    operands = operandslist;
}

template <typename T>
Symformula<T>::Symformula(const Symformula<T>& in){
    isempty = in.isempty;
    type = in.type;
    value = in.value;
    symvalue = in.symvalue;
    operands = in.operands;
}

template <typename T>
Symformula<T>& Symformula<T>::operator=(Symformula<T> in) noexcept{
    isempty = in.isempty;
    type = in.type;
    value = in.value;
    symvalue = in.symvalue;
    operands = in.operands;
    return *this;
}

template <typename T>
uint8_t Symformula<T>::getType(){
    return type;
}

template <typename T>
bool Symformula<T>::isEmpty(){
    return isempty;
}

template <typename T>
T Symformula<T>::getValue(){
    return value;
}

template <typename T>
vector<Symformula<T>> Symformula<T>::getOps(){
    return operands;
}

template <typename T>
bool Symformula<T>::hasNumericOperand(){
    for(size_t i = 0; i < operands.size(); ++i){
        if(operands[i].getType() == 0){
            return true;
        }
    }
    return false;
}

template <typename T>
void Symformula<T>::setOp(size_t index, T val){
    assert(operands[index].getType() == 0);
    operands[index] = Symformula(0,val);
}

template <typename T>
bool Symformula<T>::checkSAT(int dec_bits){
    // z3::context c;
    // z3::expr conjecture = this->getZ3(c, dec_bits);
    // z3::solver s(c);
    // s.add(conjecture);
    // cout << s.check() << endl;
    // switch(s.check()){
    //     case z3::unsat:
    //         return 0;
    //         break;
    //     case z3::sat:
    //         return 1;
    //         break;
    //     case z3::unknown:
    //         return 1;
    //         break;
    // }
    return 1;
}

template <typename T>
z3::expr Symformula<T>::getZ3(z3::context& c, int dec_bits){
    if(type == 0){
        return c.bv_val((int)value, sizeof(T)*8);
    }
    else if(type == 1){
        return c.bv_const(symvalue.c_str(), sizeof(T)*8);
    }
    else{
        if(operands.size() == 1){
            assert(value == 10);
            return !(operands[0].getZ3(c, dec_bits));
        }
        else if(operands.size() == 2){
            if(value == 0){
                return operands[0].getZ3(c, dec_bits) + operands[1].getZ3(c, dec_bits);
            }
            else if(value == 1){
                return operands[0].getZ3(c, dec_bits) - operands[1].getZ3(c, dec_bits);
            }
            else if(value == 2){
                return z3::ashr(z3::sext(operands[0].getZ3(c, dec_bits),8) * z3::sext(operands[1].getZ3(c, dec_bits), 8), dec_bits).extract(7,0);
            }
            else if(value == 3){
                return operands[0].getZ3(c, dec_bits) / operands[1].getZ3(c, dec_bits);
            }
            else if(value == 4){
                return operands[0].getZ3(c, dec_bits) < operands[1].getZ3(c, dec_bits);
            }
            else if(value == 5){
                return operands[0].getZ3(c, dec_bits) > operands[1].getZ3(c, dec_bits);
            }
            else if(value == 6){
                return operands[0].getZ3(c, dec_bits) <= operands[1].getZ3(c, dec_bits);
            }
            else if(value == 7){
                return operands[0].getZ3(c, dec_bits) >= operands[1].getZ3(c, dec_bits);
            }
            else if(value == 8){
                return operands[0].getZ3(c, dec_bits) == operands[1].getZ3(c, dec_bits);
            }
            else if(value == 9){
                return operands[0].getZ3(c, dec_bits) != operands[1].getZ3(c, dec_bits);
            }
            else if(value == 11){
                return operands[0].getZ3(c, dec_bits) && operands[1].getZ3(c, dec_bits);
            }
            else if(value == 12){
                return operands[0].getZ3(c, dec_bits) || operands[1].getZ3(c, dec_bits);
            }
        }
        else{
            //make a new one with only first 2 operands, take first 2 operands off of this one. Return the whatever expression it is of the two symformulas.
            vector<Symformula<T>> newOperands;
            newOperands.push_back(operands[0]);
            newOperands.push_back(operands[1]);
            operands.erase(operands.begin(), operands.begin()+1);
            Symformula<T> newformula(2, value, newOperands);
            newOperands.clear();
            newOperands.push_back(newformula);
            newOperands.push_back(*this);
            Symformula<T> newerformula(2, value, newOperands);
            return newerformula.getZ3(c,dec_bits);
        }
    }
    return c.bv_val(0,sizeof(T)*8);
}

template <typename T>
Symformula<T>::~Symformula(){
    //nothing to do
}

template <typename T>
ostream& Symformula<T>::print(ostream& out) const{
    if(is_floating_point<T>::value){
        if(type == 0){
            out << value;
        }
        else if(type == 1){
            out << symvalue;
        }
        else{
            string ops[13] = {"+", "-", "*", "/", "<", ">", "<=", ">=", "==", "!=", "!", "&&", "||"};
            if(value == 10){
                out << "!(" << operands[0] << ")";
            }
            else if(value == 13){
                out << "extend(" << operands[0] << ")";
            }
            else if(value == 14){
                out << "round(" << operands[0] << ")";
            }
            else{
                out << "(";
                for(size_t i = 0; i < operands.size(); ++i){
                    out << operands[i];
                    if(i != operands.size()-1){
                        out << " " << ops[(int)value] << " ";
                    }
                }
                out << ")";
            }
        }
    }
    else{
        if(type == 0){
            out << (bitset<sizeof(T)*8>)value;
        }
        else if(type == 1){
            out << symvalue;
        }
        else{
            string ops[13] = {"+", "-", "*", "/", "<", ">", "<=", ">=", "==", "!=", "!", "&&", "||"};
            if(value == 10){
                out << "!(" << operands[0] << ")";
            }
            else{
                out << "(";
                for(size_t i = 0; i < operands.size(); ++i){
                    out << operands[i];
                    if(i != operands.size()-1){
                        out << " " << ops[(int)value] << " ";
                    }
                }
                out << ")";
            }
        }
    }
    return out;
}