#include <string>
#include <vector>
#include <unordered_map>
#ifndef BIGSIZET_HPP
#define BIGSIZET_HPP

using namespace std;

class BigSizeT{
    private:
        vector<size_t> parts;
    public:
        BigSizeT();
        BigSizeT(size_t val);
        BigSizeT(vector<size_t> val);
        BigSizeT(string val);
        BigSizeT(const BigSizeT& in);
        BigSizeT& operator=(BigSizeT in) noexcept;
        bool operator==(const BigSizeT& rhs);
        bool operator!=(const BigSizeT& rhs);
        bool operator<(const BigSizeT& rhs);
        BigSizeT operator+(const BigSizeT & rhs);
        BigSizeT operator-(const BigSizeT & rhs);
        float operator/(const BigSizeT & rhs);
        ~BigSizeT();
        ostream& print(ostream& out) const;
};

inline ostream & operator<<(ostream& str, const BigSizeT& l){
    return l.print(str);
}

#endif