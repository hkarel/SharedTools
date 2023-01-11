/* clang-format off */
/*****************************************************************************
  The MIT License

  Copyright © 2013 Pavel Karelin (hkarel), <hkarel@yandex.ru>

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

  В модуле представлены механизмы конфигурирования системы логирования.

*****************************************************************************/

#pragma once

#include "logger/logger.h"
#include <yaml-cpp/yaml.h>

namespace alog {

using namespace std;

/**
Описание параметров для файла конфигурации системы логирования. Загрузка файла
конфигурации выполняется функцией loadSavers().

%YAML 1.2 нотация файла конфигурации
---
filters:
    # Наименование фильтра
  - name: filter1

    # Тип фильтрации:
    #    module_name - по именам модулей, список модулей задается
    #                  через параметр modules: [];
    #    log_level   - по уровню логирования, уровень логирования задается через
    #                  параметр level, так же для этого типа фильтрации задается
    #                  список модулей через параметр modules: []
    #                  Если режим работы  фильтра - include,  то  фильтр  будет
    #                  обрабатывать все модули из списка modules, если же режим
    #                  работы фильтра - exclude, то в этом случае будут  обрабо-
    #                  таны все модули не входящие в список modules.
    #                  Режим работы exclude - основной к использованию.  В этом
    #                  режиме все модули из списка  modules  будут отображаться
    #                  согласно уровню логирования заданному в сэйвере, а модули
    #                  не вошедшие в список modules будут отображаться согласно
    #                  значению заданному в log_level;
    #    func_name   - по именам функций, список функций задается через параметр
    #                  functions: [];
    #    file_name   - по именам файлов, список файлов задается  через параметр
    #                  files: []. Допускается после имени файла указывать номер
    #                  строки. Имя файла и номер строки должны  быть  разделены
    #                  символом ':'. Пример: file.cpp:10;
    #    thread_id   - по идентификаторам потоков, список потоков задается через
    #                  параметр threads: [];
    #    content     - по контенту сообщения, список элементов по которым  будет
    #                  выполняться обработка задается через параметр contents:[]
    type: module_name

    # Режим работы фильтра: include - включающий; exclude - исключающий
    mode: exclude

    # Определяет будут ли сообщения об ошибках фильтроваться так же,
    # как и все остальные сообщения. По умолчанию сообщения об ошибках
    # не фильтруются (false).
    filtering_errors: false

    # Определяет будут ли в лог-файл включены дополнительные сообщения
    # которые находятся в одном и том же потоке с основными фильтруемыми
    # сообщениями. По умолчанию параметр равен 'false'.
    follow_thread_context: false

    # Определяет будут ли неименованные модули обрабатываться данным фильтром.
    # По умолчанию неименованные модули не фильтруются (false).
    filtering_noname_modules: false

    # Список модулей по которым осуществляется фильтрация
    modules: [
        LaunchTask,
        ScriptRun,
        Synchronizer,
        DbDriver,
        JsonClient,
        JsonDispatcher,
    ]

  - name: filter2
    type: module_name
    mode: include
    filtering_errors: true
    filtering_noname_modules: true
    modules: [LogConfig]

  - name: filter3
    type: log_level
    mode: exclude
    level: info
    modules: [LaunchDispatcher, LaunchTask, DbDriver]

savers:
    # Наименование сэйвера. Наименование 'default' зарезервировано
    # за сейвером по умолчанию. Сейвер по умолчанию можно переопределить
    # в этом файле конфигурации.
  - name: saver1

    # Признак активности сэйвера. Если сэйвер неактивен - запись в лог-файл
    # не производится. По умолчанию параметр равен 'true'.
    # Примечание: аналогичного эффекта можно добиться если уровень
    # логгирования 'level' выставить в 'none'.
    active: true

    # Уровень логирования (error, warning, info, verbose, debug, debug2)
    level: debug

    # Устанавливает ограничение на максимальную длину строки сообщения.
    # Длина строки не ограничивается если значение равно 0, если значение
    # меньше нуля, то берется значение по умолчанию равное 5000.
    max_line_size: -1

    # Список фильтров для данного сэйвера
    filters: [filter1]

    # Имя лог-файла.
    file: ./lbucd2.log.1

    # Если параметр равен 'true', то запись данных будет продолжена
    # в существующий лог-файл, в противном случае лог-файл будет очищен
    # при создании сэйвера.
    continue: true

  - name: saver2
    active: true
    level: debug
    filters: [filter2]
    file: ./lbucd2.log.2

  - name: saver3
    active: false
    level: debug2
    filters: [filter2, filter3]
    file: ./lbucd2.log.3
...
*/

// Получает список фильтров из указанной yaml-ноды
bool loadFilters(const YAML::Node& filtersNode, FilterList& filters,
                 const string& confFile);

// Загрузка сэйверов из отдельного файла конфигурации
bool loadSavers(const string& confFile, SaverList& savers);
bool loadSavers(const string& confFile);

// Выводит в лог информацию об используемых фильтрах и сэйверах
void printSaversInfo();

} // namespace alog
