
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

#include "../_list.h"


struct A {};
struct B : A {};
struct C {};

struct CompareA
{
    int operator() (const A* item1, const A* item2, void*) const
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
void ffail_check(bool b) {if (!b)  exit(1);}

void fsucc_check(const lst::FindResult& fr) {fsucc_check(fr.success());}
void ffail_check(const lst::FindResult& fr) {ffail_check(fr.failed());}


bool findTest()
{
    printf("\n\n=== Find Test ===\n");

    lst::List<int> list;
    lst::FindResult fr;

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


    printf("\n--- Test descend, 1 elements (1) ---\n");
    list.clear();
    list.addCopy(1);
    list.sort(lst::SortDown);

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
    list.sort(lst::SortDown);

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
    list.sort(lst::SortDown);

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
    list.sort(lst::SortDown);

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

int compareFunc(const int* item1, const int* item2, void*)
{
    return LIST_COMPARE_ITEM(*item1, *item2);
}

struct CompareFunctor
{
    int operator() (const int* item1, const int* item2, void* extParam) const
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
    printf("findItem: 4 : %s \n", (ptr) ? "OK" : "FALSE");   fsucc_check(ptr);

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
    printf("findItem: 4 : %s \n", (ptr) ? "OK" : "FALSE");   fsucc_check(ptr);

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
    printf("findItem: 4 : %s \n", (ptr) ? "OK" : "FALSE");   fsucc_check(ptr);

    printf("SUCCESS\n");
}

void bruteFindTest()
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

void findNotuniqueTest(const lst::List<int>& list, int i0, int i11, int i12, int i21, int i22, int i4, int i51, int i52, int i7)
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

void findNotuniqueTest()
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

    //                      i0  i11 i12 i21 i22 i4  i51 i52 i7
    findNotuniqueTest(list, -1, 0,  1,  2,  5,  6,  7,  9, -1);

    printf("\n--- Test descend, 10 elements (5,5,5,4,2,2,2,2,1,1) ---\n");
    list.sort(lst::SortDown);

    //                      i0  i11 i12 i21 i22 i4 i51  i52 i7
    findNotuniqueTest(list, -1, 8,  9,  4,  7,  3, 0,   2,  -1);

    printf("TEST PASSED\n");
}

struct SortTwoVal
{
    int val1;
    int val2;

    struct Compare
    {
        int operator() (const SortTwoVal* item1, const SortTwoVal* item2, void* = 0) const
        {
            LIST_COMPARE_MULTI_ITEM(item1->val1,  item2->val1)
            return LIST_COMPARE_ITEM(item2->val2, item1->val2);
        }
    };

};

void findNotuniqueStructTest()
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

    list.sort(lst::SortUp);
    printf("\n--- List of SortTwoVal was sorted up: ---\n");
    for (SortTwoVal* v : list)
    {
        printf("val1: %d  val2: %d\n", v->val1, v->val2);
    }
    printf("\n");

    list.sort(lst::SortDown);
    printf("\n--- List of SortTwoVal was sorted down: ---\n");
    for (SortTwoVal* v : list)
    {
        printf("val1: %d  val2: %d\n", v->val1, v->val2);
    }

    printf("TEST PASSED\n");
}

void checkRangeFunc()
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

int main()
{
    // Проверяет корректность разрешения перегрузки имен функций поиска
    findTestCheckOverloads();

    // Тест поиска по отсортированному списку
    findTest();

    // Тест поиска по не отсортированному списку (поиск перебором)
    bruteFindTest();

    // Тест поиска по не уникальному отсортированному списку
    findNotuniqueTest();

    // Тест поиска по не уникальному отсортированному списку. Причем сортировка
    // выполнена по двум полям и оба не уникальные.
    // Требования к поиску: искомое значение должно быть >= поля1 и <= поля2.
    // Так же нужно вернуть граничные значения удовлетворяющие условию.
    findNotuniqueStructTest();

    // Проверка работы функции range()
    checkRangeFunc();

    return 0;
}
