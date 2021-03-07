/* clang-format off */

#include <cstdio>
#include <iostream>
#include <functional>
#include <vector>

#include <QtTest>
#include "../list.h"

class ListTest : public QObject
{
    Q_OBJECT

private slots:
    // Проверка работы функции compressList()
    void compressList();

    // Функция проверяет корректное разрешение имен функций, проверка идет
    // на уровне компиляции.
    void findCheckOverloads();

    // Тест поиска по отсортированному списку
    void find();

    // Тест добавления новых элементов через функцию addInSort()
    // с последующим поиском.
    void addInSortFind();


    // Тест поиска по не отсортированному списку (поиск перебором)
    void bruteFind();
    void bruteFind2();

    // Проверка работы функции range()
    void checkRange();

    // Тест поиска по не уникальному отсортированному списку
    void notUniqueFind();

    // Тест поиска по не уникальному отсортированному списку. Причем сортировка
    // выполнена по двум полям и оба не уникальные.
    // Требования к поиску: искомое значение должно быть >= поля1 и <= поля2.
    // Так же нужно вернуть граничные значения удовлетворяющие условию.
    void notUniqueStructFind();
};

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

void printElements(const char* capture,  lst::List<int>& list)
{
    QDebug output = qInfo();
    output << capture;
    for (int i = 0; i < list.count(); ++i)
    {
        if (list.item(i))
            output << list[i] << ",";
        else
            output << "null ,";
    }
}

void ListTest::compressList()
{
    lst::List<int> list;

    qInfo("Test for 3 elements (1,2,3)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    qInfo("  Remove elements 1 without compression list");
    list.remove(0, lst::CompressList::No);
    QCOMPARE(list.count(), 3);

    printElements("  List before compression : ", list);
    list.compressList();
    QCOMPARE(list.count(), 2);

    printElements("  List after compression : ", list);
    QVERIFY(list.count() == 2);

    QCOMPARE(list[0], 2);
    QCOMPARE(list[1], 3);

    //---
    qInfo("Test for 3 elements (1,2,3)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    qInfo("  Remove elements 2 without compression list");
    list.remove(1, lst::CompressList::No);
    QCOMPARE(list.count(), 3);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 2);

    QCOMPARE(list[0], 1);
    QCOMPARE(list[1], 3);

    //---
    qInfo("Test for 3 elements (1,2,3)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);

    qInfo("  Remove elements 3 without compression list");
    list.remove(2, lst::CompressList::No);
    QCOMPARE(list.count(), 3);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 2);

    QCOMPARE(list[0], 1);
    QCOMPARE(list[1], 2);

    //---
    qInfo("Test for 4 elements (1,2,3,4)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    qInfo("  Remove elements 1,2 without compression list");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);
    QCOMPARE(list.count(), 4);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 2);

    QCOMPARE(list[0], 3);
    QCOMPARE(list[1], 4);

    //---
    qInfo("Test for 4 elements (1,2,3,4)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    qInfo("  Remove elements 3,4 without compression list");
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);
    QCOMPARE(list.count(), 4);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 2);

    QCOMPARE(list[0], 1);
    QCOMPARE(list[1], 2);

    //---
    qInfo("Test for 4 elements (1,2,3,4)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    qInfo("  Remove elements 1,2,3 without compression list");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);
    QCOMPARE(list.count(), 4);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 1);

    QCOMPARE(list[0], 4);

    //---
    qInfo("Test for 4 elements (1,2,3,4)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    qInfo("  Remove elements 2,3,4 without compression list");
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);
    QCOMPARE(list.count(), 4);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 1);

    QCOMPARE(list[0], 1);

    //---
    qInfo("Test for 4 elements (1,2,3,4)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);

    qInfo("  Remove elements 1,2,3,4 without compression list");
    list.remove(0, lst::CompressList::No);
    list.remove(1, lst::CompressList::No);
    list.remove(2, lst::CompressList::No);
    list.remove(3, lst::CompressList::No);
    QCOMPARE(list.count(), 4);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 0);

    //---
    qInfo("Test for 15 elements (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15)");
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

    qInfo("  Remove elements 1,3,5,6,9,10,11,13,14,15 without compression list");
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
    QCOMPARE(list.count(), 15);

    printElements("  List before compression : ", list);
    list.compressList();
    printElements("  List after compression : ", list);
    QCOMPARE(list.count(), 5);

    QCOMPARE(list[0], 2);
    QCOMPARE(list[1], 4);
    QCOMPARE(list[2], 7);
    QCOMPARE(list[3], 8);
    QCOMPARE(list[4], 12);
}

void ListTest::findCheckOverloads()
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

void ListTest::find()
{
    lst::List<int> list;
    lst::FindResult fr;

    //---
    qInfo("Test ascend, 0 elements");
    list.clear();
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    //---
    qInfo("Test ascend, 1 elements (1)");
    list.clear();
    list.addCopy(1);
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 1);

    //---
    qInfo("Test ascend, 2 elements (1,2)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 2);

    //---
    qInfo("Test ascend, 3 elements (1,2,3)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    //---
    qInfo("Test ascend, 4 elements (1,2,3,5)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(5);
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(4);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(5);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 4);

    //---
    qInfo("Test descend, 0 elements");
    list.clear();
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    //---
    qInfo("Test descend, 1 elements (1)");
    list.clear();
    list.addCopy(1);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 1);

    //---
    qInfo("Test descend, 2 elements (2,1)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 2);

    //---
    qInfo("Test descend, 3 elements (3,2,1)");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    //---
    qInfo("Test descend, 4 elements (5,4,3,1)");
    list.clear();
    list.addCopy(1);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(5);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(4);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 4);

}

void ListTest::addInSortFind()
{
    lst::List<int> list;
    lst::FindResult fr;

    //---
    qInfo("Test ascend, 0 elements");
    list.sort();

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);


    //---
    qInfo("Test ascend, 1 elements (1)");
    fr = list.findRef(1);
    list.addCopyInSort(1, fr);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 1);

    //---
    qInfo("Test ascend, 2 elements (1,2)");
    fr = list.findRef(2);
    list.addCopyInSort(2, fr);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 2);

    //---
    qInfo("Test ascend, 3 elements (1,2,3)");
    fr = list.findRef(3);
    list.addCopyInSort(3, fr);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(5);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    //---
    qInfo("Test ascend, 4 elements (1,2,3,5)");
    fr = list.findRef(5);
    list.addCopyInSort(5, fr);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(4);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(5);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 4);

    //---
    qInfo("Test descend, 0 elements");
    list.clear();
    list.sort(lst::SortMode::Down);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    //---
    qInfo("Test descend, 1 elements (1)");
    fr = list.findRef(1);
    list.addCopyInSort(1, fr);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 1);

    //---
    qInfo("Test descend, 2 elements (2,1)");
    fr = list.findRef(2);
    list.addCopyInSort(2, fr);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 2);

    //---
    qInfo("Test descend, 3 elements (3,2,1)");
    fr = list.findRef(3);
    list.addCopyInSort(3, fr);

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    //---
    qInfo("Test descend, 4 elements (5,4,3,1)");
    list.addCopy(5);
    list.sort(lst::SortMode::Down);

    fr = list.findRef(4);
    list.addCopyInSort(4, fr);

    list.removeCond([](int* i){return *i == 2;});

    fr = list.findRef(6);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(5);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 0);

    fr = list.findRef(4);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(3);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 2);

    fr = list.findRef(2);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 3);

    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 4);
}

template<typename ListT> void bruteFindT(const ListT& list)
{
    int i;
    int* ptr;
    lst::FindResult fr;

    //---
    qInfo("  Find use function compareFunc");
    i = 0;
    fr = lst::find(&i, list, compareFunc);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = lst::findRef(2, list, compareFunc);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = lst::findRef(5, list, compareFunc);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = lst::findRef(7, list, compareFunc);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = lst::findItem(&i, list, compareFunc);
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use functor CompareFunctor");
    CompareFunctor compare_functor;
    i = 0;
    fr = lst::find(&i, list, compare_functor);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = lst::findRef(2, list, compare_functor);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = lst::findRef(5, list, compare_functor);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = lst::findRef(7, list, compare_functor);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = lst::findItem(&i, list, compare_functor);
    QVERIFY(ptr != 0);

    void* exp_param = 0;
    auto lambda_find = [&i, exp_param] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    //---
    qInfo("  Find use lambda");
    i = 0;
    fr = lst::find(list, lambda_find);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 2;
    fr = lst::find(list, lambda_find);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    i = 5;
    fr = lst::find(list, lambda_find);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    i = 7;
    fr = lst::find(list, lambda_find);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = lst::findItem(list, lambda_find);
    QVERIFY(ptr != 0);
}

void ListTest::bruteFind()
{
    std::vector<int> vec = {1, 2, 3, 4, 5};
    qInfo("Find in std::vector: 1,2,3,4,5");

    bruteFindT(vec);

    lst::List<int> list;
    qInfo("Find in lst::List: 1,2,3,4,5");
    list.clear();
    list.addCopy(1);
    list.addCopy(2);
    list.addCopy(3);
    list.addCopy(4);
    list.addCopy(5);

    bruteFindT(list);
}

void ListTest::bruteFind2()
{
    lst::List<int> list;

    qInfo("Find in lst::List: 1,2,3,4,5");
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

    //---
    qInfo("  Find use function compareFunc");
    i = 0;
    fr = list.find(&i, compareFunc);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = list.findRef(2, compareFunc);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(5, compareFunc);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = list.findRef(7, compareFunc);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItem(&i, compareFunc);
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use functor CompareFunctor");
    i = 0;
    fr = list.find(&i, compare_functor);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = list.findRef(2, compare_functor);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(5, compare_functor);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = list.findRef(7, compare_functor);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItem(&i, compare_functor);
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use lambda");
    i = 0;
    fr = list.findL(lambda_find);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 2;
    fr = list.findL(lambda_find);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    i = 5;
    fr = list.findL(lambda_find);
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    i = 7;
    fr = list.findL(lambda_find);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItemL(lambda_find);
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use function compareFunc (with BruteForce flag)");
    i = 0;
    fr = list.find(&i, compareFunc, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = list.findRef(2, compareFunc, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(5, compareFunc, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = list.findRef(7, compareFunc, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItem(&i, compareFunc, {lst::BruteForce::Yes});
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use functor CompareFunctor (with BruteForce flag)");
    i = 0;
    fr = list.find(&i, compare_functor, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    fr = list.findRef(2, compare_functor, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    fr = list.findRef(5, compare_functor, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    fr = list.findRef(7, compare_functor, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItem(&i, compare_functor, {lst::BruteForce::Yes});
    QVERIFY(ptr != 0);

    //---
    qInfo("  Find use lambda (with BruteForce flag)");
    i = 0;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 2;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 1);

    i = 5;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), true);
    QCOMPARE(fr.index(), 4);

    i = 7;
    fr = list.findL(lambda_find, {lst::BruteForce::Yes});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 5);

    i = 4;
    ptr = list.findItemL(lambda_find, {lst::BruteForce::Yes});
    QVERIFY(ptr != 0);
}

void ListTest::checkRange()
{
    lst::FindResult fr;
    lst::FindResultRange frr;
    lst::List<int> list;
    int index, i;

    auto lambda_find = [&i] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    qInfo("Test for empty list");
    lst::List<int>::RangeType range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());
    for (int* val : range)
    {
        (void) val;
        QVERIFY2(false, "Not empty list");
    }

    qInfo("Test for 10 elements (1,1,2,2,2,2,4,5,5,5)");
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
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());
    for (int* val : range)
    {
        (void) val;
        QVERIFY2(false, "Not empty list");
    }

    // === Диапазон для числа 1 ===
    i = 1;
    fr = list.findRef(i);
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    QCOMPARE(index, 0);

    index = list.indexOf(*(range.end() - 1));
    QCOMPARE(index, 1);

    // === Диапазон для числа 2 ===
    i = 2;
    fr = list.findRef(i);
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    QCOMPARE(index, 2);

    index = list.indexOf(*(range.end() - 1));
    QCOMPARE(index, 5);

    // === Диапазон для числа 4 ===
    i = 4;
    fr = list.findRef(i);
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    index = list.indexOf(*range.begin());
    QCOMPARE(index, 6);

    index = list.indexOf(*(range.end() - 1));
    QCOMPARE(index, 6);

    // === Диапазон для числа 5 ===
    i = 5;
    fr = list.findRef(i);
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   == list.end());

    index = list.indexOf(*range.begin());
    QCOMPARE(index, 7);

    index = list.indexOf(*(range.end() - 1));
    QCOMPARE(index, 9);

    // === Диапазон для числа 7 ===
    i = 7;
    fr = list.findRef(i);
    frr = lst::rangeFindResultL(list, lambda_find, fr);
    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());
    for (int* val : range)
    {
        (void) val;
        QVERIFY2(false, "Not empty list");
    }
}

void notUniqueFind_(const lst::List<int>& list, int i0, int i11, int i12, int i21, int i22, int i3, int i4, int i51, int i52, int i7)
{
    lst::FindResult fr;
    lst::FindResult fr_first;
    lst::FindResult fr_last;
    int i;

    auto lambda_find = [&i] (const int* item) -> int
    {
        return LIST_COMPARE_ITEM(i, *item);
    };

    // === Поиск числа 0 ===
    fr = list.findRef(0);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), i0);

    // Тест для lambda функций
    i = 0; // для lambda_find
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i0);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i0);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i0);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i0);

    // === Поиск числа 1 ===
    fr = list.findRef(1);
    QCOMPARE(fr.success(), true);

    // Тест для lambda функций
    i = 1;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i11);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i12);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i11);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i12);

    // === Поиск числа 2 ===
    fr = list.findRef(2);
    QCOMPARE(fr.success(), true);

    // Тест для lambda функций
    i = 2;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i21);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i22);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i21);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i22);

    // === Поиск числа 3 ===
    fr = list.findRef(3);
    QCOMPARE(fr.success(), false);

    // Тест для lambda функций
    i = 3;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i3);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i3);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i3);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i3);

    // === Поиск числа 4 ===
    fr = list.findRef(4);
    QCOMPARE(fr.success(), true);

    // Тест для lambda функций
    i = 4;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i4);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i4);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i4);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i4);

    // === Поиск числа 5 ===
    fr = list.findRef(5);
    QCOMPARE(fr.success(), true);

    // Тест для lambda функций
    i = 5;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i51);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i52);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), true);
    QCOMPARE(fr_first.index(), i51);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), true);
    QCOMPARE(fr_last.index(), i52);

    // === Поиск числа 7 ===
    fr = list.findRef(7);
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), i7);

    // Тест для lambda функций
    i = 7;
    fr_first = lst::firstFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i7);

    fr_last = lst::lastFindResultL(list, lambda_find, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i7);

    // Тест для compare
    fr_first = lst::firstFindResult(list, compareFunc, fr);
    QCOMPARE(fr_first.success(), false);
    QCOMPARE(fr_first.index(), i7);

    fr_last = lst::lastFindResult(list, compareFunc, fr);
    QCOMPARE(fr_last.success(), false);
    QCOMPARE(fr_last.index(), i7);
}

void ListTest::notUniqueFind()
{
    lst::List<int> list;

    qInfo("Test ascend, 10 elements (1,1,2,2,2,2,4,5,5,5)");
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

    //                   i0  i11 i12 i21 i22 i3 i4 i51 i52 i7
    notUniqueFind_(list, 0,  0,  1,  2,  5,  6, 6, 7,  9,  10);

    qInfo("Test descend, 10 elements (5,5,5,4,2,2,2,2,1,1)");
    list.sort(lst::SortMode::Down);

    //                   i0  i11 i12 i21 i22 i3 i4 i51 i52 i7
    notUniqueFind_(list, 10, 8,  9,  4,  7,  4, 3, 0,  2,  0);
}

struct SortTwoVal
{
    int val1;
    int val2;
    SortTwoVal() {}
    SortTwoVal(int v1, int v2) : val1(v1), val2(v2) {}

    struct Compare
    {
        int operator() (const SortTwoVal* item1, const SortTwoVal* item2) const
        {
            LIST_COMPARE_MULTI_ITEM(item1->val1,  item2->val1)
            return LIST_COMPARE_ITEM(item2->val2, item1->val2);
        }
    };

};

void ListTest::notUniqueStructFind()
{
    lst::FindResult fr;
    lst::FindResultRange frr;

    typedef lst::List<SortTwoVal, SortTwoVal::Compare> ListType;
    ListType list;

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
    v = list.add(); v->val1 = 6; v->val2 = 10;
    v = list.add(); v->val1 = 6; v->val2 = 15;
    v = list.add(); v->val1 = 6; v->val2 = 99;

    list.sort(lst::SortMode::Up);
    qInfo("List of SortTwoVal was sorted UP and DOWN");
    for (SortTwoVal* v : list)
        qInfo() << "  val1:" << v->val1 << " val2:" << v->val2;

    // Поиск пары 0,5
    fr = list.findRef(SortTwoVal{0, 5});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    ListType::RangeType range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());
    for (SortTwoVal* val : range)
    {
        (void) val;
        QVERIFY2(false, "Not empty list");
    }

    // Поиск пары 1,4
    fr = list.findRef(SortTwoVal{1, 4});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 1);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 3,7
    fr = list.findRef(SortTwoVal{3, 7});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 4);
    QCOMPARE(frr.last.index(),  5);

    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    // Поиск пары 4,8
    fr = list.findRef(SortTwoVal{4, 8});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 6);
    QCOMPARE(frr.last.index(),  6);

    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    // Поиск пары 6,10
    fr = list.findRef(SortTwoVal{6, 10});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 10);
    QCOMPARE(frr.last.index(),  11);

    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 6,6
    fr = list.findRef(SortTwoVal{6, 6});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 12);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 12);
    QCOMPARE(frr.last.index(),  12);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 7,6
    fr = list.findRef(SortTwoVal{7, 6});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 12);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 12);
    QCOMPARE(frr.last.index(),  12);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());


    //---
    list.sort(lst::SortMode::Down);
    qInfo("List of SortTwoVal was sorted DOWN and UP");
    for (SortTwoVal* v : list)
        qInfo() << "  val1:" << v->val1 << " val2:" << v->val2;

    // Поиск пары 7,6
    fr = list.findRef(SortTwoVal{7, 6});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 0);
    QCOMPARE(frr.last.index(),  0);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 6,6
    fr = list.findRef(SortTwoVal{6, 6});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 0);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 0);
    QCOMPARE(frr.last.index(),  0);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 6,10
    fr = list.findRef(SortTwoVal{6, 10});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 0);
    QCOMPARE(frr.last.index(),  1);

    range = list.range(frr);
    QVERIFY(range.begin() == list.begin());
    QVERIFY(range.end()   != list.end());

    // Поиск пары 4,8
    fr = list.findRef(SortTwoVal{4, 8});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 5);
    QCOMPARE(frr.last.index(),  5);

    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    // Поиск пары 3,7
    fr = list.findRef(SortTwoVal{3, 7});
    QCOMPARE(fr.success(), true);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 6);
    QCOMPARE(frr.last.index(),  7);

    range = list.range(frr);
    QVERIFY(range.begin() != list.end());
    QVERIFY(range.end()   != list.end());

    // Поиск пары 1,4
    fr = list.findRef(SortTwoVal{1, 4});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 11);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 11);
    QCOMPARE(frr.last.index(),  11);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());

    // Поиск пары 0,5
    fr = list.findRef(SortTwoVal{0, 5});
    QCOMPARE(fr.success(), false);
    QCOMPARE(fr.index(), 12);

    frr = lst::rangeFindResult(list, list.compare(), fr);
    QCOMPARE(frr.first.index(), 12);
    QCOMPARE(frr.last.index(),  12);

    range = list.range(frr);
    QVERIFY(range.begin() == list.end());
    QVERIFY(range.end()   == list.end());
    for (SortTwoVal* val : range)
    {
        (void) val;
        QVERIFY2(false, "Not empty list");
    }

}


QTEST_MAIN(ListTest)
#include "list_qtest.moc"
