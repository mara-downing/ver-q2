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
#include <queue>
#include <string.h>
// #include "symbextree.hpp"
// #include "bigsizet.hpp"
#include <assert.h>
#include <bitset>
#include <math.h>
#include <typeinfo>
#include <algorithm>
#include <unordered_map>
// #include <stdint.h>
// #include <sys/types.h>
// #include <cstdint>
// #include <cstdio>
// #include <cinttypes>
#include "z3++.h"

using namespace std;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::int8_t;

static size_t inputSize = 4;
static size_t outputSize = 3;
static z3::context c;

int8_t floatToFixedPoint(float f, int qlen, int dec){
    if(f*pow(2, dec) <= -1*pow(2, qlen-1)){
        return (int8_t)(-1*pow(2, qlen-1));
    }
    else if(f*pow(2, dec) >= pow(2, qlen-1)-1){
        return (int8_t)pow(2, qlen-1)-1;
    }
    else{
        return (int8_t)(round(f * (1 << dec)));
    }
}

tuple<vector<int8_t>, vector<int8_t>, z3::expr, z3::expr> parseConstraints(vector<string> knownboundslines, vector<string> lines){
    //fill mins and maxes vectors:
    vector<int8_t> inputmins;
    vector<int8_t> inputmaxes;
    z3::expr inputconstraints = c.int_val(0) < c.int_val(1);
    z3::expr outputconstraints = c.int_val(0) < c.int_val(1);
    for(size_t i = 0; i < inputSize; ++i){
        inputmins.push_back(-1*(int8_t)pow(2,8-1));
        inputmaxes.push_back((int8_t)pow(2,8-1)-1);
    }
    vector<bool> inputminschecker;
    vector<bool> inputmaxeschecker;
    for(size_t i = 0; i < inputSize; ++i){
        inputminschecker.push_back(0);
        inputmaxeschecker.push_back(0);
    }
    //parse knownbounds lines: can only have <, <=, >, >=, and can only compare var to constant
    for(size_t i = 0; i < knownboundslines.size(); ++i){
        vector<string> lexed;
        for(size_t j = 0; j < knownboundslines[i].length(); ++j){
            if(knownboundslines[i][j] == 'x' || knownboundslines[i][j] == 'y'){
                //found a variable, collect until non-numeric character found
                string var = "V:";
                var += string(1, knownboundslines[i][j]);
                ++j;
                while(j < knownboundslines[i].length()){
                    if(isdigit(knownboundslines[i][j])){
                        var += string(1, knownboundslines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(var);
            }
            else if(knownboundslines[i][j] == '<' || knownboundslines[i][j] == '>'){
                //check for equals next, optional
                string comp = "C:";
                comp += string(1,knownboundslines[i][j]);
                if(j != knownboundslines[i].length()-1 && knownboundslines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                lexed.push_back(comp);
            }
            else if(knownboundslines[i][j] == '=' || knownboundslines[i][j] == '!'){
                cerr << "Not a valid comparison on line " << i << " of known bounds file" << endl;
                exit(1);
            }
            else if(isdigit(knownboundslines[i][j]) || knownboundslines[i][j] == '.' || knownboundslines[i][j] == '-'){
                //is a number, collect until end of number
                string val = "N:";
                val += string(1, knownboundslines[i][j]);
                ++j;
                while(j < knownboundslines[i].length()){
                    if(isdigit(knownboundslines[i][j]) || knownboundslines[i][j] == '.'){
                        val += string(1, knownboundslines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(val);
            }
            else{
                assert(knownboundslines[i][j] == ' ');
            }
        }
        if(lexed.size() != 0){
            assert(lexed.size() == 3); // make sure there are three elements after lexing (should be a var, a comparison, and a val)
            assert(lexed[1][0] == 'C'); // make sure the middle element is a comparison
            assert(lexed[0][0] == 'V' || lexed[0][0] == 'N'); // make sure the first element is a var or a val
            assert(lexed[2][0] == 'V' || lexed[2][0] == 'N'); // make sure the first element is a var or a val
            if(lexed[0][0] == 'V'){
                if(lexed[2][0] == 'V'){
                    cerr << "Variable comparison not allowed in known bounds" << endl;
                    exit(1);
                }
                else{
                    //for now just determine input/output and put into correct expr
                    if(lexed[0][2] == 'x'){
                        //input expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)inputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                        if(lexed[1] == "C:<"){
                            if(quantizednumber != -1*(int8_t)pow(2,8-1)){
                                inputmaxes[intnum] = quantizednumber-1;
                                inputmaxeschecker[intnum] = 1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:<="){
                            inputmaxes[intnum] = quantizednumber;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else if(lexed[1] == "C:>"){
                            if(quantizednumber != (int8_t)pow(2,8-1)-1){
                                inputmins[intnum] = quantizednumber+1;
                                inputminschecker[intnum] = 1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:>="){
                            inputmins[intnum] = quantizednumber;
                            inputminschecker[intnum] = 1;
                        }
                        else if(lexed[1] == "C:=="){
                            // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                            inputmins[intnum] = quantizednumber;
                            inputminschecker[intnum] = 1;
                            inputmaxes[intnum] = quantizednumber;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                    else{
                        assert(lexed[0][2] == 'y');
                        //output expr
                        cerr << "Known bounds cannot include output variables" << endl;
                        exit(1);
                    }
                }
            }
            else if(lexed[2][0] == 'V'){
                //for now just determine input/output and put into correct expr
                if(lexed[2][2] == 'x'){
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)inputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                    if(lexed[1] == "C:<"){
                        if(quantizednumber != (int8_t)pow(2,8-1)-1){
                            inputmins[intnum] = quantizednumber+1;
                            inputminschecker[intnum] = 1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:<="){
                        inputmins[intnum] = quantizednumber;
                        inputminschecker[intnum] = 1;
                    }
                    else if(lexed[1] == "C:>"){
                        if(quantizednumber != -1*(int8_t)pow(2,8-1)){
                            inputmaxes[intnum] = quantizednumber-1;
                            inputmaxeschecker[intnum] = 1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:>="){
                        inputmaxes[intnum] = quantizednumber;
                        inputmaxeschecker[intnum] = 1;
                    }
                    else if(lexed[1] == "C:=="){
                        // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        inputmins[intnum] = quantizednumber;
                        inputminschecker[intnum] = 1;
                        inputmaxes[intnum] = quantizednumber;
                        inputmaxeschecker[intnum] = 1;
                    }
                    else{
                        inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                    }
                }
                else{
                    assert(lexed[2][2] == 'y');
                    //output expr
                    cerr << "Known bounds cannot include output variables" << endl;
                    exit(1);
                }
            }
            else{
                cerr << "Cannot compare constants on line " << i << " of known bounds file" << endl;
                exit(1);
            }
        }
    }
    //parse each line, put into correct spot
    for(size_t i = 0; i < lines.size(); ++i){
        vector<string> lexed;
        for(size_t j = 0; j < lines[i].length(); ++j){
            if(lines[i][j] == 'x' || lines[i][j] == 'y'){
                //found a variable, collect until non-numeric character found
                string var = "V:";
                var += string(1, lines[i][j]);
                ++j;
                while(j < lines[i].length()){
                    if(isdigit(lines[i][j])){
                        var += string(1, lines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(var);
            }
            else if(lines[i][j] == '<' || lines[i][j] == '>'){
                //check for equals next, optional
                string comp = "C:";
                comp += string(1,lines[i][j]);
                if(j != lines[i].length()-1 && lines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                lexed.push_back(comp);
            }
            else if(lines[i][j] == '=' || lines[i][j] == '!'){
                //check for equals next, required
                string comp = "C:";
                comp += string(1,lines[i][j]);
                if(j != lines[i].length()-1 && lines[i][j+1] == '='){
                    comp += string(1,'=');
                    ++j;
                }
                else{
                    cerr << "Not a valid comparison on line " << i << " of constraints file" << endl;
                    exit(1);
                }
                lexed.push_back(comp);
            }
            else if(isdigit(lines[i][j]) || lines[i][j] == '.' || lines[i][j] == '-'){
                //is a number, collect until end of number
                string val = "N:";
                val += string(1, lines[i][j]);
                ++j;
                while(j < lines[i].length()){
                    if(isdigit(lines[i][j]) || lines[i][j] == '.'){
                        val += string(1, lines[i][j]);
                        ++j;
                    }
                    else{
                        --j;
                        break;
                    }
                }
                lexed.push_back(val);
            }
            else{
                assert(lines[i][j] == ' ');
            }
        }
        if(lexed.size() != 0){
            assert(lexed.size() == 3); // make sure there are three elements after lexing (should be a var, a comparison, and a val (or another var))
            assert(lexed[1][0] == 'C'); // make sure the middle element is a comparison
            assert(lexed[0][0] == 'V' || lexed[0][0] == 'N'); // make sure the first element is a var or a val
            assert(lexed[2][0] == 'V' || lexed[2][0] == 'N'); // make sure the first element is a var or a val
            if(lexed[0][0] == 'V'){
                if(lexed[2][0] == 'V'){
                    if (lexed[0][2] == 'x' && lexed[2][2] == 'x') {
                        // check first x
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)inputSize);
                        num1 = lexed[0].substr(2);
                        if(num1[0] == 'x'){
                            num1 = "sym_" + num1;
                        }
                        // check second x
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)inputSize);
                        num2 = lexed[2].substr(2);
                        if(num2[0] == 'x'){
                            num2 = "sym_" + num2;
                        }

                        if(lexed[1] == "C:<"){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            inputconstraints = inputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }
                    } else if (lexed[0][2] == 'x' && lexed[2][2] == 'y') {
                        // check x
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)inputSize);
                        num1 = lexed[0].substr(2);
                        if(num1[0] == 'x'){
                            num1 = "sym_" + num1;
                        }
                        // check y
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)outputSize);
                        num2 = lexed[2].substr(2);

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }

                    } else if (lexed[0][2] == 'y' && lexed[2][2] == 'x') {
                        // check y
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)outputSize);
                        num1 = lexed[0].substr(2);

                        // check x
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)inputSize);
                        num2 = lexed[2].substr(2);
                        if(num2[0] == 'x'){
                            num2 = "sym_" + num1;
                        }

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }

                    }
                    else if (lexed[0][2] == 'y' && lexed[2][2] == 'y') {
                        // check first y
                        string num1 = lexed[0].substr(3);
                        int intnum1 = stoi(num1);
                        assert(intnum1 < (int)outputSize);
                        num1 = lexed[0].substr(2);

                        // check second y
                        string num2 = lexed[2].substr(3);
                        int intnum2 = stoi(num2);
                        assert(intnum2 < (int)outputSize);
                        num2 = lexed[2].substr(2);

                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) < c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) <= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) > c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) >= c.int_const(num2.c_str());
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num1.c_str()) == c.int_const(num2.c_str());
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num1.c_str()) == c.int_const(num2.c_str()));
                        }

                    }
                }
                else{
                    //for now just determine input/output and put into correct expr
                    if(lexed[0][2] == 'x'){
                        //input expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)inputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                        if(lexed[1] == "C:<"){
                            if(quantizednumber != -1*(int8_t)pow(2,8-1)){
                                inputmaxes[intnum] = quantizednumber-1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:<="){
                            inputmaxes[intnum] = quantizednumber;
                        }
                        else if(lexed[1] == "C:>"){
                            if(quantizednumber != (int8_t)pow(2,8-1)-1){
                                inputmins[intnum] = quantizednumber+1;
                            }
                            else{
                                cout << "Model Count: O" << endl;
                                exit(1);
                            }
                        }
                        else if(lexed[1] == "C:>="){
                            inputmins[intnum] = quantizednumber;
                        }
                        else if(lexed[1] == "C:=="){
                            // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                            inputmins[intnum] = quantizednumber;
                            inputmaxes[intnum] = quantizednumber;
                        }
                        else{
                            inputconstraints = inputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                    else{
                        assert(lexed[0][2] == 'y');
                        //output expr
                        string num = lexed[0].substr(3);
                        int intnum = stoi(num);
                        assert(intnum < (int)outputSize);
                        num = lexed[0].substr(2);
                        if(num[0] == 'x'){
                            num = "sym_" + num;
                        }
                        float number = stof(lexed[2].substr(2));
                        int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                        if(lexed[1] == "C:<"){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) < c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:<="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) <= c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:>"){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) > c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:>="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) >= c.int_val(quantizednumber);
                        }
                        else if(lexed[1] == "C:=="){
                            outputconstraints = outputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        }
                        else{
                            outputconstraints = outputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                        }
                    }
                }
            }
            else if(lexed[2][0] == 'V'){
                //for now just determine input/output and put into correct expr
                if(lexed[2][2] == 'x'){
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)inputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                    if(lexed[1] == "C:<"){
                        if(quantizednumber != (int8_t)pow(2,8-1)-1){
                            inputmins[intnum] = quantizednumber+1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:<="){
                        inputmins[intnum] = quantizednumber;
                    }
                    else if(lexed[1] == "C:>"){
                        if(quantizednumber != -1*(int8_t)pow(2,8-1)){
                            inputmaxes[intnum] = quantizednumber-1;
                        }
                        else{
                            cout << "Model Count: O" << endl;
                            exit(1);
                        }
                    }
                    else if(lexed[1] == "C:>="){
                        inputmaxes[intnum] = quantizednumber;
                    }
                    else if(lexed[1] == "C:=="){
                        // inputconstraints = inputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                        inputmins[intnum] = quantizednumber;
                        inputmaxes[intnum] = quantizednumber;
                    }
                    else{
                        inputconstraints = inputconstraints && c.int_const(num.c_str()) != c.int_val(quantizednumber);
                    }
                }
                else{
                    assert(lexed[2][2] == 'y');
                    //output expr
                    string num = lexed[2].substr(3);
                    int intnum = stoi(num);
                    assert(intnum < (int)outputSize);
                    num = lexed[2].substr(2);
                    if(num[0] == 'x'){
                        num = "sym_" + num;
                    }
                    float number = stof(lexed[0].substr(2));
                    int8_t quantizednumber = floatToFixedPoint(number, 8, 4);
                    if(lexed[1] == "C:<"){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) > c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:<="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) >= c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:>"){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) < c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:>="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) <= c.int_val(quantizednumber);
                    }
                    else if(lexed[1] == "C:=="){
                        outputconstraints = outputconstraints && c.int_const(num.c_str()) == c.int_val(quantizednumber);
                    }
                    else{
                        outputconstraints = outputconstraints && !(c.int_const(num.c_str()) == c.int_val(quantizednumber));
                    }
                }
            }
            else{
                cerr << "Later we will implement number to number comparison, not super useful though" << endl;
                exit(1);
            }
        }
    }
    //first int vector is mins, second is maxes (set to int_min and int_max if not specified, ordered in same order as x0, x1, . . .)
    //these two are only for inputs, not caring about outputs
    //first z3::expr is input constraints with != or ==
    //second z3::expr is output constraints (all)
    for(size_t i = 0; i < inputSize; ++i){
        if(!inputminschecker[i]){
            cout << "Warning: input var x" << i << " does not have a minimum set, using smallest fixed point as min" << endl;
        }
        if(!inputmaxeschecker[i]){
            cout << "Warning: input var x" << i << " does not have a maximum set, using largest fixed point as max" << endl;
        }
    }
    return tuple<vector<int8_t>, vector<int8_t>, z3::expr, z3::expr>(inputmins, inputmaxes, inputconstraints, outputconstraints);
}

int main(int argc, char *argv[]){
    string knownboundsfilename = argv[1];
    string constraintsfilename = argv[2];
    ifstream kbt(knownboundsfilename);
    ostringstream kbcontents;
    kbcontents << kbt.rdbuf();
    string knownboundsstring = kbcontents.str();
    string knownbound = "";
    vector<string> knownboundslines;
    size_t start = 0;
    for(size_t i = 0; i < knownboundsstring.length(); ++i){
        if(knownboundsstring[i] == '\n'){
            knownboundslines.push_back(knownboundsstring.substr(start, i-start));
            start = i + 1;
        }
    }
    ifstream t(constraintsfilename);
    ostringstream contents;
    contents << t.rdbuf();
    string constraintstring = contents.str();
    string constraint = "";
    vector<string> lines;
    start = 0;
    for(size_t i = 0; i < constraintstring.length(); ++i){
        if(constraintstring[i] == '\n'){
            lines.push_back(constraintstring.substr(start, i-start));
            start = i + 1;
        }
    }
    tuple<vector<int8_t>, vector<int8_t>, z3::expr, z3::expr> constraintinfo = parseConstraints(knownboundslines, lines);
    vector<int8_t> inputmins = get<0>(constraintinfo);
    vector<int8_t> inputmaxes = get<1>(constraintinfo);
    z3::expr inputconstraints = get<2>(constraintinfo);
    z3::expr outputconstraints = get<3>(constraintinfo);
    unordered_map<string, z3::expr> vars;
    vector<z3::expr> emptyvec;
    for(size_t i = 0; i < inputSize; ++i){
        vars.insert(pair<const string, z3::expr>("x"+to_string(i), c.int_const(("sym_x"+to_string(i)).c_str())));
    }
    z3::expr between_bounds = c.int_const(("sym_x"+to_string(0)).c_str()) >= c.int_val(inputmins[0]) && c.int_const(("sym_x"+to_string(0)).c_str()) <= c.int_val(inputmaxes[0]);
    for(size_t i = 1; i < inputSize; ++i){
        between_bounds = between_bounds && c.int_const(("sym_x"+to_string(i)).c_str()) >= c.int_val(inputmins[i]) && c.int_const(("sym_x"+to_string(i)).c_str()) <= c.int_val(inputmaxes[i]);
    }
    // cout << between_bounds << endl;
    between_bounds = between_bounds && inputconstraints; //they should be pretty simple anyway, just throwing them in with the bounds
    //get model count of input constraints (including bounds)
    ofstream abcfile;
    abcfile.open("formula.smt2");
    abcfile << "(set-logic QF_LIA)" << endl;
    string countstring = "--count-variable ";
    for(size_t i = 0; i < inputSize; ++i){
        abcfile << "(declare-fun sym_x" << i << " () Int)" << endl;
        countstring += "sym_x"+to_string(i);
        if(i != inputSize-1){
            countstring += ",";
        }
    }
    abcfile << "(assert " << between_bounds << ")" << endl;
    abcfile << "(check-sat)" << endl;
    abcfile.close();
    string countbound = "-bi ";
    if(8 == 8){ 
        countbound += "8";
    }
    else if(8 == 16){
        countbound += "16";
    }
    else{
        countbound += "32";
    }
    const string command = "abc -i formula.smt2 --count-tuple " + countbound + " " + countstring + " -v 0 >abcoutput.txt 2>abcoutput.txt";
    // cout << command << endl;
    auto sec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    system(command.c_str());
    ifstream abcresfile("abcoutput.txt");
    ostringstream abcrescontents;
    abcrescontents << abcresfile.rdbuf();
    string abcres = abcrescontents.str();
    size_t found = abcres.find("Aborted");
    if(found != string::npos){
        cerr << "Model count not able to be completed by ABC" << endl;
        exit(1);
    }
    size_t tuple_line = 0;
    for(size_t i = 0; i < abcres.length(); ++i){
        if(abcres[i] == 'T' && i < abcres.length() - 5){
            if(abcres[i+1] == 'U' && abcres[i+2] == 'P' && abcres[i+3] == 'L' && abcres[i+4] == 'E'){
                tuple_line = i+5;
                break;
            }
        }
    }
    string abcmodelcount = "";
    for(size_t i = tuple_line; i < abcres.length(); ++i){
        if(abcres[i] == 'c' && i < abcres.length() - 5){
            if(abcres[i+1] == 'o' && abcres[i+2] == 'u' && abcres[i+3] == 'n' && abcres[i+4] == 't'){
                for(size_t j = i + 7; j < abcres.length(); ++j){
                    if(isdigit(abcres[j])){
                        abcmodelcount += string(1,abcres[j]);
                    }
                    else{
                        break;
                    }
                }
                break;
            }
        }
    }
    auto sec_since_epoch_end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << sec_since_epoch_end - sec_since_epoch << endl;
    auto abctime = sec_since_epoch_end - sec_since_epoch;
    cout << "Model count of all input constraints: " << abcmodelcount << endl;

    sec_since_epoch = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    bool findingsolutions = 1;
    int z3mc = 0;
    z3::expr temptotalconstraint = between_bounds;
    z3::solver s(c);
    while(findingsolutions){
        s.reset();
        s.add(temptotalconstraint);
        switch(s.check()){
            case z3::unsat:
                findingsolutions = 0;
                break;
            case z3::sat:
                z3mc++;
                {
                    z3::model model = s.get_model();
                    unordered_map<string, int> modelVals;
                    int num_consts = model.num_consts();
                    for(int i = 0; i < num_consts; ++i){
                        string sym = model.get_const_decl(i).name().str();
                        if(sym[0] == 's'){
                            int sol = (int)(model.get_const_interp(model.get_const_decl(i))).get_numeral_int64();
                            modelVals[sym] = sol;
                        }
                        else if(sym[0] == 'c'){
                            //not including created constants in model
                        }
                        else if(sym[0] == 'y'){
                            //also not including output vars in model
                        }
                        else{
                            cout << "shouldn't happen? " << sym << endl;
                        }
                    }
                    z3::expr temp = c.int_const("sym_x0") == c.int_val(modelVals["sym_x0"]);
                    for(size_t i = 1; i < inputSize; ++i){
                        temp = temp && c.int_const(("sym_x" + to_string(i)).c_str()) == c.int_val(modelVals["sym_x"+to_string(i)]);
                    }
                    temptotalconstraint = temptotalconstraint && !(temp);
                }
                break;
            case z3::unknown:
                cerr << "unknown, cannot generate model to negate" << endl;
                findingsolutions = 0;
                break;
        }
    }
    sec_since_epoch_end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    cout << sec_since_epoch_end - sec_since_epoch << endl;
    auto z3time = sec_since_epoch_end - sec_since_epoch;
    cout << "Z3 model count: " << z3mc << endl;
    ofstream outfile;
    outfile.open("quickmccomp_results.txt", std::ios_base::app);
    outfile << constraintsfilename << "," << abctime << "," << z3time << endl;
    outfile.close();
}

