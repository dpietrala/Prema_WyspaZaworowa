#ifndef _MB_RTU_MASTER
#define _MB_RTU_MASTER
#include "Control.h"
//MSI = modbus function is idle
//MSWH = modbus function is write to hold registers
//MSRH = modbus function is read from hold registers
//MSWC = modbus function is write to coils
//MSRC = modbus function is read from coils
//MFE_IF = modbus function error - incorrect function
//MFE_IDR = modbus function error - incorrect data range
//MFE_IV = modbus function error - incorect data value
//MFE_SE = modbus function error - undefined slave error
//MFE_PC = modbus function error - positive confirmation
//MFE_SNR = modbus function error - slave not ready
//MFE_NC = modbus function error - negative confirmation
//MFE_PE = modbus function error - parity error
typedef enum 
{
	MFI = 0x00, MFWH = 0x10, MFRH = 0x03, MFWC = 0x0f, MFRC = 0x01, 
	MFE_IF = 0x81, MFE_IDR = 0x82, MFE_IV = 0x83, MFE_SE = 0x84,
	MFE_PC = 0x85, MFE_SNR = 0x86, MFE_NC = 0x87, MFE_PE = 0x88
}eModbusFun;

#define BUFMAX 				1000
#define COILMAX				100
#define REGMAX				3000

typedef struct
{
	uint8_t				address;
	uint16_t			timeout;
	uint32_t			errors;
	uint32_t			tick;
	eModbusFun 		state;
	uint16_t			addr0;
	uint16_t			numregs;
	uint8_t				coils[COILMAX];
	uint16_t			holdreg[REGMAX];
}sSlave;
typedef struct
{
	uint8_t				bufread[BUFMAX];
	uint8_t 			bufwrite[BUFMAX];
	sSlave				slave;
}sModbus;

void Modbus_Conf(void);
void Modbus_ClrStr(uint8_t*, uint32_t);
uint16_t Modbus_Crc16(uint8_t*, uint32_t);
void Modbus_WriteRegs(uint16_t, uint16_t);
void Modbus_ReadRegs(uint16_t, uint16_t);

#endif
