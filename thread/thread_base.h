/* clang-format off */
#pragma once

#ifndef NOEXCEPT
#  ifdef _MSC_VER
#    define NOEXCEPT
#  else
#    define NOEXCEPT noexcept
#  endif
#endif

#include <atomic>
#include <thread>
#include <mutex>

namespace trd {

/**
  Примитивный базовый класс для работы с потоками
*/
class ThreadBase
{
public:
    ThreadBase();
    virtual ~ThreadBase() = default;

    // Запускает поток.
    void start();

    // Останавливает поток. Если wait == TRUE, то ждем окончания работы потока.
    void stop(bool wait = true);

    // Возвращает состояние потока остановлен/запущен
    //bool joinable() const NOEXCEPT {return _thread.joinable();}

    // Возвращает TRUE если была вызвана функция stop(), сбрасывается в FALSE
    // после вызова start() или после окончания вызова функции обработчика потока.
    // Эта функция в основном используется в классах наследниках, в методе run(),
    // для проверки необходимости завершения работы потока.
    bool threadStop() const NOEXCEPT;

    // Возвращает TRUE сразу после того как была вызвана функция обработчика
    // потока, после выхода из функции обработчика будет возвращаться FALSE.
    // Эта функция в основном используется в тех случаях, когда была вызвана
    // функция stop() в асинхронном режиме и вызывающей стороне нужно убедиться
    // что поток уже остановил свою работу.
    bool threadRun() const NOEXCEPT;

    // Возвращает нативный идентификатор потока
    std::thread::native_handle_type nativeHandle() NOEXCEPT;

protected:
    virtual void startImpl();
    virtual void stopImpl(bool wait);
    virtual void run() = 0;

private:
    ThreadBase(ThreadBase&&) = delete;
    ThreadBase(const ThreadBase&) = delete;

    ThreadBase& operator= (ThreadBase&&) = delete;
    ThreadBase& operator= (const ThreadBase&) = delete;

    void runHandler();

private:
    // Для BSD-систем нужно явно указывать пространство 'std' при определении
    // типа 'thread'.
    std::thread _thread;

    volatile bool _threadRun;
    volatile bool _threadStop;

    // Вспомогательная переменная используется для предотвращения вызова функ-
    // ции stop() сразу после вызова функции start(). В этой ситуации вызов
    // функции stop() может произойти до начала старта потока, как следствие
    // при вызове stop() не произойдет ожидание окончания потока.
    std::atomic_bool _waitThreadStart;

    // *** Неудачная попытка завершить работу потока асинхронно ***
    //atomic_bool _waitThreadStop;

    // Используется для исключения одновременного вызова функций start()/stop()
    std::mutex _startStopLock;

};

} //namespace trd
