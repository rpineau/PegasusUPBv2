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
    

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\X2_PegasusUPBv2PowerLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2PowerLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/X2_PegasusUPBv2PowerLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::CPegasusUPBv2Power] version %3.2f build 2020_08_01_1800.\n", timestamp, DRIVER_VERSION_POWER);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::CPegasusUPBv2Power] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif

}

CPegasusUPBv2Power::~CPegasusUPBv2Power()
{
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	// Close LogFile
	if (Logfile) fclose(Logfile);
#endif
}

int CPegasusUPBv2Power::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    int nDevice;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect Called %s\n", timestamp, pszPort);
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
    
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	
    // get status so we can figure out what device we are connecting to.
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect getting device type\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceType(nDevice);
    if(nErr) {
        if(nDevice != UPBv2_POWER) {
            m_pSerx->close();
            m_bIsConnected = false;
            nErr = ERR_DEVICENOTSUPPORTED;
        }
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect **** ERROR **** getting device type\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    
    nErr = setLedStatus(ON_POWER); // needed on REV C and up for the focuser motor to work.
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect **** ERROR **** setting LED on for stepper motor on REV C\n", timestamp);
        fflush(Logfile);
#endif
    }

    nErr = getFirmwareVersion(m_szFirmwareVersion, TEXT_BUFFER_SIZE);

    nErr = getConsolidatedStatus();
    if(nErr) {
        m_pSerx->close();
        m_bIsConnected = false;
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CPegasusUPBv2Power::Connect **** ERROR **** getting device full status\n", timestamp);
        fflush(Logfile);
#endif
    }

    m_nPWMA = m_globalStatus.nDew1PWM;
    m_nPWMB = m_globalStatus.nDew2PWM;
    m_nPWMC = m_globalStatus.nDew3PWM;
    m_bPWMA_On = (m_nPWMA!=0);
    m_bPWMB_On = (m_nPWMB!=0);
    m_bPWMC_On = (m_nPWMC!=0);

    return nErr;
}

void CPegasusUPBv2Power::Disconnect(int nInstanceCount)
{
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

    // OK_UPB or OK_PPB
    nErr = upbCommand("P#\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    if(strstr(szResp,"_OK")) {
        if(strstr(szResp,"UPB2")) {
            m_globalStatus.nDeviceType = UPBv2_POWER;
        }
        else {
            m_globalStatus.nDeviceType = NONE_POWER;
            return ERR_DEVICENOTSUPPORTED;
        }
        nErr = PLUGIN_OK_UPBv2_POWER;
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

    nErr = upbCommand("PA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus]  about to parse response\n", timestamp);
	fflush(Logfile);
#endif

    // parse response
    parseResp(szResp, m_svParsedRespForPA);
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] parse error = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus]  response parsing done\n", timestamp);
	fflush(Logfile);
#endif

    if(m_svParsedRespForPA.size()<18) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus]  parsing returned an incomplete answer\n", timestamp);
        fflush(Logfile);
#endif
        return BAD_CMD_RESPONSE_UPBv2_POWER;
    }

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] converting value and setting them in m_globalStatus\n", timestamp);
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbDevice] = %s \n", timestamp, m_svParsedRespForPA[upbDevice].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbVoltage] = %s \n", timestamp, m_svParsedRespForPA[upbVoltage].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbCurrent] = %s \n", timestamp, m_svParsedRespForPA[upbCurrent].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbPower] = %s \n", timestamp, m_svParsedRespForPA[upbPower].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbTemp] = %s \n", timestamp, m_svParsedRespForPA[upbTemp].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbHumidity] = %s \n", timestamp, m_svParsedRespForPA[upbHumidity].c_str());
    fprintf(Logfile, "[%s][CPegasusUPBv2Power::getConsolidatedStatus] m_svParsedRespForPA[upbDewPoint] = %s \n", timestamp, m_svParsedRespForPA[upbDewPoint].c_str());
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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fVoltage = %3.2f\n", timestamp, m_globalStatus.fVoltage);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurent = %3.2f\n", timestamp, m_globalStatus.fCurent);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nPower = %d\n", timestamp, m_globalStatus.nPower);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fTemp = %3.2f\n", timestamp, m_globalStatus.fTemp);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nHumidity = %d\n", timestamp, m_globalStatus.nHumidity);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fDewPoint = %3.2f\n", timestamp, m_globalStatus.fDewPoint);
    fflush(Logfile);
#endif

    
    m_globalStatus.bPort1On = m_svParsedRespForPA[upbPortStatus].at(0) == '1'? true : false;
    m_globalStatus.bPort2On = m_svParsedRespForPA[upbPortStatus].at(1) == '1'? true : false;
    m_globalStatus.bPort3On = m_svParsedRespForPA[upbPortStatus].at(2) == '1'? true : false;
    m_globalStatus.bPort4On = m_svParsedRespForPA[upbPortStatus].at(3) == '1'? true : false;

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nPortStatus = %s\n", timestamp, m_svParsedRespForPA[upbPortStatus].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bPort1On = %s\n", timestamp, m_globalStatus.bPort1On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bPort2On = %s\n", timestamp, m_globalStatus.bPort2On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bPort3On = %s\n", timestamp, m_globalStatus.bPort3On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bPort4On = %s\n", timestamp, m_globalStatus.bPort4On?"Yes":"No");
    fflush(Logfile);
#endif
    if(m_svParsedRespForPA[upbUsbStatus].size() == 6) {
        m_globalStatus.bUsbPort1On = m_svParsedRespForPA[upbUsbStatus].at(0) == '1' ? true: false;
        m_globalStatus.bUsbPort2On = m_svParsedRespForPA[upbUsbStatus].at(1) == '1' ? true: false;
        m_globalStatus.bUsbPort3On = m_svParsedRespForPA[upbUsbStatus].at(2) == '1' ? true: false;
        m_globalStatus.bUsbPort4On = m_svParsedRespForPA[upbUsbStatus].at(3) == '1' ? true: false;
        m_globalStatus.bUsbPort5On = m_svParsedRespForPA[upbUsbStatus].at(4) == '1' ? true: false;
        m_globalStatus.bUsbPort6On = m_svParsedRespForPA[upbUsbStatus].at(5) == '1' ? true: false;
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort1On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(0));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort1On = %s\n", timestamp, m_globalStatus.bUsbPort1On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort2On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(1));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort2On = %s\n", timestamp, m_globalStatus.bUsbPort2On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort3On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(2));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort3On = %s\n", timestamp, m_globalStatus.bUsbPort3On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort4On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(3));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort4On = %s\n", timestamp, m_globalStatus.bUsbPort4On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort5On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(4));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort5On = %s\n", timestamp, m_globalStatus.bUsbPort5On?"Yes":"No");
        
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nUsbPort6On = %c\n", timestamp, m_svParsedRespForPA[upbUsbStatus].at(5));
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bUsbPort6On = %s\n", timestamp, m_globalStatus.bUsbPort6On?"Yes":"No");

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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nDew1PWM = %d\n", timestamp, m_globalStatus.nDew1PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nDew2PWM = %d\n", timestamp, m_globalStatus.nDew2PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nDew3PWM = %d\n", timestamp, m_globalStatus.nDew3PWM);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentPort1 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort1);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentPort2 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort2);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentPort3 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort3);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentPort4 = %3.2f\n", timestamp, m_globalStatus.fCurrentPort4);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentDew1 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew1);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentDew2 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew2);
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] fCurrentDew3 = %3.2f\n", timestamp, m_globalStatus.fCurrentDew3);
    fflush(Logfile);
#endif

    m_globalStatus.bOverCurrentPort1 = m_svParsedRespForPA[upbOvercurent].at(0) == '1'? true : false;
    m_globalStatus.bOverCurrentPort2 = m_svParsedRespForPA[upbOvercurent].at(1) == '1'? true : false;
    m_globalStatus.bOverCurrentPort3 = m_svParsedRespForPA[upbOvercurent].at(2) == '1'? true : false;
    m_globalStatus.bOverCurrentPort4 = m_svParsedRespForPA[upbOvercurent].at(3) == '1'? true : false;

    m_globalStatus.bOverCurrentDew1 = m_svParsedRespForPA[upbOvercurent].at(4) == '1'? true : false;
    m_globalStatus.bOverCurrentDew2 = m_svParsedRespForPA[upbOvercurent].at(5) == '1'? true : false;
    m_globalStatus.bOverCurrentDew3 = m_svParsedRespForPA[upbOvercurent].at(6) == '1'? true : false;

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] nOvercurrentStatus = %s\n", timestamp, m_svParsedRespForPA[upbOvercurent].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentPort1 = %s\n", timestamp, m_globalStatus.bOverCurrentPort1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentPort2 = %s\n", timestamp, m_globalStatus.bOverCurrentPort2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentPort3 = %s\n", timestamp, m_globalStatus.bOverCurrentPort3?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentPort4 = %s\n", timestamp, m_globalStatus.bOverCurrentPort4?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentDew1 = %s\n", timestamp, m_globalStatus.bOverCurrentDew1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentDew2 = %s\n", timestamp, m_globalStatus.bOverCurrentDew2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bOverCurrentDew2 = %s\n", timestamp, m_globalStatus.bOverCurrentDew3?"Yes":"No");
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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bAutoDew1 = %s\n", timestamp, m_globalStatus.bAutoDewCh1?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bAutoDew2 = %s\n", timestamp, m_globalStatus.bAutoDewCh2?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] bAutoDew3 = %s\n", timestamp, m_globalStatus.bAutoDewCh3?"Yes":"No");
    fflush(Logfile);
#endif

    // get on boot port state and adjustable volts port settings.

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] Getting on boot power port status\n", timestamp);
    fflush(Logfile);
#endif

    nErr = getOnBootPowerState();
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }
    nErr = getOnBootUsbState();
    if(nErr) {
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] ERROR = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getConsolidatedStatus] All data collected\n", timestamp);
    fflush(Logfile);
#endif
    return nErr;
}

int CPegasusUPBv2Power::getOnBootPowerState()
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
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
        m_globalStatus.nPortAdjVolts = std::stoi(sParsedResp[2]);
    }

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] sParsedResp[1] : %s\n", timestamp, sParsedResp[1].c_str());
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] bOnBootPort1On = %s\n", timestamp, m_globalStatus.bOnBootPort1On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] bOnBootPort2On = %s\n", timestamp, m_globalStatus.bOnBootPort2On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] bOnBootPort3On = %s\n", timestamp, m_globalStatus.bOnBootPort3On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootPowerState] bOnBootPort4On = %s\n", timestamp, m_globalStatus.bOnBootPort4On?"Yes":"No");
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2Power::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("PV\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    return nErr;
}

void CPegasusUPBv2Power::getFirmwareVersionString(std::string &sFirmware)
{
    sFirmware.assign(m_szFirmwareVersion);
}


int CPegasusUPBv2Power::getTemperature(double &dTemperature)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
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

int CPegasusUPBv2Power::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = upbCommand("SP\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
    #if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getPosition] Error getting position : %d\n", timestamp, nErr);
        fflush(Logfile);
    #endif
    return nErr;
    }
    // convert response
    nPosition = atoi(szResp);

    return nErr;
}


int CPegasusUPBv2Power::setLedStatus(int nStatus)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    switch (nStatus) {
        case ON_POWER:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_ON_POWER);
            break;
        case OFF_POWER:
            snprintf(szCmd, SERIAL_BUFFER_SIZE, "PL:%d\n", SWITCH_OFF_POWER);
            break;

        default:
            break;
    }
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int CPegasusUPBv2Power::getDeviceType(int &nDevice)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    nErr = getStatus();
    nDevice = m_globalStatus.nDeviceType;
    
    return nErr;
}

void CPegasusUPBv2Power::getDeviceTypeString(std::string &sDeviceType)
{
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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

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
            setDewHeaterPWM(DewHeaterA, bEnabled?m_nPWMA:0);
            m_bPWMA_On = bEnabled;
            break;
        case DEW2 : // Dew Heater B
            setDewHeaterPWM(DewHeaterB, bEnabled?m_nPWMB:0);
            m_bPWMB_On = bEnabled;
            break;
        case DEW3 : // Dew Heater C
            setDewHeaterPWM(DewHeaterC, bEnabled?m_nPWMC:0);
            m_bPWMC_On = bEnabled;
            break;
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
            return ERR_CMDFAILED;
            break;
    }

    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;
    
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

    sPorts.empty();
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
    int nErr = PLUGIN_OK_UPBv2_POWER;
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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];
    std::string sPorts;

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

    sPorts.empty();
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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort1On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort1On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort2On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort2On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort3On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort3On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort4On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort4On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort5On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort5On?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::getOnBootUsbState] bOnBootUsbPort6On = %s\n", timestamp, m_globalStatus.bOnBootUsbPort6On?"Yes":"No");
    fflush(Logfile);
#endif

    return nErr;
}

int CPegasusUPBv2Power::setDewHeaterPWM(const int &nDewHeater, const int &nPWM)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
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
    int nErr = PLUGIN_OK_UPBv2_POWER;
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


int CPegasusUPBv2Power::getAdjPortVolts(int &nVolts)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "P8:%d\n", nVolts);
    nErr = upbCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}


int CPegasusUPBv2Power::setAutoDewOn(const int nPWMPort,const bool &bOn)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::setAutoDewOn]  nPWMPort %d : %s\n", timestamp, nPWMPort, bOn?"Yes":"No");
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::setAutoDewOn]  nAutoDewVal : %d \n", timestamp, nAutoDewVal);
    fflush(Logfile);
#endif
    getConsolidatedStatus();
    return nErr;
}


bool CPegasusUPBv2Power::isAutoDewOn(const int nPWMPort)
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
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CPegasusUPBv2Power::isAutoDewOn]  for port %d : %s\n", timestamp, nPWMPort,  bDewOn?"Yes":"No");
    fflush(Logfile);
#endif
    return bDewOn;
}

int CPegasusUPBv2Power::getAutoDewAggressivness(int &nLevel)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
    std::vector<std::string> sParsedResp;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;


    m_nAutoDewAgressiveness = 210; // default value
    nErr = upbCommand("DA\n", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    parseResp(szResp, sParsedResp);
    if(sParsedResp.size()>1) {
        m_nAutoDewAgressiveness = std::stoi(sParsedResp[1]);
    }

#ifdef PLUGIN_DEBUG_UPBv2_POWER
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    //fprintf(Logfile, "[%s] [CPegasusPPBA::getAutoDewAggressivness] sParsedResp[1] : %s\n", timestamp, sParsedResp[1].c_str());
    fprintf(Logfile, "[%s] [CPegasusPPBA::getAutoDewAggressivness] m_nAutoDewAgressiveness : %d\n", timestamp, m_nAutoDewAgressiveness);
    fflush(Logfile);
#endif

    nLevel = m_nAutoDewAgressiveness;
    return nErr;
}

int CPegasusUPBv2Power::setAutoDewAggressivness(int nLevel)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

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
    int nErr = PLUGIN_OK_UPBv2_POWER;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CPegasusUPBv2Power::upbCommand] Sending %s\n", timestamp, pszszCmd);
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
        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE, nTimeout);
        if(nErr){
            return nErr;
        }
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CPegasusUPBv2Power::upbCommand] response \"%s\"\n", timestamp, szResp);
		fflush(Logfile);
#endif
        // printf("Got response : %s\n",resp);
        strncpy(pszResult, szResp, nResultMaxLen);
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 2
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] [CPegasusUPBv2Power::upbCommand] response copied to pszResult : \"%s\"\n", timestamp, pszResult);
		fflush(Logfile);
#endif
    }
    return nErr;
}


int CPegasusUPBv2Power::readResponse(char *szRespBuffer, unsigned long nBufferLen, int nTimeout)
{
    int nErr = PLUGIN_OK_UPBv2_POWER;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
    int nBytesWaiting = 0 ;
    int nbTimeouts = 0;

    memset(szRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->bytesWaitingRx(nBytesWaiting);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] nBytesWaiting = %d\n", timestamp, nBytesWaiting);
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] nBytesWaiting nErr = %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        if(!nBytesWaiting) {
            if(nbTimeouts++ >= NB_RX_WAIT) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] bytesWaitingRx timeout, no data for %d loops\n", timestamp, NB_RX_WAIT);
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
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] readFile error.\n", timestamp);
            fflush(Logfile);
#endif
            return nErr;
        }

        if (ulBytesRead != nBytesWaiting) { // timeout
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] readFile Timeout Error\n", timestamp);
            fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] readFile nBytesWaiting = %d\n", timestamp, nBytesWaiting);
            fprintf(Logfile, "[%s] [CPegasusUPBv2Power::readResponse] readFile ulBytesRead = %lu\n", timestamp, ulBytesRead);
            fflush(Logfile);
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

#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 3
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] [CPegasusUPBv2Power::parseResp] parsing \"%s\"\n", timestamp, pszResp);
	fflush(Logfile);
#endif
	svParsedResp.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, ':'))
    {
        svSeglist.push_back(sSegment);
#if defined PLUGIN_DEBUG_UPBv2_POWER && PLUGIN_DEBUG_UPBv2_POWER >= 3
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CPegasusUPBv2Power::parseResp] sSegment : %s\n", timestamp, sSegment.c_str());
        fflush(Logfile);
#endif
    }

    svParsedResp = svSeglist;


}
