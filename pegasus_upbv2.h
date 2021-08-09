//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1



#ifndef __PEGASUS_UPBv2_Power__
#define __PEGASUS_UPBv2_Power__

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <math.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <exception>
#include <typeinfo>
#include <stdexcept>

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

// #define PLUGIN_DEBUG_UPBv2_POWER 4

#define DRIVER_VERSION_POWER      1.16

#define SERIAL_BUFFER_SIZE 1024
#define MAX_TIMEOUT 2500
#define MAX_READ_WAIT_TIMEOUT 25
#define NB_RX_WAIT 25

#define TEXT_BUFFER_SIZE    1024

#define NB_PORTS 13

enum Errors_UPBv2_POWER         {PLUGIN_OK_UPBv2_POWER = 0, NOT_CONNECTED_UPBv2_POWER, CANT_CONNECT_UPBv2_POWER, BAD_CMD_RESPONSE_UPBv2_POWER, COMMAND_FAILED_UPBv2_POWER, COMMAND_TIMEOUT_UPBv2_POWER};
enum DeviceType_UPBv2_POWER     {NONE_POWER = 0, UPBv2_POWER};
enum GetLedStatus_UPBv2_POWER   {OFF_POWER = 0, ON_POWER};
enum SetLEdStatus_UPBv2_POWER   {SWITCH_OFF_POWER = 0, SWITCH_ON_POWER};
enum DewHeaters_UPBv2_POWER     {DewHeaterA = 0, DewHeaterB, DewHeaterC};

enum PortNumber     {DC1 = 0, DC2, DC3, DC4, DEW1, DEW2, DEW3, USB1, USB2, USB3, USB4, USB5, USB6};

typedef struct {
    int     nDeviceType;
    bool    bReady;
    char    szVersion[TEXT_BUFFER_SIZE];
    int     nLedStatus;
    float   fVoltage;
    float   fCurent;
    int     nPower;
    float   fTemp;
    int     nHumidity;
    float   fDewPoint;
    bool    bPort1On;
    bool    bPort2On;
    bool    bPort3On;
    bool    bPort4On;
    bool    bOnBootPort1On;
    bool    bOnBootPort2On;
    bool    bOnBootPort3On;
    bool    bOnBootPort4On;
    bool    bUsbPort1On;
    bool    bUsbPort2On;
    bool    bUsbPort3On;
    bool    bUsbPort4On;
    bool    bUsbPort5On;
    bool    bUsbPort6On;
    bool    bOnBootUsbPort1On;
    bool    bOnBootUsbPort2On;
    bool    bOnBootUsbPort3On;
    bool    bOnBootUsbPort4On;
    bool    bOnBootUsbPort5On;
    bool    bOnBootUsbPort6On;
    int     nDew1PWM;
    int     nDew2PWM;
    int     nDew3PWM;
    float   fCurrentPort1;
    float   fCurrentPort2;
    float   fCurrentPort3;
    float   fCurrentPort4;
    float   fCurrentDew1;
    float   fCurrentDew2;
    float   fCurrentDew3;
    int     nPortAdjVolts;
    bool    bOverCurrentPort1;
    bool    bOverCurrentPort2;
    bool    bOverCurrentPort3;
    bool    bOverCurrentPort4;
    bool    bOverCurrentDew1;
    bool    bOverCurrentDew2;
    bool    bOverCurrentDew3;
    bool    bAutoDewCh1;
    bool    bAutoDewCh2;
    bool    bAutoDewCh3;
} upbStatus;

// field indexes in response for PA command
#define upbDevice       0
#define upbVoltage      1
#define upbCurrent      2
#define upbPower        3
#define upbTemp         4
#define upbHumidity     5
#define upbDewPoint     6
#define upbPortStatus   7
#define upbUsbStatus    8
#define upbDew1PWM      9
#define upbDew2PWM      10
#define upbDew3PWM      11
#define upbCurrentPort1 12
#define upbCurrentPort2 13
#define upbCurrentPort3 14
#define upbCurrentPort4 15
#define upbCurrentDew1  16
#define upbCurrentDew2  17
#define upbCurrentDew3  18
#define upbOvercurent   19
#define upbAutodew      20


class CPegasusUPBv2Power
{
public:
    CPegasusUPBv2Power();
    ~CPegasusUPBv2Power();

    int         Connect(const char *pszPort);
    void        Disconnect(int nInstanceCount);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper;};

    // getter and setter
    int         getStatus();
    int         getConsolidatedStatus(void);
    int         getOnBootPowerState(void);

    int         getFirmwareVersion(char *pszVersion, int nStrMaxLen);
    void        getFirmwareVersionString(std::string &sFirmware);
    int         getTemperature(double &dTemperature);

    int         getPosition(int &nPosition);

    int         setLedStatus(int nStatus);

    int         getDeviceType(int &nDevice);
    void        getDeviceTypeString(std::string &sDeviceType);

    float       getVoltage();
    float       getCurrent();
    int         getPower();
    float       getTemp();
    int         getHumidity();
    float       getDewPoint();

    bool        getPortOn(const int &nPortNumber);
    int         setPortOn(const int &nPortNumber, const bool &bEnabled);
    float       getPortCurrent(const int &nPortNumber);
    bool        getOnBootPortOn(const int &nPortNumber);
    int         setOnBootPortOn(const int &nPortNumber, const bool &bEnable);
    bool        isOverCurrentPort(const int &nPortNumber);
    bool        isOverCurrentDewHeater(const int &nPortNumber);
    
    int         setUsbPortState(const int nPortID, const bool &bEnable);
    bool        getUsbPortState(const int nPortID);
    bool        getOnBootUsbPortOn(const int &nPortNumber);
    int         setOnBootUsbPortOn(const int &nPortNumber, const bool &bEnable);
    int         getOnBootUsbState(void);

    int         setDewHeaterPWM(const int &nDewHeater, const int &nPWM);
    int         getDewHeaterPWM(const int &nDewHeater);
    float       getDewHeaterCurrent(const int &nDewHeater);
    int         setDewHeaterPWMVal(const int &nDewHeater, const int &nPWM);

    int         getAdjPortVolts(int &nVolts);
    int         setAdjPortVolts(int nVolts);

    int         setAutoDewOn(const int nPWMPort, const bool &bOn);
    bool        isAutoDewOn(const int nPWMPort);
    int         getPWMPortAutoDew();
    int         getAutoDewAggressivness(int &nLevel);
    int         setAutoDewAggressivness(int nLevel);
    int         getPortCount();

protected:

    int             upbCommand(const char *pszCmd, char *pszResult, unsigned long nResultMaxLen, int nTimeout = MAX_TIMEOUT);
    int             readResponse(char *pszRespBuffer, unsigned long nBufferLen, int nTimeout = MAX_TIMEOUT);
    void            parseResp(char *pszResp, std::vector<std::string>  &sParsedRes);



    SerXInterface       *m_pSerx;
    SleeperInterface    *m_pSleeper;

    bool            m_bDebugLog;
    bool            m_bIsConnected;
    char            m_szFirmwareVersion[TEXT_BUFFER_SIZE];

    std::vector<std::string>    m_svParsedRespForSA;
    std::vector<std::string>    m_svParsedRespForPA;

    upbStatus       m_globalStatus;

    int             m_nAdjVolts;
    bool            m_bAdjOn;
    int             m_nAdjVoltsVal;
    
    int             m_nPWMA;
    bool            m_bPWMA_On;
    int             m_nPWMB;
    bool            m_bPWMB_On;
    int             m_nPWMC;
    bool            m_bPWMC_On;
    int             m_nAutoDewAgressiveness;

#ifdef PLUGIN_DEBUG_UPBv2_POWER
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};

#endif //__PEGASUS_C__
