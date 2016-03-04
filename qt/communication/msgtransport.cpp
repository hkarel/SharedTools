
#include <QTest>
#include "defmac.h"
#include "aback.h"
#include "msgtransport.h"
#include "debugmsg.h"



namespace snd {


// Функция выполняет connect одноименных сигналов и слотов. Сигнатуры сигналов
// и слотов должны совпадать. Соединения выполняется в флагом Qt::DirectConnection.
// По умолчанию сигналы и слоты берутся с последних классов в цепочке настледования.
// Если необходимо связать сигналы из базовых классов то параметр superLevel1 должен
// быть отличен от нуля. Параметр superLevel1 показыват на сколько уровней вверх
// нужно подняться в иеархии наследования.
static void signalsAutoConnect_(const QObject* obj1,
                                const QObject* obj2,
                                int superLevel = 0)
{
    //_CrtDbgBreak();
    const QMetaObject* ometa1 = obj1->metaObject();
    const QMetaObject* ometa2 = obj2->metaObject();

    const QMetaObject* sc1 = ometa1;
    while (superLevel--) {if (sc1->superClass()) sc1 = sc1->superClass(); else break;}
    const int method_offset1 = sc1->methodOffset();

    for (int i = method_offset1 /*ometa1->methodOffset()*/; i < ometa1->methodCount(); ++i) {
        QMetaMethod method1 = ometa1->method(i);
        //qDebug() << method1.signature();
        if (method1.methodType() == QMetaMethod::Signal) {
            QString sign1 = method1.signature();
            for (int j = ometa2->methodOffset(); j < ometa2->methodCount(); ++j) {
                QMetaMethod method2 = ometa2->method(j);
                if (method2.methodType() == QMetaMethod::Slot) {
                    QString sign2 = method2.signature();
                    if (sign1 == sign2) {
                        // !!! ВНИМАНИЕ !!! Если потребуется устанавливать коннект
                        // с флагом Qt::QueuedConnection, то последний параметр
                        // в QMetaObject::connect(), будет отличен от нуля.
                        // Этот параметр нужно будет определять, так как это делается
                        // в реализации QObject::connect().
                        bool res = QMetaObject::connect(obj1, method1.methodIndex(),
                                                        obj2, method2.methodIndex(),
                                                        Qt::DirectConnection/*AutoConnection*/, 0);
#ifndef NDEBUG
                        Q_ASSERT(res);
#endif
                        break;
                    }
                }
            }
        }
    }

}



//--------------------- Implementation ClientCommunicator ---------------------

CustomCommunicator::CustomCommunicator()
{
    m_buffSize = 0;

    //_CrtDbgBreak();
    chk_connect_q(&m_socket, SIGNAL(connected()), this, SIGNAL(socketConnected()));
    chk_connect_q(&m_socket, SIGNAL(disconnected()), this, SIGNAL(socketDisconnected()));

    chk_connect_d(&m_socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    chk_connect_d(&m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    chk_connect_d(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                  this, SLOT(socketError_(QAbstractSocket::SocketError)));
}

void CustomCommunicator::signalsAutoConnect(QObject* obj/*, int superLevel*/) const
{
    signalsAutoConnect_(this, obj, 1 /*superLevel*/);
}

void CustomCommunicator::connectToHost(const QHostAddress& address, quint16 port)
{
	// устанавливаем соединение с сервером
	//m_socket.connectToHost(QHostAddress::LocalHost, 1441);
    m_socket.connectToHost(address, port);

// 	if (m_socket.waitForConnected(30000)) {
//         DEBUG_OUTPUT_F("Connecting is success!");
// 	}
// 	else {
//         DEBUG_OUTPUT_F("Connecting is fail!");
// 	}
}

void CustomCommunicator::disconnectFromHost()
{
    m_socket.disconnectFromHost();
}

bool CustomCommunicator::supportCommand(const QUuidEx& command)
{
    // Эта команда всегда должна поддеживаться.
    if (command == CMD_SERVER_INFO)
        return true;

    if (command == PacketProcMessage::command())
        return true;

    return (m_serverCommands.indexOf(command) != -1);
}

bool CustomCommunicator::isConnected() const
{
    return (m_socket.state() == QAbstractSocket::ConnectedState);
}

void CustomCommunicator::send_(const ProcMessageCPtr& msg)
{
    if (!supportCommand(msg->command())) {
        DEBUG_OUTPUT_F(QString("Command not found (%1)").arg(msg->command().toString()));
        return;
    }

    //if (m_socket.state() != QAbstractSocket::ConnectedState)
    if (!isConnected())
        return;

// 	QByteArray buff;
// 	{
// 		QDataStream s(&buff, QIODevice::WriteOnly);
// 		s << qint32(0);
// 		s << *msg;
// 	}
//     *((qint32*)buff.constData()) = buff.size() - sizeof(qint32);

    QByteArray buff = msg->toByteArray();
    bool compress_buff = (buff.size() > 1024);
    //compress_buff = true;

    if (compress_buff) {
        _CrtDbgBreak();
    }

    if (compress_buff)
        buff = qCompress(buff);

    buff.prepend("    ", sizeof(qint32));
    qint32 size = buff.size() - sizeof(qint32);
    if (compress_buff) size *= -1; //(unsigned(1) << 32);
    *((qint32*)buff.constData()) = size;

    //QByteArray buff(sizeof(qint32), Qt::Uninitialized);
    //buff += (ba.size() > 1024) ? qCompress(ba) : ba;
    //qint32 size = buff.size() - sizeof(qint32);
    //buff.prepend("    ", sizeof(qint32));
    //*((qint32*)buff.constData()) = buff.size() - sizeof(qint32);
    //*((qint32*)buff.constData()) = size;

    while (m_socket.waitForBytesWritten()) {
        DEBUG_OUTPUT_F("waitForBytesWritten");
        //QCoreApplication::processEvents(QEventLoop::AllEvents, 50); - будет ошибка
        //QTest::qWait(50); - будет ошибка
        QTest::qSleep(10);
    }

    m_socket.write(buff);
}

void CustomCommunicator::send(const ProcMessageCPtr& msg) const
{
    QMetaObject::invokeMethod((QObject*)this, "send_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
}

void CustomCommunicator::send(const ProcMessage& m) const
{
    //ProcMessageCPtr msg(new ProcMessage(m));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(m);

    send(msg);
}

void CustomCommunicator::send(const PacketProcMessage& pmsg) const
{
    //_CrtDbgBreak();

    //ProcMessageCPtr msg(new ProcMessage(PacketProcMessage::command()));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(PacketProcMessage::command());

    // Здесь не нужно присваивать адрес отправителя,
    // так как он будет присвоен при получении сообщения на сервере.
    //msg->setAddress(pmsg.address);

    QList<QByteArray> list;
    for (int i = 0; i < pmsg.messages.count(); ++i)
        list << pmsg.messages.at(i).toByteArray(/*DISABLE_MESSAGE_COMPRESSION*/);

    msg->writeContent(list);
    send(msg);
}

void CustomCommunicator::send(const QUuidEx& command) const
{
    //ProcMessageCPtr msg(new ProcMessage(command));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(command);

    send(msg);
}

void CustomCommunicator::send(const QUuidEx& command, quint32 indexCrc) const
{
    //ProcMessageCPtr msg(new ProcMessage(command, indexCrc));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(command, indexCrc);

    send(msg);
}

ProcMessageCPtr CustomCommunicator::sendSync(const ProcMessageCPtr& msg, quint32 sec)
{
    if (qApp->thread() != QThread::currentThread()) {
        _CrtDbgBreak();
        return ProcMessageCPtr();
    }

    //ABACK(&CustomCommunicator::m_waitCommand, m.command());
    m_waitCommand = msg->command();
    m_waitMsg.reset();

    send(msg);

    sec *= 1000;
    quint32 i = 0;
    ProcMessageCPtr result;

    while (i <= sec) {
        QTest::qWait(100);
        i += 100;
        if (m_waitMsg && m_waitCommand.isNull()) {
            result = m_waitMsg;
            break;
        }
        DEBUG_OUTPUT_F("waitCommand");
    }
    m_waitMsg.reset();
    m_waitCommand = QUuidEx();

    return result;
}

ProcMessageCPtr CustomCommunicator::sendSync(const ProcMessage& m, quint32 sec)
{
    //ProcMessageCPtr msg(new ProcMessage(m));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(m);

    return sendSync(msg, sec);
}

ProcMessageCPtr CustomCommunicator::sendSync(const QUuidEx& command, quint32 sec)
{
    //ProcMessageCPtr msg(new ProcMessage(command));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(command);

    return sendSync(msg, sec);
}

void CustomCommunicator::readyRead()
{
    m_buff.append(m_socket.readAll());

next_read:

	if (m_buffSize == 0) {
        if (m_buff.size() < (int)sizeof(qint32)) {
            //m_readBuffSize = 0;
            //m_readBuff.clear();
            _CrtDbgBreak();
			return;
        }
		m_buffSize = *((qint32*)m_buff.constData());
        m_buffCompressed = (m_buffSize < 0);
        if (m_buffSize < 0) m_buffSize *= -1;
		m_buff.remove(0, sizeof(qint32));
    }

    if (m_buff.size() < m_buffSize)
        return;

    ProcMessageCPtr msg;

    { //Блок для QByteArray::fromRawData
        QByteArray ba = (m_buffCompressed)
            ? qUncompress((const uchar*)m_buff.constData(), m_buffSize)
            : QByteArray::fromRawData(m_buff.constData(), m_buffSize);
        msg = ProcMessage::fromByteArray(ba);
    }

    if (m_buff.size() > m_buffSize)
        m_buff.remove(0, m_buffSize);
    else  /*m_buff.size() == m_buffSize*/
        m_buff.clear();

    m_buffSize = 0;
    msg->setAddress(m_socket.peerAddress());

// #ifndef NDEBUG
//     QHostAddress ha = m_socket.peerAddress();
//     qDebug() << ha.toString();
// #endif


    if (msg) {
        if (msg->command() == CMD_SERVER_INFO) {
            msg->readContent(m_serverVersion.vers, m_serverCommands);
            //if (msg->command() == m_waitCommand) {
            //    m_waitMsg = msg;
            //    m_waitCommand = QUuidEx();
            //}
        }
        else if (m_serverVersion.vers != 0) {
            if (msg->command() == m_waitCommand) {
                m_waitMsg = msg;
                m_waitCommand = QUuidEx();
            }
            else {
                QMetaObject::invokeMethod(this, "receivingMessage_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
            }
        }
    }

    if (m_buff.size() > (int)sizeof(qint32))
        goto next_read;
}

void CustomCommunicator::socketError_(QAbstractSocket::SocketError)
{
    DEBUG_OUTPUT_F(m_socket.errorString());
	m_buffSize = 0;
    m_buff.clear();

    //_CrtDbgBreak();
    //emit socketError(int(m_socket.error()), m_socket.errorString());
    QMetaObject::invokeMethod(this, "socketError", Qt::QueuedConnection,
                              Q_ARG(int, int(m_socket.error())),
                              Q_ARG(QString, m_socket.errorString()));
}

void CustomCommunicator::disconnected()
{
    m_serverVersion.vers = 0;
    m_serverCommands.clear();
}

void CustomCommunicator::getServerInfo()
{
    QVector<QUuidEx> cmd_list;
    for (int i = 0; i < QUuidCmd::commands().count(); ++i)
        cmd_list << *QUuidCmd::commands().at(i);

	ProcMessage msg(CMD_SERVER_INFO);
	msg.writeContent(productVersion().vers, cmd_list);
    send(msg);
}


//-------------------- Implementation IndexCommunicator ---------------------

// void IndexCommunicator::signalsAutoConnect(QObject* obj) const
// {
//     signalsAutoConnect_(this, obj);
// }

void IndexCommunicator::getPluginsInit()
{
    ProcMessage msg(CMD_PLUGINS_INIT);
    send(msg);
}

IndexCommunicatorCPtr indexCommunicator()
{
    static IndexCommunicatorCPtr c(new IndexCommunicator());
    return c;
}

//------------------- Implementation SearchCommunicator ---------------------

// void SearchCommunicator::signalsAutoConnect(QObject* obj) const
// {
//     signalsAutoConnect_(this, obj);
// }

// SearchCommunicator& searchCommunicator()
// {
//     static SearchCommunicator _searchCommunicator;
//     return _searchCommunicator;
// }

SearchCommunicatorCPtr createSearchCommunicator()
{
    SearchCommunicatorCPtr c(new SearchCommunicator());
    return c;
}


//---------------------- Implementation MsgTcpSocket ------------------------

MsgTcpSocket::MsgTcpSocket(CustomSrvCommunicator* sender)
    : QTcpSocket(0),
      m_sender(sender),
      m_buffSize(0),
      m_isAdminMode(false)
{
    chk_connect_d(this, SIGNAL(readyRead()), this, SLOT(readyRead()));
    chk_connect_d(this, SIGNAL(disconnected()), this, SLOT(disconnected()));
	chk_connect_d(this, SIGNAL(error(QAbstractSocket::SocketError)),
		          this, SLOT  (error(QAbstractSocket::SocketError)));
}

void MsgTcpSocket::readyRead()
{
    m_buff.append(readAll());

next_read:

	if (m_buffSize == 0) {
        if (m_buff.size() < (int)sizeof(qint32)) {
            //m_readBuffSize = 0;
            //m_readBuff.clear();
            _CrtDbgBreak();
			return;
        }
		m_buffSize = *((qint32*)m_buff.constData());
        m_buffCompressed = (m_buffSize < 0);
        if (m_buffSize < 0) m_buffSize *= -1;
		m_buff.remove(0, sizeof(qint32));
    }

    if (m_buff.size() < m_buffSize)
        return;

    ProcMessageCPtr msg;

    { //Блок для QByteArray::fromRawData
        QByteArray ba = (m_buffCompressed)
            ? qUncompress((const uchar*)m_buff.constData(), m_buffSize)
            : QByteArray::fromRawData(m_buff.constData(), m_buffSize);
        msg = ProcMessage::fromByteArray(ba);
    }

    if (m_buff.size() > m_buffSize)
        m_buff.remove(0, m_buffSize);
    else  /*m_buff.size() == m_buffSize*/
        m_buff.clear();

    m_buffSize = 0;
    // Назначаем входящему сообщению адрес отправителя.
    msg->setAddress(peerAddress());

    if (msg) {

//         if (msg->command() == CMD_GET_INDEXING_GENERAL)
//             _CrtDbgBreak();
//
//        if (msg->command() == CMD_GET_STORE_DIRS)
//            _CrtDbgBreak();
//
//         if (msg->command() == CMD_SAVE_INDEXING_GENERAL)
//             _CrtDbgBreak();


        if (msg->command() == CMD_SERVER_INFO) {
            msg->readContent(m_productVersion.vers, m_commands);

            QVector<QUuidEx> cmd_list;
            for (int i = 0; i < QUuidCmd::commands().count(); ++i)
                cmd_list << *QUuidCmd::commands().at(i);
            msg->writeContent(productVersion().vers, cmd_list);

            //send_(msg);
            QMetaObject::invokeMethod(m_sender, "send_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
        }
        // Запрос сатуса администратора
        else if (msg->command() == CMD_GET_ADMIN_LOCK) {
            //_CrtDbgBreak();
            QHostAddress addr; //quint16 port;
            MsgTcpSocket* socket_ = m_sender->getAdminMode(addr/*, port*/);

            quint8 success = false;
            if ((socket_ == 0) || (socket_ == this)) {
                m_isAdminMode = (success = true);
                addr = peerAddress();
                //port = peerPort();
                //addr = localAddress();
                //port = localPort();
            }
            msg->writeContent(success, addr/*, port*/);
            QMetaObject::invokeMethod(m_sender, "send_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
        }
        // Условие m_productVersion.vers != 0 используем как признак инициализации
        else if (m_productVersion.vers != 0)  {
            //receivingMessage(msg);
            QMetaObject::invokeMethod(m_sender, "receivingMessage_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
        }
    }

    if (m_buff.size() > (int)sizeof(qint32))
        goto next_read;
}

void MsgTcpSocket::disconnected()
{
    m_sender->delSocket(this);
}

void MsgTcpSocket::error(QAbstractSocket::SocketError sockErr)
{
	m_buff.clear();
	m_buffSize = 0;
    DEBUG_OUTPUT_F(errorString());
}



//---------------------- Implementation MsgTcpServer ------------------------

MsgTcpServer::MsgTcpServer(CustomSrvCommunicator* sender)
    : QTcpServer(0),
      m_sender(sender)
{
}

void MsgTcpServer::incomingConnection(int socketDescriptor)
{
    MsgTcpSocket *socket = new MsgTcpSocket(m_sender);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        m_sender->addSocket(socket);
// #ifndef NDEBUG
//         QHostAddress ha = socket->peerAddress();
//         qDebug() << ha.toString();
// #endif
    }
}



//-------------------- Implementation ServerCommunicator --------------------

CustomSrvCommunicator::CustomSrvCommunicator() : m_tcpServer(this)
{
}

void CustomSrvCommunicator::signalsAutoConnect(QObject* obj) const
{
    signalsAutoConnect_(this, obj);
}

bool CustomSrvCommunicator::initialize(const QHostAddress& address, quint16 port)
{
	// устанавливаем максимально возможное количество соединений
	//m_tcpServer.setMaxPendingConnections(1);

    int attempt_count = 0;
	// запуск и работа сервера
	//while (!m_tcpServer.isListening() && !m_tcpServer.listen(QHostAddress::LocalHost, 1441)) {
    //while (!m_tcpServer.isListening() && !m_tcpServer.listen(address, port)) {
    while (!m_tcpServer.listen(address, port)) {
        if (++attempt_count > 10) break;
        QTest::qSleep(50);
	}
    if (attempt_count > 10) {
        DEBUG_OUTPUT_F("Start listen error");
    }
    return (attempt_count <= 10);
}

// void CustomSrvCommunicator::signalsAutoConnect(QObject* obj)
// {
//     signalsAutoConnect_(this, obj);
// }

void CustomSrvCommunicator::send_(const ProcMessageCPtr& msg)
{
    QMutexLocker locker(&m_socketLock);
    if (m_sockets.count() == 0)
        return;

//     QByteArray buff;
//     {
//         QDataStream s(&buff, QIODevice::WriteOnly);
//         s << qint32(0);
//         s << *msg;
//     }
//     *((qint32*)buff.constData()) = buff.size() - sizeof(qint32);

    QByteArray buff = msg->toByteArray();
    //if (ba.size() > 1024) {
    //    _CrtDbgBreak();
    //}

    bool compress_buff = (buff.size() > 1024);
    //compress_buff = true;

    if (compress_buff)
        buff = qCompress(buff);

    buff.prepend("    ", sizeof(qint32));
    qint32 size = buff.size() - sizeof(qint32);
    if (compress_buff) size *= -1; //(unsigned(1) << 32);
    *((qint32*)buff.constData()) = size;

//     QByteArray buff(sizeof(qint32), Qt::Uninitialized);
//     buff += qCompress(ba, 9);
//     *((qint32*)buff.constData()) = buff.size() - sizeof(qint32);

    //if (msg->socketId().isNull()) {
    if (msg->address().isNull()) {
        // Широковещательная рассылка сообщения
        for (int i = 0; i < m_sockets.count(); ++i)
            socketSend(m_sockets.at(i), buff);
    }
    else {
        // Отправка сообщения конкретному сокету
        for (int i = 0; i < m_sockets.count(); ++i) {
            //if (m_sockets.at(i)->id() == msg->socketId()) {
            if (m_sockets.at(i)->peerAddress() == msg->address()) {
                socketSend(m_sockets.at(i), buff);
                break;
            }
        }
    }
}

void CustomSrvCommunicator::socketSend(const MsgTcpSocketCPtr& socket,
                                       const QByteArray& buff)
{
    if (socket) {
        if (socket->state() != QAbstractSocket::ConnectedState)
            return;

        while (socket->waitForBytesWritten()) {
            DEBUG_OUTPUT_F("waitForBytesWritten");
            //QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50); - будет ошибка
            //QTest::qWait(50); - будет ошибка
            QTest::qSleep(10);
        }

        socket->write(buff);
    }
}

void CustomSrvCommunicator::send(const ProcMessageCPtr& msg) const
{
    QMetaObject::invokeMethod((QObject*)this, "send_", Qt::QueuedConnection, Q_ARG(ProcMessageCPtr, msg));
}

void CustomSrvCommunicator::send(const ProcMessage& m) const
{
    //ProcMessageCPtr msg(new ProcMessage(m));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(m);
    send(msg);
}

void CustomSrvCommunicator::send(const PacketProcMessage& pmsg) const
{
    //_CrtDbgBreak();

    //ProcMessageCPtr msg(new ProcMessage(PacketProcMessage::command()));
    ProcMessageCPtr msg = ProcMessageCPtr::create_join_ptr();
    new (msg.get()) ProcMessage(PacketProcMessage::command());

    //msg->setSocketId(pmsg.socketId);
    msg->setAddress(pmsg.address);

    QList<QByteArray> list;
    for (int i = 0; i < pmsg.messages.count(); ++i)
        list << pmsg.messages.at(i).toByteArray(/*DISABLE_MESSAGE_COMPRESSION*/);

    msg->writeContent(list);
    send(msg);
}

void CustomSrvCommunicator::addSocket(MsgTcpSocket* socket)
{
    QMutexLocker locker(&m_socketLock);
    m_sockets << MsgTcpSocketCPtr(socket);
}

void CustomSrvCommunicator::delSocket(MsgTcpSocket* socket)
{
    QMutexLocker locker(&m_socketLock);
    for (int i = 0; i < m_sockets.count(); ++i)
        if (m_sockets.at(i).get() == socket)
            m_sockets.removeAt(i--);
}

MsgTcpSocket* CustomSrvCommunicator::getAdminMode(QHostAddress& addr/*, quint16& port*/) const
{
    //_CrtDbgBreak();

    QMutexLocker locker(&m_socketLock);
    for (int i = 0; i < m_sockets.count(); ++i) {
        MsgTcpSocket* socket_ = m_sockets.at(i).get();
        if (socket_->isAdminMode()) {
            addr = socket_->peerAddress();
            //port = socket_->peerPort();
            return socket_;
        }
    }
    return 0;
}


//------------------- Implementation IndexSrvCommunicator -------------------

// void IndexSrvCommunicator::signalsAutoConnect(QObject* obj) const
// {
//     signalsAutoConnect_(this, obj);
// }

IndexSrvCommunicatorCPtr indexSrvCommunicator()
{
    static IndexSrvCommunicatorCPtr c(new IndexSrvCommunicator());
    return c;
}

//------------------- Implementation SearchSrvCommunicator ------------------

// void SearchSrvCommunicator::signalsAutoConnect(QObject* obj) const
// {
//     signalsAutoConnect_(this, obj);
// }

SearchSrvCommunicatorCPtr searchSrvCommunicator()
{
    static SearchSrvCommunicatorCPtr c(new SearchSrvCommunicator());
    return c;
}



}; /*namespace snd*/

