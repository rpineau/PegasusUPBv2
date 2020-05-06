//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1



#ifndef __PEGASUS_C__
#define __PEGASUS_C__

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

#define PLUGIN_DEBUG 2

#define DRIVER_VERSION      1.01

#define SERIAL_BUFFER_SIZE 1024
#define MAX_TIMEOUT 1000
#define TEXT_BUFFER_SIZE    1024

enum DMFC_Errors    {PLUGIN_OK = 0, NOT_CONNECTED, ND_CANT_CONNECT, UPB_BAD_CMD_RESPONSE, COMMAND_FAILED};
enum DeviceType     {NONE = 0, UPBv2};
enum GetLedStatus   {OFF = 0, ON};
enum SetLEdStatus   {SWITCH_OFF = 1, SWITCH_ON};
enum MotorDir       {NORMAL = 0 , REVERSE};
enum MotorStatus    {IDLE = 0, MOVING};


typedef struct {
    double  dTemperature;
    int     nCurPos;
    bool    bMoving;
    bool    bReverse;
    int     nBacklash;
} upbFocuser;

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
    int     nPort8Volts;
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
    upbFocuser focuser;
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


// field indexes in response for SA command
#define upbFPos         0
#define upbFMotorState  1
#define upbFMotorInvert 2
#define upbFBacklash    3

class CPegasusUPBv2
{
public:
    CPegasusUPBv2();
    ~CPegasusUPBv2();

    int         Connect(const char *pszPort);
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void        setLogger(LoggerInterface *pLogger) { m_pLogger = pLogger; };

    // move commands
    int         haltFocuser();
    int         gotoPosition(int nPos);
    int         moveRelativeToPosision(int nSteps);

    // command complete functions
    int         isGoToComplete(bool &bComplete);
    int         isMotorMoving(bool &bMoving);

    // getter and setter
    int         getStatus(int &nStatus);
    int         getStepperStatus(void);
    int         getConsolidatedStatus(void);
    int         getOnBootPowerState(void);

    int         getMotoMaxSpeed(int &nSpeed);
    int         setMotoMaxSpeed(int nSpeed);

    int         getBacklashComp(int &nSteps);
    int         setBacklashComp(int nSteps);

    int         getFirmwareVersion(char *pszVersion, int nStrMaxLen);
    int         getTemperature(double &dTemperature);

    int         getPosition(int &nPosition);

    int         getLedStatus(int &nStatus);
    int         setLedStatus(int nStatus);

    int         syncMotorPosition(int nPos);

    int         getDeviceType(int &nDevice);

    int         getPosLimit(void);
    void        setPosLimit(int nLimit);

    bool        isPosLimitEnabled(void);
    void        enablePosLimit(bool bEnable);

    int         setReverseEnable(bool bEnabled);
    int         getReverseEnable(bool &bEnabled);

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
    
    int         setUsbHubState(const bool &bEnable);
    int         getUsbHubState(bool &bEnable);

    int         setDewHeaterPWM(const int &nDewHeater, const int &nPWM);
    int         getDewHeaterPWM(const int &nDewHeater);
    float       getDewHeaterCurrent(const int &nDewHeater);
    
    int         getAdjPortVolts();
    int         setAdjPortVolts(int nVolts);
    
    int         setAutoDewOn(const int nPWMPort, const bool &bOn);
    bool        isAutoDewOn(const int nPWMPort);
    int         getPWMPortAutoDew();
    
protected:

    int             upbCommand(const char *pszCmd, char *pszResult, unsigned long nResultMaxLen);
    int             readResponse(char *pszRespBuffer, unsigned long nBufferLen);
    void            parseResp(char *pszResp, std::vector<std::string>  &sParsedRes);



    SerXInterface   *m_pSerx;
    LoggerInterface *m_pLogger;

    bool            m_bDebugLog;
    bool            m_bIsConnected;
    char            m_szFirmwareVersion[TEXT_BUFFER_SIZE];

    std::vector<std::string>    m_svParsedRespForSA;
    std::vector<std::string>    m_svParsedRespForPA;

    upbStatus       m_globalStatus;
    int             m_nTargetPos;
    int             m_nPosLimit;
    bool            m_bPosLimitEnabled;
	bool			m_bAbborted;
	
#ifdef PLUGIN_DEBUG
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};

#endif //__PEGASUS_C__
