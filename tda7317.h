#ifndef TDA7317_H
#define TDA7317_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "i2c/i2c.h"

//TDA7317 ###################
// Chip address byte
// The TDA7317 chip address is 0x84

//STATUS AFTER POWER-ON RESET
// Volume -17.25dB
// Graphic equalizer bands -12dB

#define TDA7317_CHIP 0b10000100 //0x84 chip address
#define BOOST 3 // boost function
#define CUT 3 //cut function
// BANDS
#define BAND_1_10363_Hz 	4 
#define BAND_2_261_Hz 		1
#define BAND_3_1036_Hz 		2
#define BAND_4_3168_Hz 		3
#define BAND_5_59_Hz 		0

// function prototypes
void TDA7317_init(void);
uint8_t TDA7317_send(uint8_t data);
void TDA7317_boost(uint8_t band, uint8_t value);
void TDA7317_cut(uint8_t band, uint8_t value);
void TDA7317_set_band(uint8_t band, uint8_t value);


#endif // TDA7317_H
