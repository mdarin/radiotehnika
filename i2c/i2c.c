//*******************************************************
//* Работа по протоколу I2C                             *
//*******************************************************
//Controller: ATmega64
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Version 	: 1.0
//Author	: CC Dharmani, Chennai (India)
//                www.dharmanitech.com
//Date		: 17 May 2010
//Adaptation: Головейко Александр  
//                www.avr-mc.ru
//Date		: 7.09.2012
//*******************************************************
//
#include <avr/io.h>
#include "i2c.h"



//*******************************************************
//* Вспомагательная процедура при настройке рабочей     *
//* частоты устройства                                  *
//*******************************************************
//
void i2c_set_tt(void)
{
// Если TWI работает в ведущем режиме, 
// то значение TWBR должно быть не менее 10. 
// Если значение TWBR меньше 10, то ведущее устройство шины 
// может генерировать некорректные сигналы на 
// линиях SDA и SCL во время передачи байта.
if(TWBR < 10) TWBR = 10;
 
// Настройка предделителя в регистре статуса Блока управления.
// Очищаются биты TWPS0 и TWPS1 регистра статуса, 
// устанавливая тем самым, значение предделителя = 1.
TWSR &= (~((1<<TWPS1)|(1<<TWPS0)));
}

//*******************************************************
//* Установить рабочую частоту устройства 400 Khz       *
//*******************************************************
//
void i2c_400_init(void)
{
    TWBR = (F_CPU/sF_SCL400 - 16)/(2 * /* TWI_Prescaler= 4^TWPS */1);
    i2c_set_tt();
}

//*******************************************************
//* Установить рабочую частоту устройства 100 Khz       *
//*******************************************************
//
void i2c_100_init(void)
{
    TWBR = (F_CPU/sF_SCL100 - 16)/(2 * /* TWI_Prescaler= 4^TWPS */1);
    i2c_set_tt();
}

//*******************************************************
//* Function to start i2c communication                 *
//*******************************************************
//
unsigned char i2c_start(void)
{
 
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); 	// Send START condition
	
    while (!(TWCR & (1<<TWINT)));   		    // Wait for TWINT flag set. This indicates that the
		  								        // START condition has been transmitted
    if ((TWSR & 0xF8) == START)			        // Check value of TWI Status Register
 	   return(0);
	else
	   return(1);
}

//*******************************************************
//* Function for repeat start condition                 *
//*******************************************************
//
unsigned char i2c_repeatstart(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); 	// Send START condition
    while (!(TWCR & (1<<TWINT)));   		    // Wait for TWINT flag set. This indicates that the
		  								        // START condition has been transmitted
    if ((TWSR & 0xF8) == REPEAT_START)			// Check value of TWI Status Register
 	   return(0);
	else
	   return(1);
}

//*******************************************************
//* Function to transmit address of the slave           * 
//*******************************************************
//
unsigned char i2c_sendaddress(unsigned char address)
{
   unsigned char STATUS;
   
   if((address & 0x01) == 0) 
     STATUS = MT_SLA_ACK;
   else
     STATUS = MR_SLA_ACK; 
   
   TWDR = address; 
   TWCR = (1<<TWINT)|(1<<TWEN);	                // Load SLA_W into TWDR Register. Clear TWINT bit
   		  			 				            // in TWCR to start transmission of address
   while (!(TWCR & (1<<TWINT)));	            // Wait for TWINT flag set. This indicates that the
   		 		   					            // SLA+W has been transmitted, and
									            // ACK/NACK has been received.
   if ((TWSR & 0xF8) == STATUS)	                // Check value of TWI Status Register
   	  return(0);
   else 
      return(1);
}

//*******************************************************
//* Function to transmit a data byte                    *
//*******************************************************
//
unsigned char i2c_senddata8(unsigned char data)
{
   TWDR = data; 
   TWCR = (1<<TWINT) |(1<<TWEN);	             // Load SLA_W into TWDR Register. Clear TWINT bit
   		  			 				             // in TWCR to start transmission of data
   while (!(TWCR & (1<<TWINT)));	             // Wait for TWINT flag set. This indicates that the
   		 		   					             // data has been transmitted, and
									             // ACK/NACK has been received.
   if ((TWSR & 0xF8) != MT_DATA_ACK)	         // Check value of TWI Status Register
   	  return(1);
   else
      return(0);
}

//*******************************************************
//* Function to transmit a data word                    *
//*******************************************************
//
unsigned char i2c_senddata16(uint16_t data16)
{
    if (i2c_senddata8(data16>>8)==1) return 1;
    if (i2c_senddata8(data16)==1) return 1;
 	return 0;
}

//*******************************************************
//* Function to receive a data byte and send            *
//* ACKnowledge                                         *
//*******************************************************
//
unsigned char i2c_receivedata_ack(void)
{
  unsigned char data;
  i2c_setlasterror(0); 
  TWCR = (1<<TWEA)|(1<<TWINT)|(1<<TWEN);
  while (!(TWCR & (1<<TWINT)));	   	              // Wait for TWINT flag set. This indicates that the
   		 		   					              // data has been received
  if ((TWSR & 0xF8) != MR_DATA_ACK)               // Check value of TWI Status Register
   	  i2c_setlasterror(ERROR_CODE);
  
  data = TWDR;
  return(data);
}

//*******************************************************
//* Function to receive the last data byte              *
//* (no acknowledge from master)                        *
//*******************************************************
//
unsigned char i2c_receivedata_nack(void)
{
  unsigned char data;
  i2c_setlasterror(0); 
  TWCR = (1<<TWINT)|(1<<TWEN);
  while (!(TWCR & (1<<TWINT)));	   	             // Wait for TWINT flag set. This indicates that the
   		 		   					             // data has been received
  if ((TWSR & 0xF8) != MR_DATA_NACK)             // Check value of TWI Status Register
   	  i2c_setlasterror(ERROR_CODE);
  
  data = TWDR;
  return(data);
}

//*******************************************************
//* Function to end the i2c communication               *
//*******************************************************
//
void i2c_stop(void)
{
  TWCR =  (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);	    // Transmit STOP condition
  while(TWCR & (1<<TWSTO));					  	// Ждем установки условия СТОП
}  



