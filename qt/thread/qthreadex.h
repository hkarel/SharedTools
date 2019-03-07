/*****************************************************************************
  The MIT License

  Copyright © 2015 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

#include <QMutex>
#include <QThread>
#include <atomic>

/**
  Класс QThredEx расширяет функционал QThred. Основное назначение механизмов
  добавленных в данном классе - это корректное прерывание выполнение метода
  run(). В класс добавлены два метода: stop() и threadStop(), а так же пере-
  определен метод start().
*/
class QThreadEx : public QThread
{
public:
    explicit QThreadEx(QObject * parent = 0);

    // Возвращает TRUE если была вызвана функция stop(), сбрасывается в FALSE
    // после вызова start() или после окончания вызова функции обработчика потока.
    // Эта функция в основном используется в классах наследниках, в методе run(),
    // для проверки необходимости завершения работы потока.
    bool threadStop() const noexcept;

public slots:
    // Запуск потока
    void start(Priority priority = InheritPriority);

    // Выставляет флаг _threadStop в TRUE и ожидает завершения работы потока.
    // Возвращает FALSE в случае истечения таймаута, в ином случае возвращает
    // TRUE.
    bool stop(unsigned long time = ULONG_MAX);

private slots:
    void onStarted();  // Обработчик сигнала QThread::started()
    void onFinished(); // Обработчик сигнала QThread::finished()

protected:
    virtual void startImpl(Priority priority);
    virtual bool stopImpl(unsigned long time);

    // Приостанавливает поток на заданное количество секунд. При этом если будет
    // вызвана функция прерывания работы потока threadStop(), то ожидание немед-
    // ленно прерывается.  Предполагается, что эта функция будет использоваться
    // только внутри run().
    void sleep(unsigned long timeout);

    // Вызывается из метода stopImpl() в тот момент когда параметр threadStop
    // уже установлен в TRUE, но поток еще не завершил свою работу. Основное
    // назначение данной функции - вывод потока из состояния ожидания.
    virtual void threadStopEstablished() {}

private:
    Q_OBJECT
    volatile bool _threadStop = {true};
    std::atomic_bool _waitThreadStart = {false};

    // Используется для исключения одновременного вызова функций start()/stop()
    QMutex _startStopLock;
};

#define CHECK_QTHREADEX_STOP \
    if (threadStop()) { \
        log_debug_m << "Thread stop command received"; \
        break; \
    }
