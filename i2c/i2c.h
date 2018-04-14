//*******************************************************
//* «аголовок дл€ i2c.c                                 *
//* –абота по протоколу I2C                             *
//*******************************************************
//Controller: ATmega64
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Version 	: 1.0
//Author	: CC Dharmani, Chennai (India)
//                www.dharmanitech.com
//Date		: 17 May 2010
//Adaptation: Ќичиков ћаксим
//            ƒарьин ћихаил
//            √оловейко јлександр  
//                www.avr-mc.ru
//Date		: 19.01.2014
//*******************************************************
//
#ifndef I2C_H
#define I2C_H

#define	 START				0x08
#define  REPEAT_START		0x10
#define  MT_SLA_ACK			0x18
#define  MT_SLA_NACK		0x20
#define  MT_DATA_ACK		0x28
#define  MT_DATA_NACK		0x30
#define  MR_SLA_ACK			0x40
#define  MR_SLA_NACK		0x48
#define  MR_DATA_ACK		0x50
#define  MR_DATA_NACK		0x58
#define  ARB_LOST			0x38

#define  ERROR_CODE			0x7e

/*   
 Device Address 
  1 0 1 0 A2 A1 A0 R/W
 MSB               LSB

≈сли адресна€ лини€ на земле, то это лог.0

Take now the well-known PCF8574P. This is a general purpose eight-bit input/output chip.
The hard-coded address is: 0100.A2A1A0
The 0100 part is hard-coded in the PCF8574P.
The A2A1A0 is for us to choose. Make these bits one or zero by tying the corresponding pins to Vcc or Ground:

NOTE: Philips produces two variants of this IC: the PCF8574 and the PCF8574A. The only difference is the I2C chip address!
PCF8574 0100.A2A1A0
PCF8574A 0111.A2A1A0
*/

//#define PCF8574_W           0b11011110 // адрес преобразовател€ дл€ диспле€ дл€ записи
//#define PCF8574_R           0b11011111 // адрес преобразовател€ дл€ диспле€ дл€ чтени€
//var 2
#define PCF8574_W           0b01001110 // адрес преобразовател€ дл€ диспле€ дл€ записи
#define PCF8574_R           0b01001111 // адрес преобразовател€ дл€ диспле€ дл€ чтени€

	
#define  DS1307_W			0b11010000 // адрес часов дл€ записи
#define  DS1307_R			0b11010001 // адрес часов дл€ чтени€

#define  LC512_W			0b10100000	 // адрес EEPROM дл€ записи
#define  LC512_R			0b10100001	 // адрес EEPROM дл€ чтени€


#define  EEPROM_W			0xa0
#define  EEPROM_R			0xa1

// частота, на котором работает устройство
#define sF_SCL100 100000 //100 Khz
#define sF_SCL400 400000 //400 Khz

uint8_t i2c_last_error;

#define i2c_getlasterror() i2c_last_error
#define i2c_setlasterror(error) i2c_last_error=error
void i2c_400_init(void);
void i2c_100_init(void);
unsigned char i2c_start(void);
unsigned char i2c_repeatstart(void);
unsigned char i2c_sendaddress(unsigned char);
unsigned char i2c_senddata8(unsigned char);
unsigned char i2c_senddata16(uint16_t data16);
unsigned char i2c_receivedata_ack(void);
unsigned char i2c_receivedata_nack(void);
void i2c_set_tt(void);
void i2c_stop(void);

#endif // I2C_H
	
