#include "x2powercontrol.h"

X2PowerControl::X2PowerControl(const char* pszDisplayName,
										const int& nInstanceIndex,
										SerXInterface						* pSerXIn,
										TheSkyXFacadeForDriversInterface	* pTheSkyXIn,
										SleeperInterface					* pSleeperIn,
										BasicIniUtilInterface				* pIniUtilIn,
										LoggerInterface						* pLoggerIn,
										MutexInterface						* pIOMutexIn,
										TickCountInterface					* pTickCountIn):m_bLinked(0)
{
    char portName[255];
	std::string sLabel;
    int i;

	m_pTheSkyXForMounts = pTheSkyXIn;
	m_pSleeper = pSleeperIn;
	m_pIniUtil = pIniUtilIn;

	m_pTickCount = pTickCountIn;

	m_nISIndex = nInstanceIndex;
    
    m_pIOMutex = pIOMutexIn;
    m_pSavedMutex = pIOMutexIn;
    m_pSavedSerX = pSerXIn;

    m_PegasusUPBv2Power.SetSerxPointer(pSerXIn);
    m_PegasusUPBv2Power.setSleeper(m_pSleeper);
    
    if (m_pIniUtil) {
		// load port names
		for(i=0; i<NB_PORTS; i++) {
            switch(i) {
                case DC1 :
                    sLabel = "12V port 1";
                    break;
                case DC2 :
                    sLabel = "12V port 2";
                    break;
                case DC3 :
                    sLabel = "12V port 3";
                    break;
                case DC4 :
                    sLabel = "12V port 4";
                    break;
                case DEW1 :
                    sLabel = "Dew Heater A";
                    break;
                case DEW2 :
                    sLabel = "Dew Heater B";
                    break;
                case DEW3 :
                    sLabel = "Dew Heater C";
                    break;
                case USB1 :
                    sLabel = "USB 1";
                    break;
                case USB2 :
                    sLabel = "USB 2";
                    break;
                case USB3 :
                    sLabel = "USB 3";
                    break;
                case USB4 :
                    sLabel = "USB 4";
                    break;
                case USB5 :
                    sLabel = "USB 5";
                    break;
                case USB6 :
                    sLabel = "USB 6";
                    break;
                default:
                    sLabel = "None";
                    break;
            }
            m_pIniUtil->readString(PARENT_KEY_POWER, m_IniPortKey[i].c_str(), sLabel.c_str(), portName, 255);
            m_sPortNames.push_back(std::string(portName));
		}
    }
}

X2PowerControl::~X2PowerControl()
{
	//Delete objects used through composition
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
    if (m_pSavedSerX)
        delete m_pSavedSerX;
    if (m_pSavedMutex)
        delete m_pSavedMutex;
}

int X2PowerControl::establishLink(void)
{
    int nErr = SB_OK;
    char szPort[DRIVER_MAX_STRING];
    
    X2MutexLocker ml(GetMutex());
    // get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);
    nErr = m_PegasusUPBv2Power.Connect(szPort);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;
    
    return nErr;
}

int X2PowerControl::terminateLink(void)
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        m_PegasusUPBv2Power.Disconnect(m_nInstanceCount);
    }
	m_bLinked = false;

    // We're not connected, so revert to our saved interfaces
    m_PegasusUPBv2Power.SetSerxPointer(m_pSavedSerX);
    m_pIOMutex = m_pSavedMutex;

    m_bLinked = false;

    return SB_OK;
}

bool X2PowerControl::isLinked() const
{
	return m_bLinked;
}


void X2PowerControl::deviceInfoNameShort(BasicStringInterface& str) const
{
    X2PowerControl* pMe = (X2PowerControl*)this;

    if(!m_bLinked) {
        str="NA";
    }
    else {
        pMe->deviceInfoModel(str);
    }
}

void X2PowerControl::deviceInfoNameLong(BasicStringInterface& str) const
{
    deviceInfoNameShort(str);
}

void X2PowerControl::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = "Pegasus Astro UPBv2 power port controll";
}

void X2PowerControl::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(!m_bLinked) {
        str="NA";
    }
    else {
        // get firmware version
        std::string sFirmware;
        m_PegasusUPBv2Power.getFirmwareVersionString(sFirmware);
        str = sFirmware.c_str();
    }

}

void X2PowerControl::deviceInfoModel(BasicStringInterface& str)
{

    if(!m_bLinked) {
        str="NA";
    }
    else {
        // get model version
        std::string sDeviceType;
        m_PegasusUPBv2Power.getDeviceTypeString(sDeviceType);
        str = sDeviceType.c_str();
    }
}

void X2PowerControl::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "Pegasus Astro UPBv2 X2 plugin by Rodolphe Pineau";
}

double X2PowerControl::driverInfoVersion(void) const
{
	return DRIVER_VERSION_POWER;
}

int X2PowerControl::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

    if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);

    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    else if (!strcmp(pszName, CircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<CircuitLabelsInterface*>(this);

    else if (!strcmp(pszName, SetCircuitLabelsInterface_Name))
        *ppVal = dynamic_cast<SetCircuitLabelsInterface*>(this);

    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);

    else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
        *ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);

	return 0;
}

#pragma mark - UI binding

int X2PowerControl::execModalSettingsDialog()
{
    int nErr = SB_OK;
    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded
    char tmpBuf[TEXT_BUFFER_SIZE];
    bool bPressedOK = false;
    int nDeviceType = UPBv2_POWER;
    int nTmpVal;
    double dTemperature;
    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("PegasusUPBv2Power.ui", deviceType(), m_nISIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    X2MutexLocker ml(GetMutex());

    // set controls values
    if(m_bLinked) {
        // get data from device
        m_PegasusUPBv2Power.getConsolidatedStatus();
        m_PegasusUPBv2Power.getDeviceType(nDeviceType);

        // USB
        dx->setChecked("checkBox_14", m_PegasusUPBv2Power.getOnBootUsbPortOn(1));
        dx->setChecked("checkBox_15", m_PegasusUPBv2Power.getOnBootUsbPortOn(2));
        dx->setChecked("checkBox_16", m_PegasusUPBv2Power.getOnBootUsbPortOn(3));
        dx->setChecked("checkBox_17", m_PegasusUPBv2Power.getOnBootUsbPortOn(4));
        dx->setChecked("checkBox_18", m_PegasusUPBv2Power.getOnBootUsbPortOn(5));
        dx->setChecked("checkBox_19", m_PegasusUPBv2Power.getOnBootUsbPortOn(6));

        // Controller value
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f V", m_PegasusUPBv2Power.getVoltage());
        dx->setPropertyString("voltage","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f A", m_PegasusUPBv2Power.getCurrent());
        dx->setPropertyString("current","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d W", m_PegasusUPBv2Power.getPower());
        dx->setPropertyString("totalPower","text", tmpBuf);

        dTemperature =  m_PegasusUPBv2Power.getTemp();
        if(dTemperature > -100)
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC",dTemperature);
        else
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "N/A");
        dx->setPropertyString("temperature","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d %%", m_PegasusUPBv2Power.getHumidity());
        dx->setPropertyString("humidity","text", tmpBuf);

        dTemperature =  m_PegasusUPBv2Power.getDewPoint();
        if(dTemperature > -100)
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC",dTemperature);
        else
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "N/A");
        dx->setPropertyString("dewPoint","text", tmpBuf);

        // ports on boot state
        dx->setChecked("checkBox_5", m_PegasusUPBv2Power.getOnBootPortOn(DC1));
        dx->setChecked("checkBox_6", m_PegasusUPBv2Power.getOnBootPortOn(DC2));
        dx->setChecked("checkBox_7", m_PegasusUPBv2Power.getOnBootPortOn(DC3));
        dx->setChecked("checkBox_8", m_PegasusUPBv2Power.getOnBootPortOn(DC4));

        // adjustable port
        m_PegasusUPBv2Power.getAdjPortVolts(nTmpVal);
        dx->setPropertyInt("AdjPort", "value", nTmpVal);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC1)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC1));
        dx->setPropertyString("port1Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC2)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC2));
        dx->setPropertyString("port2Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC3)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC3));
        dx->setPropertyString("port3Draw","text", tmpBuf);
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC4)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC4));
        dx->setPropertyString("port4Draw","text", tmpBuf);

        dx->setPropertyInt("dewHeaterA", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterA));
        dx->setPropertyInt("dewHeaterB", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterB));
        dx->setPropertyInt("dewHeaterC", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterC));

        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterA)) {
            dx->setChecked("checkBox_9", true);
            dx->setEnabled("dewHeaterA", false);
        }
        else {
            dx->setChecked("checkBox_9", false);
            dx->setEnabled("dewHeaterA", true);
        }

        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterB)) {
            dx->setChecked("checkBox_10", true);
            dx->setEnabled("dewHeaterB", false);
        }
        else {
            dx->setChecked("checkBox_10", false);
            dx->setEnabled("dewHeaterB", true);
        }

        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterC)) {
            dx->setChecked("checkBox_11", true);
            dx->setEnabled("dewHeaterC", false);
        }
        else {
            dx->setChecked("checkBox_11", false);
            dx->setEnabled("dewHeaterC", true);
        }

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterA)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterA));
        dx->setPropertyString("DewADraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterB)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterB));
        dx->setPropertyString("DewBDraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterC)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterC));
        dx->setPropertyString("DewCDraw","text", tmpBuf);

        m_PegasusUPBv2Power.getAutoDewAggressivness(nTmpVal);
        dx->setPropertyInt("dewAggressivness", "value", nTmpVal);

    }
    else {
        // disable unsued controls when not connected

        dx->setEnabled("checkBox_14", false);
        dx->setEnabled("checkBox_15", false);
        dx->setEnabled("checkBox_16", false);
        dx->setEnabled("checkBox_17", false);
        dx->setEnabled("checkBox_18", false);
        dx->setEnabled("checkBox_19", false);

        dx->setEnabled("checkBox_5", false);
        dx->setEnabled("checkBox_6", false);
        dx->setEnabled("checkBox_7", false);
        dx->setEnabled("checkBox_8", false);

        dx->setEnabled("checkBox_8", false);

        dx->setEnabled("dewHeaterA", false);
        dx->setEnabled("dewHeaterB", false);
        dx->setEnabled("dewHeaterC", false);
        dx->setEnabled("dewAggressivness", false);

        dx->setEnabled("pushButton_3", false);
        dx->setEnabled("pushButton_4", false);
        dx->setEnabled("pushButton_5", false);
        dx->setEnabled("pushButton_7", false);
        dx->setEnabled("checkBox_9", false);
        dx->setEnabled("checkBox_10", false);
        dx->setEnabled("checkBox_11", false);

        dx->setEnabled("checkBox_14", false);
        dx->setEnabled("checkBox_15", false);
        dx->setEnabled("checkBox_16", false);
        dx->setEnabled("checkBox_17", false);
        dx->setEnabled("checkBox_18", false);
        dx->setEnabled("checkBox_19", false);

        dx->setEnabled("AdjPort", false);
        dx->setPropertyInt("AdjPort", "value", 12);
        dx->setEnabled("pushButton_6", false);
    }

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retrieve values from the user interface
    if (bPressedOK) {
        nErr = SB_OK;
    }
    return nErr;
}

void X2PowerControl::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nTmpVal;
    char tmpBuf[TEXT_BUFFER_SIZE];
    double dTemperature;

    // printf("pszEvent = %s\n", pszEvent);

    if(!m_bLinked)
        return;

    if (!strcmp(pszEvent, "on_timer")) {
        m_PegasusUPBv2Power.getConsolidatedStatus();
        // Controller value
        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f V", m_PegasusUPBv2Power.getVoltage());
        uiex->setPropertyString("voltage","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f A", m_PegasusUPBv2Power.getCurrent());
        uiex->setPropertyString("current","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d W", m_PegasusUPBv2Power.getPower());
        uiex->setPropertyString("totalPower","text", tmpBuf);

        dTemperature =  m_PegasusUPBv2Power.getTemp();
        if(dTemperature > -100)
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC",dTemperature);
        else
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "N/A");
        uiex->setPropertyString("temperature","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%d %%", m_PegasusUPBv2Power.getHumidity());
        uiex->setPropertyString("humidity","text", tmpBuf);

        dTemperature =  m_PegasusUPBv2Power.getDewPoint();
        if(dTemperature > -100)
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "%3.2f ºC",dTemperature);
        else
            snprintf(tmpBuf, TEXT_BUFFER_SIZE, "N/A");
        uiex->setPropertyString("dewPoint","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC1)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC1));
        uiex->setPropertyString("port1Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC2)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC2));
        uiex->setPropertyString("port2Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC3)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC3));
        uiex->setPropertyString("port3Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentPort(DC4)?"ff0000":"00ff00", m_PegasusUPBv2Power.getPortCurrent(DC4));
        uiex->setPropertyString("port4Draw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterA)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterA));
        uiex->setPropertyString("DewADraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterB)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterB));
        uiex->setPropertyString("DewBDraw","text", tmpBuf);

        snprintf(tmpBuf, TEXT_BUFFER_SIZE, "<html><head/><body><p><span style=\" color:#%s;\">%3.2f A</span></p></body></html>", m_PegasusUPBv2Power.isOverCurrentDewHeater(DewHeaterC)?"ff0000":"00ff00", m_PegasusUPBv2Power.getDewHeaterCurrent(DewHeaterC));
        uiex->setPropertyString("DewCDraw","text", tmpBuf);

        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterA)) {
            uiex->setPropertyInt("dewHeaterA", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterA));
        }
        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterB)) {
            uiex->setPropertyInt("dewHeaterB", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterB));
        }
        if(m_PegasusUPBv2Power.isAutoDewOn(DewHeaterC)) {
            uiex->setPropertyInt("dewHeaterC", "value", m_PegasusUPBv2Power.getDewHeaterPWM(DewHeaterC));
        }
    }

    // USB port boot state
    else if (!strcmp(pszEvent, "on_checkBox_14_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(1, uiex->isChecked("checkBox_14") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_15_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(2, uiex->isChecked("checkBox_15") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_16_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(3, uiex->isChecked("checkBox_16") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_17_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(4, uiex->isChecked("checkBox_17") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_18_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(5, uiex->isChecked("checkBox_18") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_19_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootUsbPortOn(6, uiex->isChecked("checkBox_19") == 0 ? false : true);
    }
    // Port state on boot
    else if (!strcmp(pszEvent, "on_checkBox_5_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootPortOn(DC1, uiex->isChecked("checkBox_5") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_6_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootPortOn(DC2, uiex->isChecked("checkBox_6") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_7_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootPortOn(DC3, uiex->isChecked("checkBox_7") == 0 ? false : true);
    }
    else if (!strcmp(pszEvent, "on_checkBox_8_stateChanged")) {
        m_PegasusUPBv2Power.setOnBootPortOn(DC4, uiex->isChecked("checkBox_8") == 0 ? false : true);
    }
    // adjustable port
    else if (!strcmp(pszEvent, "on_pushButton_6_clicked")) {
        uiex->propertyInt("AdjPort", "value", nTmpVal);
        m_PegasusUPBv2Power.setAdjPortVolts(nTmpVal);
    }
    // Set Dew A/B PWM and Auto Dew
    else if (!strcmp(pszEvent, "on_pushButton_3_clicked")) {
        uiex->propertyInt("dewHeaterA", "value", nTmpVal);
        m_PegasusUPBv2Power.setDewHeaterPWMVal(DewHeaterA, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_pushButton_4_clicked")) {
        uiex->propertyInt("dewHeaterB", "value", nTmpVal);
        m_PegasusUPBv2Power.setDewHeaterPWMVal(DewHeaterB, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_pushButton_5_clicked")) {
        uiex->propertyInt("dewHeaterC", "value", nTmpVal);
        m_PegasusUPBv2Power.setDewHeaterPWMVal(DewHeaterC, nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_pushButton_7_clicked")) {
        uiex->propertyInt("dewAggressivness", "value", nTmpVal);
        m_PegasusUPBv2Power.setAutoDewAggressivness(nTmpVal);
    }
    else if (!strcmp(pszEvent, "on_checkBox_9_stateChanged")) {
        m_PegasusUPBv2Power.setAutoDewOn(DewHeaterA, uiex->isChecked("checkBox_9") == 0 ? false : true);
        if(uiex->isChecked("checkBox_9")) {
            uiex->setEnabled("dewHeaterA", 0);
            uiex->setEnabled("pushButton_3", 0);
        }
        else {
            uiex->setEnabled("dewHeaterA", 1);
            uiex->setEnabled("pushButton_3", 1);
        }
    }

    else if (!strcmp(pszEvent, "on_checkBox_10_stateChanged")) {
        m_PegasusUPBv2Power.setAutoDewOn(DewHeaterB, uiex->isChecked("checkBox_10") == 0 ? false : true);
        if(uiex->isChecked("checkBox_10")) {
            uiex->setEnabled("dewHeaterB", 0);
            uiex->setEnabled("pushButton_4", 0);
        }
        else {
            uiex->setEnabled("dewHeaterB", 1);
            uiex->setEnabled("pushButton_4", 1);
        }
    }

    else if (!strcmp(pszEvent, "on_checkBox_11_stateChanged")) {
        m_PegasusUPBv2Power.setAutoDewOn(DewHeaterC, uiex->isChecked("checkBox_11") == 0 ? false : true);
        if(uiex->isChecked("checkBox_11")) {
            uiex->setEnabled("dewHeaterC", 0);
            uiex->setEnabled("pushButton_5", 0);
        }
        else {
            uiex->setEnabled("dewHeaterC", 1);
            uiex->setEnabled("pushButton_5", 1);
        }
    }
}

#pragma mark - Port access

int X2PowerControl::numberOfCircuits(int& nNumber)
{
	nNumber = m_PegasusUPBv2Power.getPortCount();
	return SB_OK;
}

int X2PowerControl::circuitState(const int& nIndex, bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    if (nIndex >= 0 && nIndex<m_PegasusUPBv2Power.getPortCount()) {
        switch(nIndex) {
            case DC1 : // power port 1
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DC1);
                break;
            case DC2  : // power port 2
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DC2);
                break;
            case DC3 : // power port 3
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DC3);
                break;
            case DC4 : // power port 4
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DC4);
                break;
            case DEW1 : // Dew Heater A
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DEW1);
                break;
            case DEW2 : // Dew Heater B
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DEW2);
                break;
            case DEW3 : // Dew Heater C
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getPortOn(DEW3);
                break;
            case USB1 : // usb 1
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(1);
                break;
            case USB2 : // usb 2
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(2);
                break;
            case USB3 : // usb 3
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(3);
                break;
            case USB4 : // usb 4
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(4);
                break;
            case USB5 : // usb 5
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(5);
                break;
            case USB6 : // usb 6
                bZeroForOffOneForOn = m_PegasusUPBv2Power.getUsbPortState(6);
                break;
            default:
                bZeroForOffOneForOn = false;
                nErr = ERR_INDEX_OUT_OF_RANGE;
                break;
        }
    }
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;

    return nErr;
}

int X2PowerControl::setCircuitState(const int& nIndex, const bool& bZeroForOffOneForOn)
{
	int nErr = SB_OK;

	if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());
    if (nIndex >= 0 && nIndex < m_PegasusUPBv2Power.getPortCount()) {
        nErr = m_PegasusUPBv2Power.setPortOn(nIndex, bZeroForOffOneForOn);
        switch(nIndex) {
            case DC1 : // power port 1
                nErr = m_PegasusUPBv2Power.setPortOn(DC1, bZeroForOffOneForOn);
                break;
            case DC2 : // power port 2
                nErr = m_PegasusUPBv2Power.setPortOn(DC2, bZeroForOffOneForOn);
                break;
            case DC3 : // power port 3
                nErr = m_PegasusUPBv2Power.setPortOn(DC3, bZeroForOffOneForOn);
                break;
            case DC4: // power port 4
                nErr = m_PegasusUPBv2Power.setPortOn(DC4, bZeroForOffOneForOn);
                break;
            case DEW1 : // Dew Heater A
                nErr = m_PegasusUPBv2Power.setPortOn(DEW1, bZeroForOffOneForOn);
                break;
            case DEW2 : // Dew Heater B
                nErr = m_PegasusUPBv2Power.setPortOn(DEW2, bZeroForOffOneForOn);
                break;
            case DEW3 : // Dew Heater C
                nErr = m_PegasusUPBv2Power.setPortOn(DEW3, bZeroForOffOneForOn);
                break;
            case USB1 : // usb 1
                nErr = m_PegasusUPBv2Power.setUsbPortState(1,bZeroForOffOneForOn);
                break;
            case USB2 : // usb 2
                nErr = m_PegasusUPBv2Power.setUsbPortState(2,bZeroForOffOneForOn);
                break;
            case USB3 : // usb 3
                nErr = m_PegasusUPBv2Power.setUsbPortState(3,bZeroForOffOneForOn);
                break;
            case USB4 : // usb 4
                nErr = m_PegasusUPBv2Power.setUsbPortState(4,bZeroForOffOneForOn);
                break;
            case USB5 : // usb 5
                nErr = m_PegasusUPBv2Power.setUsbPortState(5,bZeroForOffOneForOn);
                break;
            case USB6 : // usb 6
                nErr = m_PegasusUPBv2Power.setUsbPortState(6,bZeroForOffOneForOn);
                break;
            default:
                nErr = ERR_INDEX_OUT_OF_RANGE;
                break;
        }
    }
	else
		nErr = ERR_INDEX_OUT_OF_RANGE;
	return nErr;
}

int X2PowerControl::circuitLabel(const int &nZeroBasedIndex, BasicStringInterface &str)
{
    int nErr = SB_OK;
    std::string sLabel;
    
    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        str = m_sPortNames[nZeroBasedIndex].c_str();
    }
    else {
        switch(nZeroBasedIndex) {
            case DC1 :
                sLabel = "12V port 1";
                break;
            case DC2 :
                sLabel = "12V port 2";
                break;
            case DC3 :
                sLabel = "12V port 3";
                break;
            case DC4:
                sLabel = "12V port 4";
                break;
            case DEW1 :
                sLabel = "Dew Heater A";
                break;
            case DEW2 :
                sLabel = "Dew Heater B";
                break;
            case DEW3 :
                sLabel = "Dew Heater C";
                break;
            case USB1 :
                sLabel = "USB 1";
                break;
            case USB2 :
                sLabel = "USB 2";
                break;
            case USB3 :
                sLabel = "USB 3";
                break;
            case USB4 :
                sLabel = "USB 4";
                break;
            case USB5 :
                sLabel = "USB 5";
                break;
            case USB6 :
                sLabel = "USB 6";
                break;
            default:
                sLabel = "None";
                break;
        }

        str = sLabel.c_str();
    }

    return nErr;
}

int X2PowerControl::setCircuitLabel(const int &nZeroBasedIndex, const char *str)
{
    int nErr = SB_OK;

    if(m_sPortNames.size() >= nZeroBasedIndex+1) {
        m_sPortNames[nZeroBasedIndex] = str;
        m_pIniUtil->writeString(PARENT_KEY_POWER, m_IniPortKey[nZeroBasedIndex].c_str(), str);
    }
    else {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}
//
// SerialPortParams2Interface
//
#pragma mark - SerialPortParams2Interface

void X2PowerControl::portName(BasicStringInterface& str) const
{
    char szPortName[SERIAL_BUFFER_SIZE];

    portNameOnToCharPtr(szPortName, SERIAL_BUFFER_SIZE);

    str = szPortName;

}

void X2PowerControl::setPortName(const char* szPort)
{
    if (m_pIniUtil) {
        m_pIniUtil->writeString(PARENT_KEY_POWER, CHILD_KEY_PORTNAME, szPort);
    }

}


void X2PowerControl::portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize, DEF_PORT_NAME_POWER);

    if (m_pIniUtil) {
        m_pIniUtil->readString(PARENT_KEY_POWER, CHILD_KEY_PORTNAME, pszPort, pszPort, nMaxSize);
    }

}



int X2PowerControl::deviceIdentifier(BasicStringInterface &sIdentifier)
{
    sIdentifier = "UPBv2";
    return SB_OK;
}

int X2PowerControl::isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible)
{
    for (int nIndex = 0; nIndex < nPeerArraySize; ++nIndex)
    {
        X2Focuser *pPeer = dynamic_cast<X2Focuser*>(ppPeerArray[nIndex]);
        if (pPeer == NULL)
        {
            bConnectionPossible = false;
            return ERR_POINTER;
        }
    }

    bConnectionPossible = true;
    return SB_OK;

}

int X2PowerControl::useResource(MultiConnectionDeviceInterface *pPeer)
{
    X2Focuser *pFocuserPeer = dynamic_cast<X2Focuser*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; // Peer must be a focuser pointer
    }

    // Use the resources held by the specified peer
    m_pIOMutex = pFocuserPeer->m_pSavedMutex;
    m_PegasusUPBv2Power.SetSerxPointer(pFocuserPeer->m_pSavedSerX);
    return SB_OK;

}

int X2PowerControl::swapResource(MultiConnectionDeviceInterface *pPeer)
{
    X2Focuser *pFocuserPeer = dynamic_cast<X2Focuser*>(pPeer);
    if (pFocuserPeer == NULL) {
        return ERR_POINTER; //  Peer must be a focuser pointer
    }
    // Swap this driver instance's resources for the ones held by pPeer
    MutexInterface* pTempMutex = m_pSavedMutex;
    SerXInterface*  pTempSerX = m_pSavedSerX;

    m_pSavedMutex = pFocuserPeer->m_pSavedMutex;
    m_pSavedSerX = pFocuserPeer->m_pSavedSerX;

    pFocuserPeer->m_pSavedMutex = pTempMutex;
    pFocuserPeer->m_pSavedSerX = pTempSerX;

    return SB_OK;
}

