//  Pegasus Ultimate power box v2 X2 plugin
//
//  Created by Rodolphe Pineau on 2020/5/1


#include "pegasus_upbv2Focuser.h"

CPegasusUPBv2Focuser::CPegasusUPBv2Focuser()
{
    m_globalStatus.nDeviceType = NONE_FOC;
    m_globalStatus.bReady = false;
    memset(m_globalStatus.szVersion,0,sizeof(UPBv2Status));

    m_nTargetPos = 0;
    m_nPosLimit = 0;
    m_bPosLimitEnabled = false;
    m_bAbborted = false;
    
    m_pSerx = NULL;
    m_pLogger = NULL;
    m_pSleeper = NULL;
    

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PegasusUPBv2FocuserLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2FocuserLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2FocuserLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::CPegasusUPBv2Focuser] version %3.2f build 2020_08_01_1800.\n", timestamp, DRIVER_VERSION_FOC);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::CPegasusUPBv2Focuser] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusUPBv2Focuser::~CPegasusUPBv2Focuser()
{
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusUPBv2Focuser::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    int nDevice;
    int nSpeed;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
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

    m_pSleeper->sleep(1500);
    
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect getting device type\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != UPBv2_FOC) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    
    nErr = setLedStatus(ON_FOC); // needed on REV C and up for the focuser motor to work.
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect **** ERROR **** setting LED on for stepper motor on REV C\n", timestamp);
        fflush(Logfile);
#endif
    }

    nErr = getFirmwareVersion(m_szFirmwareVersion, TEXT_BUFFER_SIZE);

    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::Connect **** ERROR **** getting device full status\n", timestamp);
        fflush(Logfile);
#endif
    }

    getMotoMaxSpeed(nSpeed);
    if(nSpeed == 65535) // WAY to fast for any stepper :)
        setMotoMaxSpeed(1000);
    return nErr;
}

void CPegasusUPBv2Focuser::Disconnect(int nInstanceCount)
{
    if(m_bIsConnected && m_pSerx && nInstanceCount==1)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark move commands
int CPegasusUPBv2Focuser::haltFocuser()
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = upbCommand("SH\n", szResp, SERIAL_BUFFER_SIZE);
	m_bAbborted = true;
	
	return nErr;
}

int CPegasusUPBv2Focuser::gotoPosition(int nPos)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    if (m_bPosLimitEnabled && nPos>m_nPosLimit)
        return ERR_LIMITSEXCEEDED;

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::gotoPosition moving to %d\n", timestamp, nPos);
    fflush(Logfile);
#endif

    sprintf(szCmd,"SM:%d\n", nPos);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    m_nTargetPos = nPos;

    return nErr;
}

int CPegasusUPBv2Focuser::moveRelativeToPosision(int nSteps)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CPegasusUPBv2Focuser::moveRelativeToPosision moving by %d steps\n", timestamp, nSteps);
    fflush(Logfile);
#endif

    m_nTargetPos = m_globalStatus.focuser.nCurPos + nSteps;

    nErr = setLedStatus(ON_FOC); // needed on REV C and up for the focuser motor to work.
    nErr |= gotoPosition(m_nTargetPos);
    
    return nErr;
}

#pragma mark command complete functions

int CPegasusUPBv2Focuser::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    bool bIsMoving;
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    bComplete = false;
    nErr = isMotorMoving(bIsMoving);
    
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::isGoToComplete] motor is moving ? : %s\n", timestamp, bIsMoving?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::isGoToComplete] current position  : %d\n", timestamp, m_globalStatus.focuser.nCurPos);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::isGoToComplete] target position   : %d\n", timestamp, m_nTargetPos);
    fflush(Logfile);
#endif

    if(bIsMoving) {
        return nErr;
    }
    nErr = getPosition(m_globalStatus.focuser.nCurPos);

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::isGoToComplete] current position  : %d\n", timestamp, m_globalStatus.focuser.nCurPos);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::isGoToComplete] target position   : %d\n", timestamp, m_nTargetPos);
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

int CPegasusUPBv2Focuser::isMotorMoving(bool &bMoving)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
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
int CPegasusUPBv2Focuser::getStatus()
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
            m_globalStatus.nDeviceType = UPBv2_FOC;
        }
        else {
            m_globalStatus.nDeviceType = NONE_FOC;
            return ERR_DEVICENOTSUPPORTED;
        }
        nErr = PLUGIN_OK_UPBV2_FOC;
    }
    else {
        nErr = COMMAND_FAILED_UPBV2_FOC;
    }
    return nErr;
}

int CPegasusUPBv2Focuser::getStepperStatus()
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = upbCommand("SA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse results.
    parseResp(szResp, m_svParsedRespForSA);
    if(m_svParsedRespForSA.size()<4) {
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getStepperStatus] parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_UPBV2_FOC;
    }
    m_globalStatus.focuser.nCurPos = std::stoi(m_svParsedRespForSA[upbFPos]);
    m_globalStatus.focuser.bMoving = std::stoi(m_svParsedRespForSA[upbFMotorState]) == 1?true:false;
    m_globalStatus.focuser.bReverse = std::stoi(m_svParsedRespForSA[upbFMotorInvert] )== 1?true:false;
    m_globalStatus.focuser.nBacklash = std::stoi(m_svParsedRespForSA[upbFBacklash]);

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getStepperStatus] nCurPos = %d\n", timestamp, m_globalStatus.focuser.nCurPos);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getStepperStatus] bMoving = %s\n", timestamp, m_globalStatus.focuser.bMoving?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getStepperStatus] bReverse = %s\n", timestamp, m_globalStatus.focuser.bReverse?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getStepperStatus] nBacklash = %d\n", timestamp, m_globalStatus.focuser.nBacklash);
    fflush(Logfile);
#endif
    

    return nErr;
}

int CPegasusUPBv2Focuser::getConsolidatedStatus()
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
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getConsolidatedStatus] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2Focuser::getConsolidatedStatus]  about to parse response\n", timestamp);
	fflush(Logfile);
#endif

    // parse response
    parseResp(szResp, m_svParsedRespForPA);
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getConsolidatedStatus] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2Focuser::getConsolidatedStatus]  response parsing done\n", timestamp);
	fflush(Logfile);
#endif

    if(m_svParsedRespForPA.size()<18) {
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getConsolidatedStatus]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_UPBV2_FOC;
    }

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusUPBv2Focuser::getConsolidatedStatus] converting value and setting them in m_globalStatus\n", timestamp);
    fprintf(Logfile, "[%s][CPegasusUPBv2Focuser::getConsolidatedStatus] m_svParsedRespForPA[upbDevice] = %s \n", timestamp, m_svParsedRespForPA[upbDevice].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Focuser::getConsolidatedStatus] m_svParsedRespForPA[upbTemp] = %s \n", timestamp, m_svParsedRespForPA[upbTemp].c_str());
    fflush(Logfile);
#endif

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getConsolidatedStatus] fTemp = %3.2f\n", timestamp, m_globalStatus.fTemp);
    fflush(Logfile);
#endif

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getConsolidatedStatus] All data collected\n", timestamp);
    fflush(Logfile);
#endif
    return nErr;
}


int CPegasusUPBv2Focuser::getMotoMaxSpeed(int &nSpeed)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    nErr = upbCommand("SS\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    nSpeed = atoi(szResp);
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getMotoMaxSpeed] nSpeed = %d\n", timestamp, nSpeed);
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2Focuser::setMotoMaxSpeed(int nSpeed)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    sprintf(szCmd,"SS:%d\n", nSpeed);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2Focuser::getBacklashComp(int &nSteps)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
	
    nErr = getStepperStatus();
    nSteps = m_globalStatus.focuser.nBacklash;

    return nErr;
}

int CPegasusUPBv2Focuser::setBacklashComp(int nSteps)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::setBacklashComp] setting backlash comp\n", timestamp);
    fflush(Logfile);
#endif

    sprintf(szCmd,"SB:%d\n", nSteps);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(!nErr)
        m_globalStatus.focuser.nBacklash = nSteps;

    return nErr;
}


int CPegasusUPBv2Focuser::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("PV\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

void CPegasusUPBv2Focuser::getFirmwareString(std::string &sFirmware)
{
    sFirmware.assign(m_szFirmwareVersion);

}

int CPegasusUPBv2Focuser::getTemperature(double &dTemperature)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
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

int CPegasusUPBv2Focuser::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("SP\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
    #if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getPosition] Error getting position : %d\n", timestamp, nErr);
        fflush(Logfile);
    #endif
    return nErr;
    }
    // convert response
    nPosition = atoi(szResp);

    return nErr;
}


int CPegasusUPBv2Focuser::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    switch (nStatus) {
        case ON_FOC:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_ON_FOC);
            break;
        case OFF_FOC:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_OFF_FOC);
            break;

        default:
            break;
    }
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2Focuser::syncMotorPosition(int nPos)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "SC:%d\n", nPos);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

int CPegasusUPBv2Focuser::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus();
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}

void CPegasusUPBv2Focuser::getDeviceTypeString(std::string &sDeviceType)
{
    if( m_globalStatus.nDeviceType== UPBv2_FOC)
        sDeviceType = "Ultimate Powerbox V2";
    else
        sDeviceType = "Unknown device";

}

int CPegasusUPBv2Focuser::getPosLimit()
{
    return m_nPosLimit;
}

void CPegasusUPBv2Focuser::setPosLimit(int nLimit)
{
    m_nPosLimit = nLimit;
}

bool CPegasusUPBv2Focuser::isPosLimitEnabled()
{
    return m_bPosLimitEnabled;
}

void CPegasusUPBv2Focuser::enablePosLimit(bool bEnable)
{
    m_bPosLimitEnabled = bEnable;
}


int CPegasusUPBv2Focuser::setReverseEnable(bool bEnabled)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
    char szCmd[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(bEnabled)
        sprintf(szCmd,"SR:%d\n", REVERSE);
    else
        sprintf(szCmd,"SR:%d\n", NORMAL);

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::setReverseEnable] setting reverse : %s\n", timestamp, szCmd);
    fflush(Logfile);
#endif

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    if(nErr) {
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::setReverseEnable] **** ERROR **** setting reverse (\"%s\") : %d\n", timestamp, szCmd, nErr);
        fflush(Logfile);
    }
#endif

    return nErr;
}

int CPegasusUPBv2Focuser::getReverseEnable(bool &bEnabled)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = getStepperStatus();
    bEnabled = m_globalStatus.focuser.bReverse;

    return nErr;
}



int CPegasusUPBv2Focuser::getAccelerationEnabled(bool &bEnabled)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
    int nRet;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = upbCommand("SJ:99\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    // parse response
    nRet = atoi(szResp);
    if(nRet==255) {
        setAccelerationEnabled(true);
        bEnabled = true;
        return nErr;
    }
    bEnabled = nRet == 0? true: false;
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::getAccelerationEnabled] bEnabled = %s\n", timestamp, bEnabled?"True":"False");
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2Focuser::setAccelerationEnabled(bool bEnabled)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    sprintf(szCmd,"SJ:%d\n", bEnabled?0:1);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}


#pragma mark command and response functions

int CPegasusUPBv2Focuser::upbCommand(const char *pszszCmd, char *pszResult, unsigned long nResultMaxLen, int nTimeout)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::upbCommand] Sending %s\n", timestamp, pszszCmd);
    fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszszCmd, strlen(pszszCmd), ulBytesWrite);
    m_pSerx->flushTx();

    // printf("Command %s sent. wrote %lu bytes\n", szCmd, ulBytesWrite);
    if(nErr){
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::upbCommand] writeFile error %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

    if(pszResult) {
        // read response
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE, nTimeout);
        if(nErr){
            return nErr;
        }
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::upbCommand] response \"%s\"\n", timestamp, szResp);
        fflush(Logfile);
#endif
        // printf("Got response : %s\n",resp);
        strncpy(pszResult, szResp, nResultMaxLen);
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::upbCommand] response copied to pszResult : \"%s\"\n", timestamp, pszResult);
        fflush(Logfile);
#endif
    }
    return nErr;
}


int CPegasusUPBv2Focuser::readResponse(char *szRespBuffer, unsigned long nBufferLen, int nTimeout)
{
    int nErr = PLUGIN_OK_UPBV2_FOC;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
    int nBytesWaiting = 0 ;
    int nbTimeouts = 0;

    memset(szRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->bytesWaitingRx(nBytesWaiting);
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] nBytesWaiting = %d\n", timestamp, nBytesWaiting);
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] nBytesWaiting nErr = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        if(!nBytesWaiting) {
            if(nbTimeouts++ >= NB_RX_WAIT) {
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] bytesWaitingRx timeout, no data for %d loops\n", timestamp, NB_RX_WAIT);
                fflush(Logfile);
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
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] readFile error.\n", timestamp);
            fflush(Logfile);
#endif
            return nErr;
        }

        if (ulBytesRead != nBytesWaiting) { // timeout
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] readFile Timeout Error\n", timestamp);
            fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] readFile nBytesWaiting = %d\n", timestamp, nBytesWaiting);
            fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::readResponse] readFile ulBytesRead = %lu\n", timestamp, ulBytesRead);
            fflush(Logfile);
#endif
        }

        ulTotalBytesRead += ulBytesRead;
        pszBufPtr+=ulBytesRead;
    } while (ulTotalBytesRead < nBufferLen  && *(pszBufPtr-1) != '\n');

    if(!ulTotalBytesRead)
        nErr = COMMAND_TIMEOUT_UPBv2_FOC; // we didn't get an answer.. so timeout
    else
        *(pszBufPtr-1) = 0; //remove the \n

    return nErr;
}


void CPegasusUPBv2Focuser::parseResp(char *pszResp, std::vector<std::string>  &svParsedResp)
{
    std::string sSegment;
    std::vector<std::string> svSeglist;
    std::stringstream ssTmp(pszResp);

#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 3
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::parseResp] parsing \"%s\"\n", timestamp, pszResp);
	fflush(Logfile);
#endif
	svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
#if defined PLUGIN_DEBUG_UPBv2_FOC && PLUGIN_DEBUG_UPBv2_FOC >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Focuser::parseResp] sSegment : %s\n", timestamp, sSegment.c_str());
        fflush(Logfile);
#endif
    }

    svParsedResp = svSeglist;


}
