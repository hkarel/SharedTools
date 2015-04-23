
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

    printf("\n=== Test ascend, 1 elements (1) ===\n");
    list.clear();
    list.addCopy(1);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);


    printf("\n=== Test ascend, 2 elements (1,2) ===\n");
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


    printf("\n=== Test ascend, 3 elements (1,2,3) ===\n");
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

    printf("\n=== Test ascend, 4 elements (1,2,3,5) ===\n");
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


    printf("\n=== Test descend, 1 elements (1) ===\n");
    list.clear();
    list.addCopy(1);
    list.sort(lst::SortDown);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);
    fr = list.findRef(1);
    printf("1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index()); ffail_check(fr);

    printf("\n=== Test descend, 2 elements (2,1) ===\n");
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


    printf("\n=== Test descend, 3 elements (3,2,1) ===\n");
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


    printf("\n=== Test descend, 4 elements (5,4,3,1) ===\n");
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


    return true;
}

int compareFunc2(const int* item1, const int* item2)
{
    return LIST_COMPARE_ITEM(*item1, *item2);
}

int compareFunc3(const int* item1, const int* item2, void*)
{
    return LIST_COMPARE_ITEM(*item1, *item2);
}

struct CompareFunctor
{
    int operator() (const int* item1, const int* item2, void* extParam) const
    {
      return LIST_COMPARE_ITEM(*item1, *item2);
    }

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

    printf("\nFind use function compareFunc, 3 param\n");
    i = 0;
    fr = lst::find(&i, list, compareFunc3, 0);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = lst::findRef(2, list, compareFunc3, 0);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(5, list, compareFunc3, 0);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(7, list, compareFunc3, 0);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(&i, list, compareFunc3, 0);
    printf("findItem: 4 : %s \n", (ptr) ? "OK" : "FALSE");   fsucc_check(ptr);

    printf("\nFind use function compareFunc2, 2 param\n");
    i = 0;
    fr = lst::find(&i, list, compareFunc2);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = lst::findRef(2, list, compareFunc2);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(5, list, compareFunc2);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(7, list, compareFunc2);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(&i, list, compareFunc2);
    printf("findItem: 4 : %s \n", (ptr) ? "OK" : "FALSE");   fsucc_check(ptr);


    printf("\nFind use function CompareFunctor, 2 param \n");
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

    printf("\nFind use function CompareFunctor, 3 param \n");
    i = 0;
    fr = lst::find(&i, list, compare_functor, 0);
    printf("find    : 0 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    fr = lst::findRef(2, list, compare_functor, 0);
    printf("findRef : 2 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(5, list, compare_functor, 0);
    printf("findRef : 5 : %s : %s\n", fres(fr), fsucc(fr));  fsucc_check(fr);
    fr = lst::findRef(7, list, compare_functor, 0);
    printf("findRef : 7 : %s : %s\n", fres(fr), ffail(fr));  ffail_check(fr);
    i = 4;
    ptr = lst::findItem(&i, list, compare_functor, 0);
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
}

void bruteFindTest()
{
    printf("\n\n=== Brute Find Test ===\n");

    std::vector<int> vec = {1, 2, 3, 4, 5};
    printf("Find in std::vector: 1, 2, 3, 4, 5 \n");

    bruteFindTestT(vec);

    lst::List<int> list;
    printf("\n\nFind in lst::List: 1, 2, 3, 4, 5 \n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);

    bruteFindTestT(list);
}

// Функция проверяет корректное разрешение имен функций, проверка идет
// на уровне компиляции.
void findTestCheckOverloads()
{
    int i;
    //lst::FindResult fr;
    lst::List<int> list;

    list.find(&i);
    list.find(&i, (void*)0);

    list.findItem(&i);
    list.findItem(&i, (void*)0);

    list.findRef(0);
    list.findRef(0, (void*)0);

    list.find(&i, compareFunc3, (void*)0);
    list.find(&i, compareFunc2);

    list.findItem(&i, compareFunc3, (void*)0);
    list.findItem(&i, compareFunc2);

    list.findRef(0, compareFunc3, (void*)0);
    list.findRef(0, compareFunc2);

    CompareFunctor compare_functor;
    list.find(&i, compare_functor, (void*)0);
    list.find(&i, compare_functor);

    list.findItem(&i, compare_functor, (void*)0);
    list.findItem(&i, compare_functor);

    list.findRef(0, compare_functor, (void*)0);
    list.findRef(0, compare_functor);

    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    list.findL(lambda_find);
    list.findItemL(lambda_find);

}

void findNotuniqueTest(const lst::List<int>& list, int i0, int i1, int i2, int i21, int i22, int i4, int i5, int i7)
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

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("lastFindResult (2) : 0 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("lastFindResult (2) : 0 : %s : %s (------ index: %i)\n", fres(fr_last), ffail(fr_last), index); ffail_check(fr_last);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("lastFindResult (3) : 0 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("lastFindResult (3) : 0 : %s : %s (------ index: %i)\n", fres(fr_last), ffail(fr_last), index); ffail_check(fr_last);

    // === Поиск числа 1 ===
    fr = list.findRef(1);
    printf("findRef            : 1 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 1;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("firstFindResult(2) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("lastFindResult (2) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("firstFindResult(3) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("lastFindResult (3) : 1 : %s : %s (------ index: %i)\n", fres(index == i1), fsucc(index == i1), index); fsucc_check(index == i1);

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

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("firstFindResult(2) : 2 : %s : %s (------ index: %i)\n", fres(index == i21), fsucc(index == i21), index); fsucc_check(index == i21);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("lastFindResult (2) : 2 : %s : %s (------ index: %i)\n", fres(index == i22), fsucc(index == i22), index); fsucc_check(index == i22);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("firstFindResult(3) : 2 : %s : %s (------ index: %i)\n", fres(index == i21), fsucc(index == i21), index); fsucc_check(index == i21);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("lastFindResult (3) : 2 : %s : %s (------ index: %i)\n", fres(index == i22), fsucc(index == i22), index); fsucc_check(index == i22);

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

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("firstFindResult(2) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("lastFindResult (2) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("firstFindResult(3) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("lastFindResult (3) : 4 : %s : %s (------ index: %i)\n", fres(index == i4), fsucc(index == i4), index); fsucc_check(index == i4);

    // === Поиск числа 5 ===
    fr = list.findRef(5);
    printf("findRef            : 5 : %s : %s (found  index: %i)\n", fres(fr), fsucc(fr), fr.index()); fsucc_check(fr);

    // Тест для lambda функций
    i = 5;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    index = fr_last.index();
    printf("lastFindResult (L) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("firstFindResult(2) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("lastFindResult (2) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("firstFindResult(3) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("lastFindResult (3) : 5 : %s : %s (------ index: %i)\n", fres(index == i5), fsucc(index == i5), index); fsucc_check(index == i5);

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

    // Тест для compare с двумя параметрами
    fr_first = lst::firstFindResult(list, compareFunc2, fr);
    index = fr_first.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc2, fr);
    index = fr_last.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_last),  ffail(fr_last), index); ffail_check(fr_last);

    // Тест для compare с тремя параметрами
    fr_first = lst::firstFindResult(list, compareFunc3, fr, 0);
    index = fr_first.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_first), ffail(fr_first), index); ffail_check(fr_first);

    fr_last = lst::lastFindResult(list, compareFunc3, fr, 0);
    index = fr_last.index();
    printf("firstFindResult(L) : 7 : %s : %s (------ index: %i)\n", fres(fr_last),  ffail(fr_last), index); ffail_check(fr_last);
}

void findNotuniqueTest()
{
    printf("\n\n=== Not unique Find Test ===\n");

    lst::List<int> list;

    printf("\n=== Test ascend, 6 elements (1,2,2,2,2,4,5) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(2);
    list.addCopy(4);
    list.addCopy(5);
    list.sort();

    //                      i0  i1 i2 i21 i22 i4 i5 i7
    findNotuniqueTest(list, -1, 0, 3, 1,  4,  5, 6, -1);

    printf("\n=== Test descend, 6 elements (5,4,2,2,2,2,1) ===\n");
    list.sort(lst::SortDown);

    //                      i0  i1 i2 i21 i22 i4 i5 i7
    findNotuniqueTest(list, -1, 6, 3, 2,  5,  1, 0, -1);
}

struct SortTwoVal
{
    int val1;
    int val2;

    struct Compare
    {
        int operator() (const SortTwoVal* item1, const SortTwoVal* item2, void* = 0) const
        {
            if (item1->val1 > item2->val1)
                return 1;
            else if (item1->val1 < item2->val1)
                return -1;

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

    list.sort(lst::SortDown);
    printf("List of SortTwoVal was sorted down:\n");
    for (SortTwoVal* v : list)
    {
        printf("val1: %d  val2: %d\n", v->val1, v->val2);
    }
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


    return 0;
}
