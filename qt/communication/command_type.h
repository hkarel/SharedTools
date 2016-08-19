#pragma once
#include "qglobal.h"

namespace communication {
namespace command {

// Тип пересылаемой команды
enum class Type : quint32
{
    Unknown  = 0,
    Request  = 1, // Прямая команда (запрос на выполнение действия).
    Response = 2, // Ответ на команду Request.
    Event    = 3  // Данный тип команды похож на Request, но не предполагает
                  // получения ответа (Responce). Он используется для рассылки
                  // широковещательных сообщений о событиях.
};

} // namespace command
} // namespace communication
