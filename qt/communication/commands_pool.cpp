#include "commands_pool.h"
#include "commands_base.h"

#include "logger/logger.h"
#include "qt/logger/logger_operators.h"

namespace communication {

CommandsPool& commandsPool()
{
    return ::safe_singleton<CommandsPool>();
}

bool CommandsPool::checkUnique() const
{
    for (auto it = _map.constBegin(); it != _map.constEnd(); ++it)
        if (it.value().size() != 1)
        {
            alog::Line logLine =
                log_error << "Identifier " << it.key()
                          << " was be assigned to several commands: ";
            for (const QByteArray& b : it.value())
                logLine << b << ", ";
            return false;
        }
    return true;
}

void CommandsPool::add(QUuidEx* command, const char* commandName)
{
    QSet<QByteArray>& set = _map[*command];
    set.insert(commandName);
}

QVector<QUuidEx> CommandsPool::commands() const
{
    QVector<QUuidEx> commands;
    for (auto it = _map.constBegin(); it != _map.constEnd(); ++it)
        commands.append(it.key());
    return std::move(commands);
}

QByteArray CommandsPool::commandName(const QUuidEx& command) const
{
    const auto it = _map.constFind(command);
    if (it != _map.constEnd())
    {
        const QSet<QByteArray>& set = *it;
        if (!set.empty())
            return *set.constBegin();
    }
    return QByteArray();
}

bool CommandsPool::commandExists(const QUuidEx& command) const
{
    return (_map.constFind(command) != _map.constEnd());
}

CommandsPool::Registry::Registry(const char* uuidStr, const char* commandName)
    : QUuidEx(uuidStr)
{
    commandsPool().add(this, commandName);
}

} // namespace communication
