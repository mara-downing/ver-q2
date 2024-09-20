#include <string>
#include <vector>
#include <unordered_map>
// #include "z3++.h"
#ifndef ABSTRACT_HPP
#define ABSTRACT_HPP

using namespace std;

class Abstract{
    private:
        bool isSat;
        int satLevel; // 0 for always, 1 for sometimes, 2 for never (to help find good clauses for z3, if necessary)
        bool isNumeral;
        int eqClass; // 0 is empty set, 1 is -, 2 is 0, 3 is +, 4 is -0, 5 is 0+, 6 is -+, 7 is all
    public:
        Abstract();
        Abstract(int equivalenceClass);
        Abstract(int op, int left, int right);
        Abstract(int op, Abstract left, Abstract right); //0 for +, 1 for -, 2 for *, 3 for /, 4 for <, 5 for >, 6 for <=, 7 for >=, 8 for ==, 9 for !=
        Abstract(const Abstract& in);
        Abstract& operator=(Abstract in) noexcept;
        bool getIsSat();
        int getSatLevel();
        bool getIsNumeral();
        int getEqClass();
        ~Abstract();
        ostream& print(ostream& out) const;
};

inline ostream & operator<<(ostream& str, const Abstract& a){
    return a.print(str);
}

#endif