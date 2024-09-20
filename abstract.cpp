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
#include "abstract.hpp"

using namespace std;


Abstract::Abstract(){
    isSat = 1;
    satLevel = 1;
    isNumeral = 0;
    eqClass = 0;
}

Abstract::Abstract(int equivalenceClass){
    isSat = 1;
    satLevel = 1;
    isNumeral = 1;
    eqClass = equivalenceClass;
}

Abstract::Abstract(int op, int left, int right){
    isSat = 1;
    satLevel = 1;
    isNumeral = 0;
    eqClass = 0;
    switch(op){
        case 0://+
            isNumeral = 1;
            if(left == 0 || right == 0){
                eqClass = 0;
            }
            else if(left == 2){
                eqClass = right;
            }
            else if(right == 2){
                eqClass = left;
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                eqClass = 7;
            }
            else if(left == right){
                eqClass = left;
            }
            else if(left == 1 && right == 4){
                eqClass = 1;
            }
            else if(right == 1 && left == 4){
                eqClass = 1;
            }
            else if(left == 3 && right == 5){
                eqClass = 3;
            }
            else if(right == 3 && left == 5){
                eqClass = 3;
            }
            else{
                eqClass = 7;
            }
            break;
        case 1://-
            isNumeral = 1;
            if(left == 0 || right == 0){
                eqClass = 0;
            }
            else if(right == 2){
                eqClass = left;
            }
            else if(left == 7 || right == 7){
                eqClass = 7;
            }
            else if(left == right){
                eqClass = 7;
            }
            else if(left == 6){
                eqClass = 7;
            }
            else if(right == 1){
                if(left == 4){
                    eqClass = 7;
                }
                else{
                    eqClass = 3;
                }
            }
            else if(right == 3){
                if(left == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 1;
                }
            }
            else if(right == 6){
                if(left == 2){
                    eqClass = 6;
                }
                else{
                    eqClass = 7;
                }
            }
            else if(left == 4 || left == 5){
                eqClass = left;
            }
            else if(left == 2 && right == 4){
                eqClass = 5;
            }
            else if(left == 3 && right == 4){
                eqClass = 3;
            }
            else if(left == 1 && right == 5){
                eqClass = 1;
            }
            else if(left == 2 && right == 5){
                eqClass = 4;
            }
            else{
                eqClass = 7;
            }
            break;
        case 2://*
            isNumeral = 1;
            if(left == 0 || right == 0){
                eqClass = 0;
            }
            else if(left == 2 || right == 2){
                eqClass = 2;
            }
            else if(left == 7 || right == 7){
                eqClass = 7;
            }
            else if(left + right == 5){
                eqClass = 5;
            }
            else if(left + right == 8){
                eqClass = 5;
            }
            else if(left == 6){
                if(right == 4 || right == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 6;
                }
            }
            else if(right == 6){
                if(left == 4 || left == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 6;
                }
            }
            else if(left == 5 && right == 5){
                eqClass = 5;
            }
            else if(left == 5 || right == 5){
                eqClass = 4;
            }
            else if(left + right == 7){
                eqClass = 4;
            }
            else if(left == right){
                eqClass = 3;
            }
            else{
                eqClass = 1;
            }
            break;
        case 3:// /
            isNumeral = 1;
            if(right == 0){
                cerr << "Cannot divide by 0" << endl;
                exit(1);
            }
            if(left == 0 || right == 0){
                eqClass = 0;
            }
            if(left == 2){
                eqClass = 2;
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                eqClass = 7;
            }
            else if(left + right == 8){
                eqClass = 5;
            }
            else if(left == right){
                eqClass = 5;
            }
            else if(left + right == 4){
                eqClass = 4;
            }
            else if(left + right == 5){
                eqClass = 5;
            }
            else{
                eqClass = 4;
            }
            break;
        case 4://<
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left + right == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 1 && right == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(right == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 3 && right == 3){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(right == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 1 && right == 2){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 5://>
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left + right == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 5 && right == 1){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(right == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 3 && right == 3){
                isSat = 1;
                satLevel = 1;
            }
            else if(right == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(right == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(right == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 2 && right == 1){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 6://<=
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left + right == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 5 && right == 1){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 4 && right == 1){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 4){
                isSat = 1;
                satLevel = 0;
            }
            else if(right == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(left + right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(right == 2 || right == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == 1){
                isSat = 1;
                satLevel = 1;
            }
            else{
                isSat = 0;
                satLevel = 2;
            }
            break;
        case 7://>=
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left == 7 || right == 7 || left == 6 || right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left + right == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 1 && right == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(right == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 1 && right == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(right == 4){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(left + right == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(right == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 2 || left == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(right == 1){
                isSat = 1;
                satLevel = 1;
            }
            else{
                isSat = 0;
                satLevel = 2;
            }
            break;
        case 8://==
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left + right >= 9){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 2 && right == 2){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == right){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == right + 1 || left + 1 == right){
                isSat = 0;
                satLevel = 2;
            }
            else if(left + right == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 4 || right == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 7 || right == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 6 || right == 6){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == 1 || right == 1){
                isSat = 0;
                satLevel = 2;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 9://!=
            if(left == 0 || right == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left + right >= 9){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 2 && right == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(left == right){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == right + 1 || left + 1 == right){
                isSat = 1;
                satLevel = 0;
            }
            else if(left + right == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 4 || right == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 7 || right == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left == 6 || right == 6){
                isSat = 1;
                satLevel = 0;
            }
            else if(left == 1 || right == 1){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
    }
}

Abstract::Abstract(int op, Abstract left, Abstract right){
    isSat = 1;
    satLevel = 1;
    isNumeral = 0;
    eqClass = 0;
    switch(op){
        case 0://+
            isNumeral = 1;
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                eqClass = 0;
            }
            else if(left.getEqClass() == 2){
                eqClass = right.getEqClass();
            }
            else if(right.getEqClass() == 2){
                eqClass = left.getEqClass();
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                eqClass = 7;
            }
            else if(left.getEqClass() == right.getEqClass()){
                eqClass = left.getEqClass();
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 4){
                eqClass = 1;
            }
            else if(right.getEqClass() == 1 && left.getEqClass() == 4){
                eqClass = 1;
            }
            else if(left.getEqClass() == 3 && right.getEqClass() == 5){
                eqClass = 3;
            }
            else if(right.getEqClass() == 3 && left.getEqClass() == 5){
                eqClass = 3;
            }
            else{
                eqClass = 7;
            }
            break;
        case 1://-
            isNumeral = 1;
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                eqClass = 0;
            }
            else if(right.getEqClass() == 2){
                eqClass = left.getEqClass();
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7){
                eqClass = 7;
            }
            else if(left.getEqClass() == right.getEqClass()){
                eqClass = 7;
            }
            else if(left.getEqClass() == 6){
                eqClass = 7;
            }
            else if(right.getEqClass() == 1){
                if(left.getEqClass() == 4){
                    eqClass = 7;
                }
                else{
                    eqClass = 3;
                }
            }
            else if(right.getEqClass() == 3){
                if(left.getEqClass() == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 1;
                }
            }
            else if(right.getEqClass() == 6){
                if(left.getEqClass() == 2){
                    eqClass = 6;
                }
                else{
                    eqClass = 7;
                }
            }
            else if(left.getEqClass() == 4 || left.getEqClass() == 5){
                eqClass = left.getEqClass();
            }
            else if(left.getEqClass() == 2 && right.getEqClass() == 4){
                eqClass = 5;
            }
            else if(left.getEqClass() == 3 && right.getEqClass() == 4){
                eqClass = 3;
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 5){
                eqClass = 1;
            }
            else if(left.getEqClass() == 2 && right.getEqClass() == 5){
                eqClass = 4;
            }
            else{
                eqClass = 7;
            }
            break;
        case 2://*
            isNumeral = 1;
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                eqClass = 0;
            }
            else if(left.getEqClass() == 2 || right.getEqClass() == 2){
                eqClass = 2;
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7){
                eqClass = 7;
            }
            else if(left.getEqClass() + right.getEqClass() == 5){
                eqClass = 5;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                eqClass = 5;
            }
            else if(left.getEqClass() == 6){
                if(right.getEqClass() == 4 || right.getEqClass() == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 6;
                }
            }
            else if(right.getEqClass() == 6){
                if(left.getEqClass() == 4 || left.getEqClass() == 5){
                    eqClass = 7;
                }
                else{
                    eqClass = 6;
                }
            }
            else if(left.getEqClass() == 5 && right.getEqClass() == 5){
                eqClass = 5;
            }
            else if(left.getEqClass() == 5 || right.getEqClass() == 5){
                eqClass = 4;
            }
            else if(left.getEqClass() + right.getEqClass() == 7){
                eqClass = 4;
            }
            else if(left.getEqClass() == right.getEqClass()){
                eqClass = 3;
            }
            else{
                eqClass = 1;
            }
            break;
        case 3:// /
            isNumeral = 1;
            if(right.getEqClass() == 0){
                cerr << "Cannot divide by 0" << endl;
                exit(1);
            }
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                eqClass = 0;
            }
            if(left.getEqClass() == 2){
                eqClass = 2;
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                eqClass = 7;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                eqClass = 5;
            }
            else if(left.getEqClass() == right.getEqClass()){
                eqClass = 5;
            }
            else if(left.getEqClass() + right.getEqClass() == 4){
                eqClass = 4;
            }
            else if(left.getEqClass() + right.getEqClass() == 5){
                eqClass = 5;
            }
            else{
                eqClass = 4;
            }
            break;
        case 4://<
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(right.getEqClass() == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 3 && right.getEqClass() == 3){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(right.getEqClass() == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 2){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 5://>
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 5 && right.getEqClass() == 1){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(right.getEqClass() == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 3 && right.getEqClass() == 3){
                isSat = 1;
                satLevel = 1;
            }
            else if(right.getEqClass() == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(right.getEqClass() == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(right.getEqClass() == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 2 && right.getEqClass() == 1){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 6://<=
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 5 && right.getEqClass() == 1){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 4 && right.getEqClass() == 1){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 4){
                isSat = 1;
                satLevel = 0;
            }
            else if(right.getEqClass() == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() + right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(right.getEqClass() == 2 || right.getEqClass() == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == 1){
                isSat = 1;
                satLevel = 1;
            }
            else{
                isSat = 0;
                satLevel = 2;
            }
            break;
        case 7://>=
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7 || left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() + right.getEqClass() == 8){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 5){
                isSat = 0;
                satLevel = 2;
            }
            else if(right.getEqClass() == 5){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 1 && right.getEqClass() == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(right.getEqClass() == 4){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == 5){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() + right.getEqClass() == 6){
                isSat = 1;
                satLevel = 1;
            }
            else if(right.getEqClass() == 3){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 2 || left.getEqClass() == 3){
                isSat = 1;
                satLevel = 0;
            }
            else if(right.getEqClass() == 1){
                isSat = 1;
                satLevel = 1;
            }
            else{
                isSat = 0;
                satLevel = 2;
            }
            break;
        case 8://==
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() + right.getEqClass() >= 9){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 2 && right.getEqClass() == 2){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == right.getEqClass()){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == right.getEqClass() + 1 || left.getEqClass() + 1 == right.getEqClass()){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() + right.getEqClass() == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 4 || right.getEqClass() == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == 1 || right.getEqClass() == 1){
                isSat = 0;
                satLevel = 2;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
        case 9://!=
            if(left.getEqClass() == 0 || right.getEqClass() == 0){
                cerr << "not sure how to compare empty sets" << endl;
                exit(1);
            }
            else if(left.getEqClass() + right.getEqClass() >= 9){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 2 && right.getEqClass() == 2){
                isSat = 0;
                satLevel = 2;
            }
            else if(left.getEqClass() == right.getEqClass()){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == right.getEqClass() + 1 || left.getEqClass() + 1 == right.getEqClass()){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() + right.getEqClass() == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 4 || right.getEqClass() == 4){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 7 || right.getEqClass() == 7){
                isSat = 1;
                satLevel = 1;
            }
            else if(left.getEqClass() == 6 || right.getEqClass() == 6){
                isSat = 1;
                satLevel = 0;
            }
            else if(left.getEqClass() == 1 || right.getEqClass() == 1){
                isSat = 1;
                satLevel = 0;
            }
            else{
                isSat = 1;
                satLevel = 1;
            }
            break;
    }
}


Abstract::Abstract(const Abstract& in){
    isSat = in.isSat;
    satLevel = in.satLevel;
    isNumeral = in.isNumeral;
    eqClass = in.eqClass;
}


Abstract& Abstract::operator=(Abstract in) noexcept{
    isSat = in.isSat;
    satLevel = in.satLevel;
    isNumeral = in.isNumeral;
    eqClass = in.eqClass;
    return *this;
}

bool Abstract::getIsSat(){
    return isSat;
}

int Abstract::getSatLevel(){
    return satLevel;
}

bool Abstract::getIsNumeral(){
    return isNumeral;
}

int Abstract::getEqClass(){
    return eqClass;
}

Abstract::~Abstract(){
    //nothing to do
}


ostream& Abstract::print(ostream& out) const{
    if(isNumeral){
        out << "Numeral: ";
        switch(eqClass){
            case 0:
                out << "Empty Set";
                break;
            case 1:
                out << "-";
                break;
            case 2:
                out << "0";
                break;
            case 3:
                out << "+";
                break;
            case 4:
                out << "0-";
                break;
            case 5:
                out << "+0";
                break;
            case 6:
                out << "-+";
                break;
            case 7:
                out << "a";
                break;
        }
    }
    else{
        switch(satLevel){
            case 0:
                assert(isSat);
                out << "SAT: Always";
                break;
            case 1:
                assert(isSat);
                out << "SAT: Sometimes";
                break;
            case 2:
                assert(!isSat);
                out << "UNSAT";
                break;
        }
    }
    return out;
}