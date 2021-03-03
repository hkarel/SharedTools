/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*****************************************************************************/

#pragma once

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
    ThreadBase() = default;
    virtual ~ThreadBase() = default;

    // Запускает поток.
    void start();

    // Останавливает поток. Если wait == TRUE, то ждем окончания работы потока.
    void stop(bool wait = true);

    // Возвращает состояние потока остановлен/запущен
    //bool joinable() const noexcept {return _thread.joinable();}

    // Возвращает TRUE если была вызвана функция stop(), сбрасывается в FALSE
    // после вызова start() или после окончания вызова функции обработчика потока.
    // Эта функция в основном используется в классах наследниках, в методе run(),
    // для проверки необходимости завершения работы потока.
    bool threadStop() const noexcept;

    // Возвращает TRUE сразу после того как была вызвана функция обработчика
    // потока, после выхода из функции обработчика будет возвращаться FALSE.
    // Эта функция в основном используется в тех случаях, когда была вызвана
    // функция stop() в асинхронном режиме и вызывающей стороне нужно убедиться
    // что поток уже остановил свою работу.
    bool threadRun() const noexcept;

    // Возвращает нативный идентификатор потока
    std::thread::native_handle_type nativeHandle() noexcept;

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

    volatile bool _threadRun  = {false};
    volatile bool _threadStop = {true};

    // Вспомогательная переменная используется для предотвращения вызова функ-
    // ции stop() сразу после вызова функции start(). В этой ситуации вызов
    // функции stop() может произойти до начала старта потока, как следствие
    // при вызове stop() не произойдет ожидание окончания потока.
    std::atomic_bool _waitThreadStart = {false};

    // *** Неудачная попытка завершить работу потока асинхронно ***
    //atomic_bool _waitThreadStop = {true};

    // Используется для исключения одновременного вызова функций start()/stop()
    std::mutex _startStopLock;

};

} //namespace trd
