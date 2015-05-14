
// Команда для сборки
// g++ -std=c++11 -ggdb3 -Wall -Wsign-compare -Weverything list_speed.cpp -o list_speed
// g++ -std=c++11 -ggdb3 -Wsign-compare  list_speed.cpp -o list_speed
//
// Опции компилятора
//  -fdiagnostics-show-option

#include <sys/types.h>
#include <cstdio>
#include <iostream>
#include <vector>
#include <chrono>

#include "../_list.h"

using namespace std;

typedef chrono::high_resolution_clock exact_clock;


const char* fres (bool b) {return b ? "Found    " : "Not found";}
const char* fres (const lst::FindResult& fr) {return fres(fr.success());}

const char* fsucc(bool b) {return b ? "OK" : "FAIL";}
const char* ffail(bool b) {return b ? "OK" : "FAIL";}

const char* fsucc(const lst::FindResult& fr) {return fsucc(fr.success());}
const char* ffail(const lst::FindResult& fr) {return ffail(fr.failed());}

void fsucc_check(bool b) {if (!b) exit(1);}
void ffail_check(bool b) {if (!b)  exit(1);}

void fsucc_check(const lst::FindResult& fr) {fsucc_check(fr.success());}
void ffail_check(const lst::FindResult& fr) {ffail_check(fr.failed());}



int main()
{


    return 0;
}
