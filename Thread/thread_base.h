#pragma once

#include <atomic>
#include <thread>
#include <mutex>


namespace trd
{
using namespace std;

/**
  Примитивный базовый класс для работы с потоками
*/
class ThreadBase
{
public:
    ThreadBase();
    virtual ~ThreadBase();

    // Запускает поток.
    virtual void start();

    // Останавливает поток.
    virtual void stop();

    // Возвращает состояние потока остановлен/запущен
    bool joinable() const noexcept {return _thread.joinable();}

    // Возвращает TRUE если была вызвана функция stop(), после вызова start()
    // сбрасывается в FALSE. Эта функция в основном используется в классах
    // наследниках, в методе run(), для проверки необходимости завершения работы
    // потока.
    bool threadStop() const noexcept {return _threadStop;}

    // Возвращает нативный идентификатор потока
    thread::native_handle_type nativeHandle() noexcept {return _thread.native_handle();}

    //const thread thread_() const {return _thread;}
    //int priority();
    //int setPriority(int val);

protected:
    virtual void run() = 0;

private:
    void runImpl();

    thread _thread;
    volatile bool _threadStop = {true};

    // Вспомогательная переменная используется для предотвращения вызова функ-
    // ции stop() сразу после вызова функции start(). В этой ситуации вызов
    // функции stop() может произойти до начала старта потока, как следствие
    // при вызове stop() не произойдет ожидание окончания потока.
    atomic_bool _waitThreadStart = {false};

    // Используется для исключения одновременного вызова функций start()/stop()
    mutex _startStopLock;

};


} //namespace trd

