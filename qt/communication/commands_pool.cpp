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

#include "qt/communication/commands_pool.h"
#include "qt/communication/commands_base.h"

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
