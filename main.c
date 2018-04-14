/*
 * Наименование: Radiotehnika
 * Версия 0.1
 *
 * FSM Logic graph
  // enter = кнопка энкодера
  //MainScreen -(ecn+/-)-> Volume
  //           -(preset_key)-> Presets
  //           -(enter_key)-> Gain
  //Gain -(ecn+/-)-> Bass
  //     -(enter_key)-> Gain_Settings
  //Middle -(ecn+/-)-> Treble
  //       -(enter_key)-> Middle_Setting
  //Treble -(ecn+/-)-> Equalizer
  //      -(enter_key)-> Treable_Settings
  //Equalize -(ecn+/-)-> Resst_All_Setting
  //         -(enter_key)-> Equalizer_Settings
  //Reset_All_Setting -(ecn+/-)-> Gain
  //         -(enter_key)-> Reset -> MainScreen
  //Equalizer_Settings -> 60_Hz
  //60_Hz -(ecn+/-)-> 260_Hz
  //      -(enter_key)-> 60_Hz_Settings
  //260_Hz -(ecn+/-)-> 1_kHz
  //       -(enter_key)-> 260_Hz_Settings 
  //1_kHz -(ecn+/-)-> 3_kHz
  //      -(enter_key)-> 3_kHz_Settings
  //3_kHz -(ecn+/-)-> 10_kHz
  //      -(enter_key)-> 3_kHz_Settings
  //10_kHz -(ecn+/-)-> 60_Hz
  //       -(enter_key)-> 10_kHz_Settigs
 *
 */

 
#include <avr/version.h>
#if __AVR_LIBC_VERSION__ < 10606UL
#error "please update to avrlibc 1.6.6 or newer, not tested with older versions"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

//#include "spilight/spilight.h"
#include "hd44780/hd44780.h"

#include "uart/uart.h"
#include "uart/uart_addon.h"
#include "i2c/i2c.h"
#include "adc/atmega-adc.h"

#include "tda7317.h"
#include "tda7439.h"
//#include "indication.h"
//#include "keyboard.h"


// ##################
#ifndef INFINITY
#define INFINITY 1
#endif
#ifndef F_CPU
#define F_CPU 20000000L
#endif


// ##################
#define ON 	1
#define OFF 0



// AMP settings ===============
// non volatile
#define NV_input_gain_addr  0x01
#define NV_bass_gain_addr  0x05
#define NV_mid_range_gain_addr  0x10
#define NV_treble_gain_addr  0x15
#define NV_balance_addr 0x20
#define NV_band_1_10363_Hz_addr 0x30 
#define NV_band_2_261_Hz_addr  0x35
#define NV_band_3_1036_Hz_addr  0x40
#define NV_band_4_3168_Hz_addr  0x45
#define NV_band_5_59_Hz_addr  0x50




// dynamic
uint8_t g_input_gain = 0;
uint8_t g_bass_gain = 0x07;
uint8_t g_mid_range_gain = 0x07;
uint8_t g_treble_gain = 0x07;
int8_t g_balance = 0;
uint8_t g_band_1_10363_Hz = 0; 
uint8_t g_band_2_261_Hz = 0;
uint8_t g_band_3_1036_Hz = 0;
uint8_t g_band_4_3168_Hz = 0;
uint8_t g_band_5_59_Hz = 0;


// ADC =====================
#define ADC_PIN 7 // ADC INPUT

void adc_keys_handler(uint8_t pin, uint16_t value)
{
  uart_puts("adc:\r\n");
  uart_puts("pin:");
  uart_put_int(pin);
  uart_puts("\r\n");
  uart_puts("val:");
  uart_put_int(value);
  uart_puts("\r\n");
  return;
}


// Timer 0 PWM =================
volatile static uint8_t pwm_value = 0;
#define UP 1
#define DOWN 2
volatile static uint8_t pwm_direction = UP; 
void set_pwm_off(void)
{
  OCR0B = 0; 
  DDRD &= ~(1 << PD5);
}


void set_pwm_on(void)
{
  TIMSK0 |= (1 << OCIE0B);
  TCCR0A |= (1 << COM0B1) | (1 << WGM00);
  TCCR0B |= (1 << CS02);/*(1 << CS01) | (1 << CS00);*/
  //OCR0B = pwm_value;
  DDRD |= (1 << PD5);
}

void set_pwm_value(uint8_t value)
{
  OCR0B = value;
}

//void set_pwm_direction(uint8_t direction)
//{
//  pwm_direction = direction;
//}

void soft_pwm_on()
{
  pwm_value = 0;
  pwm_direction = UP;
  set_pwm_on();
}

void soft_pwm_off()
{
  pwm_value = 255;
  pwm_direction = DOWN;
  set_pwm_on();
}

ISR (TIMER0_COMPB_vect)
{
  switch (pwm_direction) {
  case UP:
    if (pwm_value < 255) {
	  if (pwm_value >= 0) set_pwm_value(pwm_value++);
	} 
    break;
  case DOWN:
    if (pwm_value > 0) {
      if (pwm_value <= 255) set_pwm_value(pwm_value--);
	}
    break;
  }
}


 
// Timer 1 =================
volatile static uint8_t menu_timeout = 0;
//-------------------------------------------------------------
ISR (TIMER1_OVF_vect)
{
  TCCR1B = 0; // stop
  
  menu_timeout = 1; 
  //TCNT1H = 0x88; // ???????? 3 ???
  //TCNT1L = 0xB8;
  //TCCR1B = (1 << CS12) | (1 << CS10); // start
}

void init_menutimeout(void)
{
  TIMSK1 |= (1 << TOIE1);
}

void start_menutimeout(void)
{
  menu_timeout = 0;
  TCNT1H = 0x00;//0x88; // ajust
  TCNT1L = 0x60;
  TCCR1B |= (1 << CS12) | (1 << CS10);
}

void restart_menutimeout(void)
{
  TCCR1B = 0;
  TCNT1H = 0x44; // ajust
  TCNT1L = 0xB8;
  TCCR1B |= (1 << CS12) | (1 << CS10);
}

void stop_menutimeout(void)
{
  TCCR1B = 0;
}



// Deveise staets =============
#define MAIN 0
#define PRESETS 2
#define VOLUME 1
#define MENU 3
#define MUTE 4  

uint8_t dev_state = MAIN;





// Encoder =========== 
//================
volatile char irq18 = 0;
volatile char irq19 = 0;
volatile char irq20 = 0;
//volatile char irq11 = 0;
// http://www.avrfreaks.net/forum/pcint1-not-working-pcint0-works-fine
// There are three interrupt vectors:
//ISR(PCINT0_vect){} // for pins PCINT0-PCINT7   (PB0-PB7)  
//ISR(PCINT1_vect){} // for pins PCINT8-PCINT14  (PC0-PC6)
//ISR(PCINT2_vect){} // for pins PCINT16-PCINT23 (PD0-PD7)
static volatile char enc_direction = 0;

ISR (PCINT2_vect)
{

// PD4 button irq20
// PD3 enc2 irq19
// PD2 enc1 irq18

  // enc button 
  if (bit_is_set(PIND, PIND4)) {
    irq20 = 1;
  }
     
  // encoder skip undefined state
  if (bit_is_set(PIND, PIND3) && bit_is_set(PIND, PIND2)) {    	
	enc_direction = 0;
	return;
  }

  if (bit_is_clear(PIND, PIND3) && bit_is_set(PIND, PIND2) && !enc_direction) {    	
		enc_direction = 1;
  } else if (bit_is_clear(PIND, PIND2) && bit_is_set(PIND, PIND3) && !enc_direction) {
		enc_direction = 2;
  } 

  if (enc_direction) {
    if (bit_is_clear(PIND, PIND2) && bit_is_set(PIND, PIND3) && 1 == enc_direction) {
		irq19 = 1;
		enc_direction = 0;
    } else if (bit_is_clear(PIND, PIND3) && bit_is_set(PIND, PIND2) && 2 == enc_direction) {    	
		irq18 = 1;
		enc_direction = 0;
    } 
  }
  
  return; 
}


void init_keys(void)
{
  // encoder init
  PCICR |= (1 << PCIE2); 
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19) | (1 << PCINT20); 
  return;
}


#define NO_KEYS 0

#define E_BUTTON 1
#define E_UP 2
#define E_DOWN 3

#define MONO_BUTTON 4
#define MUTING_BUTTON 5
#define HI_FILTER_BUTTON 6
#define LOUDNESS_BUTTON 7
#define TONE_BUTTON 8

#define TUNER_BUTTON 9
#define PHONO_BUTTON 10 // уcтанавливается по умолчанию
#define TAPE_1_BUTTON 11
#define TAPE_2_BUTTON 12
#define PROTECT_SIG 13 // сигнал сработки защиты

/*
TONE						10000	10000	10000
HI_FILTER					8200	8200	2700
MUTING						6200	6800	1000
MONO 						5100	5600	430

TONE_HI_FILTER 				4500	4500	
HI_FILTER_MONO 	+			4100	3327	370
TONE_MUTING 				3827	4047
HI_FILTER_MUTING 			3530	3717
TONE_MONO 					3377	3939
MONO_MUTING 				2800	
TONE_HI_FILTER_MUTING 		2609			357
TONE_HI_FILTER_MONO +		2392	
MONO_MUTING_HI_FILTER_TONE 	1726	
*/


// значения состояния селектора входов
uint8_t input_selector_cur = 0;
uint8_t input_selector_prev = 0;//input_selector_cur;

uint8_t get_key(void)
{
  uint8_t key = NO_KEYS;
  
  // обработать кнопки по прерываниям
  if (irq18) {
    irq18 = 0;  
    key = E_DOWN;
  } else if (irq19) {
    irq19 = 0;  
    key = E_UP;
  } else if (irq20) {
    irq20 = 0;
    key = E_BUTTON;
  } //else if (irq11) {
   // irq11 = 0;
	//key = 30;
  //}

  // обработать кнопки на АЦП
  // сделать несколько попыток считывания показаний АЦП
  uint16_t adc_value = 0;
  adc_value = adc_read(ADC_PRESCALER_4, ADC_VREF_AVCC, ADC_PIN); 
  _delay_ms(1);
  adc_value = adc_read(ADC_PRESCALER_4, ADC_VREF_AVCC, ADC_PIN); 
  _delay_ms(1);
  adc_value = adc_read(ADC_PRESCALER_4, ADC_VREF_AVCC, ADC_PIN); 

  // значения необходимо подогнать под конкретный набор резисторов
  if (820 < adc_value && adc_value < 830) {
    // кнопка LOUDNESS
	key = LOUDNESS_BUTTON;
  } else if (430 < adc_value && adc_value < 450) {
    // кнопка HI-FILTER
	key = HI_FILTER_BUTTON;
  } else if ( 710 < adc_value && adc_value < 730) {
	// кнопка MUTING 
    key = MUTING_BUTTON;
  } 


 
  // обработать триггерные кнопки 
  // Быты порта С 
  // PINC0 - PROTECT 
  // PINC1 - TUNER
  // PINC2 - TAPE-2
  // PINC3 - TAPE-1 
  // если текущее состояние отличается от прошлого
  // получить текущее состояние селектора входов и бит сигнала сработки защиты
  input_selector_cur = 0x0F & PINC; 

  //uart_putbin_byte(input_selector_cur);
  //uart_putbin_byte(input_selector_prev);

  if (input_selector_cur != input_selector_prev) {
    // если состояние изменилос, то сохранить его и считать кнопку
    input_selector_prev = input_selector_cur;
    // полчить нажатую кнопку    
	
	if (bit_is_clear(PINC, PINC1) && bit_is_clear(PINC, PINC2) && bit_is_clear(PINC, PINC3) ) {
	  // если L L L, то нажата кнопка PHONO
	  key = PHONO_BUTTON;
	} else if (bit_is_set(PINC, PINC1) && bit_is_clear(PINC, PINC2) && bit_is_clear(PINC, PINC3) ) {
	  // если H L L, то нажата кнопка TUNER 
      key = TUNER_BUTTON;
	} else if (bit_is_clear(PINC, PINC1) && bit_is_set(PINC, PINC2) && bit_is_clear(PINC, PINC3) ) {
	  // если L H L, то нажата кнопка TAPE-2
      key = TAPE_2_BUTTON;
	} else if (bit_is_clear(PINC, PINC1) && bit_is_clear(PINC, PINC2) && bit_is_set(PINC, PINC3) ) {
	  // елси L L H, то нажата кнопка TAPE-1
      key = TAPE_1_BUTTON;
	}
  }

  // обработать сигнал защиты
  if (bit_is_set(PINC, PINC0) ) {
    key = PROTECT_SIG;
  } 

  return key;
}





// MENU ====================
typedef struct menu_t {
  char* item;
  void (*action)(void);
} menu_t;

// фукция готовить сроку уровня в зависим. от з-я параметра и выводит на дисп.
void proc_value_1(char *name, uint8_t value);
void proc_value_2(char *name, uint8_t band, uint8_t value);

// прототипы действий пунктов меню
void action_setup_gain(void); 
void action_setup_bass(void);
void action_setup_middle(void);
void action_setup_treble(void);
void action_setup_equalizer(void);
void action_setup_balance(void);
void action_reset_all(void);

void proc_value_1(char *name, uint8_t value)
{  
  if (NULL == name) return;

  char level[16] = {0}; // 16 знакомест вего в видимой части строки
  char str_val[16] = {0};  

  uint8_t reg_val = 0; // значение записываемое в рег. микросхемы

  // подготоить плоску упровня и строку со значением параметра
  switch(value) {
  case 0:
      strcpy(level, "<<<<<<<|");
      sprintf(str_val, "%s -14dB", name);
	  reg_val = value;        
      break;
  case 1:
      strcpy(level, " <<<<<<|");
      sprintf(str_val, "%s -12dB", name);
	  reg_val = value;
      break;
  case 2:
      strcpy(level, "  <<<<<|");
      sprintf(str_val, "%s -10dB", name);
	  reg_val = value;
      break;
  case 3:
      strcpy(level, "   <<<<|");
      sprintf(str_val, "%s -8dB", name);
	  reg_val = value;
      break;
  case 4:
      sprintf(str_val, "%s -6dB", name);
      strcpy(level, "    <<<|");
	  reg_val = value;
      break;
  case 5:
      strcpy(level, "     <<|");
      sprintf(str_val, "%s -4dB", name);
	  reg_val = value;
      break;
  case 6:
      strcpy(level, "      <|");
      sprintf(str_val, "%s -2dB", name);
	  reg_val = value;
      break;
  case 7:
      strcpy(level, "       |");
      sprintf(str_val, "%s 0dB", name);
	  reg_val = value;
      break;
  case 8:
      strcpy(level, "       |>");
      sprintf(str_val, "%s 2dB", name);
      reg_val = 14;
	  break;
  case 9:
      strcpy(level, "       |>>");
      sprintf(str_val, "%s 4dB", name);
	  reg_val = 13;       
      break;
  case 10:
      strcpy(level, "       |>>>");
      sprintf(str_val, "%s 6dB", name);
	  reg_val = 12;       
      break;
  case 11:
      strcpy(level, "       |>>>>");
      sprintf(str_val, "%s 8dB", name);
	  reg_val = 11;  
      break;
  case 12:
      strcpy(level, "       |>>>>>");
      sprintf(str_val, "%s 10dB", name);
	  reg_val = 10;      
      break;
  case 13:
      strcpy(level, "       |>>>>>>");
      sprintf(str_val, "%s 12dB", name);
	  reg_val = 9;       
      break;
  case 14:
      strcpy(level, "       |>>>>>>>");
      sprintf(str_val, "%s 14dB", name);
	  reg_val = 8;       
      break;
  } // eof switch


  // высести полоску уровня
  lcd_put_ln1(level);

  // вывести значение	  
  lcd_put_ln2(str_val);

  // записать значение в регистр микросхемы
  if (!strcmp(name, "Bass") ) {
    // записать значение в региср микросхемы
    TDA7439_set_bass_gain(reg_val);
	// записать значение в ПЗУ
	eeprom_write_byte(NV_bass_gain_addr, g_bass_gain);
  }

  if (!strcmp(name, "Middle") ) {
    // записать значение в региср микросхемы
	TDA7439_set_midrange_gain(reg_val);
    // записать значение в ПЗУ
	eeprom_write_byte(NV_mid_range_gain_addr, g_mid_range_gain);
  }

  if (!strcmp(name, "Treble") ) {
    // записать значение в региср микросхемы
    TDA7439_set_treble_gain(reg_val);
    // записать значение в ПЗУ
	eeprom_write_byte(NV_treble_gain_addr, g_treble_gain);
  }
  
  return;
}


void proc_value_2(char *name, uint8_t band, uint8_t value)
{  
  if (NULL == name) return;

  char level[16] = {0}; // 16 знакомест вего в видимой части строки
  char str_val[16] = {0};  

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
  
  // подготоить плоску упровня и строку со значением параметра
  switch(value) {
  case 0: // -14 dB
      strcpy(level, "<<<<<<<|");
      sprintf(str_val, "%s -14 dB", name);
	  // установить значение в регистр миросхемы
      TDA7317_set_band(band, 15);
      // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
	  
	  break;
  case 1:
      strcpy(level, " <<<<<<|");
      sprintf(str_val, "%s -12 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 14); 
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch   
      break;
  case 2:
      strcpy(level, "  <<<<<|");
      sprintf(str_val, "%s -10 dB", name);     
      // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 13);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
	  break;
  case 3:
      strcpy(level, "   <<<<|");
      sprintf(str_val, "%s -8 dB", name);      
      // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 12);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
	  break;
  case 4:
      sprintf(str_val, "%s -6 dB", name);
      strcpy(level, "    <<<|");
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 11);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
      break;
  case 5:
      strcpy(level, "     <<|");
      sprintf(str_val, "%s -4 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 10);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
      break;
  case 6:
      strcpy(level, "      <|");
      sprintf(str_val, "%s -2 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 9);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch 
      break;
  case 7:
      strcpy(level, "       |");
      sprintf(str_val, "%s 0 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 0);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch     
      break;
  case 8:
      strcpy(level, "       |>");
      sprintf(str_val, "%s 2 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 1);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
	  break;          
  case 9:
      strcpy(level, "       |>>");
      sprintf(str_val, "%s 4 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 2);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch      
      break;
  case 10:
      strcpy(level, "       |>>>");
      sprintf(str_val, "%s 6 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 3);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch    
      break;
  case 11:
      strcpy(level, "       |>>>>");
      sprintf(str_val, "%s 8 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 4);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch
      break;
  case 12:
      strcpy(level, "       |>>>>>");
      sprintf(str_val, "%s 10 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 5);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch      
      break;
  case 13:
      strcpy(level, "       |>>>>>>");
      sprintf(str_val, "%s 12 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 6); 
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch   
      break;
  case 14:
      strcpy(level, "       |>>>>>>>");
      sprintf(str_val, "%s 14 dB", name);
	  // установить значение в регистр миросхемы
	  TDA7317_set_band(band, 7);
	  // записать значение в ПЗУ	  
	  switch (band) {
      case BAND_1_10363_Hz:
        eeprom_write_byte(NV_band_1_10363_Hz_addr, g_band_1_10363_Hz); 
	    break;
      case BAND_2_261_Hz:
	    eeprom_write_byte(NV_band_2_261_Hz_addr, g_band_2_261_Hz);
	    break;
      case BAND_3_1036_Hz:
	    eeprom_write_byte(NV_band_3_1036_Hz_addr, g_band_3_1036_Hz);
	    break;
      case BAND_4_3168_Hz:
	    eeprom_write_byte(NV_band_4_3168_Hz_addr, g_band_4_3168_Hz);
	    break;
      case BAND_5_59_Hz:
	    eeprom_write_byte(NV_band_5_59_Hz_addr, g_band_5_59_Hz);
	    break;
      } // end switch        
      break;
  } // eof switch


  // высести полоску уровня
  lcd_put_ln1(level);

  // вывести значение	  
  lcd_put_ln2(str_val);

  return;
}




// основное меню
int8_t main_menu_pos = 0;
menu_t main_menu[] = {
    {"Gain", action_setup_gain}, 
	{"Bass", action_setup_bass},
	{"Middle", action_setup_middle},
	{"Treble", action_setup_treble},
	{"Equalizer", action_setup_equalizer},
	{"Balnce", action_setup_balance},
	{"Rest All", action_reset_all},
	{NULL, NULL}, // end of menu list
  };

// подчёт элементов меню
uint8_t menu_count(menu_t *menu) 
{ 
  uint8_t count = 0;
  if (NULL != menu) {
    for (uint8_t i = 0; menu[i].item != NULL; i++, count++) continue;
  }
  return count;
}

// дейсвия соотв. пунктам меню
// общая схема работы
// при возове пункта меню запускаестя таймер отсчитывающий таймаут на выход
// пока не таймаут можно энкодером установить значение (на лету)
// или по нажатия кнопки Ввод выйте из пунта меню и установка текущего занчения
// по сути таймаут управляет только логикой
//------------------------------------------------------------
void  action_setup_gain(void)
{
  uart_puts("Setup gain\r\n");
  
  uint8_t done = 0;
  // отсветка уроня установленной громкоси(полоска)
  char level[16] = {0}; // 16 знакомест вего в видимой части строки
  char gn[16] = {0};


  // Отобразить текущее значение
  // отобразить уровень громкости на дисплее(полоской)
  //тут подгонка чтобы один сегмент оставался
  for (uint8_t i = 0; i < 16; i++) {	    		   			  
    if (i == g_input_gain) {			  	  
	
      // если значение больше половины возможного 
	  if (i < 16/2) { 
	    uint8_t j;
		//тут подгонка чтобы один сегмент оставался
		for (j = 1; j < i+1; j++) {
		  level[j] = '>';
		}
		level[0] = '>';
		level[j+1] = '\0';
      } else {
        // тут подгонка чтобы один сегмент оставался
	    uint8_t j;
		for (j = 0; j < i+1; j++) {
		  level[j] = '>';
		}
		level[0] = '>';
		level[j+1] = '\0';
	  }
	  i = 16; // остановить цикл
    }
  }
  // отобразить полоску уровня
  lcd_put_ln1(level);
  // подготовить и тобразить строку со значением	  
  sprintf(gn, "Gain %d dB", g_input_gain*2);
  lcd_put_ln2(gn);  


  // Обработать изменения значения    
  // запустить таймаут на установку параметра
  start_menutimeout();
  while (!menu_timeout && !done) {
    // настроить уровень входного предуселения
	switch (get_key()) {
	case E_UP:
	  restart_menutimeout();	  
	  if (g_input_gain < 16) g_input_gain++;
	  if (g_input_gain >= 16) g_input_gain = 15;

	  //TODO: тестить ограничитель!

	  // отобразить уровень громкости на дисплее(полоской)
	  //тут подгонка чтобы один сегмент оставался
	  for (uint8_t i = 0; i < 16; i++) {
		if (i == g_input_gain) {		  
		  
		  uart_put_int(i);
		  uart_puts("\r\n");
		  
		  uint8_t j;
		  //тут подгонка чтобы один сегмент оставался
		  for (j = 1; j < i+1; j++) {
		    level[j] = '>';
		  }
		  level[0] = '>';
		  level[j+1] = '\0';
	  	  i = 16; // остановить цикл
		}
	  }
      // вывести полоску уровня
	  lcd_put_ln1(level);
	  // подготовить и вывести значение 	  
	  sprintf(gn, "Gain %d dB", g_input_gain*2);
	  lcd_put_ln2(gn);
      // установить значение в регистр миросхемы
	  TDA7439_set_input_gain(g_input_gain);

	  // записать значение в ПЗУ
	  eeprom_write_byte(NV_input_gain_addr, g_input_gain);
	  
	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	 	  
	  if (g_input_gain > 0) g_input_gain--;
	  else break; 
	  //TODO: ограничитель тестить!
		  
	  restart_menutimeout();
		  
	  // отобразить уровень громкости на дисплее(полоской)
	  //тут подгонка чтобы один сегмент оставался
	  for (uint8_t i = 0; i < 16; i++) {
	    if (i == g_input_gain) {
		  //тут подгонка чтобы один сегмент оставался
		  uint8_t j;
		  for (j = 0; j < i; j++) {
		    level[j] = '>';
		  }
		  level[0] = '>';
		  level[j+1] = '\0';
		  i = 16; // остановить цикл
		}
	  }
      // высести полоску уровня
	  lcd_put_ln1(level);
	  // подготовить и вывести значение	  
	  sprintf(gn, "Gain %d dB", g_input_gain*2);
      lcd_put_ln2(gn);
	  // записать значение в регистр микросхемы
      TDA7439_set_input_gain(g_input_gain);
	  // записать значение в ПЗУ
	  eeprom_write_byte(NV_input_gain_addr, g_input_gain); 	  
		
	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set Gain\r\n");	  
	  done = 1;	
	  //dev_state = MAIN;
	   
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  lcd_put_ln1(main_menu[main_menu_pos].item);
	  //uart_puts("menu -> main\r\n");  
	  uart_puts("menu -> menu\r\n");  
	  break;
	}
  }
  return;
} 
//------------------------------------------------------------
void  action_setup_bass(void)
{
  uart_puts("Setup bass\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отбразить на дисп. и записать в рег. микросхемы
  proc_value_1("Bass", g_bass_gain);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  //uart_puts("Bass DOWN\r\n");

      if (g_bass_gain < 14) g_bass_gain++; 
	  //TODO: ограничитель тестить!
         
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Bass", g_bass_gain);
     
	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  //uart_puts("Bass UP\r\n");

      if (g_bass_gain > 0) g_bass_gain--; 
	  //TODO: ограничитель тестить!
      
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Bass", g_bass_gain); 

	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Bass Gain\r\n");
	  done = 1;
	  
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  lcd_put_ln1(main_menu[main_menu_pos].item); 
	  uart_puts("menu -> menu\r\n");
	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void  action_setup_middle(void)
{
  uart_puts("Setup middle\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отбразить на дисп. и записать в рег. микросхемы
  proc_value_1("Middle", g_mid_range_gain);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  //uart_puts("Middle DOWN\r\n");

      if (g_mid_range_gain < 14) g_mid_range_gain++; 
	  //TODO: ограничитель тестить!
      
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Middle", g_mid_range_gain);
      
	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  //uart_puts("Middle UP\r\n");

      if (g_mid_range_gain > 0) g_mid_range_gain--; 
	  //TODO: ограничитель тестить!
      
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Middle", g_mid_range_gain);  

	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set Middle\r\n");
	  done = 1;
	  
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  lcd_put_ln1(main_menu[main_menu_pos].item); 
	  uart_puts("menu -> menu\r\n");
	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void  action_setup_treble(void)
{
  uart_puts("Setup treble\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отбразить на дисп. и записать в рег. микросхемы
  proc_value_1("Treble", g_treble_gain);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout(); 
	  
      if (g_treble_gain < 14) g_treble_gain++; 
	  //TODO: ограничитель тестить!
      
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Treble", g_treble_gain);
	  
	  break;	
	case E_DOWN:
	  restart_menutimeout();
	  
	  if (g_treble_gain > 0) g_treble_gain--; 
	  //TODO: ограничитель тестить!
      
      // отбразить на дисп. и записать в рег. микросхемы
      proc_value_1("Treble", g_treble_gain);
  	  
	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set Treble\r\n");
	  done = 1;
	  
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  lcd_put_ln1(main_menu[main_menu_pos].item); 
	  uart_puts("menu -> menu\r\n");
	  break;
	}
  }
  return;
}

//------------------------------------------------------------
void  action_setup_60Hz(void)
{
  uart_puts("Setup 60Hz\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отсветка уроня установленной громкоси(полоска)
  // записать заначение в регистр микросехемы
  proc_value_2("60Hz", BAND_5_59_Hz, g_band_5_59_Hz);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("60Hz UP\r\n");
	  if (g_band_5_59_Hz < 14) g_band_5_59_Hz++;
	  
      // отсветка уроня установленной громкоси(полоска)
	  // записать заначение в регистр микросехемы
	  proc_value_2("60Hz", BAND_5_59_Hz, g_band_5_59_Hz);
  
	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  uart_puts("60Hz DOWN\r\n");
	  if (g_band_5_59_Hz > 0) g_band_5_59_Hz--;

	  // отсветка уроня установленной громкоси(полоска)
	  // записать заначение в регистр микросехемы
	  proc_value_2("60Hz", BAND_5_59_Hz, g_band_5_59_Hz);

	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set 60Hz\r\n");                        
	  done = 1;
	  dev_state = MAIN;
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);
	  uart_puts("menu -> main\r\n");
	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void  action_setup_260Hz(void)
{
  uart_puts("Setup 260Hz\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отсветка уроня установленной громкоси(полоска)
  // записать заначение в регистр микросехемы
  proc_value_2("260Hz", BAND_2_261_Hz, g_band_2_261_Hz);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("260Hz UP\r\n");
      if (g_band_2_261_Hz < 14) g_band_2_261_Hz++;	  

	  // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("260Hz", BAND_2_261_Hz, g_band_2_261_Hz);
	  
	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  
	  if (g_band_2_261_Hz > 0) g_band_2_261_Hz--;
	  uart_puts("260Hz DOWN\r\n");
	  
	  // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("260Hz", BAND_2_261_Hz, g_band_2_261_Hz);
	  
	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set 260Hz\r\n");
	  done = 1;

	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);
	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void  action_setup_1kHz(void)
{
  uart_puts("Setup 1kHz\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отсветка уроня установленной громкоси(полоска)
  // записать заначение в регистр микросехемы
  proc_value_2("1kHz", BAND_3_1036_Hz, g_band_3_1036_Hz);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("1kHz UP\r\n");
	  if (g_band_3_1036_Hz < 14) g_band_3_1036_Hz++;

      // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("1kHz", BAND_3_1036_Hz, g_band_3_1036_Hz);


	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  uart_puts("1kHz DOWN\r\n");
	  if (g_band_3_1036_Hz > 0) g_band_3_1036_Hz--;

	  // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("1kHz", BAND_3_1036_Hz, g_band_3_1036_Hz);

	  
	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set 1kHz\r\n");
	  done = 1;
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);
	  break;
	}
  }
  return;
}

//------------------------------------------------------------
void  action_setup_3kHz(void)
{
  uart_puts("Setup 3kHz\r\n");
  start_menutimeout();
  uint8_t done = 0;

  // отсветка уроня установленной громкоси(полоска)
  // записать заначение в регистр микросехемы
  proc_value_2("3kHz", BAND_4_3168_Hz, g_band_4_3168_Hz);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("3kHz UP\r\n");

      if (g_band_4_3168_Hz < 14) g_band_4_3168_Hz++;
      // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("3kHz", BAND_4_3168_Hz, g_band_4_3168_Hz);

	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  uart_puts("3kHz DOWN\r\n");
	  if (g_band_4_3168_Hz > 0) g_band_4_3168_Hz--;

      // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("3kHz", BAND_4_3168_Hz, g_band_4_3168_Hz);

	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set 3kHz\r\n");
	  done = 1;
	  // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);
	  break;
	}
  }
  return;
}

//------------------------------------------------------------
void  action_setup_10kHz(void)
{
  uart_puts("Setup 10kHz\r\n");
  start_menutimeout();
  uint8_t done = 0;


  // отсветка уроня установленной громкоси(полоска)
  // записать заначение в регистр микросехемы
  proc_value_2("10kHz", BAND_1_10363_Hz, g_band_1_10363_Hz);

  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("10kHz UP\r\n");
      if (g_band_1_10363_Hz < 14) g_band_1_10363_Hz++;

      // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("10kHz", BAND_1_10363_Hz, g_band_1_10363_Hz);

	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  uart_puts("10Hkz DOWN\r\n");
	  if (g_band_1_10363_Hz > 0) g_band_1_10363_Hz--;

      // отсветка уроня установленной громкоси(полоска)
      // записать заначение в регистр микросехемы
      proc_value_2("10kHz", BAND_1_10363_Hz, g_band_1_10363_Hz);


	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set 10kHz\r\n");
      done = 1;
      // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void  action_setup_equalizer(void)
{
  uart_puts("Setup equalizer\r\n");
  start_menutimeout();
  uint8_t done = 0;
  int8_t eq_menu_pos = 0;
  menu_t eq_menu[] = {
    {"60 Hz", action_setup_60Hz},
	{"260 Hz", action_setup_260Hz},
    {"1 kHz", action_setup_1kHz},
	{"3 kHz", action_setup_3kHz},
	{"10 kHz", action_setup_10kHz},
	{NULL, NULL},// end of menu list
  };
  

  
  uart_puts("Go Eq menu\r\n");
  
  // отобразить пункт подменю
  lcd_put_ln1(eq_menu[eq_menu_pos].item);
  
  // войти в подменю устаноки полос эквалайзера	  
  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
      restart_menutimeout(); 
      uart_puts("Eq next\r\n");

	  if (eq_menu_pos < menu_count(eq_menu)) eq_menu_pos++;
	  
      if (NULL == eq_menu[eq_menu_pos].item) {
	    eq_menu_pos = 0;
	  }

	  lcd_put_ln1(eq_menu[eq_menu_pos].item);
	  break;
	case E_DOWN:
      restart_menutimeout();         
	  uart_puts("Eq prev\r\n");

	  eq_menu_pos--;
      if (0 > eq_menu_pos) eq_menu_pos = menu_count(eq_menu) - 1;
	  
	  if (NULL == eq_menu[eq_menu_pos].item) {
	    eq_menu_pos = 0;
	  }
	  lcd_put_ln1(eq_menu[eq_menu_pos].item);			
	  break;
	case E_BUTTON:		   
	  restart_menutimeout();
	  uart_puts("Eq cur setup\r\n");
	  eq_menu[eq_menu_pos].action();
	  done = 1;
      break;      
    }// eof switch
  }// eof while

  // отобразить пунк главного меню Equalizer
  lcd_put_ln1(main_menu[main_menu_pos].item); 
	  
  return;
}
//------------------------------------------------------------
void  action_setup_balance(void)
{
  uart_puts("Setup balance\r\n");
  start_menutimeout();
  uint8_t done = 0;
  char level[16] = {0};
  char bl[16] = {0};
  level[7] = '|'; // центр  

  // отсветка уроня установленной громкоси(полоска)
  // отобразить уровень громкости на дисплее(полоской)
  //тут подгонка чтобы один сегмент оставался
  if (g_balance > 0) {
    switch(g_balance) {
    case 17:
      strcpy(level, "       |>");
	  lcd_put_ln1(level);
	  break;
    case 34:
      strcpy(level, "       |>>");
      lcd_put_ln1(level);    
      break;
    case 51:
      strcpy(level, "       |>>>");
      lcd_put_ln1(level);      
      break;
    case 68:
      strcpy(level, "       |>>>>");
      lcd_put_ln1(level); 
      break;
    case 85:
      strcpy(level, "       |>>>>>");
      lcd_put_ln1(level);     
      break;
    case 102:
      strcpy(level, "       |>>>>>>");
      lcd_put_ln1(level);      
      break;
    case 119:
      strcpy(level, "       |>>>>>>>");
      lcd_put_ln1(level);      
      break;
    } // eof switch
  } else if (g_balance < 0) {
    switch(g_balance) {
	case -119:
      strcpy(level, "<<<<<<<|");
	  lcd_put_ln1(level);      
      break;
    case -102:
      strcpy(level, " <<<<<<|");
	  lcd_put_ln1(level);
      break;
    case -85:
      strcpy(level, "  <<<<<|");
	  lcd_put_ln1(level);
	  break;
    case -68:
      strcpy(level, "   <<<<|");
	  lcd_put_ln1(level);
      break;
    case -51:
      strcpy(level, "    <<<|");
	  lcd_put_ln1(level);
      break;
    case -34:
      strcpy(level, "     <<|");
	  lcd_put_ln1(level);
      break;
    case -17:
      strcpy(level, "      <|");
	  lcd_put_ln1(level);
      break;
	} // eof switch
  } else {
    strcpy(level, "       |");
	lcd_put_ln1(level);
  } 
  

  sprintf(bl, "Balance %d", g_balance);
  lcd_put_ln2(bl);



  while (!menu_timeout && !done) {
    switch (get_key()) {
	case E_UP:
	  restart_menutimeout();
	  uart_puts("Bal UP\r\n");
      if (g_balance < 120) g_balance++;
	     		  
      // TODO: отобразить уровень громкости на дисплее(полоской)
	  //тут подгонка чтобы один сегмент оставался
	  if (g_balance > 0) {
        switch(g_balance) {
        case 17:
          strcpy(level, "       |>");
	      lcd_put_ln1(level);
	      break;
        case 34:
          strcpy(level, "       |>>");
          lcd_put_ln1(level);    
          break;
        case 51:
          strcpy(level, "       |>>>");
          lcd_put_ln1(level);      
          break;
        case 68:
          strcpy(level, "       |>>>>");
          lcd_put_ln1(level); 
          break;
        case 85:
          strcpy(level, "       |>>>>>");
          lcd_put_ln1(level);     
          break;
        case 102:
          strcpy(level, "       |>>>>>>");
          lcd_put_ln1(level);      
          break;
        case 119:
          strcpy(level, "       |>>>>>>>");
          lcd_put_ln1(level);      
          break;
        } // eof switch
			
		// TODO: записать значение в регистр микросхемы
		TDA7439_set_speaker_att_R(g_balance);
			
      } else if (g_balance < 0) {
        
	   switch(g_balance) {
	   case -119:
         strcpy(level, "<<<<<<<|");
	     lcd_put_ln1(level);      
         break;
       case -102:
         strcpy(level, " <<<<<<|");
	     lcd_put_ln1(level);
         break;
       case -85:
         strcpy(level, "  <<<<<|");
	     lcd_put_ln1(level);
         break;
	   case -68:
         strcpy(level, "   <<<<|");
	     lcd_put_ln1(level);
         break;
       case -51:
         strcpy(level, "    <<<|");
	     lcd_put_ln1(level);
         break;
       case -34:
         strcpy(level, "     <<|");
	     lcd_put_ln1(level);
         break;
       case -17:
         strcpy(level, "      <|");
	     lcd_put_ln1(level);
         break;
	   } // eof switch
	   
	   // записать значение в регистр микросхемы
	   TDA7439_set_speaker_att_L(g_balance*-1);
			
      } else {        
		strcpy(level, "       |");
	    lcd_put_ln1(level);
        // записать значение в регистр микросхемы
		TDA7439_set_speaker_att_L(0);
		TDA7439_set_speaker_att_R(0);	
      }
	  
      sprintf(bl, "Balance %d", g_balance);
      lcd_put_ln2(bl);
	  
	  // TODO: записать значение в ПЗУ 

	  break;
    case E_DOWN:
	  restart_menutimeout(); 
	  uart_puts("Bal DOWN\r\n");
	  if (g_balance > -120) g_balance--;
      
	  
	  // TODO: отобразить уровень громкости на дисплее(полоской)
	  //тут подгонка чтобы один сегмент оставался
	  if (g_balance > 0) {
        switch(g_balance) {
        case 17:
          strcpy(level, "       |>");
	      lcd_put_ln1(level);
	      break;
        case 34:
          strcpy(level, "       |>>");
          lcd_put_ln1(level);    
          break;
        case 51:
          strcpy(level, "       |>>>");
          lcd_put_ln1(level);      
          break;
        case 68:
          strcpy(level, "       |>>>>");
          lcd_put_ln1(level); 
          break;
        case 85:
          strcpy(level, "       |>>>>>");
          lcd_put_ln1(level);     
          break;
        case 102:
          strcpy(level, "       |>>>>>>");
          lcd_put_ln1(level);      
          break;
        case 119:
          strcpy(level, "       |>>>>>>>");
          lcd_put_ln1(level);      
          break;
        } // eof switch	
		
		// записать значение в регистр микросхемы
	    TDA7439_set_speaker_att_R(g_balance);
		       
      } else if (g_balance < 0) {
        
	    switch(g_balance) {
	    case -119:
          strcpy(level, "<<<<<<<|");
	      lcd_put_ln1(level);      
          break;
        case -102:
          strcpy(level, " <<<<<<|");
	      lcd_put_ln1(level);
          break;
        case -85:
          strcpy(level, "  <<<<<|");
	      lcd_put_ln1(level);
		  break;
        case -68:
          strcpy(level, "   <<<<|");
	      lcd_put_ln1(level);
          break;
        case -51:
          strcpy(level, "    <<<|");
	      lcd_put_ln1(level);
          break;
        case -34:
          strcpy(level, "     <<|");
	      lcd_put_ln1(level);
          break;
        case -17:
          strcpy(level, "      <|");
	      lcd_put_ln1(level);
          break;
	    } // eof switch	
		
		// записать значение в регистр микросхемы
	    TDA7439_set_speaker_att_L(g_balance*-1);  
      } else {      
        strcpy(level, "       |");
	    lcd_put_ln1(level);
		// записать значение в регистр микросхемы
		TDA7439_set_speaker_att_L(0);
		TDA7439_set_speaker_att_R(0);
      }
      
	  sprintf(bl, "Balance %d", g_balance);
      lcd_put_ln2(bl);
   
	  break;
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Set Bal\r\n");
      done = 1;
      // очистить дисплей
	  lcd_clear();
	  lcd_put_ln1("");
      lcd_put_ln2("");
	  // подождать немного
	  _delay_ms(50);

	  lcd_put_ln1(main_menu[main_menu_pos].item); 
	  uart_puts("menu -> menu\r\n");

	  break;
	}
  }    
  return;
}



//------------------------------------------------------------
void  action_reset_all(void)
{
  uart_puts("Reset all\r\n");
  start_menutimeout();
  uint8_t done = 0;
  while (!menu_timeout && !done) {
    switch (get_key()) {
    case E_BUTTON:
	  stop_menutimeout();
	  uart_puts("Rest All and go main\r\n");
	  done = 1;
	  break;
	}
  }
  return;
}
//------------------------------------------------------------
void setup_amp(void)
{
  uint8_t reg_val = 0; 
  
  // тут костыли чтобы при самом первом старте не выходило за пределы размного
  g_input_gain = eeprom_read_byte(NV_input_gain_addr);
  if (g_input_gain > 50) g_input_gain = 0;
  TDA7439_set_input_gain(g_input_gain); 



  g_bass_gain = eeprom_read_byte(NV_bass_gain_addr);
  if (g_bass_gain > 20) g_bass_gain = 0;
  switch(g_bass_gain) {
  case 0:
	  reg_val = g_bass_gain;        
      break;
  case 1:
	  reg_val = g_bass_gain;
      break;
  case 2:
	  reg_val = g_bass_gain;
      break;
  case 3:
	  reg_val = g_bass_gain;
      break;
  case 4:
	  reg_val = g_bass_gain;
      break;
  case 5:
	  reg_val = g_bass_gain;
      break;
  case 6:
	  reg_val = g_bass_gain;
      break;
  case 7:
	  reg_val = g_bass_gain;
      break;
  case 8:
      reg_val = 14;
	  break;
  case 9:
	  reg_val = 13;       
      break;
  case 10:
	  reg_val = 12;       
      break;
  case 11:
	  reg_val = 11;  
      break;
  case 12:
	  reg_val = 10;      
      break;
  case 13:
	  reg_val = 9;       
      break;
  case 14:
	  reg_val = 8;       
      break;
  } // eof switch
  TDA7439_set_bass_gain(reg_val); 

  g_mid_range_gain = eeprom_read_byte(NV_mid_range_gain_addr);
  if (g_mid_range_gain > 20) g_mid_range_gain = 0;
  reg_val = 0;
  switch(g_mid_range_gain) {
  case 0:
	  reg_val = g_mid_range_gain;        
      break;
  case 1:
	  reg_val = g_mid_range_gain;
      break;
  case 2:
	  reg_val = g_mid_range_gain;
      break;
  case 3:
	  reg_val = g_mid_range_gain;
      break;
  case 4:
	  reg_val = g_mid_range_gain;
      break;
  case 5:
	  reg_val = g_mid_range_gain;
      break;
  case 6:
	  reg_val = g_mid_range_gain;
      break;
  case 7:
	  reg_val = g_mid_range_gain;
      break;
  case 8:
      reg_val = 14;
	  break;
  case 9:
	  reg_val = 13;       
      break;
  case 10:
	  reg_val = 12;       
      break;
  case 11:
	  reg_val = 11;  
      break;
  case 12:
	  reg_val = 10;      
      break;
  case 13:
	  reg_val = 9;       
      break;
  case 14:
	  reg_val = 8;       
      break;
  } // eof switch
  TDA7439_set_midrange_gain(reg_val);

  g_treble_gain = eeprom_read_byte(NV_treble_gain_addr);
  if (g_treble_gain > 20) g_treble_gain = 0;
  reg_val = 0;
  switch(g_treble_gain) {
  case 0:
	  reg_val = g_treble_gain;        
      break;
  case 1:
	  reg_val = g_treble_gain;
      break;
  case 2:
	  reg_val = g_treble_gain;
      break;
  case 3:
	  reg_val = g_treble_gain;
      break;
  case 4:
	  reg_val = g_treble_gain;
      break;
  case 5:
	  reg_val = g_treble_gain;
      break;
  case 6:
	  reg_val = g_treble_gain;
      break;
  case 7:
	  reg_val = g_treble_gain;
      break;
  case 8:
      reg_val = 14;
	  break;
  case 9:
	  reg_val = 13;       
      break;
  case 10:
	  reg_val = 12;       
      break;
  case 11:
	  reg_val = 11;  
      break;
  case 12:
	  reg_val = 10;      
      break;
  case 13:
	  reg_val = 9;       
      break;
  case 14:
	  reg_val = 8;       
      break;
  } // eof switch
  TDA7439_set_treble_gain(reg_val);


  //g_balance = eeprom_read_byte(NV_balance_addr);
  //if (g_balance > 240) g_balance = 0;


  g_band_1_10363_Hz = eeprom_read_byte(NV_band_1_10363_Hz_addr);
  if (g_band_1_10363_Hz > 20) g_band_1_10363_Hz = 0;
  //TDA7317_set_band(BAND_1_10363_Hz, g_band_1_10363_Hz); 
   switch(g_band_1_10363_Hz) {
  case 0: // -14 dB
      TDA7317_set_band(BAND_1_10363_Hz, 15);      	  
	  break;
  case 1:
	  TDA7317_set_band(BAND_1_10363_Hz, 14); 	   
      break;
  case 2:
	  TDA7317_set_band(BAND_1_10363_Hz, 13);  
	  break;
  case 3:
	  TDA7317_set_band(BAND_1_10363_Hz, 12); 
	  break;
  case 4:
	  TDA7317_set_band(BAND_1_10363_Hz, 11);  
      break;
  case 5:
	  TDA7317_set_band(BAND_1_10363_Hz, 10);  
      break;
  case 6:
	  TDA7317_set_band(BAND_1_10363_Hz, 9);
      break;
  case 7:
	  TDA7317_set_band(BAND_1_10363_Hz, 0);  
      break;
  case 8:
	  TDA7317_set_band(BAND_1_10363_Hz, 1);
	  break;
  case 9:
	  TDA7317_set_band(BAND_1_10363_Hz, 2);
	  break;
  case 10:
	  TDA7317_set_band(BAND_1_10363_Hz, 3);  
	 break;
  case 11:
	  TDA7317_set_band(BAND_1_10363_Hz, 4);
      break;
  case 12:
	  TDA7317_set_band(BAND_1_10363_Hz, 5);
	  break;
  case 13:
	  TDA7317_set_band(BAND_1_10363_Hz, 6); 
	  break;
  case 14:
	  TDA7317_set_band(BAND_1_10363_Hz, 7);     
      break;
  } // eof switch
  
  g_band_2_261_Hz = eeprom_read_byte(NV_band_2_261_Hz_addr);
  if (g_band_2_261_Hz > 20) g_band_2_261_Hz = 0;
  //TDA7317_set_band(BAND_2_261_Hz, g_band_2_261_Hz);
   switch(g_band_2_261_Hz) {
  case 0: // -14 dB
      TDA7317_set_band(BAND_2_261_Hz, 15);      	  
	  break;
  case 1:
	  TDA7317_set_band(BAND_2_261_Hz, 14); 	   
      break;
  case 2:
	  TDA7317_set_band(BAND_2_261_Hz, 13);  
	  break;
  case 3:
	  TDA7317_set_band(BAND_2_261_Hz, 12); 
	  break;
  case 4:
	  TDA7317_set_band(BAND_2_261_Hz, 11);  
      break;
  case 5:
	  TDA7317_set_band(BAND_2_261_Hz, 10);  
      break;
  case 6:
	  TDA7317_set_band(BAND_2_261_Hz, 9);
      break;
  case 7:
	  TDA7317_set_band(BAND_2_261_Hz, 0);  
      break;
  case 8:
	  TDA7317_set_band(BAND_2_261_Hz, 1);
	  break;
  case 9:
	  TDA7317_set_band(BAND_2_261_Hz, 2);
	  break;
  case 10:
	  TDA7317_set_band(BAND_2_261_Hz, 3);  
	 break;
  case 11:
	  TDA7317_set_band(BAND_2_261_Hz, 4);
      break;
  case 12:
	  TDA7317_set_band(BAND_2_261_Hz, 5);
	  break;
  case 13:
	  TDA7317_set_band(BAND_2_261_Hz, 6); 
	  break;
  case 14:
	  TDA7317_set_band(BAND_2_261_Hz, 7);     
      break;
  } // eof switch

  g_band_3_1036_Hz = eeprom_read_byte(NV_band_3_1036_Hz_addr);
  if (g_band_3_1036_Hz > 20) g_band_3_1036_Hz = 0;
  //TDA7317_set_band(BAND_3_1036_Hz, g_band_3_1036_Hz);
   switch(g_band_3_1036_Hz) {
  case 0: // -14 dB
      TDA7317_set_band(BAND_3_1036_Hz, 15);      	  
	  break;
  case 1:
	  TDA7317_set_band(BAND_3_1036_Hz, 14); 	   
      break;
  case 2:
	  TDA7317_set_band(BAND_3_1036_Hz, 13);  
	  break;
  case 3:
	  TDA7317_set_band(BAND_3_1036_Hz, 12); 
	  break;
  case 4:
	  TDA7317_set_band(BAND_3_1036_Hz, 11);  
      break;
  case 5:
	  TDA7317_set_band(BAND_3_1036_Hz, 10);  
      break;
  case 6:
	  TDA7317_set_band(BAND_3_1036_Hz, 9);
      break;
  case 7:
	  TDA7317_set_band(BAND_3_1036_Hz, 0);  
      break;
  case 8:
	  TDA7317_set_band(BAND_3_1036_Hz, 1);
	  break;
  case 9:
	  TDA7317_set_band(BAND_3_1036_Hz, 2);
	  break;
  case 10:
	  TDA7317_set_band(BAND_3_1036_Hz, 3);  
	 break;
  case 11:
	  TDA7317_set_band(BAND_3_1036_Hz, 4);
      break;
  case 12:
	  TDA7317_set_band(BAND_3_1036_Hz, 5);
	  break;
  case 13:
	  TDA7317_set_band(BAND_3_1036_Hz, 6); 
	  break;
  case 14:
	  TDA7317_set_band(BAND_3_1036_Hz, 7);     
      break;
  } // eof switch

  g_band_4_3168_Hz = eeprom_read_byte(NV_band_4_3168_Hz_addr);
  if (g_band_4_3168_Hz > 20) g_band_4_3168_Hz = 0;
  //TDA7317_set_band(BAND_4_3168_Hz, g_band_4_3168_Hz);
   switch(g_band_4_3168_Hz) {
  case 0: // -14 dB
      TDA7317_set_band(BAND_4_3168_Hz, 15);      	  
	  break;
  case 1:
	  TDA7317_set_band(BAND_4_3168_Hz, 14); 	   
      break;
  case 2:
	  TDA7317_set_band(BAND_4_3168_Hz, 13);  
	  break;
  case 3:
	  TDA7317_set_band(BAND_4_3168_Hz, 12); 
	  break;
  case 4:
	  TDA7317_set_band(BAND_4_3168_Hz, 11);  
      break;
  case 5:
	  TDA7317_set_band(BAND_4_3168_Hz, 10);  
      break;
  case 6:
	  TDA7317_set_band(BAND_4_3168_Hz, 9);
      break;
  case 7:
	  TDA7317_set_band(BAND_4_3168_Hz, 0);  
      break;
  case 8:
	  TDA7317_set_band(BAND_4_3168_Hz, 1);
	  break;
  case 9:
	  TDA7317_set_band(BAND_4_3168_Hz, 2);
	  break;
  case 10:
	  TDA7317_set_band(BAND_4_3168_Hz, 3);  
	 break;
  case 11:
	  TDA7317_set_band(BAND_4_3168_Hz, 4);
      break;
  case 12:
	  TDA7317_set_band(BAND_4_3168_Hz, 5);
	  break;
  case 13:
	  TDA7317_set_band(BAND_4_3168_Hz, 6); 
	  break;
  case 14:
	  TDA7317_set_band(BAND_4_3168_Hz, 7);     
      break;
  } // eof switch

  g_band_5_59_Hz = eeprom_read_byte(NV_band_5_59_Hz_addr);
  if (g_band_5_59_Hz > 20) g_band_5_59_Hz = 0;
  //TDA7317_set_band(BAND_5_59_Hz, g_band_5_59_Hz);
  switch(g_band_5_59_Hz) {
  case 0: // -14 dB
      TDA7317_set_band(BAND_5_59_Hz, 15);      	  
	  break;
  case 1:
	  TDA7317_set_band(BAND_5_59_Hz, 14); 	   
      break;
  case 2:
	  TDA7317_set_band(BAND_5_59_Hz, 13);  
	  break;
  case 3:
	  TDA7317_set_band(BAND_5_59_Hz, 12); 
	  break;
  case 4:
	  TDA7317_set_band(BAND_5_59_Hz, 11);  
      break;
  case 5:
	  TDA7317_set_band(BAND_5_59_Hz, 10);  
      break;
  case 6:
	  TDA7317_set_band(BAND_5_59_Hz, 9);
      break;
  case 7:
	  TDA7317_set_band(BAND_5_59_Hz, 0);  
      break;
  case 8:
	  TDA7317_set_band(BAND_5_59_Hz, 1);
	  break;
  case 9:
	  TDA7317_set_band(BAND_5_59_Hz, 2);
	  break;
  case 10:
	  TDA7317_set_band(BAND_5_59_Hz, 3);  
	 break;
  case 11:
	  TDA7317_set_band(BAND_5_59_Hz, 4);
      break;
  case 12:
	  TDA7317_set_band(BAND_5_59_Hz, 5);
	  break;
  case 13:
	  TDA7317_set_band(BAND_5_59_Hz, 6); 
	  break;
  case 14:
	  TDA7317_set_band(BAND_5_59_Hz, 7);     
      break;
  } // eof switch


  return;
}


//------------------------------------------------------------
void startup_init(void)
{
  _delay_ms(3000); // Задержка на пуск усилителя
  // убрать попробовать....
  //DDRB |= (1 << PB0);
  
  //PD6 Светодиод 3

  // тут будет ШИМ для яркости дисплея...
  //DDRD |= (1 << PD6);
  //PORTD |= (1 << PD6); // подсветка дисплея

  sei();
  
  // плавно зажечь дисплей
  //soft_pwm_on();
  
  uart_init(UART_BAUD_SELECT(19200, F_CPU));

  TDA7317_init();
  TDA7439_init();
  
  init_keys();

  lcd_init();

  init_menutimeout();

  // проинитить все переменные значение при старте, вычитывать их из ПЗУ
  // и записать в регистры микросхем
  setup_amp();

  return;
}


#define MUTED 1
#define UNMUTED 0

//------------------------------------------------------------
int main(void) 
{
  startup_init(); 
  uart_puts("R started\r\n");
  lcd_make_curs_invis(); 
  uint8_t volume = 30; // on power on default volume
  uint8_t prev_volume = 0; // previous voulume value
  char vol[16] = {0};
  //__DEBUG
  uart_puts("* -> main\r\n");

  uint8_t mute = 0;

  // main FSM loop  
  while (INFINITY) {
  	
    switch (dev_state) {
	case VOLUME: //----------------------------------------------

	  if (!menu_timeout) {
		switch (get_key()) {
	    case E_DOWN: { // скобачки это наёбка, чтобы так как в свиче нельзя объявлять переменные
          if(volume < 47) volume++;
		  //TODO: ограничитель тестить!
          // отсветка уроня установленной громкоси(полоска)
          char volume_level[16] = {0};

		  restart_menutimeout();
		  
		  // отобразить уровень громкости на дисплее(полоской)
		  //тут подгонка чтобы один сегмент оставался
		  for (uint8_t i = 0; i < (47 / 3) + 1; i++) {		   	    
			  
			if (i == (volume/3)) {
			  
			  uart_put_int(i);
			  uart_puts("\r\n");
			  
			  uint8_t j;
			  //тут подгонка чтобы один сегмент оставался
			  for (j = 0; j < (16 - i); j++) {
			    volume_level[j] = '>';
			  }
			  volume_level[j+1] = '\0';
			  i = (47 / 3) + 1; // остановить цикл
			}
		  }

		  uart_puts(volume_level);
		  uart_puts("/\r\n");

		  // вывести полоску уровня
		  lcd_put_ln1(volume_level);
		  // подготовить и вывести значение
		  if (0 == volume) {
		    sprintf(vol, "Volume %d dB", volume);
		  } else { 
		    sprintf(vol, "Volume -%d dB", volume);
		  }
		  lcd_put_ln2(vol);
		  // записать значение в регистр микросхемы
          TDA7439_set_volume(volume);
		  }
		  break; 
        case E_UP:	{ // скобачки это наёбка, чтобы так как в свиче нельзя объявлять переменные	  
          if (volume > 0) volume--; 
		  //TODO: ограничитель тестить!
		  // отсветка уроня установленной громкоси(полоска)
          char volume_level[16] = {0};

		  restart_menutimeout();
		  
		  // отобразить уровень громкости на дисплее(полоской)
		  for (uint8_t i = 0; i < (47 / 3); i++) {
		    if (i == (volume/3)) {

			  uint8_t j;
			  for (j = 0; j < (16 - i); j++) {
			    volume_level[j] = '>';
			  }
			  volume_level[j+1] = '\0';
			  i = (47 / 3); // остановить цикл
			}
		  }
		  
          uart_puts(volume_level);
		  uart_puts("\\\r\n");

		  // высести полоску уровня
		  lcd_put_ln1(volume_level);
		  // подготовить значение и вывести
		  if (0 == volume) {
		    sprintf(vol, "Volume %d dB", volume);
		  } else { 
		    sprintf(vol, "Volume -%d dB", volume);
		  }
		  lcd_put_ln2(vol);
		  // записать значение в регистр микросхемы
          TDA7439_set_volume(volume);
		  } 	  
		  break;    
	    } // eof switch
	  } else {
	    // если уже установлено MUTE то пройти через состояние MUTE
        if (mute) {
          dev_state = MUTE;
		  mute = UNMUTED;
		  start_menutimeout();
		  uart_puts("volume -> mute\r\n");
        } else {
          dev_state = MAIN;
		  	uart_puts("volume -> main\r\n");
		} 
		// очистить дисплей
		lcd_clear();
		lcd_put_ln1("");
        lcd_put_ln2("");
		// подождать немного
		_delay_ms(50);
		soft_pwm_off(); 
	
	  }

	  break;
    case MUTE:
	  if (!menu_timeout) {
	    switch (mute) {
		case MUTED:
          // включить светодиод MUTE
          DDRB |= (1 << PB0);
		  PORTB |= (1 << PB0); 
		  uart_puts("MUTE VD ON\r\n"); 
		  // запоминть значение громкости
		  prev_volume = volume;
		  // записать в регистр микросхемы
          TDA7439_set_volume(47);
		  // установить значение Volume в -47 db
		  volume = 47;
		  dev_state = MAIN;		    
		  break;
		default:
		  // выключить светодиод MUTE
		  DDRB &= ~(1 << PB0);
		  PORTB &= ~(1 << PB0);

		  uart_puts("MUTE VD OFF\r\n");
          // плавоно повысить громмкость до предыдущего значения
          for (uint8_t i = 47; i > prev_volume; i--) {
		    _delay_ms(50); // чтобы плавно было :)
			// установить значение Volume в -47 db
		    volume = i;		 
            // записать в регистр микросхемы
			TDA7439_set_volume(volume);
          }
		  // установить сохранённую до MUTE громкость
          volume = prev_volume;
		  dev_state = MAIN;        
		  break;		
		} // eof switch
	  } else {
	    dev_state = MAIN;
	    // очистить дисплей
		lcd_clear();
		lcd_put_ln1("");
        lcd_put_ln2("");
		// подождать немного
		_delay_ms(50);
		uart_puts("mute -> main\r\n");
	  }   
	  break;
    
	case PRESETS: //----------------------------------------------
       if (!menu_timeout) {
	    //работа меню
	    switch (get_key()) {
	    case E_UP: 
          restart_menutimeout();
		  uart_puts("next preset\r\n");
	      break;
	    case E_DOWN:
	      restart_menutimeout();
		  uart_puts("prev preset\r\n");			
	      break;
	    case E_BUTTON:
	      stop_menutimeout();
	      uart_puts("select preset\r\n");
          dev_state = MAIN;
		  uart_puts("presets -> main\r\n");
	      break;
	    }
	  } else {
	    dev_state = MAIN;
	    // очистить дисплей
		lcd_clear();
		lcd_put_ln1("");
        lcd_put_ln2("");
		// подождать немного
		_delay_ms(50);
		soft_pwm_off();
		uart_puts("presets -> main\r\n");
	  }     
	  break;
	
	case MENU: //----------------------------------------------
	  if (!menu_timeout) {
	    //работа меню
	    switch (get_key()) {
	    case E_UP: 
          restart_menutimeout();
		  if (main_menu_pos < menu_count(main_menu)) main_menu_pos++;

          if (NULL == main_menu[main_menu_pos].item) {
	        main_menu_pos = 0;
	      }
	      lcd_put_ln1(main_menu[main_menu_pos].item);
	      break;
	    case E_DOWN:
	      restart_menutimeout();
		  
		  main_menu_pos--;
		  if (0 > main_menu_pos) main_menu_pos = menu_count(main_menu) - 1;
	      
		  if (NULL == main_menu[main_menu_pos].item) {
	        main_menu_pos = 0;
	      }
	      lcd_put_ln1(main_menu[main_menu_pos].item);			
	      break;
	    case E_BUTTON:
	      stop_menutimeout();
	      main_menu[main_menu_pos].action();
	      break;
	    }
	  } else {
	    dev_state = MAIN;
		// очистить дисплей
		lcd_clear();
		lcd_put_ln1("");
        lcd_put_ln2("");
	    // подождать немного
		_delay_ms(50);
		soft_pwm_off();
		uart_puts("menu -> main\r\n");
	  }      
	  break;	
	default: // MAIN  //----------------------------------------------   
	  switch(get_key()) {
	  case E_UP:
	  case E_DOWN:
	    uart_puts("main -> volume\r\n");

		soft_pwm_on();

		dev_state = VOLUME;
		
        // отсветка уроня установленной громкоси(полоска)
        char volume_level[16] = {0};		

		// отобразить уровень громкости на дисплее(полоской)
		for (uint8_t i = 0; i < (47 / 3); i++) {
		  if (i == (volume/3)) {
			// если значение меньше половины хода
			if (i < (47 / 3) ) {
			  uint8_t j;
			  for (j = 0; j < (16 - i); j++) {
			    volume_level[j] = '>';
			  }
			  volume_level[j+1] = '\0';
			} else {
			  //тут подгонка чтобы один сегмент оставался
			  uint8_t j;
			  for (j = 0; j < (15 - i); j++) {
			    volume_level[j] = '>';
			  }
			  volume_level[j+1] = '\0';
			}			
			i = (47 / 3); // остановить цикл
	      }
		}
		// высести полоску уровня
		lcd_put_ln1(volume_level);
		// подготовить значение и вывести
		if (0 == volume) {
		  sprintf(vol, "Volume %d dB", volume);
		} else { 
		  sprintf(vol, "Volume -%d dB", volume);
		}
		lcd_put_ln2(vol);
		// дайт время на установку значения
		start_menutimeout();
		break;
	  case E_BUTTON:
	    uart_puts("main -> menu\r\n");

        soft_pwm_on();

	    dev_state = MENU;
		lcd_put_ln1(main_menu[main_menu_pos].item);
		start_menutimeout();
	    break;
	  case MUTING_BUTTON: // PRESETS реализовано на кнопке MUTING не реализовано!
	    _delay_ms(500);
		if (mute) {		  
		  mute = UNMUTED;
          uart_puts("main -> UNMUTED\r\n");
		  dev_state = MUTE;
		} else {
		  mute = MUTED;
		  uart_puts("main -> MUTED\r\n");
		  dev_state = MUTE;
		}
		start_menutimeout();
		break; 
      case PHONO_BUTTON:
	    uart_puts("PHONO\r\n");
        TDA7439_set_input_selector(TDA7439_IN2); 
		break;
      case TUNER_BUTTON:
	    uart_puts("TUNER\r\n");
		TDA7439_set_input_selector(TDA7439_IN4);
		break;
      case TAPE_1_BUTTON:
	    uart_puts("TAPE 1\r\n");
		TDA7439_set_input_selector(TDA7439_IN1);
		break;
      case TAPE_2_BUTTON:
	    uart_puts("TAPE 2\r\n");
		TDA7439_set_input_selector(TDA7439_IN3);
		break;
      case PROTECT_SIG:
	    uart_puts("PROTECT!\r\n");
		soft_pwm_on();
		// включить диод ЗАЩИТА
		DDRD |= (1 << PD7);
        PORTD |= (1 << PD7); 
		// закатить на дисплей PROTECT!
		lcd_put_ln1("    PROTECT!");
		lcd_put_ln2("  Outputs OFF");
		// записать в TDA7439 MUTE
        TDA7439_set_volume(0xFF);
		// установить переменую громкость = 47
		volume = 47;
		// ждать пока состание не измениться 
		while (get_key() == PROTECT_SIG) continue;
		dev_state = MAIN;
	    // выключить диод ЗАЩИТА
		DDRD &= ~(1 << PD7);
        PORTD &= ~(1 << PD7); 
		uart_puts("protect -> main\r\n");
			// очистить дисплей
		lcd_clear();
		lcd_put_ln1("");
        lcd_put_ln2("");
	    // подождать немного
		_delay_ms(50);
		soft_pwm_off();
	    break;
	  } // eof switch 
	  break;
	} // eof main logic switch

  } // eof main driver
  
  return 0;
}
