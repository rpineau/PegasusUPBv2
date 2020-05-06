#include "x2focuser.h"


X2Focuser::X2Focuser(const char* pszDisplayName, 
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn)

{
	m_pSerX							= pSerXIn;		
	m_pTheSkyXForMounts				= pTheSkyXIn;
	m_pSleeper						= pSleeperIn;
	m_pIniUtil						= pIniUtilIn;
	m_pLogger						= pLoggerIn;	
	m_pIOMutex						= pIOMutexIn;
	m_pTickCount					= pTickCountIn;

	m_bLinked = false;
	m_nPosition = 0;
    m_fLastTemp = -273.15f; // aboslute zero :)
    m_bReverseEnabled = false;

    // Read in settings
    if (m_pIniUtil) {
        m_PegasusUPBv2.setPosLimit(m_pIniUtil->readInt(PARENT_KEY, POS_LIMIT, 0));
        m_PegasusUPBv2.enablePosLimit(m_pIniUtil->readInt(PARENT_KEY, POS_LIMIT_ENABLED, 0) == 0 ? false : true);
        m_bReverseEnabled = m_pIniUtil->readInt(PARENT_KEY, REVERSE_ENABLED, 0) == 0 ? false : true;
    }
	m_PegasusUPBv2.SetSerxPointer(m_pSerX);
	m_PegasusUPBv2.setLogger(m_pLogger);
    m_PegasusUPBv2.setSleeper(m_pSleeper);

}

X2Focuser::~X2Focuser()
{

	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetMutex())
		delete GetMutex();
}

#pragma mark - DriverRootInterface

int	X2Focuser::queryAbstraction(const char* pszName, void** ppVal)
{
    *ppVal = NULL;

    if (!strcmp(pszName, LinkInterface_Name))
        *ppVal = (LinkInterface*)this;

    else if (!strcmp(pszName, FocuserGotoInterface2_Name))
        *ppVal = (FocuserGotoInterface2*)this;

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    else if (!strcmp(pszName, FocuserTemperatureInterface_Name))
        *ppVal = dynamic_cast<FocuserTemperatureInterface*>(this);

    else if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();

    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    return SB_OK;
}

#pragma mark - DriverInfoInterface
void X2Focuser::driverInfoDetailedInfo(BasicStringInterface& str) const
{
        str = "Focuser X2 plugin by Rodolphe Pineau";
}

double X2Focuser::driverInfoVersion(void) const							
{
	return DRIVER_VERSION;
}

void X2Focuser::deviceInfoNameShort(BasicStringInterface& str) const
{
    int deviceType;
    X2Focuser* pMe = (X2Focuser*)this;

    X2MutexLocker ml(pMe->GetMutex());


    if(!m_bLinked) {
        str="NA";
    }
    else {
        pMe->m_PegasusUPBv2.getDeviceType(deviceType);

        if(deviceType == UPBv2)
            str = "Ultimate Power Box";
        else
            str = "Unknown device";
    }
}

void X2Focuser::deviceInfoNameLong(BasicStringInterface& str) const				
{
    deviceInfoNameShort(str);
}

void X2Focuser::deviceInfoDetailedDescription(BasicStringInterface& str) const		
{
	str = "Pegasus Focus Controller";
}

void X2Focuser::deviceInfoFirmwareVersion(BasicStringInterface& str)				
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str="NA";
    }
    else {
    // get firmware version
        char cFirmware[TEXT_BUFFER_SIZE];
        m_PegasusUPBv2.getFirmwareVersion(cFirmware, TEXT_BUFFER_SIZE);
        str = cFirmware;
    }
}

void X2Focuser::deviceInfoModel(BasicStringInterface& str)							
{
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        str="NA";
    }
    else {
        // get model version
        int deviceType;

        m_PegasusUPBv2.getDeviceType(deviceType);
        if(deviceType == UPBv2)
            str = "Ultimate Power Box";
        else
            str = "Unknown device";
    }
}

#pragma mark - LinkInterface
int	X2Focuser::establishLink(void)
{
    char szPort[DRIVER_MAX_STRING];
    int err;

    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    err = m_PegasusUPBv2.Connect(szPort);
    if(err)
        m_bLinked = false;
    else
        m_bLinked = true;

    if(m_bLinked)
        err = m_PegasusUPBv2.setReverseEnable(m_bReverseEnabled);

    return err;
}

int	X2Focuser::terminateLink(void)
{
    if(!m_bLinked)
        return SB_OK;

    X2MutexLocker ml(GetMutex());
    m_PegasusUPBv2.Disconnect();
    m_bLinked = false;
    return SB_OK;
}

bool X2Focuser::isLinked(void) const
{
	return m_bLinked;
}

#pragma mark - ModalSettingsDialogInterface
int	X2Focuser::initModalSettingsDialog(void)
{
    return SB_OK;
}

int	X2Focuser::execModalSettingsDialog(void)
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*					ui = uiutil.X2UI();
    X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
    char tmpBuf[TEXT_BUFFER_SIZE];
    bool bPressedOK = false;
    int nMaxSpeed = 0;
    int nPosition = 0;
    bool bLimitEnabled = false;
    int nPosLimit = 0;
    int nBacklashSteps = 0;
    bool bBacklashEnabled = false;
    bool bReverse = false;
    int nLedStatus;
    int nDeviceType = UPBv2;
    bool bOn = false;
    int nTmpVal;
    
    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("PegasusUPBv2.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

	// set controls values
    if(m_bLinked) {
        // get data from device
        m_PegasusUPBv2.getConsolidatedStatus();
        m_PegasusUPBv2.getDeviceType(nDeviceType);
        
        // enable all controls
        // motor max speed
        nErr = m_PegasusUPBv2.getMotoMaxSpeed(nMaxSpeed);
        //if(nErr)
        //    return nErr;
        dx->setEnabled("maxSpeed", true);
        dx->setEnabled("pushButton", true);
        dx->setPropertyInt("maxSpeed", "value", nMaxSpeed);

        // new position (set to current )
        nErr = m_PegasusUPBv2.getPosition(nPosition);
        //if(nErr)
        //   return nErr;
        dx->setEnabled("newPos", true);
        dx->setEnabled("pushButton_2", true);
        dx->setPropertyInt("newPos", "value", nPosition);

        // reverse
        dx->setEnabled("reverseDir", true);
        nErr = m_PegasusUPBv2.getReverseEnable(bReverse);
        if(bReverse)
            dx->setChecked("reverseDir", true);
        else
            dx->setChecked("reverseDir", false);

        // backlash
        dx->setEnabled("backlashSteps", true);
        nErr = m_PegasusUPBv2.getBacklashComp(nBacklashSteps);
        //if(nErr)
        //    return nErr;
        dx->setPropertyInt("backlashSteps", "value", nBacklashSteps);

        if(!nBacklashSteps)  // backlash = 0 means disabled.
            bBacklashEnabled = false;
        else
            bBacklashEnabled = true;
        if(bBacklashEnabled)
            dx->setChecked("backlashEnable", true);
        else
            dx->setChecked("backlashEnable", false);

        // USB
        dx->setChecked("checkBox_14", m_PegasusUPBv2.getUsbPortState(1));
        dx->setChecked("checkBox_15", m_PegasusUPBv2.getUsbPortState(2));
        dx->setChecked("checkBox_16", m_PegasusUPBv2.getUsbPortState(3));
        dx->setChecked("checkBox_17", m_PegasusUPBv2.getUsbPortState(4));
        dx->setChecked("checkBox_18", m_PegasusUPBv2.getUsbPortState(5));
        dx->setChecked("checkBox_19", m_PegasusUPBv2.getUsbPortState(6));
        m_PegasusUPBv2.getUsbHubState(bOn);
        dx->setChecked("checkBox_20", bOn);

        // Controller value
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f V", m_PegasusUPBv2.getVoltage());
        dx->setPropertyString("voltage","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f A", m_PegasusUPBv2.getCurrent());
        dx->setPropertyString("current","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d W", m_PegasusUPBv2.getPower());
        dx->setPropertyString("totalPower","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC", m_PegasusUPBv2.getTemp());
        dx->setPropertyString("temperature","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d %%", m_PegasusUPBv2.getHumidity());
        dx->setPropertyString("humidity","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC", m_PegasusUPBv2.getDewPoint());
        dx->setPropertyString("dewPoint","text", tmpBuf);

        // ports status
        dx->setChecked("checkBox", m_PegasusUPBv2.getPortOn(1));
        dx->setChecked("checkBox_2", m_PegasusUPBv2.getPortOn(2));
        dx->setChecked("checkBox_3", m_PegasusUPBv2.getPortOn(3));
        dx->setChecked("checkBox_4", m_PegasusUPBv2.getPortOn(4));

        dx->setChecked("checkBox_5", m_PegasusUPBv2.getOnBootPortOn(1));
        dx->setChecked("checkBox_6", m_PegasusUPBv2.getOnBootPortOn(2));
        dx->setChecked("checkBox_7", m_PegasusUPBv2.getOnBootPortOn(3));
        dx->setChecked("checkBox_8", m_PegasusUPBv2.getOnBootPortOn(4));

        // adjustable port
        nTmpVal = m_PegasusUPBv2.getAdjPortVolts();
        dx->setPropertyInt("AdjPort", "value", nTmpVal);
        
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(1)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(1));
        dx->setPropertyString("port1Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(2)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(2));
        dx->setPropertyString("port2Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(3)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(3));
        dx->setPropertyString("port3Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(4)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(4));
        dx->setPropertyString("port4Draw","text", tmpBuf);
        
        dx->setPropertyInt("dewHeaterA", "value", m_PegasusUPBv2.getDewHeaterPWM(1));
        dx->setPropertyInt("dewHeaterB", "value", m_PegasusUPBv2.getDewHeaterPWM(2));
        dx->setPropertyInt("dewHeaterC", "value", m_PegasusUPBv2.getDewHeaterPWM(2));
        
        if(m_PegasusUPBv2.isAutoDewOn(1)) {
            dx->setChecked("checkBox_9", false);
            dx->setEnabled("dewHeaterA", false);
        }
        else {
            dx->setChecked("checkBox_9", true);
            dx->setEnabled("dewHeaterA", true);
        }

        if(m_PegasusUPBv2.isAutoDewOn(2)) {
            dx->setChecked("checkBox_10", false);
            dx->setEnabled("dewHeaterB", false);
        }
        else {
            dx->setChecked("checkBox_10", true);
            dx->setEnabled("dewHeaterB", true);
        }

        if(m_PegasusUPBv2.isAutoDewOn(3)) {
            dx->setChecked("checkBox_11", false);
            dx->setEnabled("dewHeaterC", false);
        }
        else {
            dx->setChecked("checkBox_11", true);
            dx->setEnabled("dewHeaterC", true);
        }
        
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentDewHeater(1)?"ff0000":"00ff00", m_PegasusUPBv2.getDewHeaterCurrent(1));
        dx->setPropertyString("DewADraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentDewHeater(2)?"ff0000":"00ff00", m_PegasusUPBv2.getDewHeaterCurrent(2));
        dx->setPropertyString("DewBDraw","text", tmpBuf);

        // LED
        m_PegasusUPBv2.getLedStatus(nLedStatus);
        dx->setChecked("radioButton_3", nLedStatus==ON?true:false);
        dx->setChecked("radioButton_4", nLedStatus==OFF?true:false);
    }
    else {
        // disable unsued controls when not connected
        dx->setEnabled("maxSpeed", false);
        dx->setPropertyInt("maxSpeed", "value", 0);
        dx->setEnabled("pushButton", false);
        dx->setEnabled("newPos", false);
        dx->setPropertyInt("newPos", "value", 0);
        dx->setEnabled("reverseDir", false);
        dx->setEnabled("pushButton_2", false);
        dx->setEnabled("backlashSteps", false);
        dx->setPropertyInt("backlashSteps", "value", 0);
        dx->setEnabled("backlashEnable", false);

        dx->setEnabled("radioButton", false);
        dx->setEnabled("radioButton_2", false);

        dx->setEnabled("checkBox", false);
        dx->setEnabled("checkBox_2", false);
        dx->setEnabled("checkBox_3", false);
        dx->setEnabled("checkBox_4", false);
        dx->setEnabled("checkBox_5", false);
        dx->setEnabled("checkBox_6", false);
        dx->setEnabled("checkBox_7", false);
        dx->setEnabled("checkBox_8", false);

        dx->setEnabled("checkBox_8", false);

        dx->setEnabled("dewHeaterA", false);
        dx->setEnabled("dewHeaterB", false);
        dx->setEnabled("dewHeaterC", false);
        dx->setEnabled("pushButton_3", false);
        dx->setEnabled("pushButton_4", false);
        dx->setEnabled("pushButton_5", false);
        dx->setEnabled("checkBox_9", false);
        dx->setEnabled("checkBox_10", false);
        dx->setEnabled("checkBox_11", false);

        dx->setEnabled("checkBox_14", false);
        dx->setEnabled("checkBox_15", false);
        dx->setEnabled("checkBox_16", false);
        dx->setEnabled("checkBox_17", false);
        dx->setEnabled("checkBox_18", false);
        dx->setEnabled("checkBox_19", false);
        dx->setEnabled("checkBox_20", false);

        dx->setEnabled("AdjPort", false);
        dx->setPropertyInt("AdjPort", "value", 12);
        dx->setEnabled("pushButton_6", false);
    }

    // linit is done in software so it's always enabled.
    dx->setEnabled("posLimit", true);
    dx->setEnabled("limitEnable", true);
    dx->setPropertyInt("posLimit", "value", m_PegasusUPBv2.getPosLimit());
    if(m_PegasusUPBv2.isPosLimitEnabled())
        dx->setChecked("limitEnable", true);
    else
        dx->setChecked("limitEnable", false);

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retrieve values from the user interface
    if (bPressedOK) {
        nErr = SB_OK;
        // get limit option
        bLimitEnabled = dx->isChecked("limitEnable") == 0 ? false : true;
        dx->propertyInt("posLimit", "value", nPosLimit);
        if(bLimitEnabled && nPosLimit>0) { // a position limit of 0 doesn't make sense :)
            m_PegasusUPBv2.setPosLimit(nPosLimit);
            m_PegasusUPBv2.enablePosLimit(bLimitEnabled);
        } else {
            m_PegasusUPBv2.setPosLimit(nPosLimit);
            m_PegasusUPBv2.enablePosLimit(false);
        }
		if(m_bLinked) {
			// get reverse
            bReverse = dx->isChecked("reverseDir") == 0 ? false : true;
			nErr = m_PegasusUPBv2.setReverseEnable(bReverse);
			if(nErr)
				return nErr;
            // save value to config
            nErr = m_pIniUtil->writeInt(PARENT_KEY, REVERSE_ENABLED, bReverse);
            if(nErr)
                return nErr;

			// get backlash if enable, disbale if needed
            bBacklashEnabled = dx->isChecked("backlashEnable") == 0 ? false : true;
			if(bBacklashEnabled) {
				dx->propertyInt("backlashSteps", "value", nBacklashSteps);
				nErr = m_PegasusUPBv2.setBacklashComp(nBacklashSteps);
				if(nErr)
					return nErr;
			}
			else {
				nErr = m_PegasusUPBv2.setBacklashComp(0); // disable backlash comp
				if(nErr)
					return nErr;
			}
		}
        // save values to config
        nErr |= m_pIniUtil->writeInt(PARENT_KEY, POS_LIMIT, nPosLimit);
        nErr |= m_pIniUtil->writeInt(PARENT_KEY, POS_LIMIT_ENABLED, bLimitEnabled?1:0);
    }
    return nErr;
}

void X2Focuser::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = SB_OK;
    int nTmpVal;
    char szErrorMessage[TEXT_BUFFER_SIZE];
    char tmpBuf[TEXT_BUFFER_SIZE];

    // printf("pszEvent = %s\n", pszEvent);
    
    if(!m_bLinked)
        return;
    
    if (!strcmp(pszEvent, "on_timer")) {
        m_PegasusUPBv2.getConsolidatedStatus();
        // Controller value
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f V", m_PegasusUPBv2.getVoltage());
        uiex->setPropertyString("voltage","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f A", m_PegasusUPBv2.getCurrent());
        uiex->setPropertyString("current","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d W", m_PegasusUPBv2.getPower());
        uiex->setPropertyString("totalPower","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC", m_PegasusUPBv2.getTemp());
        uiex->setPropertyString("temperature","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d %%", m_PegasusUPBv2.getHumidity());
        uiex->setPropertyString("humidity","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC", m_PegasusUPBv2.getDewPoint());
        uiex->setPropertyString("dewPoint","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(1)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(1));
        uiex->setPropertyString("port1Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(2)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(2));
        uiex->setPropertyString("port2Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(3)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(3));
        uiex->setPropertyString("port3Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentPort(4)?"ff0000":"00ff00", m_PegasusUPBv2.getPortCurrent(4));
        uiex->setPropertyString("port4Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentDewHeater(1)?"ff0000":"00ff00", m_PegasusUPBv2.getDewHeaterCurrent(1));
        uiex->setPropertyString("DewADraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentDewHeater(2)?"ff0000":"00ff00", m_PegasusUPBv2.getDewHeaterCurrent(2));
        uiex->setPropertyString("DewBDraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2.isOverCurrentDewHeater(3)?"ff0000":"00ff00", m_PegasusUPBv2.getDewHeaterCurrent(3));
        uiex->setPropertyString("DewCDraw","text", tmpBuf);
    }
    // max speed
    else if (!strcmp(pszEvent, "on_pushButton_clicked")) {
        uiex->propertyInt("maxSpeed", "value", nTmpVal);
        nErr = m_PegasusUPBv2.setMotoMaxSpeed(nTmpVal);
        if(nErr) {
            snprintf(szErrorMessage, TEXT_BUFFER_SIZE, "Error setting max speed : Error %d", nErr);
            uiex->messageBox("Set Max Speed", szErrorMessage);
            return;
        }
    }
    // new position
    else if (!strcmp(pszEvent, "on_pushButton_2_clicked")) {
        uiex->propertyInt("newPos", "value", nTmpVal);
        nErr = m_PegasusUPBv2.syncMotorPosition(nTmpVal);
        if(nErr) {
            snprintf(szErrorMessage, TEXT_BUFFER_SIZE, "Error setting new position : Error %d", nErr);
            uiex->messageBox("Set New Position", szErrorMessage);
            return;
        }
    }
    // USB On/Off
    else if (!strcmp(pszEvent, "on_checkBox_14_stateChanged")) {
         uiex->isChecked("checkBox_14") == 1 ? m_PegasusUPBv2.setUsbPortState(1,true) : m_PegasusUPBv2.setUsbPortState(1,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_15_stateChanged")) {
        uiex->isChecked("checkBox_15") == 1 ? m_PegasusUPBv2.setUsbPortState(2,true) : m_PegasusUPBv2.setUsbPortState(2,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_16_stateChanged")) {
        uiex->isChecked("checkBox_16") == 1 ? m_PegasusUPBv2.setUsbPortState(3,true) : m_PegasusUPBv2.setUsbPortState(3,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_17_stateChanged")) {
        uiex->isChecked("checkBox_17") == 1 ? m_PegasusUPBv2.setUsbPortState(4,true) : m_PegasusUPBv2.setUsbPortState(4,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_18_stateChanged")) {
        uiex->isChecked("checkBox_18") == 1 ? m_PegasusUPBv2.setUsbPortState(5,true) : m_PegasusUPBv2.setUsbPortState(5,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_19_stateChanged")) {
        uiex->isChecked("checkBox_19") == 1 ? m_PegasusUPBv2.setUsbPortState(6,true) : m_PegasusUPBv2.setUsbPortState(6,false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_20_stateChanged")) {
        uiex->isChecked("checkBox_20") == 1 ? m_PegasusUPBv2.setUsbHubState(true) : m_PegasusUPBv2.setUsbHubState(false);
    }

        // port On/Off
    else if (!strcmp(pszEvent, "on_checkBox_stateChanged")) {
        uiex->isChecked("checkBox") == 1 ? m_PegasusUPBv2.setPortOn(1, true) : m_PegasusUPBv2.setPortOn(1, false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_2_stateChanged")) {
        uiex->isChecked("checkBox_2") == 1 ? m_PegasusUPBv2.setPortOn(2, true) : m_PegasusUPBv2.setPortOn(2, false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_3_stateChanged")) {
        uiex->isChecked("checkBox_3") == 1 ? m_PegasusUPBv2.setPortOn(3, true) : m_PegasusUPBv2.setPortOn(3, false);
    }
    else if (!strcmp(pszEvent, "on_checkBox_4_stateChanged")) {
        uiex->isChecked("checkBox_4") == 1 ? m_PegasusUPBv2.setPortOn(4, true) : m_PegasusUPBv2.setPortOn(4, false);
    }
    // Port state on boot
    else if (!strcmp(pszEvent, "on_checkBox_5_stateChanged")) {
        m_PegasusUPBv2.setOnBootPortOn(1, uiex->isChecked("checkBox_5") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_6_stateChanged")) {
        m_PegasusUPBv2.setOnBootPortOn(2, uiex->isChecked("checkBox_6") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_7_stateChanged")) {
        m_PegasusUPBv2.setOnBootPortOn(3, uiex->isChecked("checkBox_7") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_8_stateChanged")) {
        m_PegasusUPBv2.setOnBootPortOn(4, uiex->isChecked("checkBox_8") == 0 ? false : true);
    }
    // adjustable port
    else if (!strcmp(pszEvent, "on_pushButton_6_clicked")) {
        uiex->propertyInt("AdjPort", "value", nTmpVal);
        m_PegasusUPBv2.setAdjPortVolts(nTmpVal);
    }
    // Set Dew A/B PWM and Auto Dew
    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        uiex->propertyInt("dewHeaterA", "value", nTmpVal);
        m_PegasusUPBv2.setDewHeaterPWM(1, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_pushButton_4_clicked")) {
        uiex->propertyInt("dewHeaterB", "value", nTmpVal);
        m_PegasusUPBv2.setDewHeaterPWM(2, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_pushButton_5_clicked")) {
        uiex->propertyInt("dewHeaterC", "value", nTmpVal);
        m_PegasusUPBv2.setDewHeaterPWM(3, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_checkBox_9_stateChanged")) {
        m_PegasusUPBv2.setAutoDewOn(1, uiex->isChecked("checkBox_9") == 0 ? false : true);
        if(uiex->isChecked("checkBox_9")) {
            uiex->setEnabled("dewHeaterA", 0);
        }
        else {
            uiex->setEnabled("dewHeaterA", 1);
        }
    }

    else if (!strcmp(pszEvent, "on_checkBox_10_stateChanged")) {
        m_PegasusUPBv2.setAutoDewOn(2, uiex->isChecked("checkBox_10") == 0 ? false : true);
        if(uiex->isChecked("checkBox_10")) {
            uiex->setEnabled("dewHeaterB", 0);
        }
        else {
            uiex->setEnabled("dewHeaterB", 1);
        }
    }

    else if (!strcmp(pszEvent, "on_checkBox_11_stateChanged")) {
        m_PegasusUPBv2.setAutoDewOn(3, uiex->isChecked("checkBox_11") == 0 ? false : true);
        if(uiex->isChecked("checkBox_11")) {
            uiex->setEnabled("dewHeaterC", 0);
        }
        else {
            uiex->setEnabled("dewHeaterC", 1);
        }
    }

    // LED On/Off
    else if (!strcmp(pszEvent, "on_radioButton_3_clicked")) {
        m_PegasusUPBv2.setLedStatus(ON);
    }
    else if (!strcmp(pszEvent, "on_radioButton_4_clicked")) {
        m_PegasusUPBv2.setLedStatus(OFF);
    }

}

#pragma mark - FocuserGotoInterface2
int	X2Focuser::focPosition(int& nPosition)
{
    int err;

    if(!m_bLinked)
        return NOT_CONNECTED;

    X2MutexLocker ml(GetMutex());

    err = m_PegasusUPBv2.getPosition(nPosition);
    m_nPosition = nPosition;
    return err;
}

int	X2Focuser::focMinimumLimit(int& nMinLimit) 		
{
	nMinLimit = 0;
    return SB_OK;
}

int	X2Focuser::focMaximumLimit(int& nPosLimit)			
{
    if(m_PegasusUPBv2.isPosLimitEnabled()) {
        nPosLimit = m_PegasusUPBv2.getPosLimit();
    }
    else
        nPosLimit = 100000;

    return SB_OK;
}

int	X2Focuser::focAbort()								
{   int err;

    if(!m_bLinked)
        return NOT_CONNECTED;

    X2MutexLocker ml(GetMutex());
    err = m_PegasusUPBv2.haltFocuser();
    return err;
}

int	X2Focuser::startFocGoto(const int& nRelativeOffset)	
{
    if(!m_bLinked)
        return NOT_CONNECTED;

    X2MutexLocker ml(GetMutex());
    m_PegasusUPBv2.moveRelativeToPosision(nRelativeOffset);
    return SB_OK;
}

int	X2Focuser::isCompleteFocGoto(bool& bComplete) const
{
    int err;

    if(!m_bLinked)
        return NOT_CONNECTED;

    X2Focuser* pMe = (X2Focuser*)this;
    X2MutexLocker ml(pMe->GetMutex());

    err = pMe->m_PegasusUPBv2.isGoToComplete(bComplete);

    return err;
}

int	X2Focuser::endFocGoto(void)
{
    int err;
    if(!m_bLinked)
        return NOT_CONNECTED;

    X2MutexLocker ml(GetMutex());
    err = m_PegasusUPBv2.getPosition(m_nPosition);
    return err;
}

int X2Focuser::amountCountFocGoto(void) const					
{ 
	return 3;
}

int	X2Focuser::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)
{
	switch (nZeroBasedIndex)
	{
		default:
		case 0: strDisplayName="10 steps"; nAmount=10;break;
		case 1: strDisplayName="100 steps"; nAmount=100;break;
		case 2: strDisplayName="1000 steps"; nAmount=1000;break;
	}
	return SB_OK;
}

int	X2Focuser::amountIndexFocGoto(void)
{
	return 0;
}

#pragma mark - FocuserTemperatureInterface
int X2Focuser::focTemperature(double &dTemperature)
{
    int err = SB_OK;

    X2MutexLocker ml(GetMutex());

    if(!m_bLinked) {
        dTemperature = -100.0f;
        return NOT_CONNECTED;
    }

    // Taken from Richard's Robofocus plugin code.
    // this prevent us from asking the temperature too often
    static CStopWatch timer;

    if(timer.GetElapsedSeconds() > 30.0f || m_fLastTemp < -99.0f) {
        X2MutexLocker ml(GetMutex());
        err = m_PegasusUPBv2.getTemperature(m_fLastTemp);
        timer.Reset();
    }

    dTemperature = m_fLastTemp;

    return err;
}

#pragma mark - SerialPortParams2Interface

void X2Focuser::portName(BasicStringInterface& str) const
{
    char szPortName[DRIVER_MAX_STRING];

    portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

    str = szPortName;

}

void X2Focuser::setPortName(const char* pszPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort);

}


void X2Focuser::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize, DEF_PORT_NAME);

    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
    
}




