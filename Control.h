#ifndef	_CONTROL
#define _CONTROL
#include <stm32f4xx.h>
#include <stm32f407xx.h>
#include <stdbool.h>
#include "NIC_Module.h"
#include "MB_RTU_Slave.h"
#include "Outputs.h"

typedef enum {Prot_Mbrtu = 0, Prot_Mbtcp = 1, Prot_Pfnet = 2, Prot_Pfbus = 3}eProtocol;

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
typedef enum 
{
	MF_I = 0x00, MF_RnDQ = 0x01, MF_RnDI = 0x02, MF_RnHR = 0x03,
	MF_RnIR = 0x04, MF_W1DQ = 0x05, MF_W1HR = 0x06, MF_RS = 0x07,
	MF_DT = 0x08, MF_WnDQ = 0x0f, MF_WnHR = 0x10, MF_ID = 0x11,
}eMBFun;
typedef enum 
{
	MFE_IF = 0x01, MFE_IDR = 0x02, MFE_IV = 0x03, MFE_SE = 0x04,
	MFE_PC = 0x05, MFE_SNR = 0x06, MFE_NC = 0x07, MFE_PE = 0x08
}eMBError;
typedef enum 
{
	NF_I = 0x00, NF_RC = 0x01, NF_RSI = 0x02, NF_RSC = 0x03,
	NF_RNS_MB = 0x04, NF_RNC_MB = 0x05
}eNicFun;

#define MBS_BUFMAX 				1000
#define NIC_BUFMAX 				1000
#define MBS_REGMAX				100
#define NIC_REGMAX				100
#define MBS_COILMAX				16

typedef struct
{
	eProtocol			protocol;
}sMode;
typedef struct
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
	uint16_t		ssioType;											//start registers: 100d
	uint16_t		ssioAddress;									//start registers: 101d
	uint32_t		ssioBaud;											//start registers: 102d
	uint16_t		ssioNumInBytes;								//start registers: 104d
	uint16_t		ssioNumOutBytes;							//start registers: 105d
	uint16_t		shifType;											//start registers: 106d
	uint16_t		shifBaud;											//start registers: 107d
	uint16_t		shifAddress;									//start registers: 108d
	uint16_t		shifConfFlags;								//start registers: 109d
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
typedef struct	//network status for ModbusTCP: registers 200d - 299d
{
	uint16_t		ns[100];
}sNIC_NS_MB;
typedef struct	//network confguration for ModbusTCP: registers 300d - 987d
{
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
	bool				ipAddressAvailabe;						//register: 321d bit 0
	bool				netMaskAvailabe;							//register: 321d bit 1
	bool				gatewayAvailabe;							//register: 321d bit 2
	bool				bootIp;												//register: 321d bit 3
	bool				dhcp;													//register: 321d bit 4
	bool				setEthAddress;								//register: 321d bit 5
	
	uint8_t			ipAddress[4];									//bytes: 51d, 52d, 49d, 50d
	uint8_t			subnetMask[4];								//bytes: 55d, 56d, 53d, 54d
	uint8_t			gateway[4];										//bytes: 59d, 60d, 57d, 58d
	
	


	uint16_t		nc[100];
	uint8_t			nc2[200];
}sNIC_NC_MB;
typedef struct	//system and command status and errors: registers 988d - 998d
{
	uint32_t		errorStatus;									//start registers: 998d, see 12.2.3 System Error, page 90
	uint32_t		comStatus;										//start registers: 990d, see 12.2.4 Comunication State, page 90
}sNIC_SCSE;
typedef struct	//system flags: register 999d
{
	uint16_t		systemFlags;									//start registers: 999d, see 12.2.5 System Flags, page 91
	bool				ready;												//bit0
	bool				error;												//bit1
	bool				communicating;								//bit2
	bool				ncfError;											//bit3
	bool				rxMbxFull;										//bit4
	bool				txMbxFull;										//bit5
	bool				busOn;												//bit6
	bool				flsCfg;												//bit7
	bool				lckCfg;												//bit8
	bool				wdgOn;												//bit9
	bool				running;											//bit10
	bool				sxWriteInd;										//bit11
	bool				remCfg;												//bit12
}sNIC_SF;
typedef struct	//cyclic input data: registers 1000d - 1998d
{
	uint16_t			coils;
}sNIC_CID;
typedef struct	//command flags: register 1999d
{
	uint16_t		commandFlags;									//start registers: 1999d, see 12.2.6 Command Flags, page 93
	bool				reset;												//bit0
	bool				bootStart;										//bit1
	bool				appReady;											//bit2
	bool				busOn;												//bit3
	bool				init;													//bit4
	bool				busOff;												//bit5
	bool				clrCfg;												//bit6
	bool				strCfg;												//bit7
	bool				lckCfg;												//bit8
	bool				unlockCfg;										//bit9
	bool				wdgOn;												//bit10
	bool				wdgOff;												//bit11
	bool				clrRemCfg;										//bit12
	bool				fbusSpecCommands[3];					//bit14 - bit16
}sNIC_CF;
typedef struct	//cyclic output data: registers 2000d - 2993d
{
	uint16_t			coils;
}sNIC_COD;
typedef struct
{
	uint32_t			baud;
	uint32_t			unittime;
	uint8_t				bufread[NIC_BUFMAX];
	uint8_t 			bufwrite[NIC_BUFMAX];
	
	uint8_t				address;
	eMBError			error;
	eNicFun 			nicFun;
	
	sNIC_SI				si;
	sNIC_SC				sc;
	sNIC_SCSE			scse;
	sNIC_SF				sf;
	sNIC_CID			cid;
	sNIC_CF				cf;
	sNIC_COD			cod;
	sNIC_NS_MB		nsMb;
	sNIC_NC_MB		ncMbRead;
	sNIC_NC_MB		ncMbWrite;
}sNIC;
typedef struct
{
	uint16_t			coils;
}sOutputs;
typedef struct
{
	sMode					Mode;
	sMBS					Mbs;
	sNIC					Nic;
	sOutputs			Outs;
	
}sControl;

void SystemStart(void);
void delay_ms(uint32_t ms);

#endif
