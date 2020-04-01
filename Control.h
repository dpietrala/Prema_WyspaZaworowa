#ifndef	_CONTROL
#define _CONTROL
#include <stm32f4xx.h>
#include <stm32f407xx.h>
#include "NIC_Module.h"
#include "MB_RTU_Slave.h"
#include "Outputs.h"

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
//MF_ID 	= 0x11 - modbus function is read ID of devie

//MFE_IF 	= modbus function error - incorrect function
//MFE_IDR = modbus function error - incorrect data range
//MFE_IV 	= modbus function error - incorect data value
//MFE_SE 	= modbus function error - undefined slave error
//MFE_PC 	= modbus function error - positive confirmation
//MFE_SNR = modbus function error - slave not ready
//MFE_NC 	= modbus function error - negative confirmation
//MFE_PE 	= modbus function error - parity error
typedef enum {false = 0, true = 1}eBool;
typedef enum {workTypeStop = 0, workTypeRun, workTypeConf, workTypeError}eWorkType;
typedef enum {Prot_Mbrtu = 0, Prot_Mbtcp, Prot_Pfnet, Prot_Pfbus}eProtocol;
typedef enum 
{
	MF_I = 0x00, MF_RnDQ = 0x01, MF_RnDI = 0x02, MF_RnHR = 0x03,
	MF_RnIR = 0x04, MF_W1DQ = 0x05, MF_W1HR = 0x06, MF_RS = 0x07,
	MF_DT = 0x08, MF_WnDQ = 0x0f, MF_WnHR = 0x10, MF_ID = 0x11,
}eMBFun;
typedef enum {MFE_IF = 0x01, MFE_IDR = 0x02, MFE_IV = 0x03, MFE_SE = 0x04,MFE_PC = 0x05, MFE_SNR = 0x06, MFE_NC = 0x07, MFE_PE = 0x08}eMBError;
typedef enum {NF_I = 0, NF_RC, NF_RSI, NF_RSC, NF_RNS_MB, NF_RNC_MB, NF_RNS_PFB, NF_RNC_PFB, NF_RNS_PFN, NF_RNC_PFN, NF_RSSCEF, NF_RCF, NF_WR}eNicFun;
typedef enum {NCS_isIdle = 0, NCS_isSending, NCS_isWaiting, NCS_isReading}eNicComStatus;

//discovery ledy
#define LED_PORT		GPIOD
#define LED1_PIN		GPIO_ODR_ODR_12
#define LED2_PIN		GPIO_ODR_ODR_13
#define LED3_PIN		GPIO_ODR_ODR_14
#define LED4_PIN		GPIO_ODR_ODR_15

////plytka od doktoratu ledy
//#define LED_PORT		GPIOE
//#define LED1_PIN		GPIO_ODR_ODR_2
//#define LED2_PIN		GPIO_ODR_ODR_3
//#define LED3_PIN		GPIO_ODR_ODR_4
//#define LED4_PIN		GPIO_ODR_ODR_5

#define LED1_ON			LED_PORT->ODR |= LED1_PIN;
#define LED1_OFF		LED_PORT->ODR &= ~LED1_PIN;
#define LED1_TOG		LED_PORT->ODR ^= LED1_PIN;
#define LED2_ON			LED_PORT->ODR |= LED2_PIN;
#define LED2_OFF		LED_PORT->ODR &= ~LED2_PIN;
#define LED2_TOG		LED_PORT->ODR ^= LED2_PIN;
#define LED3_ON			LED_PORT->ODR |= LED3_PIN;
#define LED3_OFF		LED_PORT->ODR &= ~LED3_PIN;
#define LED3_TOG		LED_PORT->ODR ^= LED3_PIN;
#define LED4_ON			LED_PORT->ODR |= LED4_PIN;
#define LED4_OFF		LED_PORT->ODR &= ~LED4_PIN;
#define LED4_TOG		LED_PORT->ODR ^= LED4_PIN;
#define LEDALL_ON		LED_PORT->ODR |= LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;
#define LEDALL_OFF	LED_PORT->ODR &= ~LED1_PIN & ~LED2_PIN & ~LED3_PIN & ~LED4_PIN;
#define LEDALL_TOG	LED_PORT->ODR ^= LED1_PIN | LED2_PIN | LED3_PIN | LED4_PIN;

#define MBS_BUFMAX 				1000
#define NIC_BUFMAX 				1000
#define MBS_REGMAX				100
#define NIC_REGMAX				100
#define NIC_FRAMEMAX			10
#define MBS_COILMAX				16

typedef struct	//modbus rtu slave
{
	uint32_t			baud;
	uint32_t			unittime;
	uint8_t				bufread[MBS_BUFMAX];
	uint8_t 			bufwrite[MBS_BUFMAX];
	
	uint8_t				address;
	eMBError			error;
	uint32_t			tick;
	eMBFun 				fun;
	uint16_t			numregs;
	uint16_t			coils;
	uint16_t			hregs[MBS_REGMAX];
}sMBS;
typedef struct	//system information: registers 0d - 99d
{
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
	uint16_t		prodCode;											//start registers: 13d
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
	uint16_t		regs[100];
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
	//uint16_t	reserved[4];									//start registers: 116d - 119d
	uint16_t		offsetAddressOutDataImage;		//start registers: 120d
	uint16_t		numMappData;									//start registers: 121d
	uint16_t		mapData[78];									//start registers: 122d - 199d
}sNIC_SC;
typedef struct	//system and command status and errors: registers 988d - 998d
{
	uint32_t		systemStatus;									//registers: 988d - 989d, see 12.2.3 System Error, page 90
	uint32_t		systemError;									//registers: 990d - 991d, see 12.2.4 Comunication State, page 90
	uint16_t		errorLogInd;									//register: 992d, not suported!!!
	uint16_t		errorCounter;									//register: 993d
	uint32_t		comError;											//registers: 994d - 995d
	uint32_t		comStatus;										//registers: 996d - 997d
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
	uint16_t			coils;
}sNIC_COD;
typedef struct	//network status for ModbusTCP: registers 200d - 299d
{
	uint16_t		regs[100];
}sNIC_NS_MB;
typedef struct	//network confguration for ModbusTCP: registers 300d - 987d
{
	uint16_t		regs[100];
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
	eBool				flgSetEthAddress;							//register: 321d bit 5
	
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
	uint16_t		confData[122];								//registers: 309d - 430d
}sNIC_NC_PFB;
typedef struct	//network status for ProfiNet: registers 200d - 299d
{
	uint16_t		regs[100];
}sNIC_NS_PFN;
typedef struct	//network confguration for ProfiNet: registers 300d - 987d
{
	uint16_t		regs[100];
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
	eBool				flgSetEthAddress;							//register: 321d bit 5
	
	uint8_t			ipAddress[4];									//registers: 323d - 324d
	uint8_t			subnetMask[4];								//registers: 325d - 326d
	uint8_t			gateway[4];										//registers: 327d - 328d
	uint8_t			ethAddress[6];								//registers: 329d - 331d
	uint32_t		flagsReg332_333;							//registers: 332d - 333d
	eBool				flagMapFc1ToFc3;							//register: 332d bit 0
	eBool				flagSkipConfTcpipStack;				//register: 332d bit 1
}sNIC_NC_PFN;
typedef struct
{
	uint8_t				address;
	eNicFun 			nicFun;
	uint32_t			time;
	uint32_t			timeout;
	eBool					errorTimeout;
	eNicComStatus	comStatus;
	uint8_t				numFunToSend;
	void					(*tabFunToSendMb[NIC_FRAMEMAX])(void);
	void					(*tabFunToSendPfbus[NIC_FRAMEMAX])(void);
	void					(*tabFunToSendPfnet[NIC_FRAMEMAX])(void);
}sNIC_Mode;
typedef struct
{
	uint8_t				bufread[NIC_BUFMAX];
	uint8_t 			bufwrite[NIC_BUFMAX];

	sNIC_Mode			mode;
	sNIC_SI				si;
	sNIC_SC				scRead;
	sNIC_SC				scWrite;
	sNIC_SSCEF		sscef;
	sNIC_CID			cid;
	sNIC_CF				cfRead;
	sNIC_CF				cfWrite;
	sNIC_COD			cod;
	sNIC_NS_MB		nsMb;
	sNIC_NC_MB		ncMbRead;
	sNIC_NC_MB		ncMbWrite;
	sNIC_NS_PFB		nsPfbus;
	sNIC_NC_PFB		ncPfbusRead;
	sNIC_NC_PFB		ncPfbusWrite;
	sNIC_NS_PFN		nsPfnet;
	sNIC_NC_PFN		ncPfnetRead;
	sNIC_NC_PFN		ncPfnetWrite;
}sNIC;
typedef struct	//outputs
{
	uint16_t			coils;
}sOutputs;
typedef struct
{
	uint32_t			tick;
	eProtocol			protocol;
	eWorkType			workType;
}sMode;
typedef struct
{
	sMode					Mode;
	sMBS					Mbs;
	sNIC					Nic;
	sOutputs			Outs;
}sControl;

void Control_SystemStart(void);
void delay_ms(uint32_t ms);

#endif
