/****************************************************************************
  В модуле представлены механизмы по расширенному логированию.

****************************************************************************/

#pragma once

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include "logger.h"

namespace lblog
{
using namespace std;

/**
  Функция выполняет разбор файла конфигурации и назначает в логгер новые
  сэйверы и фильтры.
  Файл конфигурации имеет следующий формат:
  [SAVER]          Наименование группы параметров для сэйвера
    name           Наименование сэйвера
    level          Уровень логирования (error, warning, info, verbose, debug, debug2)
    filters        Список фильтров для данного сэйвера (перечисляются через запятую)
    file           Имя лог-файла.

  [FILTER_MODULE]  Наименование группы параметров для фильтра по имени модуля
    name           Наименование фильтра
    mode           Режим работы фильтра (include/exclude)
    modules        Список модулей по которым осуществляется фильтрование
                   (перечисляются через запятую).

  [FILTER_LEVEL]   Наименование группы параметров для фильтра по уровню логирования
    name           Наименование фильтра
    level          Режим работы фильтра (include/exclude)
    modules        Список модулей по которым осуществляется фильтрование
                   (перечисляются через запятую).
*/
void configParser(const string& filePath, Logger&);




} // namespace lblog