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
namespace command {

Pool& pool()
{
    return ::safe_singleton<Pool>();
}

bool Pool::checkUnique() const
{
    for (auto it = _map.constBegin(); it != _map.constEnd(); ++it)
        if (it.value().size() != 1)
        {
            alog::Line logLine =
                log_error << "Identifier " << it.key()
                          << " was be assigned to several commands:";
            for (const CommandTraits& t : it.value())
                logLine << " name=" << t.commandName << ", multiproc=" << t.multiproc << ";";
            return false;
        }
    return true;
}

void Pool::add(QUuidEx* command, const char* commandName, bool multiproc)
{
    QSet<CommandTraits>& set = _map[*command];
    set.insert({commandName, multiproc});
}

QVector<QUuidEx> Pool::commands() const
{
    QVector<QUuidEx> commands;
    for (auto it = _map.constBegin(); it != _map.constEnd(); ++it)
        commands.append(it.key());
    return std::move(commands);
}

const char* Pool::commandName(const QUuidEx& command) const
{
    const auto it = _map.constFind(command);
    if (it != _map.constEnd())
    {
        const QSet<CommandTraits>& set = *it;
        return (*set.constBegin()).commandName;
    }
    return "";
}

quint32 Pool::commandExists(const QUuidEx& command) const
{
    //return (_map.constFind(command) != _map.constEnd());

    auto it = _map.constFind(command);
    if (it == _map.constEnd())
        return 0;

    const QSet<CommandTraits>& set = *it;
    return (*set.constBegin()).multiproc ? 2 : 1;
}

bool Pool::commandIsMultiproc(const QUuidEx& command) const
{
    return (commandExists(command) == 2);
}

Pool::Registry::Registry(const char* uuidStr, const char* commandName, bool multiproc)
    : QUuidEx(uuidStr)
{
    pool().add(this, commandName, multiproc);
}

Pool::CommandTraits::CommandTraits(const char* commandName, bool multiproc)
    : commandName(commandName), multiproc(multiproc)
{}

bool Pool::CommandTraits::operator== (const CommandTraits& ct) const
{
    return (strcmp(commandName, ct.commandName) == 0) && (multiproc == ct.multiproc);
}

uint qHash(const Pool::CommandTraits& ct)
{
    const QByteArray& commandName =
        QByteArray::fromRawData(ct.commandName, strlen(ct.commandName));
    return qHash(commandName) + uint(ct.multiproc);
}

} // namespace command
} // namespace communication
