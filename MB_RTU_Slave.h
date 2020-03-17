#ifndef _MB_RTU_SLAVE
#define _MB_RTU_SLAVE
#include "Control.h"
#include "math.h"
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
}eModbusFun;
typedef enum 
{
	MFE_IF = 0x01, MFE_IDR = 0x02, MFE_IV = 0x03, MFE_SE = 0x04,
	MFE_PC = 0x05, MFE_SNR = 0x06, MFE_NC = 0x07, MFE_PE = 0x08
}eModbusError;

#define MBS_BUFMAX 				1000
#define MBS_COILMAX				16
#define MBS_REGMAX				100

typedef struct
{
	uint32_t			baud;
	uint32_t			unittime;
	uint8_t				bufread[MBS_BUFMAX];
	uint8_t 			bufwrite[MBS_BUFMAX];
	
	uint8_t				address;
	eModbusError	error;
	uint32_t			tick;
	eModbusFun 		fun;
	uint16_t			numregs;
	uint16_t			coils;
	uint16_t			hregs[MBS_REGMAX];
}sMBS;

void MBS_Conf(void);
void MBS_ClrStr(uint8_t*, uint32_t);
uint16_t MBS_Crc16(uint8_t*, uint32_t);

#endif
