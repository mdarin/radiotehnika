#include "tda7317.h"

//------------------------------------------------------
void TDA7317_init(void)
{
  // set all bands to zero db
  //-------TDA7317 init------------------
  TDA7317_send(0);
  TDA7317_send(0);
  TDA7317_send(0x80); //band1
  TDA7317_send(0x90); //band2
  TDA7317_send(0xA0); //band3
  TDA7317_send(0xB0); //band4
  TDA7317_send(0xC0); //band5
  //-------------------------------------
}
//------------------------------------------------------
uint8_t TDA7317_send(uint8_t data)
{
  uint8_t status = 0;
  i2c_100_init();
  // start condition
  if (i2c_start()) {
    status = 1; // nak
  }
  
  // send chip address
  //uart_puts("i2c chip addr\r\n");
  if (i2c_sendaddress(TDA7317_CHIP)) {
    status = 1; // nak
  }

  //data |= (1 << 8);

  // send a byte of a data
  //uart_puts("i2c send data\r\n");
  if (i2c_senddata8(data)) {
    status = 1; // nak
  }
  
  // stop condition
  i2c_stop();
  
  return status;
} 
//------------------------------------------------------
void TDA7317_boost(uint8_t band, uint8_t value)
{  
	// check the value on out of range
	if (value > 0x07) {
		value = 0x07; // set max cut/boost dB value forced 
	}
	
	uint8_t packet = 0;
	// fill in packet
	packet |= value; // 3 bits of band dB value
	packet &= ~(1 << BOOST); // set boost function
	
	switch (band) {
	case BAND_1_10363_Hz:
		packet |= 0x80; // set band 1
		break;
	case BAND_2_261_Hz:
		packet |= 0x90; // set band 2
		break;
	case BAND_3_1036_Hz:
		packet |= 0xA0; // set band 3
		break;
	case BAND_4_3168_Hz:
		packet |= 0xB0; // set band 4
		break;
	case BAND_5_59_Hz:
		packet |= 0xC0; // set band 5	
		break;
	} // end switch

	TDA7317_send(packet);
	
	return;
}
//------------------------------------------------------
void TDA7317_cut(uint8_t band, uint8_t value)
{
	// check the value on out of range
	if (value > 0x07) {
		value = 0x07; // set max cut/boost dB value forced 
	}
	
	uint8_t packet = 0;
	// fill in packet
	packet |= value; // 3 bits of band dB value
	packet |= (1 << CUT); // set boost function
	
	// set band
	switch (band) {
	case BAND_1_10363_Hz:
		packet |= 0x80; // set band 1
		break;
	case BAND_2_261_Hz:
		packet |= 0x90; // set band 2
		break;
	case BAND_3_1036_Hz:
		packet |= 0xA0; // set band 3
		break;
	case BAND_4_3168_Hz:
		packet |= 0xB0; // set band 4
		break;
	case BAND_5_59_Hz:
		packet |= 0xC0; // set band 5	
		break;
	} // end switch
	
	TDA7317_send(packet);
	
	return;
}

//------------------------------------------------------
void TDA7317_set_band(uint8_t band, uint8_t value)
{
  // check the value on out of range
  if (value > 15) {
    value = 15; 
  }

  if (value < 0) {
    value = 0;
  }

  /*
  Diapasone

  0000  0 	 0 dB
  0001  1 	 2 dB
  0010  2	 4 dB
  0011  3	 6 dB
  0100  4	 8 dB
  0101  5	 10 dB
  0110  6	 12 dB
  0111  7 	 14 dB
  1000  8 	 0 dB
  1001  9	-2 dB
  1010  10	-4 dB
  1011  11	-6 dB
  1100  12	-8 dB
  1101  13	-10 dB
  1110  14	-12 dB
  1111  15 	-14 dB

  */

  uint8_t packet = 0;
  // fill in packet
  packet |= value;

  switch (band) {
  case BAND_1_10363_Hz:
    packet |= 0x80; // set band 1
	break;
  case BAND_2_261_Hz:
	packet |= 0x90; // set band 2
	break;
  case BAND_3_1036_Hz:
	packet |= 0xA0; // set band 3
	break;
  case BAND_4_3168_Hz:
	packet |= 0xB0; // set band 4
	break;
  case BAND_5_59_Hz:
	packet |= 0xC0; // set band 5	
	break;
  } // end switch
	
  TDA7317_send(packet);
  
  return;
}
