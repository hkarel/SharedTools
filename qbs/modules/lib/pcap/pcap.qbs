/*****************************************************************************
  В модуле определены пути до libpcap библиотеки.

*****************************************************************************/

import qbs
import '../LibModule.qbs' as LibModule

LibModule {
    id: pcap
    prefix: "/opt/libpcap"
    version: "1.7.x"
}