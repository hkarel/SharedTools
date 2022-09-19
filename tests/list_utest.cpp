/* clang-format off */

// Команда для сборки
// g++ -std=c++11 -ggdb3 -Wall -Wsign-compare -Weverything list_utest.cpp -o list_utest
// g++ -std=c++11 -ggdb3 -Wsign-compare  list_utest.cpp -o list_utest
//
// Опции компилятора
//  -fdiagnostics-show-option

#include <cstdio>
#include <iostream>
#include <functional>
#include <vector>

#include "../list.h"


struct A {};
struct B : A {};
struct C {};

struct CompareA
{
    int operator() (const A* item1, const A* item2) const
    {
        //return LIST_COMPARE_ITEM(*item1, *item2);
        return 0;
    }
};

const char* fres (bool b) {return b ? "Found    " : "Not found";}
const char* fres (const lst::FindResult& fr) {return fres(fr.success());}

const char* fsucc(bool b) {return b ? "OK" : "FAIL";}
const char* ffail(bool b) {return b ? "OK" : "FAIL";}

const char* fsucc(const lst::FindResult& fr) {return fsucc(fr.success());}
const char* ffail(const lst::FindResult& fr) {return ffail(fr.failed());}

void fsucc_check(bool b) {if (!b) exit(1);}
void ffail_check(bool b) {if (!b) exit(1);}

void fsucc_check(const lst::FindResult& fr) {fsucc_check(fr.success());}
void ffail_check(const lst::FindResult& fr) {ffail_check(fr.failed());}


bool find_Test()
{
    printf("\n\n=== Find Test ===\n");

    lst::List<int> list;
    lst::FindResult fr;

    printf("\n--- Test ascend, 0 elements ---\n");
    list.clear();
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test ascend, 1 elements (1) ---\n");
    list.clear();
    list.addCopy(1);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n--- Test ascend, 2 elements (1,2) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n--- Test ascend, 3 elements (1,2,3) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test ascend, 4 elements (1,2,3,5) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(5);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(4);
    printf("4 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(5);
    printf("5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n--- Test descend, 0 elements ---\n");
    list.clear();
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test descend, 1 elements (1) ---\n");
    list.clear();
    list.addCopy(1);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test descend, 2 elements (2,1) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n--- Test descend, 3 elements (3,2,1) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n--- Test descend, 4 elements (5,4,3,1) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(5);
    printf("5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(4);
    printf("4 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("TEST PASSED\n");
    return true;
}

bool find_addInSort_Test()
{
    printf("\n\n=== Find addInSort Test ===\n");

    lst::List<int> list;
    lst::FindResult fr;

    printf("\n--- Test ascend, 0 elements ---\n");
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test ascend, 1 elements (1) ---\n");
    fr = list.findRef(1);
    list.addCopyInSort(1, fr);

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: 1, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 1);

    printf("\n--- Test ascend, 2 elements (1,2) ---\n");
    fr = list.findRef(2);
    list.addCopyInSort(2, fr);

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: 2, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 2);

    printf("\n--- Test ascend, 3 elements (1,2,3) ---\n");
    fr = list.findRef(3);
    list.addCopyInSort(3, fr);

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(5);
    printf("5 : %s : %s (insert index: 3, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 3);
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: 3, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 3);

    printf("\n--- Test ascend, 4 elements (1,2,3,5) ---\n");
    fr = list.findRef(5);
    list.addCopyInSort(5, fr);

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(4);
    printf("4 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(5);
    printf("5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: 4, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 4);


    printf("\n--- Test descend, 0 elements ---\n");
    list.clear();
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n--- Test descend, 1 elements (1) ---\n");
    fr = list.findRef(1);
    list.addCopyInSort(1, fr);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 1, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 1);

    printf("\n--- Test descend, 2 elements (2,1) ---\n");
    fr = list.findRef(2);
    list.addCopyInSort(2, fr);

    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 2, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 2);

    printf("\n--- Test descend, 3 elements (3,2,1) ---\n");
    fr = list.findRef(3);
    list.addCopyInSort(3, fr);

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 3, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 3);

    printf("\n--- Test descend, 4 elements (5,4,3,1) ---\n");
    list.addCopy(5);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(4);
    list.addCopyInSort(4, fr);

    list.removeCond([](int* i){return *i == 2;});

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: 0, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 0);
    fr = list.findRef(5);
    printf("5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(4);
    printf("4 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(3);
    printf("3 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: 3, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 3);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: 4, detect insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr); fsucc_check(fr.index() == 4);

    printf("TEST PASSED\n");
    return true;
}


int compareFunc(const int* item1, const int* item2)
{
    return LIST_COMPARE_ITEM(*item1, *item2);
}

struct CompareFunctor
{
    int operator() (const int* item1, const int* item2) const
    {
      return LIST_COMPARE_ITEM(*item1, *item2);
    }
};

template<typename ListT> void bruteFindTestT(const ListT& list)
{
    int i;
    int* ptr;
    lst::FindResult fr;

    printf("\nFind use function compareFunc\n");
    i = 0;
    fr = lst::find(&i, list, compareFunc);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = lst::findRef(2, list, compareFunc);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(5, list, compareFunc);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(7, list, compareFunc);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(&i, list, compareFunc);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("\nFind use functor CompareFunctor \n");
    CompareFunctor compare_functor;
    i = 0;
    fr = lst::find(&i, list, compare_functor);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = lst::findRef(2, list, compare_functor);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(5, list, compare_functor);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(7, list, compare_functor);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(&i, list, compare_functor);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    printf("\nFind use lambda\n");
    i = 0;
    fr = lst::find(list, lambda_find);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 2;
    fr = lst::find(list, lambda_find);
    printf("find    : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 5;
    fr = lst::find(list, lambda_find);
    printf("find    : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 7;
    fr = lst::find(list, lambda_find);
    printf("find    : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(list, lambda_find);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("SUCCESS\n");
}

void bruteFind_Test()
{
    printf("\n\n=== Brute Find Test ===\n");

    std::vector<int> vec = {1, 2, 3, 4, 5};
    printf("\n--- Find in std::vector: 1, 2, 3, 4, 5 ---");

    bruteFindTestT(vec);

    lst::List<int> list;
    printf("\n--- Find in lst::List: 1, 2, 3, 4, 5 ---");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);

    bruteFindTestT(list);

    printf("TEST PASSED\n");
}

void bruteFind_Test2()
{
    printf("\n\n=== Brute Member Find Test ===\n");

    lst::List<int> list;
    printf("\n--- Find in lst::List: 1, 2, 3, 4, 5 ---");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);

    int i;
    int* ptr;
    lst::FindResult fr;
    CompareFunctor compare_functor;

    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    printf("\nFind use function compareFunc\n");
    i = 0;
    fr = list.find(&i, compareFunc);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = list.findRef(2, compareFunc);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(5, compareFunc);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(7, compareFunc);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItem(&i, compareFunc);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("\nFind use functor CompareFunctor \n");
    i = 0;
    fr = list.find(&i, compare_functor);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = list.findRef(2, compare_functor);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(5, compare_functor);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(7, compare_functor);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItem(&i, compare_functor);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("\nFind use lambda\n");
    i = 0;
    fr = list.findL(lambda_find);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 2;
    fr = list.findL(lambda_find);
    printf("find    : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 5;
    fr = list.findL(lambda_find);
    printf("find    : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 7;
    fr = list.findL(lambda_find);
    printf("find    : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItemL(lambda_find);
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    //---
    printf("\nFind use function compareFunc (with BruteForce flag)\n");
    i = 0;
    fr = list.find(&i, compareFunc, {lst::BruteForce::Yes});
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = list.findRef(2, compareFunc, {lst::BruteForce::Yes});
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(5, compareFunc, {lst::BruteForce::Yes});
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(7, compareFunc, {lst::BruteForce::Yes});
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItem(&i, compareFunc, {lst::BruteForce::Yes});
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("\nFind use functor CompareFunctor (with BruteForce flag)\n");
    i = 0;
    fr = list.find(&i, compare_functor, {lst::BruteForce::Yes});
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = list.findRef(2, compare_functor, {lst::BruteForce::Yes});
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(5, compare_functor, {lst::BruteForce::Yes});
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = list.findRef(7, compare_functor, {lst::BruteForce::Yes});
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItem(&i, compare_functor, {lst::BruteForce::Yes});
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("\nFind use lambda (with BruteForce flag)\n");
    i = 0;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 2;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    printf("find    : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 5;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    printf("find    : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    i = 7;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    printf("find    : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = list.findItemL(lambda_find, {lst::BruteForce::Yes});
    printf("findItem: 4 : %s : %s\n", fres(ptr), fsucc(ptr)); fsucc_check(ptr);

    printf("TEST PASSED\n");
}

// Функция проверяет корректное разрешение имен функций, проверка идет
// на уровне компиляции.
void findTestCheckOverloads()
{
    int i;
    //lst::FindResult fr;
    lst::List<int> list;

    list.find(&i);

    list.findItem(&i);

    list.findRef(0);

    list.find(&i, compareFunc);

    list.findItem(&i, compareFunc);

    list.findRef(0, compareFunc);

    CompareFunctor compare_functor;
    list.find(&i, compare_functor);

    list.findItem(&i, compare_functor);

    list.findRef(0, compare_functor);

    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    list.findL(lambda_find);
    list.findItemL(lambda_find);

}

void findNotunique_Test(const lst::List<int>& list, int i0, int i11, int i12, int i21, int i22, int i4, int i51, int i52, int i7)
{
    lst::FindResult fr;
    lst::FindResult fr_first;
    lst::FindResult fr_last;
    int index, i;

    auto lambda_find = [&i] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    // === Поиск числа 0 ===
    fr = list.findRef(0);
    printf("findRef            : 0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    // Тест для lambda функций
    i = 0;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 0 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 0 : %s : %s (------ index: %i)\n", fres(fr_last), ffail(fr_last), index); ffail_check(fr_last);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("lastFindResult     : 0 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("lastFindResult     : 0 : %s : %s (------ index: %i)\n", fres(fr_last), ffail(fr_last), index); ffail_check(fr_last);

    // === Поиск числа 1 ===
    fr = list.findRef(1);
    printf("findRef            : 1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 1;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 1 : %s : %s (------ index: %i)\n", fres(index == i11), fsucc(index == i11), index); fsucc_check(index == i11);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 1 : %s : %s (------ index: %i)\n", fres(index == i12), fsucc(index == i12), index); fsucc_check(index == i12);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("firstFindResult    : 1 : %s : %s (------ index: %i)\n", fres(index == i11), fsucc(index == i11), index); fsucc_check(index == i11);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("lastFindResult     : 1 : %s : %s (------ index: %i)\n", fres(index == i12), fsucc(index == i12), index); fsucc_check(index == i12);

    // === Поиск числа 2 ===
    fr = list.findRef(2);
    printf("findRef            : 2 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 2;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 2 : %s : %s (------ index: %i)\n", fres(index == i21), fsucc(index == i21), index); fsucc_check(index == i21);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 2 : %s : %s (------ index: %i)\n", fres(index == i22), fsucc(index == i22), index); fsucc_check(index == i22);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("firstFindResult    : 2 : %s : %s (------ index: %i)\n", fres(index == i21), fsucc(index == i21), index); fsucc_check(index == i21);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("lastFindResult     : 2 : %s : %s (------ index: %i)\n", fres(index == i22), fsucc(index == i22), index); fsucc_check(index == i22);

    // === Поиск числа 4 ===
    fr = list.findRef(4);
    printf("findRef            : 4 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 4;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("firstFindResult    : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("lastFindResult     : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    // === Поиск числа 5 ===
    fr = list.findRef(5);
    printf("findRef            : 5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 5;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 5 : %s : %s (------ index: %i)\n", fres(index == i51), fsucc(index == i51), index); fsucc_check(index == i51);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 5 : %s : %s (------ index: %i)\n", fres(index == i52), fsucc(index == i52), index); fsucc_check(index == i52);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("firstFindResult    : 5 : %s : %s (------ index: %i)\n", fres(index == i51), fsucc(index == i51), index); fsucc_check(index == i51);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("lastFindResult     : 5 : %s : %s (------ index: %i)\n", fres(index == i52), fsucc(index == i52), index); fsucc_check(index == i52);

    // === Поиск числа 7 ===
    fr = list.findRef(7);
    printf("findRef            : 7 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    // Тест для lambda функций
    i = 7;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_last),  ffail(fr_last), index); ffail_check(fr_last);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    index = fr_first.index();
    printf("firstFindResult    : 7 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    index = fr_last.index();
    printf("firstFindResult    : 7 : %s : %s (------ index: %i)\n", fres(fr_last),  ffail(fr_last), index); ffail_check(fr_last);

    printf("SUCCESS\n");
}

void findNotunique_Test()
{
    printf("\n\n=== Not unique Find Test ===\n");

    lst::List<int> list;

    printf("\n--- Test ascend, 10 elements (1,1,2,2,2,2,4,5,5,5) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(4);
    list.addCopy(5);
    list.addCopy(5);
    list.addCopy(5);
    list.sort();

    //                       i0  i11 i12 i21 i22 i4  i51 i52 i7
    findNotunique_Test(list, -1, 0,  1,  2,  5,  6,  7,  9,  -1);

    printf("\n--- Test descend, 10 elements (5,5,5,4,2,2,2,2,1,1) ---\n");
    list.sort(lst::SortMode::Down);

    //                       i0  i11 i12 i21 i22 i4 i51  i52 i7
    findNotunique_Test(list, -1, 8,  9,  4,  7,  3, 0,   2,  -1);

    printf("TEST PASSED\n");
}

struct SortTwoVal
{
    int val1;
    int val2;

    struct Compare
    {
        int operator() (const SortTwoVal* item1, const SortTwoVal* item2) const
        {
            LIST_COMPARE_MULTI_ITEM(item1->val1,  item2->val1)
            return LIST_COMPARE_ITEM(item2->val2, item1->val2);
        }
    };

};

void findNotuniqueStruct_Test()
{
    printf("\n\n=== Not unique Find Structure Test ===\n");

    lst::List<SortTwoVal, SortTwoVal::Compare> list;

    // 4

    SortTwoVal* v;
    v = list.add(); v->val1 = 1; v->val2 = 2;
    v = list.add(); v->val1 = 1; v->val2 = 3;
    v = list.add(); v->val1 = 1; v->val2 = 5;
    v = list.add(); v->val1 = 3; v->val2 = 7;
    v = list.add(); v->val1 = 3; v->val2 = 7;
    v = list.add(); v->val1 = 3; v->val2 = 8;
    v = list.add(); v->val1 = 4; v->val2 = 7;
    v = list.add(); v->val1 = 4; v->val2 = 8;
    v = list.add(); v->val1 = 6; v->val2 = 10;
    v = list.add(); v->val1 = 6; v->val2 = 15;
    v = list.add(); v->val1 = 6; v->val2 = 99;

    list.sort(lst::SortMode::Up);
    printf("\n--- List of SortTwoVal was sorted up/down ---\n");
    for (SortTwoVal* v : list)
    {
        printf("val1: %d  val2: %d\n", v->val1, v->val2);
    }
    printf("\n");

    list.sort(lst::SortMode::Down);
    printf("\n--- List of SortTwoVal was sorted down/up ---\n");
    for (SortTwoVal* v : list)
    {
        printf("val1: %d  val2: %d\n", v->val1, v->val2);
    }

    printf("TEST PASSED\n");
}

void checkRangeFunc_Test()
{
    printf("\n\n=== Check range() function ===\n");

    lst::FindResult fr;
    lst::FindResultRange frr;
    lst::List<int> list;
    int index, i;

    auto lambda_find = [&i] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    printf("\n--- Test for empty list ---\n");
    lst::List<int>::RangeType range = list.range(frr);
    for (int* val : range)
    {
        (void) val;
        printf("TEST FAIL");
        exit(1);
    }
    printf("range.begin() == list.end() : %s \n", fsucc(range.begin() == list.end())); fsucc_check(range.begin() == list.end());
    printf("range.end()   == list.end() : %s \n", fsucc(range.end()   == list.end())); fsucc_check(range.end()   == list.end());
    printf("SUCCESS\n");

    printf("\n--- Test for 10 elements (1,1,2,2,2,2,4,5,5,5) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(4);
    list.addCopy(5);
    list.addCopy(5);
    list.addCopy(5);
    list.sort();

    // === Диапазон для числа 0 ===
    i = 0;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), ffail(fr)); ffail_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() == list.end() : %s \n", i, fsucc(range.begin() == list.end())); fsucc_check(range.begin() == list.end());
    printf("          %i : range.end()   == list.end() : %s \n", i, fsucc(range.end()   == list.end())); fsucc_check(range.end()   == list.end());
    for (int* val : range)
    {
        (void) val;
        printf("TEST FAIL");
        exit(1);
    }

    // === Диапазон для числа 1 ===
    i = 1;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), fsucc(fr)); fsucc_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() != list.end() : %s \n", i, fsucc(range.begin() != list.end())); fsucc_check(range.begin() != list.end());
    printf("          %i : range.end()   != list.end() : %s \n", i, fsucc(range.end()   != list.end())); fsucc_check(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    printf("          %i : index first element: %i, %s \n", i, index, fsucc(index == 0)); fsucc_check(index == 0);

    index = list.indexOf(*(range.end() - 1));
    printf("          %i : index last  element: %i, %s \n", i, index, fsucc(index == 1)); fsucc_check(index == 1);

    printf("          %i : Print : ", i);
    for (int* val : range) {printf("%i, ", *val);}
    printf("\n");

    // === Диапазон для числа 2 ===
    i = 2;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), fsucc(fr)); fsucc_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() != list.end() : %s \n", i, fsucc(range.begin() != list.end())); fsucc_check(range.begin() != list.end());
    printf("          %i : range.end()   != list.end() : %s \n", i, fsucc(range.end()   != list.end())); fsucc_check(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    printf("          %i : index first element: %i, %s \n", i, index, fsucc(index == 2)); fsucc_check(index == 2);

    index = list.indexOf(*(range.end() - 1));
    printf("          %i : index last  element: %i, %s \n", i, index, fsucc(index == 5)); fsucc_check(index == 5);

    printf("          %i : Print : ", i);
    for (int* val : range) {printf("%i, ", *val);}
    printf("\n");

    // === Диапазон для числа 4 ===
    i = 4;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), fsucc(fr)); fsucc_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() != list.end() : %s \n", i, fsucc(range.begin() != list.end())); fsucc_check(range.begin() != list.end());
    printf("          %i : range.end()   != list.end() : %s \n", i, fsucc(range.end()   != list.end())); fsucc_check(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    printf("          %i : index first element: %i, %s \n", i, index, fsucc(index == 6)); fsucc_check(index == 6);

    index = list.indexOf(*(range.end() - 1));
    printf("          %i : index last  element: %i, %s \n", i, index, fsucc(index == 6)); fsucc_check(index == 6);

    printf("          %i : Print : ", i);
    for (int* val : range) {printf("%i, ", *val);}
    printf("\n");

    // === Диапазон для числа 5 ===
    i = 5;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), fsucc(fr)); fsucc_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() != list.end() : %s \n", i, fsucc(range.begin() != list.end())); fsucc_check(range.begin() != list.end());
    printf("          %i : range.end()   == list.end() : %s \n", i, fsucc(range.end()   == list.end())); fsucc_check(range.end()   == list.end());

    index = list.indexOf(*range.begin());
    printf("          %i : index first element: %i, %s \n", i, index, fsucc(index == 7)); fsucc_check(index == 7);

    index = list.indexOf(*(range.end() - 1));
    printf("          %i : index last  element: %i, %s \n", i, index, fsucc(index == 9)); fsucc_check(index == 9);

    printf("          %i : Print : ", i);
    for (int* val : range) {printf("%i, ", *val);}
    printf("\n");

    // === Диапазон для числа 7 ===
    i = 7;
    fr = list.findRef(i);
    printf("findRef : %i : %s : %s \n", i, fres(fr), ffail(fr)); ffail_check(fr);

    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);

    printf("          %i : range.begin() == list.end() : %s \n", i, fsucc(range.begin() == list.end())); fsucc_check(range.begin() == list.end());
    printf("          %i : range.end()   == list.end() : %s \n", i, fsucc(range.end()   == list.end())); fsucc_check(range.end()   == list.end());
    for (int* val : range)
    {
        (void) val;
        printf("TEST FAIL");
        exit(1);
    }
    printf("SUCCESS\n");

    printf("TEST PASSED\n");
}

void compressList_Test()
{
    printf("\n\n=== Check compressList() function ===\n");

    lst::List<int> list;

    auto print_elements = [](lst::List<int>& list, int count)
    {
        printf("Print elements: ");
        for (int i = 0; i < list.count(); ++i)
        {
            if (list.item(i))
                printf("%i,", list[i]);
            else
                printf("null,");
        }
        printf(" count: %i : %s", list.count(), fsucc(list.count() == count)); fsucc_check(list.count() == count);
        printf("\n");
    };

    auto print_elements_after_compression = [](lst::List<int>& list, int count)
    {
        printf("Print elements after compression list: ");
        for (int i = 0; i < list.count(); ++i)
        {
            if (list.item(i))
                printf("%i,", list[i]);
            else
                printf("null,");
        }
        printf(" count: %i : %s", list.count(), fsucc(list.count() == count)); fsucc_check(list.count() == count);
        printf("\n");
    };

    //---
    printf("\n--- Test for 3 elements (1,2,3) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    printf("Remove elements 1 without compression list\n");
    list.remove(0, lst::CompressList::No);

    print_elements(list, 3);

    list.compressList();

    print_elements_after_compression(list, 2);

    printf("Index 0 : %i == 2 : %s \n",  list[0], fsucc(list[0] == 2));  fsucc_check(list[0] == 2);
    printf("Index 1 : %i == 3 : %s \n",  list[1], fsucc(list[1] == 3));  fsucc_check(list[1] == 3);


    //---
    printf("\n--- Test for 3 elements (1,2,3) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    printf("Remove elements 2 without compression list\n");
    list.remove(1, lst::CompressList::No);

    print_elements(list, 3);

    list.compressList();

    print_elements_after_compression(list, 2);

    printf("Index 0 : %i == 1 : %s \n",  list[0], fsucc(list[0] == 1));  fsucc_check(list[0] == 1);
    printf("Index 1 : %i == 3 : %s \n",  list[1], fsucc(list[1] == 3));  fsucc_check(list[1] == 3);


    //---
    printf("\n--- Test for 3 elements (1,2,3) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    printf("Remove elements 3 without compression list\n");
    list.remove(2, lst::CompressList::No);

    print_elements(list, 3);

    list.compressList();

    print_elements_after_compression(list, 2);

    printf("Index 0 : %i == 1 : %s \n",  list[0], fsucc(list[0] == 1));  fsucc_check(list[0] == 1);
    printf("Index 1 : %i == 2 : %s \n",  list[1], fsucc(list[1] == 2));  fsucc_check(list[1] == 2);

    //---
    printf("\n--- Test for 4 elements (1,2,3,4) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    printf("Remove elements 1,2 without compression list\n");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);

    print_elements(list, 4);

    list.compressList();

    print_elements_after_compression(list, 2);

    printf("Index 0 : %i == 3 : %s \n",  list[0], fsucc(list[0] == 3));  fsucc_check(list[0] == 3);
    printf("Index 1 : %i == 4 : %s \n",  list[1], fsucc(list[1] == 4));  fsucc_check(list[1] == 4);

    //---
    printf("\n--- Test for 4 elements (1,2,3,4) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    printf("Remove elements 3,4 without compression list\n");
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);

    print_elements(list, 4);

    list.compressList();

    print_elements_after_compression(list, 2);

    printf("Index 0 : %i == 1 : %s \n",  list[0], fsucc(list[0] == 1));  fsucc_check(list[0] == 1);
    printf("Index 1 : %i == 2 : %s \n",  list[1], fsucc(list[1] == 2));  fsucc_check(list[1] == 2);

    //---
    printf("\n--- Test for 4 elements (1,2,3,4) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    printf("Remove elements 1,2,3 without compression list\n");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);

    print_elements(list, 4);

    list.compressList();

    print_elements_after_compression(list, 1);

    printf("Index 0 : %i == 4 : %s \n",  list[0], fsucc(list[0] == 4));  fsucc_check(list[0] == 4);

    //---
    printf("\n--- Test for 4 elements (1,2,3,4) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    printf("Remove elements 2,3,4 without compression list\n");
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);

    print_elements(list, 4);

    list.compressList();

    print_elements_after_compression(list, 1);

    printf("Index 0 : %i == 1 : %s \n",  list[0], fsucc(list[0] == 1));  fsucc_check(list[0] == 1);

    //---
    printf("\n--- Test for 4 elements (1,2,3,4) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    printf("Remove elements 1,2,3,4 without compression list\n");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);

    print_elements(list, 4);

    list.compressList();

    print_elements_after_compression(list, 0);

    //---
    printf("\n--- Test for 15 elements (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15) ---\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);
    list.addCopy(6);
    list.addCopy(7);
    list.addCopy(8);
    list.addCopy(9);
    list.addCopy(10);
    list.addCopy(11);
    list.addCopy(12);
    list.addCopy(13);
    list.addCopy(14);
    list.addCopy(15);

    printf("Remove elements 1,3,5,6,9,10,11,13,14,15 without compression list\n");
    list.remove(0,  lst::CompressList::No);
    list.remove(2,  lst::CompressList::No);
    list.remove(4,  lst::CompressList::No);
    list.remove(5,  lst::CompressList::No);
    list.remove(8,  lst::CompressList::No);
    list.remove(9,  lst::CompressList::No);
    list.remove(10, lst::CompressList::No);
    list.remove(12, lst::CompressList::No);
    list.remove(13, lst::CompressList::No);
    list.remove(14, lst::CompressList::No);

    print_elements(list, 15);

    list.compressList();

    print_elements_after_compression(list, 5);

    printf("Index 0 : %i == 2 : %s \n",  list[0], fsucc(list[0] == 2));  fsucc_check(list[0] == 2);
    printf("Index 1 : %i == 4 : %s \n",  list[1], fsucc(list[1] == 4));  fsucc_check(list[1] == 4);
    printf("Index 2 : %i == 7 : %s \n",  list[2], fsucc(list[2] == 7));  fsucc_check(list[2] == 7);
    printf("Index 3 : %i == 8 : %s \n",  list[3], fsucc(list[3] == 8));  fsucc_check(list[3] == 8);
    printf("Index 4 : %i == 12 : %s \n", list[4], fsucc(list[4] == 12)); fsucc_check(list[4] == 12);

    printf("TEST PASSED\n");
}

int main()
{
    // Проверка работы функции compressList()
    compressList_Test();
    //return 0;

    // Проверяет корректность разрешения перегрузки имен функций поиска
    findTestCheckOverloads();

    // Тест поиска по отсортированному списку
    find_Test();

    // Тест добавления новых элементов через функцию addInSort()
    // с последующим поиском.
    find_addInSort_Test();

    // Тест поиска по не отсортированному списку (поиск перебором)
    bruteFind_Test();
    bruteFind_Test2();

    // Тест поиска по не уникальному отсортированному списку
    findNotunique_Test();

    // Тест поиска по не уникальному отсортированному списку. Причем сортировка
    // выполнена по двум полям и оба не уникальные.
    // Требования к поиску: искомое значение должно быть >= поля1 и <= поля2.
    // Так же нужно вернуть граничные значения удовлетворяющие условию.
    findNotuniqueStruct_Test();

    // Проверка работы функции range()
    checkRangeFunc_Test();


    return 0;
}
