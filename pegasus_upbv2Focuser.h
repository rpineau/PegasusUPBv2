//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1



#ifndef __PEGASUS_UPBV2_FOC__
#define __PEGASUS_UPBV2_FOC__

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
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

// #define PLUGIN_DEBUG_UPBv2_FOC 4

#define DRIVER_VERSION_FOC     1.16

#define SERIAL_BUFFER_SIZE 1024
#define MAX_TIMEOUT 2500
#define MAX_READ_WAIT_TIMEOUT 25
#define NB_RX_WAIT 25

#define TEXT_BUFFER_SIZE    1024

enum Errors_UPBV2_FOC    {PLUGIN_OK_UPBV2_FOC = 0, NOT_CONNECTED_UPBV2_FOC, CANT_CONNECT_UPBV2_FOC, BAD_CMD_RESPONSE_UPBV2_FOC, COMMAND_FAILED_UPBV2_FOC, COMMAND_TIMEOUT_UPBv2_FOC};
enum DeviceType_UPBV2_FOC     {NONE_FOC = 0, UPBv2_FOC};
enum GetLedStatus_UPBV2_FOC   {OFF_FOC = 0, ON_FOC};
enum SetLEdStatus_UPBV2_FOC   {SWITCH_OFF_FOC = 0, SWITCH_ON_FOC};
enum MotorDir_UPBV2_FOC       {NORMAL = 0 , REVERSE};
enum MotorStatus_UPBV2_FOC    {IDLE = 0, MOVING};


typedef struct {
    double  dTemperature;
    int     nCurPos;
    bool    bMoving;
    bool    bReverse;
    int     nBacklash;
} UPBv2Focuser;

typedef struct {
    int     nDeviceType;
    bool    bReady;
    char    szVersion[TEXT_BUFFER_SIZE];
    int     nLedStatus;
    float   fTemp;
    UPBv2Focuser focuser;
} UPBv2Status;

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


// field indexes in response for SA command
#define upbFPos         0
#define upbFMotorState  1
#define upbFMotorInvert 2
#define upbFBacklash    3

class CPegasusUPBv2Focuser
{
public:
    CPegasusUPBv2Focuser();
    ~CPegasusUPBv2Focuser();

    int         Connect(const char *pszPort);
    void        Disconnect(int nInstanceCount);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void        getSerxPointer(SerXInterface *p) { p = m_pSerx; };
    void        setLogger(LoggerInterface *pLogger) { m_pLogger = pLogger; };
    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper;};

    // move commands
    int         haltFocuser();
    int         gotoPosition(int nPos);
    int         moveRelativeToPosision(int nSteps);

    // command complete functions
    int         isGoToComplete(bool &bComplete);
    int         isMotorMoving(bool &bMoving);

    // getter and setter
    int         getStatus();
    int         getStepperStatus(void);
    int         getConsolidatedStatus(void);

    int         getMotoMaxSpeed(int &nSpeed);
    int         setMotoMaxSpeed(int nSpeed);

    int         getBacklashComp(int &nSteps);
    int         setBacklashComp(int nSteps);

    int         getFirmwareVersion(char *pszVersion, int nStrMaxLen);
    void        getFirmwareString(std::string &sFirmware);

    int         getTemperature(double &dTemperature);

    int         getPosition(int &nPosition);

    int         setLedStatus(int nStatus);

    int         syncMotorPosition(int nPos);

    int         getDeviceType(int &nDevice);
    void        getDeviceTypeString(std::string &sDeviceType);
    int         getPosLimit(void);
    void        setPosLimit(int nLimit);

    bool        isPosLimitEnabled(void);
    void        enablePosLimit(bool bEnable);

    int         setReverseEnable(bool bEnabled);
    int         getReverseEnable(bool &bEnabled);

    int         getAccelerationEnabled(bool &bEnabled);
    int         setAccelerationEnabled(bool bEnabled);

    float       getTemp();

protected:
    int             upbCommand(const char *pszCmd, char *pszResult, unsigned long nResultMaxLen, int nTimeout = MAX_TIMEOUT);
    int             readResponse(char *pszRespBuffer, unsigned long nBufferLen, int nTimeout = MAX_TIMEOUT);
    void            parseResp(char *pszResp, std::vector<std::string>  &sParsedRes);



    SerXInterface       *m_pSerx;
    LoggerInterface     *m_pLogger;
    SleeperInterface    *m_pSleeper;

    bool            m_bDebugLog;
    bool            m_bIsConnected;
    char            m_szFirmwareVersion[TEXT_BUFFER_SIZE];

    std::vector<std::string>    m_svParsedRespForSA;
    std::vector<std::string>    m_svParsedRespForPA;

    UPBv2Status     m_globalStatus;
    int             m_nTargetPos;
    int             m_nPosLimit;
    bool            m_bPosLimitEnabled;
	bool			m_bAbborted;
	
#ifdef PLUGIN_DEBUG_UPBv2_FOC
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};

#endif //__PEGASUS_C__
