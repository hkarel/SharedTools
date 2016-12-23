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
#include "qt/version/version_number.h"
#include "command_type.h"
#include "communication_bserialize.h"

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
  данных, а так же задать направления передачи данных (Request/Response).
  В дальнейшем эти параметры будут использоваться для проверки возможности
  преобразования Message-сообщения в конкретную структуру.
*/
template<
    const QUuidEx* Command,
    command::Type CommandType1,
    command::Type CommandType2 = command::Type::Unknown,
    command::Type CommandType3 = command::Type::Unknown
>
struct Data
{
    // Идентификатор команды
    static constexpr const QUuidEx& command() {return *Command;}

    // Статус состояния данных. Выставляется в TRUE когда данные были корректно
    // прочитаны из сообщения, во всех остальных случаях флаг должен быть FALSE.
    bool isValid = {false};

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Request
    static constexpr bool forRequest() {
        return (CommandType1 == command::Type::Request
                || CommandType2 == command::Type::Request
                || CommandType3 == command::Type::Request);
    }

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Response
    static constexpr bool forResponse() {
        return (CommandType1 == command::Type::Response
                || CommandType2 == command::Type::Response
                || CommandType3 == command::Type::Response);
    }

    // Признак, что данные могут быть использованы для Message-сообщений
    // с типом Event
    static constexpr bool forEvent() {
        return (CommandType1 == command::Type::Event
                || CommandType2 == command::Type::Event
                || CommandType3 == command::Type::Event);
    }
};

/**
  Структура содержит информацию об ошибке произошедшей в процессе обработки
  Request-сообщения. Данная структура  отправляется  вызывающей стороне
  как Response-сообщение, при этом статус обработки команды Message::ExecStatus
  равен Error.
  См. так же описание enum Message::ExecStatus
*/
struct MessageError
{
    QString description;  // Описание ошибки.
    DECLARE_B_SERIALIZE_FUNC
};

/**
  Сообщение о неудачной обработке Request-сообщения, которое не является ошиб-
  кой. Такая ситуация может возникнуть когда запрашиваемое  действие не может
  быть выполнено в силу разных причин,  например когда  недостаточно  прав на
  запрашиваемое действие. Данная структура отправляется вызывающей стороне как
  Response-сообщение, при этом статус обработки команды Message::ExecStatus
  равен Failed.
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
                       command::Type::Request>
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
                     command::Type::Request>
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
                              command::Type::Request,
                              command::Type::Response>
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
struct CompatibleInfo_Response : Data<&command::CompatibleInfo,
                                       command::Type::Response>
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
                               command::Type::Request>
{
    QString description;  // Описание причины закрытия соединения
    DECLARE_B_SERIALIZE_FUNC
};

} // namespace data
} // namespace communication


