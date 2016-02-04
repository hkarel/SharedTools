/* clang-format off */
/****************************************************************************
  Author:  Karelin Pavel (hkarel), hkarel@yandex.ru

  Хэдер определяет макросы общего назначения.
****************************************************************************/

#pragma once

/* Запрет конструктора по умолчанию и копирующего конструктора */
#define DISABLE_DEFAULT_CONSTRUCT( ClassName )    \
    ClassName () = delete;                        \
    ClassName ( ClassName && ) = delete;          \
    ClassName ( const ClassName & ) = delete;

/* Запрет копирующего конструктора и оператора присваивания */
#define DISABLE_DEFAULT_COPY( ClassName )         \
    ClassName ( ClassName && ) = delete;          \
    ClassName ( const ClassName & ) = delete;     \
    ClassName & operator = ( ClassName && ) = delete; \
    ClassName & operator = ( const ClassName & ) = delete;

/* Запрет конструктора по умолчанию, копирующего конструктора и оператора присваивания */
#define DISABLE_DEFAULT_FUNC( ClassName )         \
    ClassName () = delete;                        \
    ClassName ( ClassName && ) = delete;          \
    ClassName ( const ClassName & ) = delete;     \
    ClassName & operator = ( ClassName && ) = delete; \
    ClassName & operator = ( const ClassName & ) = delete;


// Макрос chk_connect используется для проверки возвращаемого функцией результата
// в отладочном режиме, этим он похож на функцию assert. Однако в релизном
// режиме, в отличии от ф-ции assert - тестируемое выражение не удаляется.
// Для использования макроса в код нужно включить <assert.h>
#ifndef NDEBUG
#define chk_connect(SOURCE_, SIGNAL_, DEST_, SLOT_, CONNECT_TYPE_) \
            Q_ASSERT(QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, CONNECT_TYPE_));

// Соответствует параметру соединения Qt::AutoConnection
#define chk_connect_a(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            Q_ASSERT(QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::AutoConnection));

// Соответствует параметру соединения Qt::DirectConnection
#define chk_connect_d(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            Q_ASSERT(QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::DirectConnection));

// Соответствует параметру соединения Qt::QueuedConnection
#define chk_connect_q(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            Q_ASSERT(QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::QueuedConnection));

// Соответствует параметру соединения Qt::BlockingQueuedConnection
#define chk_connect_bq(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            Q_ASSERT(QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::BlockingQueuedConnection));

#else //NDEBUG
#define chk_connect(SOURCE_, SIGNAL_, DEST_, SLOT_, CONNECT_TYPE_) \
            QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, CONNECT_TYPE_);

// Соответствует параметру соединения Qt::AutoConnection
#define chk_connect_a(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::AutoConnection);

// Соответствует параметру соединения Qt::DirectConnection
#define chk_connect_d(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::DirectConnection);

// Соответствует параметру соединения Qt::QueuedConnection
#define chk_connect_q(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::QueuedConnection);

// Соответствует параметру соединения Qt::BlockingQueuedConnection
#define chk_connect_bq(SOURCE_, SIGNAL_, DEST_, SLOT_) \
            QObject::connect(SOURCE_, SIGNAL_, DEST_, SLOT_, Qt::BlockingQueuedConnection);

#endif //NDEBUG


#define DECLSP_SELECTANY  extern "C" __declspec(selectany)


/* Фиктивные описатели для параметров функций */
#ifndef IN
#define IN
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef OUT
#define OUT
#endif

