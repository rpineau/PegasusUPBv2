#include "powerMain.h"

extern "C" PlugInExport int sbPlugInName2(BasicStringInterface& str)
{
    str = "Pegasus Astro Ultimate Powerbox v2 power PlugIn";

    return 0;
}

extern "C" PlugInExport int sbPlugInFactory2(    const char* pszDisplayName,
                                             const int& nInstanceIndex,
                                             SerXInterface                    * pSerXIn,
                                             TheSkyXFacadeForDriversInterface    * pTheSkyXIn,
                                             SleeperInterface                    * pSleeperIn,
                                             BasicIniUtilInterface            * pIniUtilIn,
                                             LoggerInterface                    * pLoggerIn,
                                             MutexInterface                    * pIOMutexIn,
                                             TickCountInterface                * pTickCountIn,
                                             void** ppObjectOut)
{
    *ppObjectOut = NULL;
    X2PowerControl* gpMyImpl=NULL;

    if (NULL == gpMyImpl)
        gpMyImpl = new X2PowerControl(    pszDisplayName,
                                      nInstanceIndex,
                                      pSerXIn,
                                      pTheSkyXIn,
                                      pSleeperIn,
                                      pIniUtilIn,
                                      pLoggerIn,
                                      pIOMutexIn,
                                      pTickCountIn);

    *ppObjectOut = gpMyImpl;

    return 0;
}

