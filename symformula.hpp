#include <string>
#include <vector>
#include <unordered_map>
#include "z3++.h"
#ifndef SYMFORMULA_HPP
#define SYMFORMULA_HPP

using namespace std;

template <typename T>
class Symformula{
    private:
        bool isempty;
        uint8_t type; //0 for number, 1 for symval, 2 for node symbol with leaves
        T value; //0 for +, 1 for -, 2 for *, 3 for /, 4 for <, 5 for >, 6 for <=, 7 for >=, 8 for ==, 9 for !=, 10 for !, 11 for &&, 12 for ||, 13 for extend, 14 for round
        string symvalue;
        vector<Symformula<T>> operands;
    public:
        Symformula();
        Symformula(uint8_t ty, T val);
        Symformula(uint8_t ty, string val);
        Symformula(uint8_t ty, T val, vector<Symformula<T>> operandslist);
        Symformula(const Symformula<T>& in);
        Symformula<T>& operator=(Symformula<T> in) noexcept;
        uint8_t getType();
        bool isEmpty();
        T getValue();
        vector<Symformula<T>> getOps();
        bool hasNumericOperand();
        void setOp(size_t index, T val);
        bool checkSAT(int dec_bits);
        z3::expr getZ3(z3::context& c, int dec_bits);
        ~Symformula();
        ostream& print(ostream& out) const;
};

template <typename T>
inline ostream & operator<<(ostream& str, const Symformula<T>& sf){
    return sf.print(str);
}

#endif