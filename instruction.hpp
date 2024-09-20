#include <string>
#include <vector>
#include <unordered_map>
#ifndef INSTRUCTION_HPP
#define INSTRUCTION_HPP

using namespace std;

class Instruction{
    private:
        size_t type; //0 for dot product/bias step, 1 for relu condition, add more here once I need them
        size_t layer; // which index are we at in the weights/biases array
        size_t value1; //either stores number of inputs to dot product or index of relu instruction
        size_t value2; //stores the number of outputs from dot product (number of new_in values to be created)
    public:
        Instruction();
        Instruction(uint8_t ty, size_t l, size_t val1, size_t val2);
        Instruction(const Instruction& in);
        Instruction& operator=(Instruction in) noexcept;
        size_t getType();
        size_t getLayer();
        size_t getValue1();
        size_t getValue2();
        ~Instruction();
        ostream& print(ostream& out) const;
};

inline ostream & operator<<(ostream& str, const Instruction& l){
    return l.print(str);
}

#endif