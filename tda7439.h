#ifndef TDA7439_H
#define TDA7439_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "i2c/i2c.h"

// TDA7439 ################## 
// Power-on-reset conditions Parameter POR value
// Input selection               IN2
// Input gain                    28 dB
// Volume                        MUTE
// Bass                          0 dB
// Mid-range                     2 dB
// Treble                        2 dB
// Speaker                       MUTE
//volatile static uint8_t TDA7439_input_selection = 0b00000010;
//volatile static uint8_t TDA7439_input_gain = 0b00001110;
//volatile static uint8_t TDA7439_volueme = 0b00111000;
//volatile static uint8_t TDA7439_bass = 0b00000111;
//volatile static uint8_t TDA7439_mid_range = 0b00001110;
//volatile static uint8_t TDA7439_treble = 0b00001110;
//volatile static uint8_t TDA7439_speaker_L = 0b01111000;
//volatile static uint8_t TDA7439_speaker_R = 0b01111000;

// Chip address byte
// The TDA7439 chip address is 0x88.
#define TDA7439_CHIP 0b10001000 //0x88 // 10001000 chip address

// Sub-address byte
// The function is selected by the 4-bit sub address as given in Ta bl e 7. The three MSBs are 
// not used and bit D4 selects address incrementing (B = 1) or single data byte (B = 0).
// Function selection: sub-address byte
// MSB                  LSB
// Function
// D7 D6 D5 D4 D3 D2 D1 D0   
// X  X  X  B  0  0  0  0 Input selector
// X  X  X  B  0  0  0  1 Input gain
// X  X  X  B  0  0  1  0 Volume
// X  X  X  B  0  0  1  1 Bass gain
// X  X  X  B  0  1  0  0 Mid-range gain
// X  X  X  B  0  1  0  1 Treble gain
// X  X  X  B  0  1  1  0 Speaker attenuation, R
// X  X  X  B  0  1  1  1 Speaker attenuation, L
#define TDA7439_INPUT_SELECTOR 0b00010000
#define TDA7439_INPUT_GAIN 0b00010001
#define TDA7439_VOLUME 0b00010010
#define TDA7439_BASS_GAIN 0b00010011
#define TDA7439_MIDRANGE_GAIN 0b00010100
#define TDA7439_TREBLE_GAIN 0b00010101
#define TDA7439_SPEAKER_ATT_R 0b00010110
#define TDA7439_SPEAKER_ATT_L 0b00010111

#define TDA7439_IN4 0x00
#define TDA7439_IN3 0x01
#define TDA7439_IN2 0x02
#define TDA7439_IN1 0x03

// Data bytes
// The function value is changed by the data byte as given in the following tables, Ta bl e 8to 
// Table 14.
// In the tables of input gain, volume and attenuation, not all values are shown. A desired 
// intermediate value is obtained by setting the three LSBs to the appropriate value.
// see datasheet :)




// Interface protocol
// The interface protocol comprises:
// * a start condition (S)
// * a chip-address byte, containing the TDA7439 address
// * a sub-address byte including an auto address-increment bit
// * a sequence of data bytes (N bytes + acknowledge) 
// * a stop condition (P).
// TDA7439 packet
// [Chip address] [Subaddress] [Data 1] to [Data N]

void TDA7439_init(void);
uint8_t TDA7439_send(uint8_t subaddr, uint8_t data);
void TDA7439_set_input_selector(uint8_t selector);
void TDA7439_set_input_gain(uint8_t gain);
void TDA7439_set_volume(uint8_t volume);
void TDA7439_set_bass_gain(uint8_t gain);
void TDA7439_set_midrange_gain(uint8_t gain);
void TDA7439_set_treble_gain(uint8_t gain);
void TDA7439_set_speaker_att_R(uint8_t att);
void TDA7439_set_speaker_att_L(uint8_t att);



#endif // TDA7439_H
