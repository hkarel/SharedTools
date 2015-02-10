
// Команда для сборки
// g++ -std=c++11 -ggdb3 -Wall -Wsign-compare -Weverything list_utest.cpp -o list_utest
// g++ -std=c++11 -ggdb3 -Wsign-compare  list_utest.cpp -o list_utest
//
// Опции компилятора
//  -fdiagnostics-show-option

#include <cstdio>
#include <iostream>
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

const char* fres(const lst::FindResult& fr) {
    return fr.success() ? "Found    " : "Not found";
};
const char* fsucc(const lst::FindResult& fr) {
    return fr.success() ? "OK" : "FAIL";
};
const char* ffail(const lst::FindResult& fr) {
    return fr.failed() ? "OK" : "FAIL";
};

bool findTest()
{
    lst::List<int> list;
    lst::FindResult fr;

//     auto fres = [] (const lst::FindResult& fr) {
//         return fr.success() ? "Found    " : "Not found";
//     };
//     auto fsucc = [] (const lst::FindResult& fr) {
//         return fr.success() ? "OK" : "FAIL";
//     };
//     auto ffail = [] (const lst::FindResult& fr) {
//         return fr.failed() ? "OK" : "FAIL";
//     };

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

    findTest();

    return 0;
}
