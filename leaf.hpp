#include <string>
#include <vector>
#include <unordered_map>
#include "symformula.hpp"
#include "abstract.hpp"
#include "z3++.h"
#ifndef LEAF_HPP
#define LEAF_HPP

using namespace std;

template <typename T>
class Leaf{
    private:
        unordered_map<string, z3::expr> vars;
        unordered_map<string, Abstract> abstractvars;
        vector<z3::expr> pc;
        z3::expr bounds;
        bool det;
        bool hasModel;
        // unordered_map<string, T> model;
        z3::model* model;
        size_t clauseCount;
    public:
        Leaf() = delete;
        Leaf(unordered_map<string, z3::expr> inputvars, unordered_map<string, Abstract> abstractinputvars, vector<z3::expr> pathconstraints, z3::expr varbounds, size_t clausecount);
        Leaf(unordered_map<string, z3::expr> inputvars, unordered_map<string, Abstract> abstractinputvars, vector<z3::expr> pathconstraints, z3::expr varbounds, size_t clausecount, z3::model inputmodel);
        Leaf(const Leaf<T>& in);
        Leaf<T>& operator=(Leaf<T> in) noexcept;
        vector<z3::expr> getPC();
        z3::expr getBounds();
        unordered_map<string, z3::expr> getVars();
        void setVars(unordered_map<string, z3::expr> newvars);
        unordered_map<string, Abstract> getAbstractVars();
        void setAbstractVars(unordered_map<string, Abstract> newvars);
        bool isDet();
        bool getHasModel();
        z3::model getModel();
        size_t getClauseCount();
        ~Leaf();
        ostream& print(ostream& out) const;
};

template <typename T>
inline ostream & operator<<(ostream& str, const Leaf<T>& l){
    return l.print(str);
}

#endif