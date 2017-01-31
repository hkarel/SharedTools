#include "qt/logger/logger_operators.h"
#include "qt/communication/logger_operators.h"
#include "qt/communication/commands_pool.h"

namespace alog {

Line& operator<< (Line& line, const communication::CommandNameLog& cnl)
{
    if (line.toLogger())
    {
        QByteArray commandName = communication::commandsPool().commandName(cnl.command);
        if (!commandName.isEmpty())
            line << commandName << "/";
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
            line << commandName << "/";
        line << cnl.command;
    }
    return std::move(line);
}

} // namespace alog
