#ifdef SB_WIN_BUILD
#define PlugInExport __declspec(dllexport)
#else
#define PlugInExport
#endif

#include <stdio.h>
#include "x2powercontrol.h"
#include "../../licensedinterfaces/basicstringinterface.h"

class BasicStringInterface;
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class BasicIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class TickCountInterface;

extern "C" PlugInExport int sbPlugInName2(BasicStringInterface& str);

extern "C" PlugInExport int sbPlugInFactory2(   const char* pszDisplayName,
                                             const int& nInstanceIndex,
                                             SerXInterface                    * pSerXIn,
                                             TheSkyXFacadeForDriversInterface    * pTheSkyXIn,
                                             SleeperInterface                    * pSleeperIn,
                                             BasicIniUtilInterface            * pIniUtilIn,
                                             LoggerInterface                    * pLoggerIn,
                                             MutexInterface                    * pIOMutexIn,
                                             TickCountInterface                * pTickCountIn,
                                             void** ppObjectOut);
