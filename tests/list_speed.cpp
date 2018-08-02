/****************************************************************************
  В модуле реализованы тесты для нагрузочного и скоростного тестирования
  lst::List, а так же тесты сравнения производительности с контейнерами STL.

****************************************************************************/

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

#include "../list.h"
#include "../mem_manager.h"

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

exact_clock::time_point tpoint;
chrono::milliseconds elapsed;

template<typename T, bool Destroy = true>
struct ListMemAlloc
{
    typedef MemManager<T, MemLockDummy>  MemManagerType;

    ListMemAlloc() : memManager(100010)
    {}

    T* create()
    {
        return memManager.create();
    }

    void destroy(T* x)
    {
        if (Destroy)
            memManager.destroy(x);
    }

    MemManagerType memManager;
};


struct TestStruct
{
    uint8_t uuid[16];
    int field_1;
    int field_2;
};

void allocateAndFreeMemory()
{
    printf("\n\n=== Allocate and free memory lst::List (test on 10 mln elements) ===");

    printf("\nUsing 'new'/'delete' operators (element of 'long' type)");
    {
        lst::List<long> list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

    printf("\n\nUsing memory allocator (element of 'long' type)");
    {

        lst::List<long, lst::CompareItemDummy, ListMemAlloc<long>>  list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

    printf("\n\nUsing memory allocator, no-destroy mode (element of 'long' type)");
    {

        lst::List<long, lst::CompareItemDummy, ListMemAlloc<long, false>>  list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

    printf("\n");

    printf("\nUsing 'new'/'delete' operators (element of 'TestStruct' type)");
    {
        lst::List<TestStruct> list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

    printf("\n\nUsing memory allocator (element of 'TestStruct' type)");
    {

        lst::List<TestStruct, lst::CompareItemDummy, ListMemAlloc<TestStruct>>  list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

    printf("\n\nUsing memory allocator; no-destroy mode (element of 'TestStruct' type)");
    {

        lst::List<TestStruct, lst::CompareItemDummy, ListMemAlloc<TestStruct, false>>  list;

        tpoint = exact_clock::now();
        for (int i = 0; i < 10000000; ++i)
        {
            list.add();
        }
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime create: %lu ms", size_t(elapsed.count()));

        tpoint = exact_clock::now();
        list.clear();
        elapsed = chrono::duration_cast<chrono::milliseconds>(exact_clock::now() - tpoint);
        printf("\nTime destroyed: %lu ms", size_t(elapsed.count()));
    }

}


int main()
{
    allocateAndFreeMemory();


    printf("\n");
    return 0;
}
