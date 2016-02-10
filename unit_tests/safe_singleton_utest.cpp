/* clang-format off */
/*****************************************************************************
  Тест проверяет корректность работы функции safe_singleton(). Неудачным тест
  будет считаться в том случае если в разных потоках будут возвращены разные
  адреса для создаваемой структуры DummyStruct. Это будет означать, что функ-
  ция safe_singleton() создала несколько экземпляров DummyStruct.
*****************************************************************************/


// g++ -std=c++11 -O2 -Wall -pthread safe_singleton_utest.cpp -o safe_singleton_utest

#include <cstdio>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../safe_singleton.h"

using namespace std;

struct DummyStruct
{
    int i;
    DummyStruct()
    {
        // Имитируем затраты на выполнение конструктора
        this_thread::sleep_for(chrono::milliseconds(10));
    }
};

constexpr int thread_count = 50;
vector<DummyStruct*> thread_ptrs;
vector<thread::id> thread_ids;

int main()
{
    thread_ptrs.reserve(thread_count);
    thread_ids.reserve(thread_count);
    vector<thread> threads;
    for(int i = 0; i < thread_count; ++i){
        threads.push_back(std::thread([i](){
            thread_ptrs[i] = & ::safe_singleton<DummyStruct>();
            thread_ids[i] = this_thread::get_id();
        }));
    }

    for(auto& thread : threads)
        thread.join();

    DummyStruct* f = thread_ptrs[0];
    for(int i = 0; i < thread_count; ++i)
    {
        if (thread_ptrs[i] != f)
        {
            cout << "TEST FAILED\n";
            return 1;
        }
        cout << thread_ids[i] << "  " << (void*)thread_ptrs[i] << std::endl;
    }

    cout << "TEST PASSED\n";
    return 0;
}

