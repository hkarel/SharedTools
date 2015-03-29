
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

const char* fres (const lst::FindResult& fr) {return fr.success() ? "Found    " : "Not found";}
const char* fsucc(const lst::FindResult& fr) {return fr.success() ? "OK" : "FAIL";}
const char* ffail(const lst::FindResult& fr) {return fr.failed()  ? "OK" : "FAIL";}

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
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    printf("\n=== Test ascend, 2 elements (1,2) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    printf("\n=== Test ascend, 3 elements (1,2,3) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(3);
    printf("3 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());

    printf("\n=== Test ascend, 4 elements (1,2,3,5) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(5);
    list.sort();

    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(3);
    printf("3 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(4);
    printf("4 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(5);
    printf("5 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    printf("\n=== Test descend, 1 elements (1) ===\n");
    list.clear();
    list.addCopy(1);
    list.sort(lst::SortDown);

    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());

    printf("\n=== Test descend, 2 elements (2,1) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort(lst::SortDown);

    fr = list.findRef(3);
    printf("3 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(2);
    printf("2 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    printf("\n=== Test descend, 3 elements (3,2,1) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort(lst::SortDown);

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(3);
    printf("3 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    printf("\n=== Test descend, 4 elements (5,4,3,1) ===\n");
    list.clear();
    list.addCopy(1);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);
    list.sort(lst::SortDown);

    fr = list.findRef(6);
    printf("6 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(5);
    printf("5 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(4);
    printf("4 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(3);
    printf("3 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(2);
    printf("2 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());
    fr = list.findRef(1);
    printf("1 : %s : %s\n", fres(fr), fsucc(fr));
    fr = list.findRef(0);
    printf("0 : %s : %s (insert index: %i)\n", fres(fr), ffail(fr), fr.index());


    return true;
}

int compareFunc(const int* item1, const int* item2, void*)
{
    return LIST_COMPARE_ITEM(*item1, *item2);
}

int compareFunc2(const int* item1, const int* item2)
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
    fr = lst::find(&i, list, compareFunc, 0);
    printf("Find 0      : %s : %s\n", fres(fr), ffail(fr));
    fr = lst::findRef(2, list, compareFunc, 0);
    printf("Find 2 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(5, list, compareFunc, 0);
    printf("Find 5 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(7, list, compareFunc, 0);
    printf("Find 7 ref  : %s : %s\n", fres(fr), ffail(fr));
    i = 4;
    ptr = lst::findItem(&i, list, compareFunc, 0);
    printf("Find 4 item : %s \n", (ptr) ? "OK" : "FALSE");

    printf("\nFind use function compareFunc2, 2 param\n");
    i = 0;
    fr = lst::find(&i, list, compareFunc2);
    printf("Find 0      : %s : %s\n", fres(fr), ffail(fr));
    fr = lst::findRef(2, list, compareFunc2);
    printf("Find 2 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(5, list, compareFunc2);
    printf("Find 5 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(7, list, compareFunc2);
    printf("Find 7 ref  : %s : %s\n", fres(fr), ffail(fr));
    i = 4;
    ptr = lst::findItem(&i, list, compareFunc2);
    printf("Find 4 item : %s \n", (ptr) ? "OK" : "FALSE");


    printf("\nFind use function CompareFunctor, 2 param \n");
    CompareFunctor compare_functor;
    i = 0;
    fr = lst::find(&i, list, compare_functor);
    printf("Find 0      : %s : %s\n", fres(fr), ffail(fr));
    fr = lst::findRef(2, list, compare_functor);
    printf("Find 2 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(5, list, compare_functor);
    printf("Find 5 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(7, list, compare_functor);
    printf("Find 7 ref  : %s : %s\n", fres(fr), ffail(fr));
    i = 4;
    ptr = lst::findItem(&i, list, compare_functor);
    printf("Find 4 item : %s \n", (ptr) ? "OK" : "FALSE");

    printf("\nFind use function CompareFunctor, 3 param \n");
    i = 0;
    fr = lst::find(&i, list, compare_functor, 0);
    printf("Find 0      : %s : %s\n", fres(fr), ffail(fr));
    fr = lst::findRef(2, list, compare_functor, 0);
    printf("Find 2 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(5, list, compare_functor, 0);
    printf("Find 5 ref  : %s : %s\n", fres(fr), fsucc(fr));
    fr = lst::findRef(7, list, compare_functor, 0);
    printf("Find 7 ref  : %s : %s\n", fres(fr), ffail(fr));
    i = 4;
    ptr = lst::findItem(&i, list, compare_functor, 0);
    printf("Find 4 item : %s \n", (ptr) ? "OK" : "FALSE");


    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    printf("\nFind use lambda\n");
    i = 0;
    fr = lst::find(list, lambda_find);
    printf("Find 0      : %s : %s\n", fres(fr), ffail(fr));
    i = 2;
    fr = lst::find(list, lambda_find);
    printf("Find 2      : %s : %s\n", fres(fr), fsucc(fr));
    i = 5;
    fr = lst::find(list, lambda_find);
    printf("Find 5      : %s : %s\n", fres(fr), fsucc(fr));
    i = 7;
    fr = lst::find(list, lambda_find);
    printf("Find 7      : %s : %s\n", fres(fr), ffail(fr));
    i = 4;
    ptr = lst::findItem(list, lambda_find);
    printf("Find 4 item : %s \n", (ptr) ? "OK" : "FALSE");
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

    list.find(&i, compareFunc, (void*)0);
    list.find(&i, compareFunc2);

    list.findItem(&i, compareFunc, (void*)0);
    list.findItem(&i, compareFunc2);

    list.findRef(0, compareFunc, (void*)0);
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

int main()
{
//     lst::List<A, CompareA> list(lst::CONTAINER_CLASS);
//     list.add(new A());
//     list.add(new A());
//     list.add(new B());
//
//     list.sort();
//     list.findRef(A());
//
//     for (int i = 0; i < list.count(); ++i)
//     {
//         printf("test %i", i);
//     }

    findTestCheckOverloads();

    findTest();

    bruteFindTest();

    return 0;
}
