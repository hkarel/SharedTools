#pragma once

#include <string>
#include "thread_base.h"


namespace trd
{
using namespace std;

/**
  В классе реализован механизм повышения приоритета потока
*/
class ThreadUp : public ThreadBase
{
public:
    // Функция устанавливает тип и приоритет потока. В случае успешного выполне-
    // ния возвращает 0, иначе - код ошибки. Строковое описание кода ошибки можно
    // получить из функции priorityUpError().
    // Параметр schedulAlgorithm может принимать одно из следующих значений:
    // SCHED_OTHER, SCHED_FIFO, SCHED_RR (см. http://man.sourcentral.org/f17/ru/2+sched_setscheduler)
    // Значения параметра priority зависят от schedulAlgorithm.
    int priorityUp(int schedulAlgorithm, int priority);

    // Строковое описание ошибки
    string priorityUpError() const {return _priorityUpError;}

private:
    string _priorityUpError;
};


} //namespace trd


