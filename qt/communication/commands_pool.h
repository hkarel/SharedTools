/*****************************************************************************
  В модуле реализован пул идентификаторов команд.

  Все функции в пуле НЕ потокозащищенные, несмотря на это к ним можно обра-
  щаться из разных потоков. Причина этого в том, что список идентификаторов
  заполняется единожды при старте программы и потом более не меняется.
*****************************************************************************/

#pragma once

#include "defmac.h"
#include "safe_singleton.h"
#include "qt/quuidex.h"
#include <QtCore>

namespace communication {

class CommandsPool
{
public:
    // Возвращает TRUE в случае если в пуле находятся только уникальные команды.
    // Если команды не уникальны в лог выводится сообщение об этом.
    bool checkUnique() const;

    // Добавляет команду в пул.
    void add(QUuidEx* command, const char* commandName);

    // Возвращает список идентификаторов команд.
    QVector<QUuidEx> commands() const;

    // Возвращает строковое наименование команды по ее идентификатору
    QByteArray commandName(const QUuidEx& command) const;

    // Возвращает TRUE если команда существует в пуле команд
    bool commandExists(const QUuidEx& command) const;

    // Используется для регистрации команд в пуле
    struct Registry : public QUuidEx
    {
        Registry(const char* uuidStr, const char* commandName);
    };

private:
    CommandsPool() = default;
    DISABLE_DEFAULT_COPY(CommandsPool)

private:
    // Параметр _map используется для проверки уникальности идентификаторов
    // команд. Если идентификатор окажется не уникальным, то QSet будет
    // содержать более одного значения.
    QMap<QUuidEx, QSet<QByteArray>> _map;

    template<typename T, int> friend T& ::safe_singleton();
};
CommandsPool& commandsPool();


} // namespace communication
