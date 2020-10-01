#ifndef	_CONTROL
#define _CONTROL
#include <stm32f4xx.h>
#include <STM32F410Rx.h>

//MF_I 		= 0x00 - modbus function is idle
//MF_RnDQ = 0x01 - modbus function is read 1 digital quits
//MF_RnDI = 0x02 - modbus function is read n digital inputs
//MF_RnHR = 0x03 - modbus function is read n holding registers
//MF_RnIR = 0x04 - modbus function is read n input registers
//MF_W1DQ = 0x05 - modbus function is write 1 digital quit
//MF_W1HR = 0x06 - modbus function is write 1 holding register
//MF_RS 	= 0x07 - modbus function is read status of device
//MF_DT 	= 0x08 - modbus function is diagnostic test
//MF_WnDQ = 0x0f - modbus function is write n digital quits
//MF_WnHR = 0x10 - modbus function is write n holding registers
//MF_ID 	= 0x11 - modbus function is read ID of device

//MFE_IF 	= modbus function error - incorrect function
//MFE_IDR = modbus function error - incorrect data range
//MFE_IV 	= modbus function error - incorect data value
//MFE_SE 	= modbus function error - undefined slave error
//MFE_PC 	= modbus function error - positive confirmation
//MFE_SNR = modbus function error - slave not ready
//MFE_NC 	= modbus function error - negative confirmation
//MFE_PE 	= modbus function error - parity error
typedef enum {false = 0, true = 1}eBool;
typedef enum {workTypeStop = 0, workTypeInit = 1, workTypeRun = 2, workTypeError = 3}eWorkType;
typedef enum {Prot_Mbrtu = 0, Prot_Mbtcp, Prot_Pfbus, Prot_Pfnet}eProtocol;
typedef enum 
{
	MF_I = 0x00, MF_RnDQ = 0x01, MF_RnDI = 0x02, MF_RnHR = 0x03,
	MF_RnIR = 0x04, MF_W1DQ = 0x05, MF_W1HR = 0x06, MF_RS = 0x07,
	MF_DT = 0x08, MF_WnDQ = 0x0f, MF_WnHR = 0x10, MF_ID = 0x11,
}eMBFun;
typedef enum {MFE_IF = 0x01, MFE_IDR = 0x02, MFE_IV = 0x03, MFE_SE = 0x04,MFE_PC = 0x05, MFE_SNR = 0x06, MFE_NC = 0x07, MFE_PE = 0x08}eMBError;
typedef enum
{
	NF_I = 0, 
	NF_RC, NF_RSI, 
	NF_RSC, 
	NF_RNS_MB, 
	NF_RNC_MB_300_333, 
	NF_RNS_PFB, 
	NF_RNC_PFB_300_399, 
	NF_RNC_PFB_400_430, 
	NF_RNS_PFN, 
	NF_RNC_PFN_300_399,
	NF_RNC_PFN_400_499,
	NF_RNC_PFN_500_598,
	NF_RNC_PFN_599_699,
	NF_RNC_PFN_700_799,
	NF_RNC_PFN_800_899,
	NF_RNC_PFN_900_987,
	NF_RSSCEF, 
	NF_RCF, 
	NF_WR
}eNicFun;
typedef enum {NCS_comIsIdle = 0, NCS_comIsSending = 1, NCS_comIsWaiting = 2, NCS_comIsReading = 3, NCS_comIsDone = 4}eNicComStatus;
typedef enum {NCS_confIsntDone = 0, NCS_confIsReading = 1, NCS_confIsChecking = 2, NCS_confIsWriting = 3, NCS_confIsDone = 4}eNicConfStatus;
typedef enum 
{
	RES_OK = 0,
	RES_NicComTimeout,
	RES_NicFlagTimeout,
	RES_NicSiIncompatible,
	RES_NicScIncompatible,
	RES_NicNcMbIncompatible,
	RES_NicNcPfbusIncompatible,
	RES_NicNcPfnetIncompatible,
	RES_EeReadingConfigIncorrect,
}eResult;

typedef enum
{
	EeAdd_configWasUploaded = 0,
	EeAdd_stmProt 					= 1,
	
	EeAdd_mbrtuTimeout 			= 2,
	EeAdd_mbrtuDataSwap 		= 3,
	EeAdd_mbrtuAddress 			= 4,
	EeAdd_mbrtuBaudrate 		= 5,
	EeAdd_mbrtuParity 			= 6,
	
	EeAdd_mbtcpTimeout 			= 7,
	EeAdd_mbtcpDataSwap 		= 8,
	EeAdd_mbtcpIP0 					= 9,
	EeAdd_mbtcpIP1 					= 10,
	EeAdd_mbtcpIP2					= 11,
	EeAdd_mbtcpIP3 					= 12,
	EeAdd_mbtcpMask0 				= 13,
	EeAdd_mbtcpMask1 				= 14,
	EeAdd_mbtcpMask2 				= 15,
	EeAdd_mbtcpMask3 				= 16,
	EeAdd_mbtcpGateway0 		= 17,
	EeAdd_mbtcpGateway1 		= 18,
	EeAdd_mbtcpGateway2 		= 19,
	EeAdd_mbtcpGateway3 		= 20,
	EeAdd_mbtcpSerwerCons 	= 21,
	EeAdd_mbtcpSendAckTimeoutHigh 		= 22,
	EeAdd_mbtcpSendAckTimeoutLow 			= 23,
	EeAdd_mbtcpConnectAckTimeoutHigh 	= 24,
	EeAdd_mbtcpConnectAckTimeoutLow 	= 25,
	EeAdd_mbtcpCloseAckTimeoutHigh 		= 26,
	EeAdd_mbtcpCloseAckTimeoutLow 		= 27,
	
	EeAdd_pfbusTimeout 						= 28,
	EeAdd_pfbusDataSwap 					= 29,
	EeAdd_pfbusIdentNumber 				= 30,
	EeAdd_pfbusAdress 						= 31,
	EeAdd_pfbusBaudrate 					= 32,
	EeAdd_pfbusDPV1Enable 				= 33,
	EeAdd_pfbusSyncSupported 			= 34,
	EeAdd_pfbusFreezeSupported 		= 35,
	EeAdd_pfbusFailSafeSupported 	= 36,

	EeAdd_pfnetTimeout 							= 37,
	EeAdd_pfnetDataSwap 						= 38,
	EeAdd_pfnetVendorId 						= 39,
	EeAdd_pfnetDeviceId 						= 40,
	EeAdd_pfnetLengthNameOfStation 	= 41,
	EeAdd_pfnetNameOfStation 				= 42,
	EeAdd_pfnetLengthTypeOfStation 	= 282,
	EeAdd_pfnetTypeOfStation 				= 283,
	EeAdd_pfnetDeviceType 					= 523,
	EeAdd_pfnetOrderId 							= 551,
	EeAdd_pfnetIP0 									= 571,
	EeAdd_pfnetIP1 									= 572,
	EeAdd_pfnetIP2 									= 573,
	EeAdd_pfnetIP3 									= 574,
	EeAdd_pfnetMask0 								= 575,
	EeAdd_pfnetMask1 								= 576,
	EeAdd_pfnetMask2 								= 577,
	EeAdd_pfnetMask3 								= 578,
	EeAdd_pfnetGateway0 						= 579,
	EeAdd_pfnetGateway1 						= 580,
	EeAdd_pfnetGateway2 						= 581,
	EeAdd_pfnetGateway3 						= 582,
	EeAdd_pfnetHardwareRevision 		= 583,
	EeAdd_pfnetSoftwareRevision1 		= 584,
	EeAdd_pfnetSoftwareRevision2 		= 585,
	EeAdd_pfnetSoftwareRevision3 		= 586,
	EeAdd_pfnetSoftwareRevisionPrefix = 587,
	
}eEeAdd;
typedef enum
{
	frameConfig_Null = 0,
	frameConfig_ConfReq = 1,
	frameConfig_ConfStmToPc = 2,
	frameConfig_ConfPcToStm = 3,
	frameConfig_TelemReq = 4,
	frameConfig_TelemStmToPc = 5,
	frameConfig_Echo = 6,
}eFrameConfig;

#include "NIC_Module.h"
#include "MB_RTU_Slave.h"
#include "Outputs.h"
#include "Flash.h"
#include "Eeprom.h"

#define LED_PORT		GPIOA
#define LED1_PIN		GPIO_ODR_ODR_8

#define LED1_OFF		LED_PORT->ODR |= LED1_PIN;
#define LED1_ON			LED_PORT->ODR &= ~LED1_PIN;
#define LED1_TOG		LED_PORT->ODR ^= LED1_PIN;


#define MBS_BUFMAX 							1000
#define NIC_BUFMAX 							1000
#define MBS_HREGMAX							5
#define MBS_IREGMAX							3
#define NIC_FRAMEMAX						20
#define MBS_COILMAX							16
#define EE_VARMAX								588
#define EE_CONFIGWASUPLOADED		0xACAD
#define STATUS_TABMAX						10
#define NIC_COMPOSSETBAUDMAX		8
#define NIC_COMPOSSETPARRMAX		3
#define NIC_COMADDRESSMIN				1
#define NIC_COMADDRESSMAX				247

#define SI_REGMAX								100
#define SC_REGMAX								100
#define MBTCP_REGMAX						34
#define PFBUS_REGMAX						131
#define PFNET_REGMAX						688

typedef struct	//MBS
{
	uint32_t			baud;
	uint8_t				address;
	uint8_t				parity;
	uint32_t			time;
	uint16_t			timeout;
	uint8_t				bufread[MBS_BUFMAX];
	uint8_t 			bufwrite[MBS_BUFMAX];
	uint16_t			statusMbrtu[STATUS_TABMAX];

	eMBError			error;
	eMBFun 				fun;
	uint16_t			hregs[MBS_HREGMAX];
	uint16_t			iregs[MBS_HREGMAX];
}sMBS;
typedef struct	//system information: registers 0d - 99d
{
	uint16_t		regs[SI_REGMAX];
	uint32_t		devNumber;										//start registers: 0d
	uint32_t		serNumber;										//start registers: 2d
	uint16_t		devClass;											//start registers: 4d
	uint8_t			hardRev;											//start registers: 5d
	uint8_t			hardCompIndex;								//start registers: 5d
	uint16_t		hardOpChann0;									//start registers: 6d
	uint16_t		hardOpChann1;									//start registers: 7d
	uint16_t		hardOpChann2;									//start registers: 8d
	uint16_t		hardOpChann3;									//start registers: 9d
	uint32_t		virtualDPMSize;								//start registers: 10d
	uint16_t		manufCode;										//start registers: 12d
	uint16_t		prodDate;											//start registers: 13d
	uint8_t			ethMACAddr[6];								//start registers: 14d - 16d
	//uint8_t		reserved[4];									//start registers: 17d - 18d
	uint16_t		firm;													//start registers: 19d
	uint8_t			firmVer[8];										//start registers: 20d
	uint8_t			firmDate[4];									//start registers: 24d
	uint8_t			firmName[64];									//start registers: 26d
	uint16_t		comClass;											//start registers: 58d
	uint16_t		protClass;										//start registers: 59d
	uint16_t		protConfClass;								//start registers: 60d
	//uint8_t		reserved[18];									//start registers: 61d - 69d
	uint8_t			inputConfShiftRegs[10];				//start registers: 70d - 74d
	uint8_t			outputStatusShiftRegs[10];		//start registers: 75d - 79d
	//uint8_t		reserved[40];									//start registers: 80d - 99d
}sNIC_SI;
typedef struct	//system configuration: registers 100d - 199d
{
	uint16_t		regs[SC_REGMAX];
	uint16_t		ssioType;											//start registers: 100d
	uint16_t		ssioAddress;									//start registers: 101d
	uint32_t		ssioBaud;											//start registers: 102d
	uint16_t		ssioNumInBytes;								//start registers: 104d
	uint16_t		ssioNumOutBytes;							//start registers: 105d
	uint16_t		shifType;											//start registers: 106d
	uint16_t		shifBaud;											//start registers: 107d
	uint16_t		shifAddress;									//start registers: 108d
	uint16_t		flagsShifConf;								//start registers: 109d
	uint16_t		ssioNumBytesSsioIn;						//start registers: 110d
	uint16_t		ssioNumBytesSsioOut;					//start registers: 111d
	uint16_t		ssioOffsetAddressFbIn;				//start registers: 112d
	uint16_t		ssioOffsetAddressFbOut;				//start registers: 113d
	uint16_t		ssioWatchdogTime;							//start registers: 114d
	uint16_t		ssioSwapShiftDir;							//start registers: 115d
	uint16_t		reserved[4];									//start registers: 116d - 119d
	uint16_t		offsetAddressOutDataImage;		//start registers: 120d
	uint16_t		numMappData;									//start registers: 121d
	uint16_t		mapData[78];									//start registers: 122d - 199d
}sNIC_SC;
typedef struct	//system and command status and errors: registers 988d - 998d
{
	uint32_t		systemStatus;									//registers: 988d - 989d, currently not suported
	uint32_t		systemError;									//registers: 990d - 991d, see 12.2.3 System Error, page 90
	uint16_t		errorLogInd;									//register: 992d, not suported!!!
	uint16_t		errorCounter;									//register: 993d
	uint32_t		comError;											//registers: 994d - 995d
	uint32_t		comStatus;										//registers: 996d - 997d, see 12.2.4 Comunication State, page 90
	uint16_t		recPacketSize;								//register: 998d
	uint16_t		flagsSystem;									//start registers: 999d, see 12.2.5 System Flags, page 91
	eBool				flagReady;										//bit 0
	eBool				flagError;										//bit 1
	eBool				flagCommunicating;						//bit 2
	eBool				flagNcfError;									//bit 3
	eBool				flagRxMbxFull;								//bit 4
	eBool				flagTxMbxFull;								//bit 5
	eBool				flagBusOn;										//bit 6
	eBool				flagFlsCfg;										//bit 7
	eBool				flagLckCfg;										//bit 8
	eBool				flagWdgOn;										//bit 9
	eBool				flagRunning;									//bit 10
	eBool				flagSxWriteInd;								//bit 11
	eBool				flagRemCfg;										//bit 12
}sNIC_SSCEF;
typedef struct	//cyclic input data: registers 1000d - 1998d
{
	uint16_t			coils;
}sNIC_CID;
typedef struct	//command flags: register 1999d
{
	uint16_t		flagsCommand;									//start registers: 1999d, see 12.2.6 Command Flags, page 93
	eBool				flagReset;										//bit 0
	eBool				flagBootStart;								//bit 1
	eBool				flagAppReady;									//bit 2
	eBool				flagBusOn;										//bit 3
	eBool				flagInit;											//bit 4
	eBool				flagBusOff;										//bit 5
	eBool				flagClrCfg;										//bit 6
	eBool				flagStrCfg;										//bit 7
	eBool				flagLckCfg;										//bit 8
	eBool				flagUnlockCfg;								//bit 9
	eBool				flagWdgOn;										//bit 10
	eBool				flagWdgOff;										//bit 11
	eBool				flagClrRemCfg;								//bit 12
	//eBool			reserved;											//bit 13
	eBool				flagFbusSpecCommands[2];			//bit 14 - bit 15
}sNIC_CF;
typedef struct	//cyclic output data: registers 2000d - 2993d
{
	uint16_t		statusMbtcp[STATUS_TABMAX];
	uint16_t		statusPfbus[STATUS_TABMAX];
	uint16_t		statusPfnet[STATUS_TABMAX];
}sNIC_COD;
typedef struct	//network status for ModbusTCP: registers 200d - 299d
{
	uint16_t		regs[100];
}sNIC_NS_MB;
typedef struct	//network confguration for ModbusTCP: registers 300d - 987d
{
	uint16_t		regs[MBTCP_REGMAX];
	uint16_t		length;												//register: 300d
	uint32_t		busStartup;										//registers: 301d - 302d bit0
	uint32_t		wdgTimeout;										//registers: 303d - 304d
	uint32_t		provSerwerConn;								//registers: 305d - 306d
	uint32_t		responseTimeout;							//registers: 307d - 308d * 100ms
	uint32_t		clientConWdgTimeout;					//registers: 309d - 310d * 100ms
	uint32_t		protMode;											//registers: 311d - 312d
	uint32_t		sendAckTimeout;								//registers: 313d - 314d
	uint32_t		conAckTimeout;								//registers: 315d - 316d
	uint32_t		closeAckTimeout;							//registers: 317d - 318d
	uint32_t		dataSwap;											//registers: 319d - 320d
	uint32_t		flagsReg321_322;							//registers: 321d - 322d
	eBool				flagIpAddressAvailabe;				//register: 321d bit 0
	eBool				flagNetMaskAvailabe;					//register: 321d bit 1
	eBool				flagGatewayAvailabe;					//register: 321d bit 2
	eBool				flagBootIp;										//register: 321d bit 3
	eBool				flagDhcp;											//register: 321d bit 4
	eBool				flagSetEthAddress;						//register: 321d bit 5
	
	uint8_t			ipAddress[4];									//registers: 323d - 324d
	uint8_t			subnetMask[4];								//registers: 325d - 326d
	uint8_t			gateway[4];										//registers: 327d - 328d
	uint8_t			ethAddress[6];								//registers: 329d - 331d
	uint32_t		flagsReg332_333;							//registers: 332d - 333d
	eBool				flagMapFc1ToFc3;							//register: 332d bit 0
	eBool				flagSkipConfTcpipStack;				//register: 332d bit 1
}sNIC_NC_MB;
typedef struct	//network status for ProfiBus: registers 200d - 299d
{
	uint16_t		regs[100];
}sNIC_NS_PFB;
typedef struct	//network confguration for ProfiBus: registers 300d - 987d
{
	uint16_t		regs[PFBUS_REGMAX];
	uint16_t		length;												//register: 300d

	uint32_t		flagsReg301_302;							//registers: 301d - 302d
	eBool				flagBusStartup;								//registers: 301d - 302d bit0
	eBool				flagAddressSwitchEnable;			//registers: 301d - 302d bit4
	eBool				flagBaudrateSwitchEnable;			//registers: 301d - 302d bit5
	
	uint32_t		wdgTimeout;										//registers: 303d - 304d
	uint16_t		identNumber;									//register: 305d
	uint8_t			stationAddress;								//register: 306d lowbyte
	uint8_t			baudrate;											//register: 306d highbyte
	
	uint16_t		flagsReg307;									//registers: 307d
	eBool				flagDpv1Enable;								//register: 307d bit0
	eBool				flagSyncSupperted;						//register: 307d bit1
	eBool				flagFreezeSuported;						//register: 307d bit2
	eBool				flagFailSafeSuported;					//register: 307d bit3
	eBool				flagAlarmSap50Deactivate;			//register: 307d bit4
	eBool				flagIoDataSwap;								//register: 307d bit5
	eBool				flagAutoConfiguration;				//register: 307d bit6
	eBool				flagAddressChangeNotAllowed;	//register: 307d bit7
	
	uint8_t			lengthConfData;								//register: 308d highbyte
	uint16_t		confData[PFBUS_REGMAX-9];			//registers: 309d - 430d
}sNIC_NC_PFB;
typedef struct	//network status for ProfiNet: registers 200d - 299d
{
	uint16_t		regs[100];
}sNIC_NS_PFN;
typedef struct	//network confguration for ProfiNet: registers 300d - 987d
{
	uint16_t		regs[PFNET_REGMAX];
	uint16_t		length;												//register: 300d
	uint32_t		busStartup;										//registers: 301d - 302d bit0
	uint32_t		wdgTimeout;										//registers: 303d - 304d
	uint32_t		vendorId;											//registers: 305d - 306d
	uint32_t		deviceId;											//registers: 307d - 308d
	uint32_t		maxAR;												//registers: 309d - 310d, currently not used, set to 0
	uint32_t		inputBytes;										//registers: 311d - 312d
	uint32_t		outputBytes;									//registers: 313d - 314d
	
	uint32_t		lengthNameOfStation;					//registers: 315d - 316d
	uint8_t			nameOfStation[240];						//registers: 317d - 436d
	uint32_t		lengthTypeOfStation;					//registers: 437d - 438d
	uint8_t			typeOfStation[240];						//registers: 439d - 558d
	uint8_t			deviceType[28];								//registers: 559d - 572d, used only for NIC-52RE, Character string zero-terminated,
	uint8_t			orderId[20];									//registers: 573d - 582d
	uint8_t			ipAddress[4];									//registers: 583d - 584d
	uint8_t			subnetMask[4];								//registers: 585d - 586d
	uint8_t			gateway[4];										//registers: 587d - 588d
	uint16_t		hardwareRevision;							//register: 589d
	uint16_t		softwareRevision1;						//register: 590d
	uint16_t		softwareRevision2;						//register: 591d
	uint16_t		softwareRevision3;						//register: 592d
	uint16_t		softwareRevisionPrefix;				//register: 593d, high byte
	uint16_t		maximumDiagRecords;						//register: 594d
	uint16_t		instanceId;										//register: 595d
	uint16_t		reserved1;										//register: 596d, must be set to 0
	
	uint32_t		numApi;												//registers: 597d - 598d
	uint32_t		profileApi;										//registers: 599d - 600d
	uint32_t		numSubmoduleItem;							//registers: 601d - 602d
	uint16_t		slot;													//register: 603d
	uint16_t		subslot;											//register: 604d
	uint32_t		moduleId;											//registers: 605d - 606d
	uint32_t		subModuleId;									//registers: 607d - 608d
	uint32_t		provDataLen;									//registers: 609d - 610d
	uint32_t		consDataLen;									//registers: 611d - 612d
	uint32_t		dpmOffsetIn;									//registers: 613d - 614d
	uint32_t		dpmOffsetOut;									//registers: 615d - 616d
	uint16_t		offsetIopsProvider;						//register: 617d
	uint16_t		offsetIopsConsumer;						//register: 618d
	uint16_t		reserved2[4];									//registers: 619d - 622d, must be set to 0
	uint16_t		structureSubmoduleOrApi[365];	//registers: 623d - 987d
}sNIC_NC_PFN;
typedef struct	//NIC Mode
{
	uint32_t				comBaud;
	uint8_t					comAddress;
	uint32_t				comPossibleSettings[NIC_COMPOSSETBAUDMAX];
	eNicFun 				nicFun;
	uint32_t				comTime;
	uint32_t				comTimeout;
	uint32_t				flagTime;
	uint32_t				flagTimeout;
	eBool						flagWaitingForFlag;
	eNicComStatus		comStatus; 
	eNicConfStatus	confStatus;
	uint8_t					numFunToSend;
	uint8_t					maxFunToSend;
	void						(*tabFunToSend[NIC_FRAMEMAX])(void);
}sNIC_Mode;
typedef struct	//NIC
{
	uint8_t				bufread[NIC_BUFMAX];
	uint8_t 			bufwrite[NIC_BUFMAX];
	
	sNIC_Mode			mode;
	sNIC_SI				si;
	sNIC_SI				siDefMb;
	sNIC_SI				siDefPfbus;
	sNIC_SI				siDefPfnet;
	sNIC_SC				scDef;
	sNIC_SC				scRead;
	sNIC_SC				scWrite;
	sNIC_SSCEF		sscef;
	sNIC_CID			cid;
	sNIC_CF				cfRead;
	sNIC_CF				cfWrite;
	sNIC_COD			cod;
	sNIC_NS_MB		nsMb;
	sNIC_NC_MB		ncMbDef;
	sNIC_NC_MB		ncMbRead;
	sNIC_NC_MB		ncMbWrite;
	sNIC_NS_PFB		nsPfbus;
	sNIC_NC_PFB		ncPfbusDef;
	sNIC_NC_PFB		ncPfbusRead;
	sNIC_NC_PFB		ncPfbusWrite;
	sNIC_NS_PFN		nsPfnet;
	sNIC_NC_PFN		ncPfnetDef;
	sNIC_NC_PFN		ncPfnetRead;
	sNIC_NC_PFN		ncPfnetWrite;
}sNIC;
typedef struct	//outputs
{
	uint16_t			coils;
	uint16_t			coilsReq;
}sOutputs;
typedef struct	//Mode
{
	uint32_t			tick;
	eProtocol			protocol;
	eWorkType			workType;
	uint32_t			ledPeriod;
	uint32_t			ledTime;
	eBool					dataSwapMbrtu;
	eBool					dataSwapMbtcp;
	eBool					dataSwapPfbus;
	eBool					dataSwapPfnet;
}sMode;
typedef struct	//Eeprom emulation
{
	uint16_t 			VirtAddVarTab[EE_VARMAX];
	uint16_t			rData[EE_VARMAX];
	uint16_t			wData[EE_VARMAX];
}sEe;
typedef struct	//Status
{
	uint16_t	flagsStm32;
	eBool			ConfigReadingFromFlashError;
	eBool			flagNicComTimeoutError;
	eBool			flagNicFlagTimeoutError;
	eBool			flagNicCrcError;
	uint16_t	statAdcValue[200];
	double		statTemp;
	
	uint16_t	flagsMbrtu;
	uint16_t	statBusMbrtuCounterError;
	eBool			flagBusMbrtuRunning;
	eBool			flagBusMbrtuError;
	eBool			flagBusMbrtuErrorTimeout;
	eBool			flagBusMbrtuErrorIllegalFunction;
	eBool			flagBusMbrtuErrorIllegalDataRange;
	
	uint16_t	flagsMbtcp;
	uint32_t	statBusMbtcpSystemError;
	uint16_t	statBusMbtcpCounterError;
	uint32_t	statBusMbtcpComError;
	uint32_t	statBusMbtcpComStatus;
	eBool			flagBusMbtcpCorrectModule;
	eBool			flagBusMbtcpCorrectDevNumber;
	eBool			flagBusMbtcpCorrectDevClass;
	eBool			flagBusMbtcpCorrectProtClass;
	eBool			flagBusMbtcpCorrectFirmName;
	eBool			flagBusMbtcpConfigUploadError;
	eBool			flagBusMbtcpConfigUploaded;
	eBool			flagBusMbtcpReady;
	eBool			flagBusMbtcpError;
	eBool			flagBusMbtcpCommunicating;
	eBool			flagBusMbtcpNcfError;
	eBool			flagBusMbtcpBusOn;
	eBool			flagBusMbtcpRunning;
	
	uint16_t	flagsPfbus;
	uint32_t	statBusPfbusSystemError;
	uint16_t	statBusPfbusCounterError;
	uint32_t	statBusPfbusComError;
	uint32_t	statBusPfbusComStatus;
	eBool			flagBusPfbusCorrectModule;
	eBool			flagBusPfbusCorrectDevNumber;
	eBool			flagBusPfbusCorrectDevClass;
	eBool			flagBusPfbusCorrectProtClass;
	eBool			flagBusPfbusCorrectFirmName;
	eBool			flagBusPfbusConfigUploadError;
	eBool			flagBusPfbusConfigUploaded;
	eBool			flagBusPfbusReady;
	eBool			flagBusPfbusError;
	eBool			flagBusPfbusCommunicating;
	eBool			flagBusPfbusNcfError;
	eBool			flagBusPfbusBusOn;
	eBool			flagBusPfbusRunning;
	
	uint16_t	flagsPfnet;
	uint32_t	statBusPfnetSystemError;
	uint16_t	statBusPfnetCounterError;
	uint32_t	statBusPfnetComError;
	uint32_t	statBusPfnetComStatus;
	eBool			flagBusPfnetCorrectModule;
	eBool			flagBusPfnetCorrectDevNumber;
	eBool			flagBusPfnetCorrectDevClass;
	eBool			flagBusPfnetCorrectProtClass;
	eBool			flagBusPfnetCorrectFirmName;
	eBool			flagBusPfnetConfigUploadError;
	eBool			flagBusPfnetConfigUploaded;
	eBool			flagBusPfnetReady;
	eBool			flagBusPfnetError;
	eBool			flagBusPfnetCommunicating;
	eBool			flagBusPfnetNcfError;
	eBool			flagBusPfnetBusOn;
	eBool			flagBusPfnetRunning;
}sStatus;
typedef struct
{
	sMode					Mode;
	sMBS					Mbs;
	sNIC					Nic;
	sOutputs			Outs;
	sEe						Ee;
	sStatus				Status;
}sControl;

void delay_ms(uint32_t ms);
void Control_workTypeInit(void);
eResult Control_WriteConfigToFlash(void);
uint16_t Control_DataSwap(uint16_t in, eBool flag);

#endif
