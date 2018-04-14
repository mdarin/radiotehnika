/** \file
	\brief TEA5767
*/

#include "tea5767.h"



static unsigned char write_bytes[5] = {0};
static unsigned char read_bytes[5] = {0};
static uint32_t tune = 99900UL; // default freq

//------------------------------------------------------------------
/*
At power on, the mute bit (bit 7 of data byte 1) is set. 
All other bits are set to low. 
To initialise the IC all bytes should be transferred. 
If a stop condition is created before the whole transmission iscompleted, the remaining
bytes will keep their old setting. In case a byte is not completely transferred, 
the new bits will change their setting but a new tuning cycle will not be started. 
*/
int TEA5767_init(void)
{
  /* Write mode TEA5767 register values */

/* 0 register */
//#define TEA5767_MUTE			(1<<7)	/* Mute output */
//#define TEA5767_SM				(1<<6)	/* Search mode */
/* PLL 13..8 bits */

/* 1 register */
/* PLL 7..0 bits */

/* 2 register */
//#define TEA5767_SUD				(1<<7)	/* Search Up(1) / Down(0) */
//#define TEA5767_SSL_LOW			(1<<5)	/* Search stop level = 5 */
//#define TEA5767_SSL_MID			(2<<5)	/* Search stop level = 7 */
//#define TEA5767_SSL_HIGH		(3<<5)	/* Search stop level = 10 */
//#define TEA5767_HLSI			(1<<4)	/* High(1) / Low(0) Side Injection */
//#define TEA5767_MS				(1<<3)	/* Mono to stereo */
//#define TEA5767_MR				(1<<2)	/* Mute Right */
//#define TEA5767_ML				(1<<1)	/* Mute Left */
//#define TEA5767_SWP1			(1<<0)	/* Software programmable port 1 high */

/* 3 register */
//#define TEA5767_SWP2			(1<<7)	/* Software programmable port 2 high */
//#define TEA5767_STBY			(1<<6)	/* Standby */
//#define TEA5767_BL				(1<<5)	/* Band limit: Japan(1) / Europe(0) */
//#define TEA5767_XTAL			(1<<4)	/* Clock frequency 32768 (1) */
//#define TEA5767_SMUTE			(1<<3)	/* Soft mute */
//#define TEA5767_HCC				(1<<2)	/* High cut control */
//#define TEA5767_SNC				(1<<1)	/* Stereo Noise Cancelling */
//#define TEA5767_SI				(1<<0)	/* Search indicator */

/* 4 register */
//#define TEA5767_PLLREF			(1<<7)	/* 6.5MHz reference frequency */
//#define TEA5767_DTC				(1<<6)	/* De-emphasis 75us(1) / 50us(0) */
/* Not used 5..0 bits */
  
  write_bytes[0] = 0b00101111;  // default: 99.9 MHz
  write_bytes[1] = 0x87; // 0b1000 0111  
  write_bytes[2] = TEA5767_SUD | TEA5767_SRCH_MID_LVL;
  //write_bytes[2] |= TEA5767_MONO;
  write_bytes[3] = TEA5767_XTAL;                                                                
  write_bytes[4] = 0b00000000; // all params disabled
  // experimental
  TEA5767_write(); // wtire settings to chip register 
  /** \todo chip detection/identification */
  return 0;
}

//------------------------------------------------------------------
int TEA5767_write(void)
{
  uint8_t status = 0;
  
  i2c_100_init();
  
  // start condition
  if (i2c_start()) {
    uart_puts("i2c not started\r\n");
    status = 1; // nak
  }

/*	
	TWDR = SLA_W;                            	// send address
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	twst = TW_STATUS & 0xF8;
	if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) )
	{
		LOG(("Write error @ADDR, twst = %02X\n", twst));
		ret = 1;	
	}
	*/
  // send chip address
  //uart_puts("i2c chip addr\r\n");
  if (i2c_sendaddress(TEA5767_W)) {
    status = 1; // nak
	uart_puts("i2c send chip addr NAK\r\n");
  }	
	
/*	for (uint8_t i = 0; i < 5; i++)			// send registers
	{
		TWDR = write_bytes[i];                 
		TWCR = (1<<TWINT) | (1<<TWEN);      
		while (!(TWCR & (1<<TWINT)));
		// check value of TWI Status Register. Mask prescaler bits
		twst = TW_STATUS & 0xF8;
		if( twst != TW_MT_DATA_ACK)
		{
			LOG(("Write error, twst = %02X\n", twst));
			ret = 1;
			break;
		}
	}
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);	// I2C stop
	// wait until stop condition is executed and bus released
	while(TWCR & (1<<TWSTO));
	*/

  // send data bytes 
  //uart_puts("i2c send data\r\n");
  for (uint8_t i = 0; i < 5; i++) {
    if (i2c_senddata8(write_bytes[i])) {
      status = 1; // nak
	 // uart_puts("i2c send data NAK\r\n");
    }
  }
  
  // stop condition
  i2c_stop();
  
  return status;
}
 
//------------------------------------------------------------------ 
//setup the I2C hardware to ACK the next transmission
//and indicate that we've handled the last one.
#define TWACK (TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA))
//setup the I2C hardware to NACK the next transmission
#define TWNACK (TWCR=(1<<TWINT)|(1<<TWEN)) 
 
int TEA5767_read(void)  // Odczyt danych z TEA5767
{
	uint8_t ret = 0;
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// I2C start
	while (!(TWCR & (1<<TWINT)));   
	
	// check value of TWI Status Register. Mask prescaler bits.
	uint8_t twst = TW_STATUS & 0xF8;
	if ( (twst != TW_START) && (twst != TW_REP_START))
	{
	//	LOG(("Read error @START, twst = %02X\n", twst));
		ret = 1;
	}	
	
	TWDR = TEA5767_R;                           	// send address
	TWCR = (1<<TWINT) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	// check value of TWI Status Register. Mask prescaler bits.
	twst = TW_STATUS & 0xF8;
	if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) )
	{
	//	LOG(("Read error @ADDR, twst = %02X\n", twst));
		ret = 1;	
	}
		
	for (uint8_t i = 0; i < 5; i++)
	{
		if (i != 4)
			TWACK;
		else
			TWNACK;
		while (!(TWCR & (1<<TWINT)));
		read_bytes[i] = TWDR;    
	}
	
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);	// I2C stop
	// wait until stop condition is executed and bus released
	while(TWCR & (1<<TWSTO));	
	return ret;
}

//------------------------------------------------------------------
void TEA5767_tune(uint32_t value)
{
	// reference frequency = 32768 Hz
	// low side injection formula
	//LOG(("Tune %u\n", (unsigned int)(value/100)));
	uint16_t n = (uint32_t)4*(value*1000 - 225000) >> 15;
	write_bytes[0] = (write_bytes[0] & 0xC0) | (n >> 8);
	write_bytes[1] = n & 0xFF;
	tune = value;
	// experimental
	TEA5767_write(); // wtire settings to chip register 
}
//------------------------------------------------------------------
void TEA5767_search(uint8_t up)
{
	if (up)
	{
	//	LOG(("Search up\n"));
		write_bytes[2] |= TEA5767_SUD;
		TEA5767_tune(tune+150);
	}
	else
	{
	//	LOG(("Search down\n"));
		write_bytes[2] &= ~TEA5767_SUD;		
		TEA5767_tune(tune-150);
	}
	write_bytes[0] |= TEA5767_SEARCH | TEA5767_MUTE;
	TEA5767_write();
}
//------------------------------------------------------------------
void TEA5767_exit_search(void)
{
	write_bytes[0] = (read_bytes[0] & 0x3f);
	write_bytes[1] = read_bytes[1];
	write_bytes[0] &= ~(TEA5767_SEARCH | TEA5767_MUTE);
	TEA5767_write();
	tune = ((((read_bytes[0]&0x3F)<<8)+read_bytes[1])*32768/4 + 225000)/1000;
	//LOG(("Exit search, tuned = %u\n", (unsigned int)(tune/100)));
}
//------------------------------------------------------------------	
int TEA5767_get_status(struct TEA5767_STATUS *status)
{
	TEA5767_read();
	//LOG(("read_bytes %02X %02X %02X %02X %02X\n", read_bytes[0], read_bytes[1], read_bytes[2], read_bytes[3], read_bytes[4]));
	memset(status, 0, sizeof(*status));
	
	uint32_t freq = ((((read_bytes[0]&0x3F)<<8)+read_bytes[1])*32768/4 + 225000)/1000;
	//LOG(("Freq = %u\n", (unsigned int)(freq/100)));	
	
	if (read_bytes[0] & TEA5767_READY_FLAG) /* ready */
	{
	//	LOG(("Ready\n"));
		status->ready = 1;
		uint8_t val = read_bytes[2] & 0x7F; /* IF counter */
		if (abs(val - 0x36) < 2) 			 /* close match */
		{
	//		LOG(("Tuned!\n"));
			status->tuned = 1;
		}
	}
	if (read_bytes[0] & TEA5767_BAND_LIMIT_FLAG)
	{
	//	LOG(("Band limit\n"));
		status->band_limit = 1;
	}
	if (read_bytes[2] & TEA5767_STEREO)
	{
	//	LOG(("Stereo reception\n"));
		status->stereo = 1;
	}
	status->rx_power = read_bytes[3] >> 4;	
	//LOG(("rx_power = %d\n", status->rx_power));
	return 0;
}
