/*****************************************************************************
  В модуле представлен список идентификаторов команд для коммуникации между
  клиентской и серверной частями приложения.
  Здесь представлен базовый список команд общий для всех приложений. Так же
  модуль содержит структуры данных соответствующие определенным командам.
  Фактически структуры описанные в модуле являются неким подобием публичного
  интерфейса. И хотя для бинарного протокола обмена данных публичный интерфейс
  не является обязательным, его наличие облегчит ориентацию в передаваемых
  структурах данных.

  Требование надежности коммуникаций: однажды назначенный идентификатор коман-
  ды не должен более меняться.
*****************************************************************************/

#pragma once

#include "qt/quuidex.h"
#include "qt/communication/message.h"
#include "qt/communication/communication_bserialize.h"
#include "qt/version/version_number.h"

#include <QtCore>
#include <QHostAddress>

namespace communication {

//------------------------- Список базовых команд ----------------------------
namespace command {

// Идентификатор неизвестной команды
extern const QUuidEx Unknown;

// Идентификатор сообщения об ошибке
extern const QUuidEx Error;

// Запрос информации о совместимости. При подключении клиент и сервер
// отправляют друг другу информацию о совместимости.
extern const QUuidEx CompatibleInfo;

// Требование закрыть TCP-соединение. Эта команда работает следующим образом:
// сторона, которая хочет закрыть соединение отправляет это сообщение с инфор-
// мацией о причине необходимости закрыть соединение. Принимающая сторона
// записывает эту информацию в свой лог (или использует иным образом), затем
// отправляет обратное пустое сообщение. После того, как ответное сообщение
// получено - TCP-соединение может быть разорвано. Такое поведение реализовано
// для того чтобы сторона с которой разрывают соединение имела информацию о
// причинах разрыва.
extern const QUuidEx CloseConnection;

} // namespace command


//------------------------ Список базовых структур ---------------------------
namespace data {

/**
  Структура Data используется для ассоциации целевой структуры данных с опреде-
  ленной командой. Она позволяют связать идентификатор команды со структурой
  данных, а так же задать направления передачи данных.
  В дальнейшем эти параметры будут использоваться для проверки возможности
  преобразования Message-сообщения в конкретную структуру.
*/
template<
    const QUuidEx* Command,
    Message::Type MessageType1,
    Message::Type MessageType2 = Message::Type::Unknown,
    Message::Type MessageType3 = Message::Type::Unknown
>
struct Data
{
    // Идентификатор команды
    static constexpr const QUuidEx& command() {return *Command;}

    // Статус состояния данных. Выставляется в TRUE когда данные были корректно
    // прочитаны из сообщения, во всех остальных случаях флаг должен быть FALSE.
    bool isValid = {false};

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Command
    static constexpr bool forCommandMessage() {
        return (MessageType1 == Message::Type::Command
                || MessageType2 == Message::Type::Command
                || MessageType3 == Message::Type::Command);
    }

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Answer
    static constexpr bool forAnswerMessage() {
        return (MessageType1 == Message::Type::Answer
                || MessageType2 == Message::Type::Answer
                || MessageType3 == Message::Type::Answer);
    }

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Event
    static constexpr bool forEventMessage() {
        return (MessageType1 == Message::Type::Event
                || MessageType2 == Message::Type::Event
                || MessageType3 == Message::Type::Event);
    }
};

/**
  Структура содержит информацию об ошибке произошедшей в процессе обработки
  сообщения. Данная структура  отправляется  вызывающей стороне как Answer-
  сообщение, при этом статус обработки команды Message::ExecStatus
  равен Error.
  См. так же описание enum Message::ExecStatus
*/
struct MessageError
{
    QString description;  // Описание ошибки.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Сообщение о неудачной обработке сообщения, которое не является ошибкой. Такая
  ситуация может возникнуть когда запрашиваемое  действие не может быть выполне-
  но в силу разных причин,  например когда  недостаточно  прав на запрашиваемое
  действие. Данная структура отправляется вызывающей стороне как Answer-сообще-
  ние, при этом статус обработки команды Message::ExecStatus равен Failed.
  См. так же описание enum Message::ExecStatus
*/
struct MessageFailed
{
    QString description;  // Описание неудачи.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Информационное сообщение о неизвестной команде
*/
struct Unknown : Data<&command::Unknown,
                       Message::Type::Command>
{
    QUuidEx      commandId; // Идентификатор неизвестной команды.
    QHostAddress address;   // Адрес хоста для которого команда неизвестна,
    quint16      port;      // порт,
    quint64      socketDescriptor; // идентификатор сокета.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Сообщение об ошибке возникшей на стороне сервера или клиента. Если по какой-
  либо причине невозможно передать сообщение при помощи MessageError, то
  используется эта структура, причем как самостоятельное сообщение.
*/
struct Error : Data<&command::Error,
                     Message::Type::Command>
{
    QUuidEx commandId;   // Идентификатор команды для которой произошла ошибка.
    QString description; // Описание ошибки.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Сообщение содержит информацию о совместимости команд и бинарного протокола
  сервера и клиента. Сразу после установки TCP-соединения сервер и клиент
  обмениваются этой информацией.
*/
struct CompatibleInfo : Data<&command::CompatibleInfo,
                              Message::Type::Command,
                              Message::Type::Answer>
{
    VersionNumber    version;               // Текущая версия программы
    VersionNumber    minCompatibleVersion;  // Минимально совместимая версия
                                            // по бинарному протоколу.
    QVector<QUuidEx> commands;              // Список поддерживаемых команд
                                            // клиентом/сервером.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Структура описана как пример оформления возвращаемых данных.
  На практике эта структура не используется.
*/
struct CompatibleInfo_Answer : Data<&command::CompatibleInfo,
                                     Message::Type::Answer>
{
    VersionNumber    version;               // Текущая версия программы
    VersionNumber    minCompatibleVersion;  // Минимально совместимая версия.
                                            // по бинарному протоколу.
    QVector<QUuidEx> commands;              // Список поддерживаемых команд
                                            // клиентом/сервером.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Структура содержит информацию о причинах закрытия TCP-соединения
*/
struct CloseConnection : Data<&command::CloseConnection,
                               Message::Type::Command>
{
    QString description;  // Описание причины закрытия соединения
    DECLARE_B_SERIALIZE_FUNC
};

} // namespace data
} // namespace communication


