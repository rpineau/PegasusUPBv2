// X2Focuser.h : Declaration of the X2Focuser

#ifndef __X2Focuser_H_
#define __X2Focuser_H_

#include <stdio.h>
#include <string.h>

#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/basiciniutilinterface.h"
#include "../../licensedinterfaces/mutexinterface.h"
#include "../../licensedinterfaces/basicstringinterface.h"
#include "../../licensedinterfaces/tickcountinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serialportparams2interface.h"
#include "../../licensedinterfaces/serialportparams2interface.h"
#include "../../licensedinterfaces/modalsettingsdialoginterface.h"
#include "../../licensedinterfaces/x2guiinterface.h"
#include "../../licensedinterfaces/focuserdriverinterface.h"
#include "../../licensedinterfaces/focuser/focusertemperatureinterface.h"
#include "../../licensedinterfaces/multiconnectiondeviceinterface.h"

#include "StopWatch.h"
#include "pegasus_upbv2Focuser.h"
#include "x2powercontrol.h"


// Forward declare the interfaces that this device is dependent upon
class SerXInterface;
class TheSkyXFacadeForDriversInterface;
class SleeperInterface;
class SimpleIniUtilInterface;
class LoggerInterface;
class MutexInterface;
class BasicIniUtilInterface;
class TickCountInterface;



#define PARENT_KEY_FOC			"PegasusUPBv2Focuser"
#define CHILD_KEY_PORTNAME_FOC	"PortName"
#define POS_LIMIT           "PosLimit"
#define POS_LIMIT_ENABLED   "PosLimitEnable"
#define REVERSE_ENABLED     "ReverseEnable"

#if defined(SB_WIN_BUILD)
#define DEF_PORT_NAME_FOC       "COM1"
#elif defined(SB_MAC_BUILD)
#define DEF_PORT_NAME_FOC       "/dev/cu.KeySerial1"
#elif defined(SB_LINUX_BUILD)
#define DEF_PORT_NAME_FOC       "/dev/ttyUSB0"
#endif

/*!
\brief The X2Focuser example.

\ingroup Example

Use this example to write an X2Focuser driver.
*/
class __attribute__((weak,visibility("default"))) X2Focuser : public FocuserDriverInterface, public SerialPortParams2Interface, public ModalSettingsDialogInterface, public X2GUIEventInterface, public FocuserTemperatureInterface, public MultiConnectionDeviceInterface
{
public:
	X2Focuser(const char                        *pszDisplayName,
            const int                           &nInstanceIndex,
            SerXInterface						* pSerXIn, 
            TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
            SleeperInterface					* pSleeperIn,
            BasicIniUtilInterface				* pIniUtilIn,
            LoggerInterface						* pLoggerIn,
            MutexInterface						* pIOMutexIn,
            TickCountInterface					* pTickCountIn);

	~X2Focuser();

public:

	/*!\name DriverRootInterface Implementation
	See DriverRootInterface.*/
	//@{ 
	virtual int                                 queryAbstraction(const char* pszName, void** ppVal);
	//@} 

	/*!\name DriverInfoInterface Implementation
	See DriverInfoInterface.*/
	//@{ 
	virtual void								driverInfoDetailedInfo(BasicStringInterface& str) const;
	virtual double								driverInfoVersion(void) const;
	//@} 

	/*!\name HardwareInfoInterface Implementation
	See HardwareInfoInterface.*/
	//@{ 
	virtual void                                deviceInfoNameShort(BasicStringInterface& str) const;
	virtual void                                deviceInfoNameLong(BasicStringInterface& str) const;
	virtual void                                deviceInfoDetailedDescription(BasicStringInterface& str) const;
	virtual void                                deviceInfoFirmwareVersion(BasicStringInterface& str);
	virtual void                                deviceInfoModel(BasicStringInterface& str);
	//@} 

	/*!\name LinkInterface Implementation
	See LinkInterface.*/
	//@{ 
	virtual int									establishLink(void);
	virtual int									terminateLink(void);
	virtual bool								isLinked(void) const;
	//@} 

    // ModalSettingsDialogInterface
    virtual int                                 initModalSettingsDialog(void);
    virtual int                                 execModalSettingsDialog(void);
    virtual void                                uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent);

	/*!\name FocuserGotoInterface2 Implementation
	See FocuserGotoInterface2.*/
	virtual int									focPosition(int& nPosition);
	virtual int									focMinimumLimit(int& nMinLimit);
	virtual int									focMaximumLimit(int& nMaxLimit);
	virtual int									focAbort();

	virtual int                                 startFocGoto(const int& nRelativeOffset)	;
	virtual int                                 isCompleteFocGoto(bool& bComplete) const;	virtual int                                 endFocGoto(void);

	virtual int                                 amountCountFocGoto(void) const;
	virtual int                                 amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount);
	virtual int                                 amountIndexFocGoto(void);
	//@}

    // FocuserTemperatureInterface
    virtual int                                 focTemperature(double &dTemperature);

    //SerialPortParams2Interface
    virtual void                                portName(BasicStringInterface& str) const;
    virtual void                                setPortName(const char* pszPort);
    virtual unsigned int                        baudRate() const			{return 9600;};
    virtual void                                setBaudRate(unsigned int)	{};
    virtual bool                                isBaudRateFixed() const		{return true;}

    virtual SerXInterface::Parity               parity() const				{return SerXInterface::B_NOPARITY;}
    virtual void                                setParity(const SerXInterface::Parity& parity){};
    virtual bool                                isParityFixed() const		{return true;}

    // MultiConnectionDeviceInterface
    virtual int deviceIdentifier(BasicStringInterface &sIdentifier);
    virtual int isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible);
    virtual int useResource(MultiConnectionDeviceInterface *pPeer);
    virtual int swapResource(MultiConnectionDeviceInterface *pPeer);

    SerXInterface*  m_pSavedSerX;
    MutexInterface* m_pSavedMutex;


private:

    int                                     m_nPrivateMulitInstanceIndex;

	//Standard device driver tools
	SerXInterface*							m_pSerX;		
	TheSkyXFacadeForDriversInterface* 		m_pTheSkyXForMounts;
	SleeperInterface*						m_pSleeper;
	BasicIniUtilInterface*					m_pIniUtil;
	LoggerInterface*						m_pLogger;
	mutable MutexInterface*					m_pIOMutex;
	TickCountInterface*						m_pTickCount;

	SerXInterface 							*GetSerX() {return m_pSerX; }		
	TheSkyXFacadeForDriversInterface		*GetTheSkyXFacadeForDrivers() {return m_pTheSkyXForMounts;}
	SleeperInterface						*GetSleeper() {return m_pSleeper; }
	BasicIniUtilInterface					*GetSimpleIniUtil() {return m_pIniUtil; }
	LoggerInterface							*GetLogger() {return m_pLogger; }
	MutexInterface							*GetMutex()  {return m_pIOMutex;}
	TickCountInterface						*GetTickCountInterface() {return m_pTickCount;}

    void                                    portNameOnToCharPtr(char* pszPort, const int& nMaxSize) const;

	bool                                    m_bLinked;
	int                                     m_nPosition;
    double                                  m_fLastTemp;
    CPegasusUPBv2Focuser                    m_PegasusUPBv2Foc;
    bool                                    m_bReverseEnabled;

};


#endif //__X2Focuser_H_
