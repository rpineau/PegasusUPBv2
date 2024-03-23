//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1


#include "pegasus_upbv2.h"


CPegasusUPBv2Power::CPegasusUPBv2Power()
{
    m_globalStatus.nDeviceType = NONE_POWER;
    m_globalStatus.bReady = false;
    memset(m_globalStatus.szVersion,0,sizeof(upbStatus));

    m_pSerx = NULL;
    m_pSleeper = NULL;
    

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\X2_PegasusUPBv2PowerLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2PowerLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2PowerLog.txt";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPegasusUPBv2Power] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPegasusUPBv2Power] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif

}

CPegasusUPBv2Power::~CPegasusUPBv2Power()
{
#ifdef    PLUGIN_DEBUG
    // Close LogFile
    if(m_sLogFile.is_open())
        m_sLogFile.close();
#endif
}

int CPegasusUPBv2Power::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK;
    int nDevice;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // 9600 8N1
    if (!m_pSerx->isConnected()) {
        nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
        if(nErr == 0)
            m_bIsConnected = true;
        else
            m_bIsConnected = false;
    }
    else
        m_bIsConnected = true;
    
    if(!m_bIsConnected)
        return nErr;

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::this_thread::yield();
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] connected to " << pszPort << std::endl;
    m_sLogFile.flush();
#endif

    // get status so we can figure out what device we are connecting to.
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] getting device type." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != UPBv2_POWER) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect]  **** ERROR **** getting device type." << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }
    
    nErr = setLedStatus(ON_POWER); // needed on REV C and up for the focuser motor to work.
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect]  **** ERROR **** setting LED on for stepper motor on REV C." << std::endl;
        m_sLogFile.flush();
#endif
    }

    nErr = getFirmwareVersion(m_szFirmwareVersion, TEXT_BUFFER_SIZE);

    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect]  **** ERROR **** getting device full status." << std::endl;
        m_sLogFile.flush();
#endif
    }


    setPortOn(DEW1, m_bPWMA_On);
    setPortOn(DEW2, m_bPWMB_On);
    setPortOn(DEW3, m_bPWMC_On);

    return nErr;
}

void CPegasusUPBv2Power::Disconnect(int nInstanceCount)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Disconnect] Called." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_bIsConnected && m_pSerx && nInstanceCount==1)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark getters and setters
int CPegasusUPBv2Power::getStatus()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getStatus] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // calling twice to clear any previous errors
    nErr = upbCommand("P#\n", szResp, SERIAL_BUFFER_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for a bit, background thread will update m_CCA_Settings.bIsMoving
    std::this_thread::yield();
    nErr = upbCommand("P#\n", szResp, SERIAL_BUFFER_SIZE);

    if(nErr) {
        return nErr;
    }
    if(strstr(szResp,"_OK")) {
        if(strstr(szResp,"UPB2")) {
            m_globalStatus.nDeviceType = UPBv2_POWER;
        }
        else {
            m_globalStatus.nDeviceType = NONE_POWER;
             return ERR_DEVICENOTSUPPORTED;
        }
        nErr = PLUGIN_OK;
    }
    else {
        nErr = COMMAND_FAILED_UPBv2_POWER;
    }
    return nErr;
}


int CPegasusUPBv2Power::getConsolidatedStatus()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] Called." << std::endl;
    m_sLogFile.flush();
#endif


    nErr = upbCommand("PA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] ERROR = " << nErr << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] about to parse response." << std::endl;
    m_sLogFile.flush();
#endif

    // parse response
    parseResp(szResp, m_svParsedRespForPA);
    if(nErr) {

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] parse error = " << nErr << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] response parsing done." << std::endl;
    m_sLogFile.flush();
#endif

    if(m_svParsedRespForPA.size()<18) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] parsing returned an incomplete answer." << std::endl;
        m_sLogFile.flush();
#endif
        return BAD_CMD_RESPONSE_UPBv2_POWER;
    }


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] converting value and setting them in m_globalStatus." << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbDevice]   = " << m_svParsedRespForPA[upbDevice] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbVoltage]  = " << m_svParsedRespForPA[upbVoltage] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbCurrent]  = " << m_svParsedRespForPA[upbCurrent] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbPower]    = " << m_svParsedRespForPA[upbPower] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbTemp]     = " << m_svParsedRespForPA[upbTemp] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbHumidity] = " << m_svParsedRespForPA[upbHumidity] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] m_svParsedRespForPA[upbDewPoint] = " << m_svParsedRespForPA[upbDewPoint] << std::endl;
    m_sLogFile.flush();
#endif


    m_globalStatus.fVoltage = std::stof(m_svParsedRespForPA[upbVoltage]);
    m_globalStatus.fCurent = std::stof(m_svParsedRespForPA[upbCurrent]);
    m_globalStatus.nPower = std::stoi(m_svParsedRespForPA[upbPower]);
    if (m_svParsedRespForPA[upbTemp].find("nan") == -1) {
        m_globalStatus.fTemp = std::stof(m_svParsedRespForPA[upbTemp]);
    }
    else
        m_globalStatus.fTemp = -100.0f;

    m_globalStatus.nHumidity = std::stoi(m_svParsedRespForPA[upbHumidity]);

    if (m_svParsedRespForPA[upbDewPoint].find("nan") == -1) {
        m_globalStatus.fDewPoint = std::stof(m_svParsedRespForPA[upbDewPoint]);
    }
    else
        m_globalStatus.fDewPoint = -273.15f;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] converting value and setting them in m_globalStatus." << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fVoltage   = " << std::fixed << std::setprecision(2) << m_globalStatus.fVoltage << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurent    = " << std::fixed << std::setprecision(2) << m_globalStatus.fCurent << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nPower     = " << std::fixed << std::setprecision(2) << m_globalStatus.nPower << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fTemp      = " << std::fixed << std::setprecision(2) << m_globalStatus.fTemp << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nHumidity  = " << std::fixed << std::setprecision(2)<< m_globalStatus.nHumidity << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] mfDewPoint = " << std::fixed << std::setprecision(2) << m_globalStatus.fDewPoint << std::endl;
    m_sLogFile.flush();
#endif

    m_globalStatus.bPort1On = m_svParsedRespForPA[upbPortStatus].at(0) == '1'? true : false;
    m_globalStatus.bPort2On = m_svParsedRespForPA[upbPortStatus].at(1) == '1'? true : false;
    m_globalStatus.bPort3On = m_svParsedRespForPA[upbPortStatus].at(2) == '1'? true : false;
    m_globalStatus.bPort4On = m_svParsedRespForPA[upbPortStatus].at(3) == '1'? true : false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nPortStatus = " << m_svParsedRespForPA[upbPortStatus] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bPort1On = " << (m_globalStatus.bPort1On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bPort2On = " << (m_globalStatus.bPort2On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bPort3On = " << (m_globalStatus.bPort3On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bPort4On = " << (m_globalStatus.bPort4On?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    if(m_svParsedRespForPA[upbUsbStatus].size() == 6) {
        m_globalStatus.bUsbPort1On = m_svParsedRespForPA[upbUsbStatus].at(0) == '1' ? true: false;
        m_globalStatus.bUsbPort2On = m_svParsedRespForPA[upbUsbStatus].at(1) == '1' ? true: false;
        m_globalStatus.bUsbPort3On = m_svParsedRespForPA[upbUsbStatus].at(2) == '1' ? true: false;
        m_globalStatus.bUsbPort4On = m_svParsedRespForPA[upbUsbStatus].at(3) == '1' ? true: false;
        m_globalStatus.bUsbPort5On = m_svParsedRespForPA[upbUsbStatus].at(4) == '1' ? true: false;
        m_globalStatus.bUsbPort6On = m_svParsedRespForPA[upbUsbStatus].at(5) == '1' ? true: false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort1On = " << m_svParsedRespForPA[upbUsbStatus].at(0) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort1On = " << (m_globalStatus.bUsbPort1On?"Yes":"No") << std::endl;

        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort2On = " << m_svParsedRespForPA[upbUsbStatus].at(1) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort2On = " << (m_globalStatus.bUsbPort2On?"Yes":"No") << std::endl;

        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort3On = " << m_svParsedRespForPA[upbUsbStatus].at(2) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort3On = " << (m_globalStatus.bUsbPort3On?"Yes":"No") << std::endl;

        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort4On = " << m_svParsedRespForPA[upbUsbStatus].at(3) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort4On = " << (m_globalStatus.bUsbPort4On?"Yes":"No") << std::endl;

        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort5On = " << m_svParsedRespForPA[upbUsbStatus].at(4) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort5On = " << (m_globalStatus.bUsbPort5On?"Yes":"No") << std::endl;

        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nUsbPort6On = " << m_svParsedRespForPA[upbUsbStatus].at(5) << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bUsbPort6On = " << (m_globalStatus.bUsbPort6On?"Yes":"No") << std::endl;

        m_sLogFile.flush();
#endif

    }


    m_globalStatus.nDew1PWM = std::stoi(m_svParsedRespForPA[upbDew1PWM]);
    m_globalStatus.nDew2PWM = std::stoi(m_svParsedRespForPA[upbDew2PWM]);
    m_globalStatus.nDew3PWM = std::stoi(m_svParsedRespForPA[upbDew3PWM]);

    m_globalStatus.fCurrentPort1 = std::stof(m_svParsedRespForPA[upbCurrentPort1])/480;
    m_globalStatus.fCurrentPort2 = std::stof(m_svParsedRespForPA[upbCurrentPort2])/480;
    m_globalStatus.fCurrentPort3 = std::stof(m_svParsedRespForPA[upbCurrentPort3])/480;
    m_globalStatus.fCurrentPort4 = std::stof(m_svParsedRespForPA[upbCurrentPort4])/480;

    m_globalStatus.fCurrentDew1 = std::stof(m_svParsedRespForPA[upbCurrentDew1])/480;
    m_globalStatus.fCurrentDew2 = std::stof(m_svParsedRespForPA[upbCurrentDew2])/480;
    m_globalStatus.fCurrentDew3 = std::stof(m_svParsedRespForPA[upbCurrentDew3])/700;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nDew1PWM     = " <<  m_globalStatus.nDew1PWM << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nDew2PWM      = " <<  m_globalStatus.nDew2PWM << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nDew3PWM      = " <<  m_globalStatus.nDew3PWM << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentPort1 = " << std::fixed << std::setprecision(2) << m_globalStatus.fCurrentPort1 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentPort2 = " << std::fixed << std::setprecision(2) << m_globalStatus.fCurrentPort2 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentPort3 = " << std::fixed << std::setprecision(2) << m_globalStatus.fCurrentPort3 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentPort4 = " << std::fixed << std::setprecision(2) << m_globalStatus.fCurrentPort4 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentDew1  = " << std::fixed << std::setprecision(2)<< m_globalStatus.fCurrentDew1 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentDew2  = " << std::fixed << std::setprecision(2)<< m_globalStatus.fCurrentDew2 << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] fCurrentDew3  = " << std::fixed << std::setprecision(2)<< m_globalStatus.fCurrentDew3 << std::endl;
    m_sLogFile.flush();
#endif

    m_globalStatus.bOverCurrentPort1 = m_svParsedRespForPA[upbOvercurent].at(0) == '1'? true : false;
    m_globalStatus.bOverCurrentPort2 = m_svParsedRespForPA[upbOvercurent].at(1) == '1'? true : false;
    m_globalStatus.bOverCurrentPort3 = m_svParsedRespForPA[upbOvercurent].at(2) == '1'? true : false;
    m_globalStatus.bOverCurrentPort4 = m_svParsedRespForPA[upbOvercurent].at(3) == '1'? true : false;

    m_globalStatus.bOverCurrentDew1 = m_svParsedRespForPA[upbOvercurent].at(4) == '1'? true : false;
    m_globalStatus.bOverCurrentDew2 = m_svParsedRespForPA[upbOvercurent].at(5) == '1'? true : false;
    m_globalStatus.bOverCurrentDew3 = m_svParsedRespForPA[upbOvercurent].at(6) == '1'? true : false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] nOvercurrentStatus = " << m_svParsedRespForPA[upbOvercurent] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentPort1 = " << (m_globalStatus.bOverCurrentPort1?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentPort2 = " << (m_globalStatus.bOverCurrentPort2?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentPort3 = " << (m_globalStatus.bOverCurrentPort3?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentPort4 = " << (m_globalStatus.bOverCurrentPort4?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentDew1 = " << (m_globalStatus.bOverCurrentDew1?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentDew2 = " << (m_globalStatus.bOverCurrentDew2?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bOverCurrentDew3 = " << (m_globalStatus.bOverCurrentDew3?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    switch (m_svParsedRespForPA[upbAutodew].at(0)) {
        case '0' :
            m_globalStatus.bAutoDewCh1 = false;
            m_globalStatus.bAutoDewCh2 = false;
            m_globalStatus.bAutoDewCh3 = false;
            break;

        case '1' :
            m_globalStatus.bAutoDewCh1 = true;
            m_globalStatus.bAutoDewCh2 = true;
            m_globalStatus.bAutoDewCh3 = true;
            break;

        case '2' :
            m_globalStatus.bAutoDewCh1 = true;
            m_globalStatus.bAutoDewCh2 = false;
            m_globalStatus.bAutoDewCh3 = false;
            break;

        case '3' :
            m_globalStatus.bAutoDewCh1 = false;
            m_globalStatus.bAutoDewCh2 = true;
            m_globalStatus.bAutoDewCh3 = false;
            break;

        case '4' :
            m_globalStatus.bAutoDewCh1 = false;
            m_globalStatus.bAutoDewCh2 = false;
            m_globalStatus.bAutoDewCh3 = true;
            break;

        case '5' :
            m_globalStatus.bAutoDewCh1 = true;
            m_globalStatus.bAutoDewCh2 = true;
            m_globalStatus.bAutoDewCh3 = false;
            break;

        case '6' :
            m_globalStatus.bAutoDewCh1 = true;
            m_globalStatus.bAutoDewCh2 = false;
            m_globalStatus.bAutoDewCh3 = true;
            break;

        case '7' :
            m_globalStatus.bAutoDewCh1 = false;
            m_globalStatus.bAutoDewCh2 = true;
            m_globalStatus.bAutoDewCh3 = true;
            break;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bAutoDew1 = " << (m_globalStatus.bAutoDewCh1?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bAutoDew2 = " << (m_globalStatus.bAutoDewCh2?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] bAutoDew3 = " << (m_globalStatus.bAutoDewCh3?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif


    // get on boot port state and adjustable volts port settings.
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus]  Getting on boot power port status" << std::endl;
    m_sLogFile.flush();
#endif


    nErr = getOnBootPowerState();
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] getOnBootPowerState Error detting on boot power port status, ERROR = " << nErr  << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }
    nErr = getOnBootUsbState();
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus] getOnBootUsbState Error detting on boot usb port status, ERROR = " << nErr  << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getConsolidatedStatus]  All data collected" << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CPegasusUPBv2Power::getOnBootPowerState()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] Called." << std::endl;
    m_sLogFile.flush();
#endif

    // get power state for all 4 ports
    nErr = upbCommand("PS\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>1 && sParsedResp[1].size()>=4) {
        m_globalStatus.bOnBootPort1On = sParsedResp[1].at(0) == '1'? true : false;
        m_globalStatus.bOnBootPort2On = sParsedResp[1].at(1) == '1'? true : false;
        m_globalStatus.bOnBootPort3On = sParsedResp[1].at(2) == '1'? true : false;
        m_globalStatus.bOnBootPort4On = sParsedResp[1].at(3) == '1'? true : false;
        m_globalStatus.nPortAdjVolts = std::stoi(sParsedResp[2]);
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] sParsedResp[1] = " << sParsedResp[1] << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootPort1On = " << (m_globalStatus.bOnBootPort1On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootPort2On = " << (m_globalStatus.bOnBootPort2On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootPort3On = " << (m_globalStatus.bOnBootPort3On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootPort4On = " << (m_globalStatus.bOnBootPort4On?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CPegasusUPBv2Power::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersion] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = upbCommand("PV\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

void CPegasusUPBv2Power::getFirmwareVersionString(std::string &sFirmware)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersionString] Called." << std::endl;
    m_sLogFile.flush();
#endif

    sFirmware.assign(m_szFirmwareVersion);
}


int CPegasusUPBv2Power::getTemperature(double &dTemperature)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getTemperature] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = upbCommand("ST\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // convert response
    if(strstr(szResp,"nan")) {
        dTemperature = -100.0f;
    }
    else
        dTemperature = atof(szResp);

    return nErr;
}

int CPegasusUPBv2Power::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPosition] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = upbCommand("SP\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPosition] Error getting position : " << nErr << std::endl;
        m_sLogFile.flush();
#endif
    return nErr;
    }
    // convert response
    nPosition = atoi(szResp);

    return nErr;
}


int CPegasusUPBv2Power::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setLedStatus] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch (nStatus) {
        case ON_POWER:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_ON_POWER);
            break;
        case OFF_POWER:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_OFF_POWER);
            break;

        default:
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getPosition] Error nStatus : " << nStatus << std::endl;
            m_sLogFile.flush();
#endif
            return ERR_CMDFAILED;
            break;
    }
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2Power::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getDeviceType] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = getStatus();
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}

void CPegasusUPBv2Power::getDeviceTypeString(std::string &sDeviceType)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getDeviceTypeString] Called." << std::endl;
    m_sLogFile.flush();
#endif

    if( m_globalStatus.nDeviceType== UPBv2_POWER)
        sDeviceType = "Ultimate Powerbox V2";
    else
        sDeviceType = "Unknown device";

}


float CPegasusUPBv2Power::getVoltage()
{
    return m_globalStatus.fVoltage;
}

float CPegasusUPBv2Power::getCurrent()
{
    return m_globalStatus.fCurent;
}

int CPegasusUPBv2Power::getPower()
{
    return m_globalStatus.nPower;
}

float CPegasusUPBv2Power::getTemp()
{
    return m_globalStatus.fTemp;
}

int CPegasusUPBv2Power::getHumidity()
{
    return m_globalStatus.nHumidity;
}

float CPegasusUPBv2Power::getDewPoint()
{
    return m_globalStatus.fDewPoint;

}

bool CPegasusUPBv2Power::getPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case DC1:
            return m_globalStatus.bPort1On;
            break;

        case DC2:
            return m_globalStatus.bPort2On;
            break;

        case DC3:
            return m_globalStatus.bPort3On;
            break;

        case DC4:
            return m_globalStatus.bPort4On;
            break;

        case DEW1 : // Dew Heater A
            return m_bPWMA_On;
            break;
        case DEW2 : // Dew Heater B
            return m_bPWMB_On;
            break;
        case DEW3 : // Dew Heater C
            return m_bPWMC_On;
            break;
        case USB1 : // usb 1
            return getUsbPortState(1);
            break;
        case USB2 : // usb 2
            return getUsbPortState(2);
            break;
        case USB3 : // usb 3
            return getUsbPortState(3);
            break;
        case USB4 : // usb 4
            return getUsbPortState(4);
            break;
        case USB5 : // usb 5
            return getUsbPortState(5);
            break;
        case USB6 : // usb 6
            return getUsbPortState(6);
            break;

        default:
            return false;
            break;
    }
}

int CPegasusUPBv2Power::setPortOn(const int &nPortNumber, const bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPortOn] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nPortNumber) {
        case DC1 :
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P1:%d\n", bEnabled?1:0);
            m_globalStatus.bPort1On = bEnabled;
            break;
        case DC2 :
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P2:%d\n", bEnabled?1:0);
            m_globalStatus.bPort2On = bEnabled;
            break;
        case DC3 :
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P3:%d\n", bEnabled?1:0);
            m_globalStatus.bPort3On = bEnabled;
            break;
        case DC4 :
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P4:%d\n", bEnabled?1:0);
            m_globalStatus.bPort4On = bEnabled;
            break;
        case DEW1 : // Dew Heater A
            nErr = setDewHeaterPWM(DewHeaterA, bEnabled?m_nPWMA:0);
            m_bPWMA_On = bEnabled;
            return nErr;
            break;
        case DEW2 : // Dew Heater B
            nErr = setDewHeaterPWM(DewHeaterB, bEnabled?m_nPWMB:0);
            m_bPWMB_On = bEnabled;
            return nErr;
            break;
        case DEW3 : // Dew Heater C
            nErr = setDewHeaterPWM(DewHeaterC, bEnabled?m_nPWMC:0);
            m_bPWMC_On = bEnabled;
            return nErr;
            break;
        case USB1 : // usb 1
            setUsbPortState(1,bEnabled);
            break;
        case USB2 : // usb 2
            setUsbPortState(2,bEnabled);
            break;
        case USB3 : // usb 3
            setUsbPortState(3,bEnabled);
            break;
        case USB4 : // usb 4
            setUsbPortState(4,bEnabled);
            break;
        case USB5 : // usb 5
            setUsbPortState(5,bEnabled);
            break;
        case USB6 : // usb 6
            setUsbPortState(6,bEnabled);
            break;
        default:
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPortOn] Error nPortNumber : " << nPortNumber << std::endl;
            m_sLogFile.flush();
#endif
            return ERR_CMDFAILED;
            break;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPortOn] calling upbCommand : " << szCmd << std::endl;
    m_sLogFile.flush();
#endif


    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setPortOn] calling upbCommand nErr : " << nErr << std::endl;
    m_sLogFile.flush();
#endif


    return nErr;
}


float CPegasusUPBv2Power::getPortCurrent(const int &nPortNumber)
{
    switch(nPortNumber) {
        case DC1:
            return m_globalStatus.fCurrentPort1;
            break;

        case DC2:
            return m_globalStatus.fCurrentPort2;
            break;

        case DC3:
            return m_globalStatus.fCurrentPort3;
            break;

        case DC4:
            return m_globalStatus.fCurrentPort4;
            break;
        case DEW1 : // Dew Heater A
            return getDewHeaterCurrent(DewHeaterA);
            break;
        case DEW2 : // Dew Heater B
            return getDewHeaterCurrent(DewHeaterB);
            break;
        case DEW3 : // Dew Heater C
            return getDewHeaterCurrent(DewHeaterC);
            break;
        default:
            return 0.0f;
            break;
    }
}


bool CPegasusUPBv2Power::getOnBootPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case DC1:
            return m_globalStatus.bOnBootPort1On;
            break;
        case DC2:
            return m_globalStatus.bOnBootPort2On;
            break;
        case DC3:
            return m_globalStatus.bOnBootPort3On;
            break;
        case DC4:
            return m_globalStatus.bOnBootPort4On;
            break;
        default:
            return false;
            break;
    }
}

int CPegasusUPBv2Power::setOnBootPortOn(const int &nPortNumber, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setOnBootPortOn] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nPortNumber) {
        case DC1 :
            m_globalStatus.bOnBootPort1On = bEnable;
            break;
            
        case DC2 :
            m_globalStatus.bOnBootPort2On = bEnable;
            break;
            
        case DC3 :
            m_globalStatus.bOnBootPort3On = bEnable;
            break;
            
        case DC4 :
            m_globalStatus.bOnBootPort4On = bEnable;
            break;
            
        default:
            return false;
            break;
    }

    sPorts.clear();
    sPorts += m_globalStatus.bOnBootPort1On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort2On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort3On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort4On? "1" : "0";

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PE:%s\n", sPorts.c_str());
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    
    return nErr;
}


bool CPegasusUPBv2Power::isOverCurrentPort(const int &nPortNumber)
{
    switch(nPortNumber) {
        case DC1 :
            return m_globalStatus.bOverCurrentPort1;
            break;

        case DC2 :
            return m_globalStatus.bOverCurrentPort2;
            break;

        case DC3 :
            return m_globalStatus.bOverCurrentPort3;
            break;

        case DC4 :
            return m_globalStatus.bOverCurrentPort4;
            break;

        default:
            return false;
            break;
    }
}

bool CPegasusUPBv2Power::isOverCurrentDewHeater(const int &nPortNumber)
{
    switch(nPortNumber) {
        case DewHeaterA:
            return m_globalStatus.bOverCurrentDew1;
            break;

        case DewHeaterB:
            return m_globalStatus.bOverCurrentDew2;
            break;

        case DewHeaterC:
            return m_globalStatus.bOverCurrentDew3;
            break;

        default:
            return false;
            break;
    }
}


int CPegasusUPBv2Power::setUsbPortState(const int nPortID, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setUsbPortState] Called." << std::endl;
    m_sLogFile.flush();
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "U%d:%s\n", nPortID, bEnable?"1":"0");
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    switch(nPortID) {
        case 1:
            m_globalStatus.bUsbPort1On = bEnable;
            break;
        case 2:
            m_globalStatus.bUsbPort2On = bEnable;
            break;
        case 3:
            m_globalStatus.bUsbPort3On = bEnable;
            break;
        case 4:
            m_globalStatus.bUsbPort4On = bEnable;
            break;
        case 5:
            m_globalStatus.bUsbPort5On = bEnable;
            break;
        case 6:
            m_globalStatus.bUsbPort6On = bEnable;
            break;
    }
    return nErr;
}

bool CPegasusUPBv2Power::getUsbPortState(const int nPortID)
{

    switch(nPortID) {
        case 1:
            return m_globalStatus.bUsbPort1On;
            break;
        case 2:
            return m_globalStatus.bUsbPort2On;
            break;
        case 3:
            return m_globalStatus.bUsbPort3On;
            break;
        case 4:
            return m_globalStatus.bUsbPort4On;
            break;
        case 5:
            return m_globalStatus.bUsbPort5On;
            break;
        case 6:
            return m_globalStatus.bUsbPort6On;
            break;
    }

    return false;
}

bool CPegasusUPBv2Power::getOnBootUsbPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.bOnBootUsbPort1On;
            break;
        case 2:
            return m_globalStatus.bOnBootUsbPort2On;
            break;
        case 3:
            return m_globalStatus.bOnBootUsbPort3On;
            break;
        case 4:
            return m_globalStatus.bOnBootUsbPort4On;
            break;
        case 5:
            return m_globalStatus.bOnBootUsbPort5On;
            break;
        case 6:
            return m_globalStatus.bOnBootUsbPort6On;
            break;
        default:
            return false;
            break;
    }

}

int CPegasusUPBv2Power::setOnBootUsbPortOn(const int &nPortNumber, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setOnBootUsbPortOn] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nPortNumber) {
        case 1 :
            m_globalStatus.bOnBootUsbPort1On = bEnable;
            break;

        case 2 :
            m_globalStatus.bOnBootUsbPort2On = bEnable;
            break;
        case 3 :
            m_globalStatus.bOnBootUsbPort3On = bEnable;
            break;
        case 4 :
            m_globalStatus.bOnBootUsbPort4On = bEnable;
            break;
        case 5 :
            m_globalStatus.bOnBootUsbPort5On = bEnable;
            break;
        case 6 :
            m_globalStatus.bOnBootUsbPort6On = bEnable;
            break;

        default:
            return false;
            break;
    }

    sPorts.clear();
    sPorts += m_globalStatus.bOnBootUsbPort1On? "1" : "0";
    sPorts += m_globalStatus.bOnBootUsbPort2On? "1" : "0";
    sPorts += m_globalStatus.bOnBootUsbPort3On? "1" : "0";
    sPorts += m_globalStatus.bOnBootUsbPort4On? "1" : "0";
    sPorts += m_globalStatus.bOnBootUsbPort5On? "1" : "0";
    sPorts += m_globalStatus.bOnBootUsbPort6On? "1" : "0";

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "US:%s\n", sPorts.c_str());
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2Power::getOnBootUsbState(void)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootUsbState] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = upbCommand("US:99\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
    // parse response
    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>=1 && sParsedResp[0].size()>=6) {
        m_globalStatus.bOnBootUsbPort1On = sParsedResp[0].at(0) == '1'? true : false;
        m_globalStatus.bOnBootUsbPort2On = sParsedResp[0].at(1) == '1'? true : false;
        m_globalStatus.bOnBootUsbPort3On = sParsedResp[0].at(2) == '1'? true : false;
        m_globalStatus.bOnBootUsbPort4On = sParsedResp[0].at(3) == '1'? true : false;
        m_globalStatus.bOnBootUsbPort5On = sParsedResp[0].at(4) == '1'? true : false;
        m_globalStatus.bOnBootUsbPort6On = sParsedResp[0].at(5) == '1'? true : false;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort1On = " << (m_globalStatus.bOnBootUsbPort1On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort2On = " << (m_globalStatus.bOnBootUsbPort2On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort3On = " << (m_globalStatus.bOnBootUsbPort3On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort4On = " << (m_globalStatus.bOnBootUsbPort4On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort5On = " << (m_globalStatus.bOnBootUsbPort5On?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getOnBootPowerState] bOnBootUsbPort6On = " << (m_globalStatus.bOnBootUsbPort6On?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CPegasusUPBv2Power::setDewHeaterPWM(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDewHeaterPWM] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nDewHeater) {
        case DewHeaterA:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P5:%d\n", nPWM);
            m_globalStatus.nDew1PWM = nPWM;
            break;
        case DewHeaterB:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P6:%d\n", nPWM);
            m_globalStatus.nDew2PWM = nPWM;
            break;
        case DewHeaterC:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P7:%d\n", nPWM);
            m_globalStatus.nDew2PWM = nPWM;
            break;
        default:
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setDewHeaterPWM] Error nDewHeater : " << nDewHeater << std::endl;
            m_sLogFile.flush();
#endif
            return ERR_CMDFAILED;
            break;
    }

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

int CPegasusUPBv2Power::getDewHeaterPWM(const int &nDewHeater)
{
    switch(nDewHeater) {
        case DewHeaterA:
            return m_nPWMA;
            break;
        case DewHeaterB:
            return m_nPWMB;
            break;
        case DewHeaterC:
            return m_nPWMC;
            break;
        default:
            return -1;
            break;
    }
}

float CPegasusUPBv2Power::getDewHeaterCurrent(const int &nDewHeater)
{
    switch(nDewHeater) {
        case DewHeaterA:
            return m_globalStatus.fCurrentDew1;
            break;
        case DewHeaterB:
            return m_globalStatus.fCurrentDew2;
            break;
        case DewHeaterC:
            return m_globalStatus.fCurrentDew3;
            break;
        default:
            return -1;
            break;
    }
}

int CPegasusUPBv2Power::setDewHeaterPWMVal(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK;
    bool bOn = false;

    switch(nDewHeater) {
        case DewHeaterA:
            m_nPWMA = nPWM;
            bOn = m_bPWMA_On;
            break;
        case DewHeaterB:
            m_nPWMB = nPWM;
            bOn = m_bPWMB_On;
            break;
        case DewHeaterC:
            m_nPWMC = nPWM;
            bOn = m_bPWMC_On;
            break;
        default:
            break;
    }
    if(bOn)
        nErr = setDewHeaterPWM(nDewHeater, nPWM);

    return nErr;
}

void CPegasusUPBv2Power::setDewHeaterOnVal(const int &nDewHeater, const bool bOn)
{
    int nErr = PLUGIN_OK;

    switch(nDewHeater) {
        case DewHeaterA:
            m_bPWMA_On = bOn;
            break;
        case DewHeaterB:
            m_bPWMB_On = bOn;
            break;
        case DewHeaterC:
            m_bPWMC_On = bOn;
            break;
        default:
            break;
    }
}


int CPegasusUPBv2Power::getAdjPortVolts(int &nVolts)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getAdjPortVolts] Called." << std::endl;
    m_sLogFile.flush();
#endif

    nErr = upbCommand("P8\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>1) {
        m_nAdjVolts = std::stoi(sParsedResp[1]);
    }

    nVolts = m_nAdjVolts;
    return nErr;
}


int CPegasusUPBv2Power::setAdjPortVolts(int nVolts)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setAdjPortVolts] Called." << std::endl;
    m_sLogFile.flush();
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "P8:%d\n", nVolts);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}


int CPegasusUPBv2Power::setAutoDewOn(const int nPWMPort,const bool &bOn)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    int nAutoDewVal;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setAutoDewOn] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nPWMPort) {
        case DewHeaterA:
            m_globalStatus.bAutoDewCh1 = bOn;
            break;

        case DewHeaterB:
            m_globalStatus.bAutoDewCh2 = bOn ;
            break;

        case DewHeaterC:
            m_globalStatus.bAutoDewCh3 = bOn;
            break;
    }

    // test all combo :(
    nAutoDewVal = 0;
    if (m_globalStatus.bAutoDewCh1 && m_globalStatus.bAutoDewCh2 && m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 1;
    }
    else if (m_globalStatus.bAutoDewCh1 && !m_globalStatus.bAutoDewCh2 && !m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 2;
       }
    else if (!m_globalStatus.bAutoDewCh1 && m_globalStatus.bAutoDewCh2 && !m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 3;
       }
    else if (!m_globalStatus.bAutoDewCh1 && !m_globalStatus.bAutoDewCh2 && m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 4;
       }
    else if (m_globalStatus.bAutoDewCh1 && m_globalStatus.bAutoDewCh2 && !m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 5;
       }
    else if (m_globalStatus.bAutoDewCh1 && !m_globalStatus.bAutoDewCh2 && m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 6;
       }
    else if (!m_globalStatus.bAutoDewCh1 && m_globalStatus.bAutoDewCh2 && m_globalStatus.bAutoDewCh3 ) {
        nAutoDewVal = 7;
       }

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PD:%d\n", nAutoDewVal);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setAutoDewOn] nPWMPort " << nPWMPort << " : " << (bOn?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setAutoDewOn] nAutoDewVal : " << nAutoDewVal << std::endl;
    m_sLogFile.flush();
#endif

    getConsolidatedStatus();
    return nErr;
}


bool CPegasusUPBv2Power::isAutoDewOn(const int nPWMPort)
{
    bool bDewOn = false;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isAutoDewOn] Called." << std::endl;
    m_sLogFile.flush();
#endif

    switch(nPWMPort) {
        case DewHeaterA:
            bDewOn = m_globalStatus.bAutoDewCh1;
            break;

        case DewHeaterB:
            bDewOn = m_globalStatus.bAutoDewCh2;
            break;

        case DewHeaterC:
            bDewOn = m_globalStatus.bAutoDewCh3;
            break;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isAutoDewOn] Port " << nPWMPort << " : " << (bDewOn?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return bDewOn;
}

int CPegasusUPBv2Power::getAutoDewAggressivness(int &nLevel)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getAutoDewAggressivness] Called." << std::endl;
    m_sLogFile.flush();
#endif


    m_nAutoDewAgressiveness = 210; // default value
    nErr = upbCommand("DA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>1) {
        m_nAutoDewAgressiveness = std::stoi(sParsedResp[1]);
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getAutoDewAggressivness] m_nAutoDewAgressiveness " << m_nAutoDewAgressiveness << std::endl;
    m_sLogFile.flush();
#endif

    nLevel = m_nAutoDewAgressiveness;
    return nErr;
}

int CPegasusUPBv2Power::setAutoDewAggressivness(int nLevel)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setAutoDewAggressivness] Called." << std::endl;
    m_sLogFile.flush();
#endif

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PD:%d\n", nLevel);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    // setting auto dew agressiveness also enable auto-dew
    m_globalStatus.bAutoDewCh1 = true;
    m_globalStatus.bAutoDewCh2 = true;
    m_globalStatus.bAutoDewCh3 = true;
    m_bPWMA_On = true;
    m_bPWMB_On = true;
    m_bPWMC_On = true;
    return nErr;
}

int CPegasusUPBv2Power::getPortCount()
{
    return NB_PORTS;
}


#pragma mark command and response functions

int CPegasusUPBv2Power::upbCommand(const char *pszszCmd, char *pszResult, unsigned long nResultMaxLen, int nTimeout)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [upbCommand] Sending : " << pszszCmd << std::endl;
    m_sLogFile.flush();
#endif

    nErr = m_pSerx->writeFile((void *)pszszCmd, strlen(pszszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    // printf("Command %s sent. wrote %lu bytes\n", szCmd, ulBytesWrite);
    if(nErr){
        return nErr;
    }

    if(pszResult) {
        // read response
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE, nTimeout);
        if(nErr){
            return nErr;
        }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [upbCommand] response : \"" << szResp <<"\"" << std::endl;
        m_sLogFile.flush();
#endif

        // printf("Got response : %s\n",resp);
        strncpy(pszResult, szResp, nResultMaxLen);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [upbCommand] response copied to pszResult : \"" << pszResult <<"\"" << std::endl;
        m_sLogFile.flush();
#endif

    }
    return nErr;
}


int CPegasusUPBv2Power::readResponse(char *szRespBuffer, unsigned long nBufferLen, int nTimeout)
{
    int nErr = PLUGIN_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
    int nBytesWaiting = 0 ;
    int nbTimeouts = 0;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] called" << std::endl;
    m_sLogFile.flush();
#endif

    memset(szRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->bytesWaitingRx(nBytesWaiting);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] nBytesWaiting : " << nBytesWaiting << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] nBytesWaiting nErr : " << nErr << std::endl;
        m_sLogFile.flush();
#endif

        if(!nBytesWaiting) {
            if(nbTimeouts++ >= NB_RX_WAIT) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] bytesWaitingRx timeout, no data for " << NB_RX_WAIT  << " loops" << std::endl;
                m_sLogFile.flush();
#endif
                nErr = ERR_RXTIMEOUT;
                break;
            }
            m_pSleeper->sleep(MAX_READ_WAIT_TIMEOUT);
            continue;
        }
        nbTimeouts = 0;
        if(ulTotalBytesRead + nBytesWaiting <= nBufferLen)
            nErr = m_pSerx->readFile(pszBufPtr, nBytesWaiting, ulBytesRead, nTimeout);
        else {
            nErr = ERR_RXTIMEOUT;
            break; // buffer is full.. there is a problem !!
        }
        if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] readFile error : " << nErr << std::endl;
            m_sLogFile.flush();
#endif

            return nErr;
        }

        if (ulBytesRead != nBytesWaiting) { // timeout
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] readFile Timeout Error" << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] readFile nBytesWaiting  : " << nBytesWaiting << std::endl;
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [readResponse] readFile ulBytesRead    : " << ulBytesRead << std::endl;
            m_sLogFile.flush();
#endif
        }

        ulTotalBytesRead += ulBytesRead;
        pszBufPtr+=ulBytesRead;
    } while (ulTotalBytesRead < nBufferLen  && *(pszBufPtr-1) != '\n');

    if(!ulTotalBytesRead)
        nErr = COMMAND_TIMEOUT_UPBv2_POWER; // we didn't get an answer.. so timeout
    else
        *(pszBufPtr-1) = 0; //remove the \n

    return nErr;
}



void CPegasusUPBv2Power::parseResp(char *pszResp, std::vector<std::string>  &svParsedResp)
{
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [parseResp] called" << std::endl;
    m_sLogFile.flush();
#endif

	svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [parseResp] sSegment : " << sSegment << std::endl;
        m_sLogFile.flush();
#endif
    }

    svParsedResp = svSeglist;


}


#ifdef PLUGIN_DEBUG
const std::string CPegasusUPBv2Power::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void CPegasusUPBv2Power::log(std::string sText)
{
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [log] " << sText << std::endl;
    m_sLogFile.flush();
}
#endif
