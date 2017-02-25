#include "qt/logger/logger_operators.h"
#include "qt/communication/logger_operators.h"
#include "qt/communication/commands_pool.h"
#include "qt/communication/host_point.h"

namespace alog {

Line& operator<< (Line&  line, const communication::HostPoint& hp)
{
    if (line.toLogger())
        line << hp.address() << ":" << hp.port();
    return line;
}

Line  operator<< (Line&& line, const communication::HostPoint& hp)
{
    if (line.toLogger())
        line << hp.address() << ":" << hp.port();
    return std::move(line);
}

Line& operator<< (Line& line, const communication::CommandNameLog& cnl)
{
    if (line.toLogger())
    {
        QByteArray commandName = communication::commandsPool().commandName(cnl.command);
        if (!commandName.isEmpty())
        {
            line << commandName;
            if (!cnl.onlyCommandName)
                line << "/";
        }
        if (commandName.isEmpty() || !cnl.onlyCommandName)
            line << cnl.command;
    }
    return line;
}

Line operator<< (Line&& line, const communication::CommandNameLog& cnl)
{
    if (line.toLogger())
    {
        QByteArray commandName = communication::commandsPool().commandName(cnl.command);
        if (!commandName.isEmpty())
        {
            line << commandName;
            if (!cnl.onlyCommandName)
                line << "/";
        }
        if (commandName.isEmpty() || !cnl.onlyCommandName)
            line << cnl.command;
    }
    return std::move(line);
}

} // namespace alog
