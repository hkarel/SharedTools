/*****************************************************************************
  В модуле определены пути до mysql_client библиотеки.

*****************************************************************************/

import qbs
import '../LibModule.qbs' as LibModule

LibModule {
    id: mysql_client
    prefix: "/opt/mysql-connector-c"
    version: "6.0.x";
}
