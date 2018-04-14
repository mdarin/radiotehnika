#include "tda7439.h"

//--------------------------------------------------------------
void TDA7439_init(void)
{
	TDA7439_send(TDA7439_INPUT_SELECTOR, 0x02);
	TDA7439_send(TDA7439_INPUT_GAIN, 0);
	TDA7439_send(TDA7439_VOLUME, 30);
	TDA7439_send(TDA7439_BASS_GAIN, 0x07);
	TDA7439_send(TDA7439_MIDRANGE_GAIN, 0x07);
	TDA7439_send(TDA7439_TREBLE_GAIN, 0x07);
	TDA7439_send(TDA7439_SPEAKER_ATT_R, 0);
	TDA7439_send(TDA7439_SPEAKER_ATT_L, 0);
	return;
}
//--------------------------------------------------------------
uint8_t TDA7439_send(uint8_t subaddr, uint8_t data)
{
	uint8_t status = 0;
	i2c_100_init();
	// start condition
	//uart_puts("i2c init 100\r\n");
  
	//uart_puts("i2c start\r\n");
	if (i2c_start()) {
		//uart_puts("NOT STARTED\r\n");
		status = 1;
	}
    
	// send chip address
	//uart_puts("i2c chip addr\r\n");
	if (i2c_sendaddress(TDA7439_CHIP)) {
		//uart_puts("NAK\r\n");
		status = 1;
	}
  
	//uart_puts("i2c send sub\r\n");
	// send subaddress
	if (i2c_senddata8(subaddr)) {
		//uart_puts("NAK\r\n");
		status = 1;
	}

	// send a byte of a data
	//uart_puts("i2c send data\r\n");
	if (i2c_senddata8(data)) {
		//uart_puts("NAK\r\n");
		status = 1;
	}
  
	// stop condition
	i2c_stop();
	//uart_puts("i2c stop\r\n");
	return status;
}
//--------------------------------------------------------------
void TDA7439_set_input_selector(uint8_t selector)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_INPUT_SELECTOR, selector);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_input_gain(uint8_t gain)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_INPUT_GAIN, gain);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_volume(uint8_t volume)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_VOLUME, volume);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_bass_gain(uint8_t gain)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_BASS_GAIN, gain);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_midrange_gain(uint8_t gain)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_MIDRANGE_GAIN, gain);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_treble_gain(uint8_t gain)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_TREBLE_GAIN, gain);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_speaker_att_R(uint8_t att)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_SPEAKER_ATT_R, att);
	return;
}
//--------------------------------------------------------------
void TDA7439_set_speaker_att_L(uint8_t att)
{
	//TODO: check on out of range!
	TDA7439_send(TDA7439_SPEAKER_ATT_L, att);
	return;
}
//--------------------------------------------------------------
