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
#include <math.h>
#include "bigsizet.hpp"

using namespace std;

BigSizeT::BigSizeT(){
    parts.push_back(0);
}

BigSizeT::BigSizeT(size_t val){
    parts.push_back(val);
}

BigSizeT::BigSizeT(vector<size_t> val){
    parts = val;
}

BigSizeT::BigSizeT(string val){
    //this one will take more work: each size_t should be at most 9 digits long
    //storing in little endian form
    //start from end of string, put every 9 digits into a size_t and push to parts
    for(int i = val.length(); i >= 0; --i){
        if(i - 9 >= 0){
            string part = val.substr(i-9,9);
            if(part == ""){
                break;
            }
            size_t partconverted = (size_t)stoi(part);
            parts.push_back(partconverted);
            i -= 8;
        }
        else{
            string part = val.substr(0,i);
            if(part == ""){
                break;
            }
            size_t partconverted = (size_t)stoi(part);
            parts.push_back(partconverted);
            break;
        }
    }
}

BigSizeT::BigSizeT(const BigSizeT& in){
    parts = in.parts;
}

BigSizeT& BigSizeT::operator=(BigSizeT in) noexcept{
    parts = in.parts;
    return *this;
}

bool BigSizeT::operator==(const BigSizeT& rhs){
    size_t smallestlen = parts.size();
    bool diffsizes = 0;
    bool smallestone = 0;
    if(rhs.parts.size() < smallestlen){
        smallestlen = rhs.parts.size();
        diffsizes = 1;
        smallestone = 1;
    }
    else if(rhs.parts.size() > smallestlen){
        diffsizes = 1;
    }
    bool equal = 1;
    for(size_t i = 0; i < smallestlen; ++i){
        if(parts[i] != rhs.parts[i]){
            equal = 0;
            break;
        }
    }
    if(diffsizes){
        if(!smallestone){
            //this is smallest
            for(size_t i = smallestlen; i < rhs.parts.size(); ++i){
                if(rhs.parts[i] != 0){
                    equal = 0;
                }
            }
        }
        else{
            //rhs is smallest
            for(size_t i = smallestlen; i < parts.size(); ++i){
                if(parts[i] != 0){
                    equal = 0;
                }
            }
        }
    }
    return equal;
}

bool BigSizeT::operator!=(const BigSizeT& rhs){
    return !(*this == rhs);
}

bool BigSizeT::operator<(const BigSizeT& rhs){
    if(parts.size() < rhs.parts.size()){
        return 1;
    }
    else if(parts.size() == rhs.parts.size()){
        for(int i = (int)parts.size(); i >= 0; --i){
            if(parts[i] < rhs.parts[i]){
                return 1;
            }
            else if(parts[i] > rhs.parts[i]){
                return 0;
            }
        }
        return 0; //if it gets to the end of the for loop, they're equal
    }
    else{
        return 0;
    }
}

BigSizeT BigSizeT::operator+(const BigSizeT & rhs){
    size_t smallestlen = parts.size();
    bool diffsizes = 0;
    bool smallestone = 0;
    if(rhs.parts.size() < smallestlen){
        smallestlen = rhs.parts.size();
        diffsizes = 1;
        smallestone = 1;
    }
    else if(rhs.parts.size() > smallestlen){
        diffsizes = 1;
    }
    vector<size_t> newparts;
    size_t carryover = 0;
    for(size_t i = 0; i < smallestlen; ++i){
        if(parts[i] + rhs.parts[i] + carryover > 999999999){
            newparts.push_back((parts[i] + rhs.parts[i] + carryover) % 1000000000);
            carryover = (parts[i] + rhs.parts[i] + carryover) / 1000000000;
        }
        else{
            newparts.push_back(parts[i] + rhs.parts[i] + carryover);
            carryover = 0;
        }
    }
    if(diffsizes){
        if(!smallestone){
            //this is smallest
            for(size_t i = smallestlen; i < rhs.parts.size(); ++i){
                if(rhs.parts[i] + carryover > 999999999){
                    newparts.push_back((rhs.parts[i] + carryover) % 1000000000);
                    carryover = (rhs.parts[i] + carryover) / 1000000000;
                }
                else{
                    newparts.push_back(rhs.parts[i] + carryover);
                    carryover = 0;
                }
            }
        }
        else{
            //rhs is smallest
            for(size_t i = smallestlen; i < parts.size(); ++i){
                if(parts[i] + carryover > 999999999){
                    newparts.push_back((parts[i] + carryover) % 1000000000);
                    carryover = (parts[i] + carryover) / 1000000000;
                }
                else{
                    newparts.push_back(parts[i] + carryover);
                    carryover = 0;
                }
            }
        }
    }
    if(carryover != 0){
        newparts.push_back(carryover);
    }
    return BigSizeT(newparts);
}

BigSizeT BigSizeT::operator-(const BigSizeT & rhs){
    if(*this < rhs){
        cerr << "Cannot make negative BigSizeT" << endl;
        exit(1);
    }
    else{
        vector<size_t> newparts;
        size_t carryover = 0;
        for(size_t i = 0; i < rhs.parts.size(); ++i){
            if(parts[i] == 0 && carryover == 1){
                newparts.push_back(999999999 - rhs.parts[i]);
            }
            else if(parts[i] - carryover >= rhs.parts[i]){
                newparts.push_back(parts[i] - carryover - rhs.parts[i]);
                carryover = 0;
            }
            else{
                carryover = 1;
                newparts.push_back(parts[i] + 1000000000 - rhs.parts[i]);
            }
        }
        //now loop through any elements of parts longer than rhs.parts
        for(size_t i = rhs.parts.size(); i < parts.size(); ++i){
            if(parts[i] == 0 && carryover == 1){
                newparts.push_back(999999999 - rhs.parts[i]);
            }
            else{
                newparts.push_back(parts[i] - carryover);
                carryover = 0;
            }
        }
        assert(carryover == 0);
        //at end, run through newparts and remove trailing (leading?) zeroes
        for(size_t i = newparts.size()-1; i > 0; --i){ //ok to do size_t since loop ends before zero
            if(newparts[i] == 0){
                newparts.pop_back();
            }
            else{
                break;
            }
        }
        return BigSizeT(newparts);
    }
    return BigSizeT(0); //not reachable, but the compiler will be happier
}

float BigSizeT::operator/(const BigSizeT & rhs){
    if(*this < rhs){
        if(parts.size() < rhs.parts.size()){
            if(parts.size() == rhs.parts.size()-1 && rhs.parts[rhs.parts.size()-1] < 1000000){
                cout << "Make note of this if this happens" << endl;
                float temp = (float)(parts[parts.size()-1])/(1000000000*(float)(rhs.parts[rhs.parts.size()-1]) + (float)rhs.parts[rhs.parts.size()-2]);//untested
                temp = round(temp * 1000000)/1000000;
                return temp;
            }
            return 0;
        }
        else{
            float temp = (float)(parts[parts.size()-1])/(float)(rhs.parts[rhs.parts.size()-1]);
            temp = round(temp * 1000000)/1000000;
            return temp;
        }
    }
    return 0;
}

BigSizeT::~BigSizeT(){
    //nothing to do
}

ostream& BigSizeT::print(ostream& out) const{
    //out << type << " " << layer << " " << value1 << " " << value2;
    bool started = 0;
    for(int i = (int)parts.size() - 1; i >= 0; --i){
        if(parts[i] != 0){
            started = 1;
            if(parts[i] > 99999999){
                out << parts[i];
            }
            else{
                //need to print leading zeroes
                if(i != (int)parts.size() - 1){
                    size_t num_zeroes = 9 - to_string(parts[i]).length();
                    for(size_t j = 0; j < num_zeroes; ++j){
                        out << "0";
                    }
                }
                out << parts[i];
            }
        }
        else{
            if(started){
                out << "000000000";
            }
            else if(i == 0){
                out << "0";
            }
        }
    }
    return out;
}
