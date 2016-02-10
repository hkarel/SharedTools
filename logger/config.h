/* clang-format off */
/****************************************************************************
  В модуле представлены механизмы конфигурирования расширенного логирования.

****************************************************************************/

#pragma once

#include "logger.h"
#include <list>

namespace alog
{
using namespace std;

/**
    Функция выполняет разбор файла конфигурации и составляет список сэйверов.

    %YAML 1.2 нотация файла конфигурации
    ---
    filters:
        # Наименование фильтра
      - name: filter1

        # Тип фильтрации:
        #    module_name - по именам модулей, список модулей задается
        #                  через параметр modules: [];
        #    log_level   - по уровню логирования, уровень логирования задается
        #                  через параметр level.
        #    func_name   - по именам функций, список функций задается
        #                  через параметр functions: [];
        #    file_name   - по именам файлов, список файлов задается
        #                  через параметр files: [];
        #    thread_id   - по идентификаторам потоков, список потоков
        #                  задается через параметр threads: [].
        type: module_name

        # Режим работы фильтра: include - включающий; exclude - исключающий
        mode: exclude

        # Определяет будут ли сообщения об ошибках фильтроваться так же,
        # как и все остальные сообщения. по умолчанию сообщения об ошибках
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
        # active: true

        # Уровень логирования (error, warning, info, verbose, debug, debug2)
        level: debug

        # Устанавливает ограничение на максимальную длину строки сообщения.
        # Длина строки не ограничивается если значение равно 0, если значение
        # меньше нуля, то берется значение по умолчанию равное 5000.
        max_line_size: -1

        # Список фильтров для данного сэйвера
        filters: [filter1]

        # Имя лог-файла.
        file: ./lbucd2.log1

      - name: saver2
        level: debug
        filters: [filter2]
        file: ./lbucd2.log2

      - name: saver3
        level: debug2
        filters: [filter2, filter3]
        file: ./lbucd2.log3
        ...
*/
bool loadSavers(const string& confFile, std::list<SaverLPtr>& savers);

//
void printSaversInfo();




} // namespace alog
