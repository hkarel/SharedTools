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

  Макросы для регистрации метатипов.

*****************************************************************************/

#pragma once

//#define METATYPES_REGISTER(UNIQUE, REGFUNC) \
//extern "C" __declspec(selectany) RegistryMetaTypes registratorMetaTypes##UNIQUE(REGFUNC);

// !!! Не объявлять переменную как extern "C" __declspec(selectany), в этом
// случае линковщик в режиме Release удалит эту переменную и регистрация типов
// не будет  произведена.
// #define METATYPES_REGISTER(UNIQUE, REGFUNC) \
//    static RegistryMetaTypes registratorMetaTypes##UNIQUE(REGFUNC);

// !!! НЕ ИСПОЛЬЗОВАТЬ МАКРОС METATYPES_REGISTER2. В некоторых случаях линковщик
// в режиме Release считает статическую переменную неиспользуемой и удаляет ее,
// при этом регистрация типов не будет произведена. Отследить "размышления" лин-
// ковщика по этому поводу сложно.
// #define METATYPES_REGISTER2(REGFUNC) \
//    static RegistryMetaTypes registratorMetaTypes(REGFUNC);


// Как работает макрос METATYPES_REGISTER_CUSTOM_BEGIN__ можно почитать здесь:
// http://rsdn.ru/forum/cpp.applied/3570438.1.aspx

// !!! ВНИМАНИЕ !!!
// В конструкторе RegMetaTypes_##UNIQUE_(void (*registrator)()) не удалять
// условие if (registrator != 0), если этого условия не будет компилятор
// производит излишнюю оптимизацию и функция регистации registrator()
// вызываться не будет. Нельзя так же заменить условие if (registrator != 0)
// на фиктивное обращение к параметру через конструкцию (void)registrator,
// иногда возникают ситуации когда функция регистрации не вызывается.

#define METATYPES_REGISTER_CUSTOM_BEGIN__(UNIQUE_) \
    inline void regmt_func_##UNIQUE_(); \
    struct RegMetaTypes_##UNIQUE_ { \
        RegMetaTypes_##UNIQUE_(); \
        RegMetaTypes_##UNIQUE_(const RegMetaTypes_##UNIQUE_ &); \
        RegMetaTypes_##UNIQUE_ & operator= (const RegMetaTypes_##UNIQUE_ &); \
        RegMetaTypes_##UNIQUE_(void (*registrator)()) {if (registrator != 0) registrator();} \
    }; \
    extern "C" __declspec(selectany) const RegMetaTypes_##UNIQUE_ regMetaTypes_##UNIQUE_(regmt_func_##UNIQUE_); \
    extern "C" __declspec(dllexport) inline const void* __stdcall fictmt_func2_##UNIQUE_() \
        {return (void*)&regMetaTypes_##UNIQUE_;} \
    __pragma(comment(linker, "/include:_fictmt_func2_" #UNIQUE_ "@0")); \
    inline /*static*/ void regmt_func_##UNIQUE_() {



/**
  @brief Макросы METATYPES_REGISTER_BEGIN и METATYPES_REGISTER_END используются
  для релистрации типов для механизма signal/slot, при этом обеспечивается
  гарантия того, что линковщик не удалит константную переменную регистрации
  типов в режиме Release.
  Запись о регистрации типов должна быть сделана обязательно в хедер файле.
*/
#define METATYPES_REGISTER_BEGIN(UNIQUE_) \
    METATYPES_REGISTER_CUSTOM_BEGIN__(UNIQUE_)

#define METATYPES_REGISTER_END  };



/**
  @brief  Макрос расширенной регистрации метатипов: METAFUNC_REGISTER

  Для передачи пользовательских типов через механизм SIGNAL/SLOT их нужно
  зарегистрировать при помощи функции qRegisterMetaType<>(). При этом если
  пользовательский тип будет передаваться через очередь сообщений - очень
  важно что бы было соответствие между именем класса и его строковым экви-
  валентом. Ручная запись не может обеспечить 100%-го соответствия.
  Например: qRegisterMetaType<StringProcMessage>("StringProcMessage");
  При имзменении имени класса <StringProcMessage> надпись "StringProcMessage"
  сама не изменится. Это приведет к рассогласованию параметров и невозмож-
  ности передавать тип StringProcMessage через очередь сообщений.
  Для решения этой проблемы предназначен макрос METAFUNC_REGISTER.
  Макрос производит регистрацию типа как у четом пространства имен, так
  и без него и гарантирует, что строковый эквивалент имени класса всегда
  будет соответствовать реальному имени класса.
  Примечание: не рекомендуется использовать макрос расширенной регистрации
  метатипов для регистрации типов потокового ввода/вывода.
  Как пример: qRegisterMetaTypeStreamOperators<HeavyFiles>("HeavyFiles");
  При изменении имени класса (HeavyFiles) - автоматически изменится и его
  строковый эквивалент, что не позволит считывать ранее сериализованные
  данные.
  Пример использования:
    namespace mynamespace {
        MyClass {};
    };

    // Запись о регистрации должна быть сделана обязательно в хедер файле.
    METATYPES_REGISTER_BEGIN
        METAFUNC_REGISTER(mynamespace::MyClass)
    METATYPES_REGISTER_END
*/
#define METAFUNC_REGISTER(TYPE_) qRegisterMetaTypeExt<TYPE_>(#TYPE_);


/// @brief Функция регистрирует метатип с учетом и без учета пространства имен.
template<typename T> void qRegisterMetaTypeExt(const char* type_name)
{
    // Регистрация типа с учетом пространства имен
    qRegisterMetaType<T>(type_name);
    while (*type_name) {
        if ((*type_name == ':') && ((*(type_name + 1) == ':'))) {
            // Регистрация типа без учета пространства имен
            qRegisterMetaType<T>(type_name += 2);
            return;
        }
        ++type_name;
    }
}


/**
  @brief Макрос расширенной регистрации потоковых опереторов.
  При регистрации операторов так же происходит регистрация метатипа.
  Метатип регистрируется с учетом и без учета пространства имен.
*/
#define METAFUNC_REGISTER_STREAM(TYPE_) \
    qRegisterMetaTypeStreamOperatorsExt<TYPE_>(#TYPE_);


/**
  @brief Макрос расширенной регистрации потоковых опереторов,
  используется для совместимости со старыми наименованиями типов данных
*/
#define METAFUNC_REGISTER_STREAM_OLD(TYPE_, OLD_TYPE_NAME_) \
    qRegisterMetaTypeStreamOperatorsExt<TYPE_>(OLD_TYPE_NAME_);


/**
  @brief Функция регистрирует потоковые операторы и метатип с учетом
  и без учета пространства имен.
*/
template<typename T> void qRegisterMetaTypeStreamOperatorsExt(const char* type_name)
{
    // Регистрация типа с учетом пространства имен
    qRegisterMetaTypeStreamOperators<T>(type_name);
    while (*type_name) {
        if ((*type_name == ':') && ((*(type_name + 1) == ':'))) {
            // Регистрация типа без учета пространства имен
            qRegisterMetaTypeStreamOperators<T>(type_name += 2);
            return;
        }
        ++type_name;
    }
}
