/*****************************************************************************
  The MIT License

  Copyright © 2010 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле представлены механизмы обмена сообщениями и данными
  между HulbeeServer и HulbeeClient c использованием протокола Tcp/Ip.
*****************************************************************************/

#pragma once
#ifndef MSGTRANSPORT_H
#define MSGTRANSPORT_H


#include <QtCore>
#include <QtNetwork>

#include "defmac.h"
#include "msgcmd.h"
#include "msgproc.h"
#include "ProductVersion.h"


#define BEGIN_QSIGNAL_MAP \
    void receivingMessage(const ProcMessageCPtr& msg) { \
        if (msg->command() == PacketProcMessage::command()) { \
            QList<QByteArray> commands; \
            msg->readContent(commands); \
            for (int i = 0; i < commands.count(); ++i) { \
                ProcMessageCPtr m = ProcMessage::fromByteArray(commands.at(i)); \
                m->setAddress(msg->address()); \
                receivingMessage(m); \
            } \
            return;  \
        }

#define REG_QSIGNAL(QSIGNAL_, CMD_) \
    if (msg->command() == CMD_) { \
	    emit QSIGNAL_(msg); \
        return; \
    }

#define REG_PLUGINS_INIT() \
    if(msg->command() == CMD_PLUGINS_INIT) { \
		msg->readContent(m_isPluginsInit); \
        return; \
    }

#define END_QSIGNAL_MAP }




namespace snd {


class CustomCommunicator;
typedef container_ptr<CustomCommunicator> CustomCommunicatorCPtr;

class IndexCommunicator;
typedef container_ptr<IndexCommunicator> IndexCommunicatorCPtr;

class SearchCommunicator;
typedef container_ptr<SearchCommunicator> SearchCommunicatorCPtr;

class IndexSrvCommunicator;
typedef container_ptr<IndexSrvCommunicator> IndexSrvCommunicatorCPtr;

class SearchSrvCommunicator;
typedef container_ptr<SearchSrvCommunicator> SearchSrvCommunicatorCPtr;


/**
  Класс ClientCommunicator используется для обмена сообщениями и
  данными с HulbeeServer.
*/
class CustomCommunicator : public QObject
{
	Q_OBJECT

public:
    // Выполняет автоподключение сигналов к одноименным слотам.
    // Cигнатуры сигналов и слотов должны совпадать.
    void signalsAutoConnect(QObject* obj/*, int superLevel = 0*/) const;

    void connectToHost(const QHostAddress& address, quint16 port);
    void disconnectFromHost();

    // Функции для отправки ассинхронных сообщений
    void send(const ProcMessage&) const;
    void send(const ProcMessageCPtr&) const;
    void send(const PacketProcMessage&) const;
    void send(const QUuidEx& command) const;
    void send(const QUuidEx& command, quint32 indexCrc) const;

    // Функции для отправки синхронных сообщений
    ProcMessageCPtr sendSync(const ProcMessage&, quint32 sec /*ожидание в сек.*/);
    ProcMessageCPtr sendSync(const ProcMessageCPtr&, quint32 sec);
    ProcMessageCPtr sendSync(const QUuidEx& command, quint32 sec);

    // Запрос у сервера информации о совместимости
    void getServerInfo();

    // Возвращает версию сервера
	ProductVersion serverVersion() const {return m_serverVersion;}

    // Проверяет поддерживается ли команда сервером
    bool supportCommand(const QUuidEx& command);
    //
    const QTcpSocket& socket() const {return m_socket;}
    bool isConnected() const;

signals:
    void socketConnected();
    void socketDisconnected();
    void socketError(int codeError, QString error);

protected:
    CustomCommunicator();
    DISABLE_DEFAULT_FUNC1(CustomCommunicator);
    virtual void receivingMessage(const ProcMessageCPtr& msg) = 0;

private slots:
	// чтение информации из слота
	void readyRead();
    void disconnected();
    void send_(const ProcMessageCPtr&);
    void socketError_(QAbstractSocket::SocketError);
    void receivingMessage_(const ProcMessageCPtr& msg) {receivingMessage(msg);}

private:
	QTcpSocket m_socket;
    //
	qint32 m_buffSize;
    bool m_buffCompressed;
    QByteArray m_buff;

    // Версия сервера
    ProductVersion m_serverVersion;
	// Список доступных комманд полученных клиентом с сервера.
    QVector<QUuidEx> m_serverCommands;

    // Обеспечиваю синхронный вызов
    QUuidEx m_waitCommand;
    ProcMessageCPtr m_waitMsg;
};



/**
  IndexCommunicator
*/
class IndexCommunicator : public CustomCommunicator
{
    Q_OBJECT

public:
    // Запрос на проверку инициализации плагинов
    void getPluginsInit(); //  {ProcMessage msg(CMD_PLUGINS_INIT); send(msg);}

    // Признак инициализации плагинов на сервере
    bool isPluginsInit() const {return m_isPluginsInit;}

signals:
    void respGetIndexes(const ProcMessageCPtr&);
    void respSaveIndexes(const ProcMessageCPtr&);
    void respDeleteIndexes(const ProcMessageCPtr&);
    void respVisualElements(const ProcMessageCPtr&);
    void respFiltersPlugins(const ProcMessageCPtr&);
    void respIndexingElements(const ProcMessageCPtr&);
    void respGetIndexingGeneral(const ProcMessageCPtr&);
    void respSaveIndexingGeneral(const ProcMessageCPtr&);
    //void respGetStoreDirs(const ProcMessageCPtr&);
    //void respSaveStoreDirs(const ProcMessageCPtr&);
    //void respCreateIndex(const ProcMessageCPtr&);
    void respStoreDirsTree(const ProcMessageCPtr&);
    void respGetPluginsSettings(const ProcMessageCPtr&);
    void respSavePluginsSettings(const ProcMessageCPtr&);

    void eventStartIndexing(const ProcMessageCPtr&);
    void eventStopIndexing(const ProcMessageCPtr&);
    void eventFinishIndexing(const ProcMessageCPtr&);
    void eventStartItemIndexing(const ProcMessageCPtr&);
    void eventFinishItemIndexing(const ProcMessageCPtr&);
    void eventStartOptimizeItemIndex(const ProcMessageCPtr&);

private:
    BEGIN_QSIGNAL_MAP
        REG_QSIGNAL(respGetIndexes, CMD_GET_INDEXES)
        REG_QSIGNAL(respSaveIndexes, CMD_SAVE_INDEXES)
        REG_QSIGNAL(respDeleteIndexes, CMD_DELETE_INDEXES)
        REG_QSIGNAL(respVisualElements, CMD_GET_VISUAL_ELEMENTS)
        REG_QSIGNAL(respFiltersPlugins, CMD_PLUGINS_FILTERS)
        REG_QSIGNAL(respIndexingElements, CMD_GET_INDEXING_ELEMENTS)
        REG_QSIGNAL(respGetIndexingGeneral, CMD_GET_INDEXING_GENERAL)
        REG_QSIGNAL(respSaveIndexingGeneral, CMD_SAVE_INDEXING_GENERAL)
        //REG_QSIGNAL(respGetStoreDirs, CMD_GET_STORE_DIRS)
        //REG_QSIGNAL(respSaveStoreDirs, CMD_SAVE_STORE_DIRS)
        //REG_QSIGNAL(respCreateIndex, CMD_CREATE_INDEX)
        REG_QSIGNAL(respStoreDirsTree, CMD_GET_STORE_DIRS_TREE)
        REG_QSIGNAL(respGetPluginsSettings, CMD_GET_PLUGINS_SETTINGS)
        REG_QSIGNAL(respSavePluginsSettings, CMD_SAVE_PLUGINS_SETTINGS)

        REG_QSIGNAL(eventStartIndexing, CMD_START_INDEXING)
        REG_QSIGNAL(eventStopIndexing, CMD_STOP_INDEXING)
        REG_QSIGNAL(eventFinishIndexing, CMD_FINISH_INDEXING)
        REG_QSIGNAL(eventStartItemIndexing, CMD_START_ITEM_INDEXING)
        REG_QSIGNAL(eventFinishItemIndexing, CMD_FINISH_ITEM_INDEXING)
        REG_QSIGNAL(eventStartOptimizeItemIndex, CMD_START_OPTIMIZE_ITEM_INDEX)

        REG_PLUGINS_INIT()
    END_QSIGNAL_MAP

private:
    IndexCommunicator() : CustomCommunicator(), m_isPluginsInit(false) {}
    DISABLE_DEFAULT_FUNC1(IndexCommunicator)

    // Флаг инициализации плагинов
    bool m_isPluginsInit;
    //
    friend IndexCommunicatorCPtr indexCommunicator();
};
IndexCommunicatorCPtr indexCommunicator();


/**
  SearchCommunicator
*/
class SearchCommunicator : public CustomCommunicator
{
    Q_OBJECT

signals:
    void respGetIndexes(const ProcMessageCPtr&);
    void respSearchRequest(const ProcMessageCPtr&);

private:
    BEGIN_QSIGNAL_MAP
        REG_QSIGNAL(respGetIndexes, CMD_GET_INDEXES)
        REG_QSIGNAL(respSearchRequest, CMD_SEARCH_REQUEST)

    END_QSIGNAL_MAP

private:
    SearchCommunicator() : CustomCommunicator() {}
    DISABLE_DEFAULT_FUNC1(SearchCommunicator)

    //friend SearchCommunicator& searchCommunicator();
    friend SearchCommunicatorCPtr createSearchCommunicator();
};
//SearchCommunicator& searchCommunicator();
SearchCommunicatorCPtr createSearchCommunicator();


// Предварительная декларация
class CustomSrvCommunicator;

/**
  MsgTcpSocket
*/
class MsgTcpSocket : public QTcpSocket
{
    Q_OBJECT

public:
    MsgTcpSocket(CustomSrvCommunicator* sender/*, const QHostAddress&*/);
    //const QUuidEx& id() const {return m_id;}
    //QHostAddress address() const {return peerAddress();}
    //void setAddress(const QHostAddress& val) {m_address = val;}
    bool isAdminMode() const {return m_isAdminMode;}

private slots:
    void readyRead();
    void disconnected();
    void error(QAbstractSocket::SocketError);

private:
    CustomSrvCommunicator* m_sender;
    //QUuidEx m_id;
    //QHostAddress m_address;
    ProductVersion m_productVersion;
    QVector<QUuidEx> m_commands;
	qint32 m_buffSize;
    bool m_buffCompressed;
	QByteArray m_buff;
    bool m_isAdminMode;
};

template<typename T> struct MsgTcpSocketAlloc
{
    inline static void destroy(T* x, bool) {if (x) x->deleteLater();}
};
typedef container_ptr<MsgTcpSocket, MsgTcpSocketAlloc> MsgTcpSocketCPtr;


/**
  MsgTcpServer
*/
struct MsgTcpServer : public QTcpServer
{
    //Q_OBJECT
    MsgTcpServer(CustomSrvCommunicator* sender);
    virtual void incomingConnection (int socketDescriptor);
    CustomSrvCommunicator* m_sender;
};


/**
  Класс ServerCommunicator используется для обмена сообщениями и
  данными с HulbeeClient.
*/
class CustomSrvCommunicator : public QObject
{
	Q_OBJECT

public:
    // Выполняет автоподключение сигналов к одноименным слотам.
    // Cигнатуры сигналов и слотов должны совпадать.
    void signalsAutoConnect(QObject* obj) const;

	bool initialize(const QHostAddress& address, quint16 port);
    void send(const ProcMessage&) const;
    void send(const ProcMessageCPtr&) const;
    void send(const PacketProcMessage&) const;

private slots:
    void send_(const ProcMessageCPtr&);
    void socketSend(const MsgTcpSocketCPtr&, const QByteArray& buff);
    void receivingMessage_(const ProcMessageCPtr& msg) {receivingMessage(msg);}

    void addSocket(MsgTcpSocket*);
    void delSocket(MsgTcpSocket*);

    MsgTcpSocket* getAdminMode(QHostAddress&/*, quint16& port*/) const;

protected:
    CustomSrvCommunicator();
    DISABLE_DEFAULT_FUNC1(CustomSrvCommunicator)
    virtual void receivingMessage(const ProcMessageCPtr& msg) = 0;

private:
    MsgTcpServer m_tcpServer;

    mutable QMutex m_socketLock;
    QList<MsgTcpSocketCPtr> m_sockets;

    friend class MsgTcpSocket;
    friend class MsgTcpServer;
};


/**
  IndexSrvCommunicator
*/
class IndexSrvCommunicator : public CustomSrvCommunicator
{
    Q_OBJECT

signals:
    void getIndexes(const ProcMessageCPtr&);
    void getIndexesState(const ProcMessageCPtr&);
    void saveIndexes(const ProcMessageCPtr&);
    void deleteIndexes(const ProcMessageCPtr&);
    void getVisualElements(const ProcMessageCPtr&);
    void getPluginsInitialize(const ProcMessageCPtr&);
    void getPluginsFilters(const ProcMessageCPtr&);
    void getIndexingElements(const ProcMessageCPtr&);
    void startIndexing(const ProcMessageCPtr&);
    void stopIndexing(const ProcMessageCPtr&);
    void getIndexingGeneral(const ProcMessageCPtr&);
    void saveIndexingGeneral(const ProcMessageCPtr&);
    //void getStoreDirs(const ProcMessageCPtr&);
    //void saveStoreDirs(const ProcMessageCPtr&);
    void createIndex(const ProcMessageCPtr&);
    void getStoreDirsTree(const ProcMessageCPtr&);
    void getPluginsSettings(const ProcMessageCPtr&);
    void savePluginsSettings(const ProcMessageCPtr&);

private:
    BEGIN_QSIGNAL_MAP
        REG_QSIGNAL(getIndexes, CMD_GET_INDEXES)
        REG_QSIGNAL(getIndexesState, CMD_GET_INDEXES_STATE)
        REG_QSIGNAL(saveIndexes, CMD_SAVE_INDEXES)
        REG_QSIGNAL(deleteIndexes, CMD_DELETE_INDEXES)
        REG_QSIGNAL(getVisualElements, CMD_GET_VISUAL_ELEMENTS)
        REG_QSIGNAL(getPluginsInitialize, CMD_PLUGINS_INIT)
        REG_QSIGNAL(getPluginsFilters, CMD_PLUGINS_FILTERS)
        REG_QSIGNAL(getIndexingElements, CMD_GET_INDEXING_ELEMENTS)
        REG_QSIGNAL(startIndexing, CMD_START_INDEXING)
        REG_QSIGNAL(stopIndexing, CMD_STOP_INDEXING)
        REG_QSIGNAL(getIndexingGeneral, CMD_GET_INDEXING_GENERAL)
        REG_QSIGNAL(saveIndexingGeneral, CMD_SAVE_INDEXING_GENERAL)
        //REG_QSIGNAL(getStoreDirs, CMD_GET_STORE_DIRS)
        //REG_QSIGNAL(saveStoreDirs, CMD_SAVE_STORE_DIRS)
        REG_QSIGNAL(createIndex, CMD_CREATE_INDEX)
        REG_QSIGNAL(getStoreDirsTree, CMD_GET_STORE_DIRS_TREE)
        REG_QSIGNAL(getPluginsSettings, CMD_GET_PLUGINS_SETTINGS)
        REG_QSIGNAL(savePluginsSettings, CMD_SAVE_PLUGINS_SETTINGS)

    END_QSIGNAL_MAP

private:
    IndexSrvCommunicator() : CustomSrvCommunicator() {}
    DISABLE_DEFAULT_FUNC1(IndexSrvCommunicator)

    friend IndexSrvCommunicatorCPtr indexSrvCommunicator();
};
IndexSrvCommunicatorCPtr indexSrvCommunicator();



/**
  SearchSrvCommunicator
*/
class SearchSrvCommunicator : public CustomSrvCommunicator
{
    Q_OBJECT

// public:
//     // Выполняет автоподключение сигналов к одноименным слотам.
//     // Cигнатуры сигналов и слотов должны совпадать.
//     void signalsAutoConnect(QObject* obj) const;

signals:
    void getIndexes(const ProcMessageCPtr&);
    void searchRequest(const ProcMessageCPtr&);

private:
    BEGIN_QSIGNAL_MAP
        REG_QSIGNAL(getIndexes, CMD_GET_INDEXES)
        REG_QSIGNAL(searchRequest, CMD_SEARCH_REQUEST)

    END_QSIGNAL_MAP

private:
    SearchSrvCommunicator() : CustomSrvCommunicator() {}
    DISABLE_DEFAULT_FUNC1(SearchSrvCommunicator)

    friend SearchSrvCommunicatorCPtr searchSrvCommunicator();
};
SearchSrvCommunicatorCPtr searchSrvCommunicator();



}; /*namespace snd*/


/**
 ----------------------------- Регистрация метатипов --------------------------
*/
//METATYPES_REGISTER_BEGIN(msgtransport_)
    //METAFUNC_REGISTER(QAbstractSocket::SocketError);
//METATYPES_REGISTER_END



#undef BEGIN_QSIGNAL_MAP
#undef REG_QSIGNAL
#undef REG_PLUGINS_INIT
#undef END_QSIGNAL_MAP


#endif //MSGTRANSPORT_H


