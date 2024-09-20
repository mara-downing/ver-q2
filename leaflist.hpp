#include <string>
#include <vector>
#include <unordered_map>
#include "symformula.hpp"
#include "leaf.hpp"
#include "abstract.hpp"
#include "z3++.h"
#ifndef LEAFLIST_HPP
#define LEAFLIST_HPP

using namespace std;

template <typename T>
class Leaflist{
    private:
        vector<Leaf<T>*> freeLeaves;
        vector<Leaf<T>*> detLeaves;
    public:
        Leaflist();
        Leaflist(const Leaflist<T>& in);
        Leaflist& operator=(Leaflist<T> in) noexcept;
        vector<Leaf<T>*> getFreeLeaves();
        vector<Leaf<T>*> getDetLeaves();
        void addLeaf(Leaf<T> l);
        void setFreeLeafVars(size_t index, unordered_map<string, z3::expr> newvars);
        void setAbstractFreeLeafVars(size_t index, unordered_map<string, Abstract> newvars);
        size_t getFreeLeavesSize();
        ~Leaflist();
        ostream& print(ostream& out) const;
};

template <typename T>
inline ostream & operator<<(ostream& str, const Leaflist<T>& llist){
    return llist.print(str);
}

#endif