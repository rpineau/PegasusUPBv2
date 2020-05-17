//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1


#include "pegasus_upbv2.h"


CPegasusUPBv2::CPegasusUPBv2()
{
    m_globalStatus.nDeviceType = NONE;
    m_globalStatus.bReady = false;
    memset(m_globalStatus.szVersion,0,TEXT_BUFFER_SIZE);

    m_nTargetPos = 0;
    m_nPosLimit = 0;
    m_bPosLimitEnabled = false;
    m_bAbborted = false;
    
    m_pSerx = NULL;
    m_pLogger = NULL;
    m_pSleeper = NULL;
    

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PegasusUPBv2Log.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2Log.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2Log.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::CPegasusUPBv2] version %3.2f build 2020_05_16_13_00.\n", timestamp, DRIVER_VERSION);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::CPegasusUPBv2] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusUPBv2::~CPegasusUPBv2()
{
#ifdef	PLUGIN_DEBUG
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusUPBv2::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK;
    int nDevice;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif

    // 9600 8N1
    nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
    if(nErr == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return nErr;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2::Connect getting device type\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != UPBv2) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusUPBv2::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    
    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusUPBv2::Connect **** ERROR **** getting device full status\n", timestamp);
        fflush(Logfile);
#endif
    }

    return nErr;
}

void CPegasusUPBv2::Disconnect()
{
    if(m_bIsConnected && m_pSerx)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark move commands
int CPegasusUPBv2::haltFocuser()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = upbCommand("SH\n", szResp, SERIAL_BUFFER_SIZE);
	m_bAbborted = true;
	
	return nErr;
}

int CPegasusUPBv2::gotoPosition(int nPos)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    if (m_bPosLimitEnabled && nPos>m_nPosLimit)
        return ERR_LIMITSEXCEEDED;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusUPBv2::gotoPosition moving to %d\n", timestamp, nPos);
    fflush(Logfile);
#endif

    sprintf(szCmd,"SM:%d\n", nPos);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    m_nTargetPos = nPos;

    return nErr;
}

int CPegasusUPBv2::moveRelativeToPosision(int nSteps)
{
    int nErr = PLUGIN_OK;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusUPBv2::moveRelativeToPosision moving by %d steps\n", timestamp, nSteps);
    fflush(Logfile);
#endif

    m_nTargetPos = m_globalStatus.focuser.nCurPos + nSteps;

    nErr = gotoPosition(m_nTargetPos);
    
    return nErr;
}

#pragma mark command complete functions

int CPegasusUPBv2::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    bool bIsMoving;
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


    nErr = getPosition(m_globalStatus.focuser.nCurPos);
    nErr |= isMotorMoving(bIsMoving);
    
    #ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::isGoToComplete] motor is moving ? : %s\n", timestamp, bIsMoving?"Yes":"No");
        fprintf(Logfile, "[%s] [CPegasusUPBv2::isGoToComplete] current position  : %d\n", timestamp, m_globalStatus.focuser.nCurPos);
        fprintf(Logfile, "[%s] [CPegasusUPBv2::isGoToComplete] target position   : %d\n", timestamp, m_nTargetPos);
        fflush(Logfile);
    #endif

	if(m_bAbborted) {
		bComplete = true;
		m_nTargetPos = m_globalStatus.focuser.nCurPos;
		m_bAbborted = false;
	}
    else if(m_globalStatus.focuser.nCurPos == m_nTargetPos)
        bComplete = true;
    else
        bComplete = false;
    return nErr;
}

int CPegasusUPBv2::isMotorMoving(bool &bMoving)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("SI\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    if(atoi(szResp)) {
        bMoving = true;
        m_globalStatus.focuser.bMoving = MOVING;
    }
    else {
        bMoving = false;
        m_globalStatus.focuser.bMoving = IDLE;
    }

    return nErr;
}

#pragma mark getters and setters
int CPegasusUPBv2::getStatus()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    // OK_UPB or OK_PPB
    nErr = upbCommand("P#\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    if(strstr(szResp,"_OK")) {
        if(strstr(szResp,"UPB2")) {
            m_globalStatus.nDeviceType = UPBv2;
        }
        else {
            m_globalStatus.nDeviceType = NONE;
            return ERR_DEVICENOTSUPPORTED;
        }
        nErr = PLUGIN_OK;
    }
    else {
        nErr = COMMAND_FAILED;
    }
    return nErr;
}

int CPegasusUPBv2::getStepperStatus()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = upbCommand("SA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse results.
    parseResp(szResp, m_svParsedRespForSA);
    if(m_svParsedRespForSA.size()<4) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getStepperStatus] parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return UPB_BAD_CMD_RESPONSE;
    }
    m_globalStatus.focuser.nCurPos = std::stoi(m_svParsedRespForSA[upbFPos]);
    m_globalStatus.focuser.bMoving = std::stoi(m_svParsedRespForSA[upbFMotorState]) == 1?true:false;
    m_globalStatus.focuser.bReverse = std::stoi(m_svParsedRespForSA[upbFMotorInvert] )== 1?true:false;
    m_globalStatus.focuser.nBacklash = std::stoi(m_svParsedRespForSA[upbFBacklash]);

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getStepperStatus] nCurPos = %d\n", timestamp, m_globalStatus.focuser.nCurPos);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getStepperStatus] bMoving = %s\n", timestamp, m_globalStatus.focuser.bMoving?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getStepperStatus] bReverse = %s\n", timestamp, m_globalStatus.focuser.bReverse?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getStepperStatus] nBacklash = %d\n", timestamp, m_globalStatus.focuser.nBacklash);
    fflush(Logfile);
#endif
    

    return nErr;
}

int CPegasusUPBv2::getConsolidatedStatus()
{
    int nErr;
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = getStepperStatus();
    if(nErr)
        return nErr;

    nErr = upbCommand("PA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus]  about to parse response\n", timestamp);
	fflush(Logfile);
#endif

    // parse response
    parseResp(szResp, m_svParsedRespForPA);
    if(nErr) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus]  response parsing done\n", timestamp);
	fflush(Logfile);
#endif

    if(m_svParsedRespForPA.size()<18) {
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return UPB_BAD_CMD_RESPONSE;
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] converting value and setting them in m_globalStatus\n", timestamp);
    fflush(Logfile);
#endif

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbDevice] = %s \n", timestamp, m_svParsedRespForPA[upbDevice].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbVoltage] = %s \n", timestamp, m_svParsedRespForPA[upbVoltage].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbCurrent] = %s \n", timestamp, m_svParsedRespForPA[upbCurrent].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbPower] = %s \n", timestamp, m_svParsedRespForPA[upbPower].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbTemp] = %s \n", timestamp, m_svParsedRespForPA[upbTemp].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbHumidity] = %s \n", timestamp, m_svParsedRespForPA[upbHumidity].c_str());
    fflush(Logfile);
    fprintf(Logfile, "[%s][CPegasusUPBv2::getConsolidatedStatus] m_svParsedRespForPA[upbDewPoint] = %s \n", timestamp, m_svParsedRespForPA[upbDewPoint].c_str());
    fflush(Logfile);
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

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fVoltage = %3.2f\n", timestamp, m_globalStatus.fVoltage);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurent = %3.2f\n", timestamp, m_globalStatus.fCurent);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nPower = %d\n", timestamp, m_globalStatus.nPower);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fTemp = %3.2f\n", timestamp, m_globalStatus.fTemp);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nHumidity = %d\n", timestamp, m_globalStatus.nHumidity);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fDewPoint = %3.2f\n", timestamp, m_globalStatus.fDewPoint);
    fflush(Logfile);
#endif

    
    m_globalStatus.bPort1On = m_svParsedRespForPA[upbPortStatus].at(0) == '1'? true : false;
    m_globalStatus.bPort2On = m_svParsedRespForPA[upbPortStatus].at(1) == '1'? true : false;
    m_globalStatus.bPort3On = m_svParsedRespForPA[upbPortStatus].at(2) == '1'? true : false;
    m_globalStatus.bPort4On = m_svParsedRespForPA[upbPortStatus].at(3) == '1'? true : false;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nPortStatus = %s\n", timestamp, m_svParsedRespForPA[upbPortStatus].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bPort1On = %s\n", timestamp, m_globalStatus.bPort1On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bPort2On = %s\n", timestamp, m_globalStatus.bPort2On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bPort3On = %s\n", timestamp, m_globalStatus.bPort3On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bPort4On = %s\n", timestamp, m_globalStatus.bPort4On?"Yes":"No");
    fflush(Logfile);
#endif
    if(m_svParsedRespForPA[upbUsbStatus].size() == 6) {
        m_globalStatus.bUsbPort1On = m_svParsedRespForPA[upbUsbStatus].at(0) == '1' ? true: false;
        m_globalStatus.bUsbPort2On = m_svParsedRespForPA[upbUsbStatus].at(1) == '1' ? true: false;
        m_globalStatus.bUsbPort3On = m_svParsedRespForPA[upbUsbStatus].at(2) == '1' ? true: false;
        m_globalStatus.bUsbPort4On = m_svParsedRespForPA[upbUsbStatus].at(3) == '1' ? true: false;
        m_globalStatus.bUsbPort5On = m_svParsedRespForPA[upbUsbStatus].at(4) == '1' ? true: false;
        m_globalStatus.bUsbPort6On = m_svParsedRespForPA[upbUsbStatus].at(5) == '1' ? true: false;
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort1On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(0));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort1On = %s\n", timestamp, m_globalStatus.bUsbPort1On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort2On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(1));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort2On = %s\n", timestamp, m_globalStatus.bUsbPort2On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort3On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(2));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort3On = %s\n", timestamp, m_globalStatus.bUsbPort3On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort4On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(3));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort4On = %s\n", timestamp, m_globalStatus.bUsbPort4On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort5On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(4));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort5On = %s\n", timestamp, m_globalStatus.bUsbPort5On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nUsbPort6On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(5));
        fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bUsbPort6On = %s\n", timestamp, m_globalStatus.bUsbPort6On?"Yes":"No");

        fflush(Logfile);
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

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nDew1PWM = %d\n", timestamp, m_globalStatus.nDew1PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nDew2PWM = %d\n", timestamp, m_globalStatus.nDew2PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nDew3PWM = %d\n", timestamp, m_globalStatus.nDew3PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentPort1 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort1);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentPort2 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort2);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentPort3 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort3);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentPort4 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort4);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentDew1 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew1);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentDew2 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew2);
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] fCurrentDew3 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew3);
    fflush(Logfile);
#endif

    m_globalStatus.bOverCurrentPort1 = m_svParsedRespForPA[upbOvercurent].at(0) == '1'? true : false;
    m_globalStatus.bOverCurrentPort2 = m_svParsedRespForPA[upbOvercurent].at(1) == '1'? true : false;
    m_globalStatus.bOverCurrentPort3 = m_svParsedRespForPA[upbOvercurent].at(2) == '1'? true : false;
    m_globalStatus.bOverCurrentPort4 = m_svParsedRespForPA[upbOvercurent].at(3) == '1'? true : false;

    m_globalStatus.bOverCurrentDew1 = m_svParsedRespForPA[upbOvercurent].at(4) == '1'? true : false;
    m_globalStatus.bOverCurrentDew2 = m_svParsedRespForPA[upbOvercurent].at(5) == '1'? true : false;
    m_globalStatus.bOverCurrentDew3 = m_svParsedRespForPA[upbOvercurent].at(6) == '1'? true : false;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] nOvercurrentStatus = %s\n", timestamp, m_svParsedRespForPA[upbOvercurent].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentPort1 = %s\n", timestamp, m_globalStatus.bOverCurrentPort1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentPort2 = %s\n", timestamp, m_globalStatus.bOverCurrentPort2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentPort3 = %s\n", timestamp, m_globalStatus.bOverCurrentPort3?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentPort4 = %s\n", timestamp, m_globalStatus.bOverCurrentPort4?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentDew1 = %s\n", timestamp, m_globalStatus.bOverCurrentDew1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentDew2 = %s\n", timestamp, m_globalStatus.bOverCurrentDew2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bOverCurrentDew2 = %s\n", timestamp, m_globalStatus.bOverCurrentDew3?"Yes":"No");
    fflush(Logfile);
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

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bAutoDew1 = %s\n", timestamp, m_globalStatus.bAutoDewCh1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bAutoDew2 = %s\n", timestamp, m_globalStatus.bAutoDewCh2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] bAutoDew3 = %s\n", timestamp, m_globalStatus.bAutoDewCh3?"Yes":"No");
    fflush(Logfile);
#endif

    // get on boot port state and adjustable volts port settings.

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] Getting on boot power port status\n", timestamp);
    fflush(Logfile);
#endif

    nErr = getOnBootPowerState();

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getConsolidatedStatus] All data collected\n", timestamp);
    fflush(Logfile);
#endif
    return nErr;
}

int CPegasusUPBv2::getOnBootPowerState()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

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
        m_globalStatus.nPort8Volts = std::stoi(sParsedResp[2]);
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getOnBootPowerState] sParsedResp[1] : %s\n", timestamp, sParsedResp[1].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getOnBootPowerState] bOnBootPort1On = %s\n", timestamp, m_globalStatus.bOnBootPort1On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getOnBootPowerState] bOnBootPort2On = %s\n", timestamp, m_globalStatus.bOnBootPort2On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getOnBootPowerState] bOnBootPort3On = %s\n", timestamp, m_globalStatus.bOnBootPort3On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getOnBootPowerState] bOnBootPort4On = %s\n", timestamp, m_globalStatus.bOnBootPort4On?"Yes":"No");
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2::getMotoMaxSpeed(int &nSpeed)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = upbCommand("SS\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    nSpeed = atoi(szResp);
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::getMotoMaxSpeed] nSpeed = %d\n", timestamp, nSpeed);
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2::setMotoMaxSpeed(int nSpeed)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    sprintf(szCmd,"SS:%d\n", nSpeed);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2::getBacklashComp(int &nSteps)
{
    int nErr = PLUGIN_OK;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
	
    nErr = getStepperStatus();
    nSteps = m_globalStatus.focuser.nBacklash;

    return nErr;
}

int CPegasusUPBv2::setBacklashComp(int nSteps)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::setBacklashComp] setting backlash comp\n", timestamp);
    fflush(Logfile);
#endif

    sprintf(szCmd,"SB:%d\n", nSteps);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(!nErr)
        m_globalStatus.focuser.nBacklash = nSteps;

    return nErr;
}


int CPegasusUPBv2::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("PV\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

int CPegasusUPBv2::getTemperature(double &dTemperature)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

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

int CPegasusUPBv2::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(m_globalStatus.nDeviceType != UPBv2) {
        nPosition = m_globalStatus.focuser.nCurPos;
        return nErr;
    }

    nErr = upbCommand("SP\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // convert response
    nPosition = atoi(szResp);

    return nErr;
}

int CPegasusUPBv2::getLedStatus(int &nStatus)
{
    int nErr = PLUGIN_OK;
    int nLedStatus = 0;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("PL\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    parseResp(szResp, sParsedResp);
    nLedStatus = atoi(sParsedResp[1].c_str());
    switch(nLedStatus) {
        case 0:
            nStatus = OFF;
            break;
        case 1:
            nStatus = ON;
            break;
    }

    return nErr;
}

int CPegasusUPBv2::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    switch (nStatus) {
        case ON:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_ON);
            break;
        case OFF:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_OFF);
            break;

        default:
            break;
    }
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2::syncMotorPosition(int nPos)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "SC:%d\n", nPos);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

int CPegasusUPBv2::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus();
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}

int CPegasusUPBv2::getPosLimit()
{
    return m_nPosLimit;
}

void CPegasusUPBv2::setPosLimit(int nLimit)
{
    m_nPosLimit = nLimit;
}

bool CPegasusUPBv2::isPosLimitEnabled()
{
    return m_bPosLimitEnabled;
}

void CPegasusUPBv2::enablePosLimit(bool bEnable)
{
    m_bPosLimitEnabled = bEnable;
}


int CPegasusUPBv2::setReverseEnable(bool bEnabled)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    char szCmd[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(bEnabled)
        sprintf(szCmd,"SR:%d\n", REVERSE);
    else
        sprintf(szCmd,"SR:%d\n", NORMAL);

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::setReverseEnable] setting reverse : %s\n", timestamp, szCmd);
    fflush(Logfile);
#endif

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

#ifdef PLUGIN_DEBUG
    if(nErr) {
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::setReverseEnable] **** ERROR **** setting reverse (\"%s\") : %d\n", timestamp, szCmd, nErr);
        fflush(Logfile);
    }
#endif

    return nErr;
}

int CPegasusUPBv2::getReverseEnable(bool &bEnabled)
{
    int nErr = PLUGIN_OK;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = getStepperStatus();
    bEnabled = m_globalStatus.focuser.bReverse;

    return nErr;
}

float CPegasusUPBv2::getVoltage()
{
    return m_globalStatus.fVoltage;
}

float CPegasusUPBv2::getCurrent()
{
    return m_globalStatus.fCurent;
}

int CPegasusUPBv2::getPower()
{
    return m_globalStatus.nPower;
}

float CPegasusUPBv2::getTemp()
{
    return m_globalStatus.fTemp;
}

int CPegasusUPBv2::getHumidity()
{
    return m_globalStatus.nHumidity;
}

float CPegasusUPBv2::getDewPoint()
{
    return m_globalStatus.fDewPoint;

}

bool CPegasusUPBv2::getPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.bPort1On;
            break;

        case 2:
            return m_globalStatus.bPort2On;
            break;

        case 3:
            return m_globalStatus.bPort3On;
            break;

        case 4:
            return m_globalStatus.bPort4On;
            break;

        default:
            return false;
            break;
    }
}

int CPegasusUPBv2::setPortOn(const int &nPortNumber, const bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    switch(nPortNumber) {
        case 1:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P1:%d\n", bEnabled?1:0);
            m_globalStatus.bPort1On = bEnabled;
            break;
        case 2:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P2:%d\n", bEnabled?1:0);
            m_globalStatus.bPort2On = bEnabled;
            break;
        case 3:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P3:%d\n", bEnabled?1:0);
            m_globalStatus.bPort3On = bEnabled;
            break;
        case 4:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "P4:%d\n", bEnabled?1:0);
            m_globalStatus.bPort4On = bEnabled;
            break;
        default:
            return ERR_CMDFAILED;
            break;
    }

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}


float CPegasusUPBv2::getPortCurrent(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.fCurrentPort1;
            break;

        case 2:
            return m_globalStatus.fCurrentPort2;
            break;

        case 3:
            return m_globalStatus.fCurrentPort3;
            break;

        case 4:
            return m_globalStatus.fCurrentPort4;
            break;

        default:
            return 0.0f;
            break;
    }
}


bool CPegasusUPBv2::getOnBootPortOn(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.bOnBootPort1On;
            break;

        case 2:
            return m_globalStatus.bOnBootPort2On;
            break;

        case 3:
            return m_globalStatus.bOnBootPort3On;
            break;

        case 4:
            return m_globalStatus.bOnBootPort4On;
            break;

        default:
            return false;
            break;
    }
}

int CPegasusUPBv2::setOnBootPortOn(const int &nPortNumber, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;
    
    switch(nPortNumber) {
        case 1:
            m_globalStatus.bOnBootPort1On = bEnable;
            break;
            
        case 2:
            m_globalStatus.bOnBootPort2On = bEnable;
            break;
            
        case 3:
            m_globalStatus.bOnBootPort3On = bEnable;
            break;
            
        case 4:
            m_globalStatus.bOnBootPort4On = bEnable;
            break;
            
        default:
            return false;
            break;
    }

    sPorts.empty();
    sPorts += m_globalStatus.bOnBootPort1On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort2On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort3On? "1" : "0";
    sPorts += m_globalStatus.bOnBootPort4On? "1" : "0";

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "PE:%s\n", sPorts.c_str());
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    
    return nErr;
}


bool CPegasusUPBv2::isOverCurrentPort(const int &nPortNumber)
{
    switch(nPortNumber) {
        case 1:
            return m_globalStatus.bOverCurrentPort1;
            break;

        case 2:
            return m_globalStatus.bOverCurrentPort2;
            break;

        case 3:
            return m_globalStatus.bOverCurrentPort3;
            break;

        case 4:
            return m_globalStatus.bOverCurrentPort4;
            break;

        default:
            return false;
            break;
    }
}

bool CPegasusUPBv2::isOverCurrentDewHeater(const int &nPortNumber)
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


int CPegasusUPBv2::setUsbPortState(const int nPortID, const bool &bEnable)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

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

bool CPegasusUPBv2::getUsbPortState(const int nPortID)
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

int CPegasusUPBv2::setDewHeaterPWM(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

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
            return ERR_CMDFAILED;
            break;
    }

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

int CPegasusUPBv2::getDewHeaterPWM(const int &nDewHeater)
{
    switch(nDewHeater) {
        case DewHeaterA:
            return m_globalStatus.nDew1PWM;
            break;
        case DewHeaterB:
            return m_globalStatus.nDew2PWM;
            break;
        case DewHeaterC:
            return m_globalStatus.nDew3PWM;
            break;
        default:
            return -1;
            break;
    }
}

float CPegasusUPBv2::getDewHeaterCurrent(const int &nDewHeater)
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


int CPegasusUPBv2::getAdjPortVolts()
{
    return m_globalStatus.nPort8Volts;
}

int CPegasusUPBv2::setAdjPortVolts(int nVolts)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "P8:%d\n", nVolts);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}


int CPegasusUPBv2::setAutoDewOn(const int nPWMPort,const bool &bOn)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    int nAutoDewVal;
    
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

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::setAutoDewOn]  nPWMPort %d : %s\n", timestamp, nPWMPort, bOn?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2::setAutoDewOn]  nAutoDewVal : %d \n", timestamp, nAutoDewVal);
    fflush(Logfile);
#endif
    return nErr;
}


bool CPegasusUPBv2::isAutoDewOn(const int nPWMPort)
{
    bool bDewOn = false;
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
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2::isAutoDewOn]  for port %d : %s\n", timestamp, nPWMPort,  bDewOn?"Yes":"No");
    fflush(Logfile);
#endif
    return bDewOn;
}



#pragma mark command and response functions

int CPegasusUPBv2::upbCommand(const char *pszszCmd, char *pszResult, unsigned long nResultMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CPegasusUPBv2::upbCommand] Sending %s\n", timestamp, pszszCmd);
	fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszszCmd, strlen(pszszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    // printf("Command %s sent. wrote %lu bytes\n", szCmd, ulBytesWrite);
    if(nErr){
        return nErr;
    }

    if(pszResult) {
        // read response
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
        if(nErr){
            return nErr;
        }
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CPegasusUPBv2::upbCommand] response \"%s\"\n", timestamp, szResp);
		fflush(Logfile);
#endif
        // printf("Got response : %s\n",resp);
        strncpy(pszResult, szResp, nResultMaxLen);
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CPegasusUPBv2::upbCommand] response copied to pszResult : \"%s\"\n", timestamp, pszResult);
		fflush(Logfile);
#endif
    }
    return nErr;
}

int CPegasusUPBv2::readResponse(char *pszRespBuffer, unsigned long nBufferLen)
{
    int nErr = PLUGIN_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    memset(pszRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = pszRespBuffer;

    do {
        nErr = m_pSerx->readFile(pszBufPtr, 1, ulBytesRead, MAX_TIMEOUT);
        if(nErr) {
            return nErr;
        }

        if (ulBytesRead !=1) {// timeout
#ifdef PLUGIN_DEBUG
			ltime = time(NULL);
			timestamp = asctime(localtime(&ltime));
			timestamp[strlen(timestamp) - 1] = 0;
			fprintf(Logfile, "[%s] [CPegasusUPBv2::readResponse] timeout\n", timestamp);
			fflush(Logfile);
#endif
            nErr = ERR_NORESPONSE;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
    } while (*pszBufPtr++ != '\n' && ulTotalBytesRead < nBufferLen );

    if(ulTotalBytesRead)
        *(pszBufPtr-1) = 0; //remove the \n

    return nErr;
}


void CPegasusUPBv2::parseResp(char *pszResp, std::vector<std::string>  &svParsedResp)
{
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CPegasusUPBv2::parseResp] parsing \"%s\"\n", timestamp, pszResp);
	fflush(Logfile);
#endif
	svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2::parseResp] sSegment : %s\n", timestamp, sSegment.c_str());
        fflush(Logfile);
#endif
    }

    svParsedResp = svSeglist;


}
