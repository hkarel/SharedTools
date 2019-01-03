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
  ---

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
#include "qt/communication/bserialize_space.h"
#include "qt/version/version_number.h"

#ifdef JSON_SERIALIZATION
#include "qt/communication/serialization/json.h"
#endif

#include <QtCore>
#include <QHostAddress>
#include <stdexcept>

namespace communication {

//------------------------- Список базовых команд ----------------------------
namespace command {

// Идентификатор неизвестной команды
extern const QUuidEx Unknown;

// Идентификатор сообщения об ошибке
extern const QUuidEx Error;

// Запрос информации о совместимости. При подключении клиент и сервер
// отправляют друг другу информацию о совместимости.
extern const QUuidEx ProtocolCompatible;

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

    // Фиктивные функции, необходимые для сборки проекта когда не используется
    // бинарная сериализация.
    bserial::RawVector toRaw() const {
        throw std::logic_error("Data::toRaw(): You must override this function "
                               "in the inherited structure");
    }
    void fromRaw(const bserial::RawVector&) {
        throw std::logic_error("Data::fromRaw(): You must override this function "
                               "in the inherited structure");
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
    qint32  code = {0};   // Код ошибки
    QString description;  // Описание ошибки (сериализуется в utf8)
    DECLARE_B_SERIALIZE_FUNC

#ifdef JSON_SERIALIZATION
    DECLARE_J_SERIALIZE_FUNC
#endif
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
    qint32  code = {0};   // Код неудачи
    QString description;  // Описание неудачи (сериализуется в utf8)
    DECLARE_B_SERIALIZE_FUNC

#ifdef JSON_SERIALIZATION
    DECLARE_J_SERIALIZE_FUNC
#endif
};

/**
  Информационное сообщение о неизвестной команде
*/
struct Unknown : Data<&command::Unknown,
                       Message::Type::Command>
{
    QUuidEx       commandId;        // Идентификатор неизвестной команды.
    SocketType    socketType;       // Тип сокета для которого было создано
                                    // сообщение.
    quint64       socketDescriptor; // Идентификатор сокета.
    QString       socketName;       // Наименование локального сокета,
                                    // (сериализуется в utf8).
    QHostAddress  address;          // Адрес и порт хоста для которого
    quint16       port;             // команда неизвестна.
    DECLARE_B_SERIALIZE_FUNC

#ifdef JSON_SERIALIZATION
    DECLARE_J_SERIALIZE_FUNC
#endif
};

/**
  Сообщение об ошибке возникшей на стороне сервера или клиента. Если по какой-
  либо причине невозможно передать сообщение при помощи MessageError, то
  используется эта структура, причем как самостоятельное сообщение.
*/
struct Error : Data<&command::Error,
                     Message::Type::Command>
{
    QUuidEx commandId;   // Идентификатор команды для которой произошла ошибка
    qint32  code = {0};  // Код ошибки
    QString description; // Описание ошибки (сериализуется в utf8)
    DECLARE_B_SERIALIZE_FUNC

#ifdef JSON_SERIALIZATION
    DECLARE_J_SERIALIZE_FUNC
#endif
};

/**
  Структура содержит информацию о причинах закрытия TCP-соединения
*/
struct CloseConnection : Data<&command::CloseConnection,
                               Message::Type::Command>
{
    qint32  code = {0};   // Код причины. Нулевой код соответствует
                          // несовместимости версий протоколов.
    QString description;  // Описание причины закрытия соединения,
                          // (сериализуется в utf8)
    DECLARE_B_SERIALIZE_FUNC

#ifdef JSON_SERIALIZATION
    DECLARE_J_SERIALIZE_FUNC
#endif
};

//----------------------- Implementation JSerialize --------------------------

#ifdef JSON_SERIALIZATION
template <typename Packer>
Packer& MessageError::jserialize(Packer& p)
{
    p.startObject();
    p.member("code")        & code;
    p.member("description") & description;
    return p.endObject();
}

template <typename Packer>
Packer& MessageFailed::jserialize(Packer& p)
{
    p.startObject();
    p.member("code")        & code;
    p.member("description") & description;
    return p.endObject();
}

template <typename Packer>
Packer& Unknown::jserialize(Packer& p)
{
    p.startObject();
    p.member("commandId")        & commandId;
    p.member("socketType")       & socketType;
    p.member("socketDescriptor") & socketDescriptor;
    p.member("socketName")       & socketName;

    QString addressProtocol = "ip4";
    QString address_;
    QString addressScopeId;

    if (p.isWriter())
    {
        address_ = address.toString();
        if (address.protocol() == QAbstractSocket::IPv6Protocol)
        {
            addressProtocol = "ip6";
            addressScopeId = address.scopeId();
        }
    }
    p.member("addressProtocol") & addressProtocol;
    p.member("address")         & address_;
    p.member("addressScopeId")  & addressScopeId;

    if (p.isReader())
    {
        address = QHostAddress(address_);
        if (addressProtocol == "ip6")
            address.setScopeId(addressScopeId);
    }
    p.member("port") & port;
    return p.endObject();
}

template <typename Packer>
Packer& Error::jserialize(Packer& p)
{
    p.startObject();
    p.member("commandId")   & commandId;
    p.member("code")        & code;
    p.member("description") & description;
    return p.endObject();
}

template <typename Packer>
Packer& CloseConnection::jserialize(Packer& p)
{
    p.startObject();
    p.member("code")        & code;
    p.member("description") & description;
    return p.endObject();
}
#endif // JSON_SERIALIZATION

} // namespace data
} // namespace communication


