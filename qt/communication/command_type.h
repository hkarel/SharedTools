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

// Статус выполнения/обработки команды. Используется в команде с типом Responce
// для того чтобы уведомить другую сторону о статусе выполнения команды с типом
// Request.
enum class ExecStatus : quint32
{
    Unknown = 0,
    Success = 1, // Команда была обработана успешно и содержит корректные
                 // ответные данные.
    Failed  = 2, // Команда не была обработана успешно, но результат
                 // не является ошибкой.
                 // В данном случае сообщение (Message) будет содержать данные
                 // в формате communication::data::MessageFailed.
    Error   = 3  // При обработке команды произошла ошибка, и в качестве
                 // ответа отправляется сообщения с описанием причины ошибки.
                 // В данном случае сообщение (Message) будет содержать данные
                 // в формате communication::data::MessageError.
};

} // namespace command
} // namespace communication