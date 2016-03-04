/*****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  В модуле реализован класс QThredEx, он расширяет функционал QThred.
  Основное назначение механизмов добавленных в данном классе - это корректное
  прерывание выполнение метода run().
  В класс добавлены два метода: stop() и threadStop(), а так же переопределен
  метод start().
*****************************************************************************/

#pragma once

#include <QMutex>
#include <QThread>


class QThreadEx : public QThread
{
    Q_OBJECT
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
    void stop(unsigned long time = ULONG_MAX);

private slots:
    void onFinished(); // Обработчик сигнала QThread::finished()
    void onStarted();  // Обработчик сигнала QThread::started()

protected:
    virtual void startImpl(Priority priority);
    virtual void stopImpl(unsigned long time);

    // Приостанавливает поток на заданное количество секунд. При этом если будет
    // вызвана функция прерывания работы потока threadStop(), то ожидание немед-
    // ленно прерывается.  Предполагается, что эта функция будет использоваться
    // только внутри run().
    void sleep(unsigned long timeout);

private:
    volatile bool _threadStop = {true};

    // Используется для исключения одновременного вызова функций start()/stop()
    QMutex _startStopLock;
};

