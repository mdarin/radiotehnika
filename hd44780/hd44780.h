/* * * * *
 * Library for Hitachi HD44780 chip or a different one compatible with the HD44780 
 * via I2C PCF8574 expander.
 * 
 * Version: 0.1
 * Date: Сб. янв. 25 18:15:45 VOLT 2014
 *
 * Copyright (C) 2014  Michael DARIN
 * 
 * This program is distributed under the of the GNU Lesser Public License. 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Change log:
 * 	0.1 lib created
 *
 * * */
#ifndef HD44780_H
#define HD44780_H

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "spilight/spilight.h"


// LCD ###############
//P0-P7 Pins PCF8574
//Display I/O pins:
#define LCD_D0 3 //
#define LCD_D1 4 //
#define LCD_D2 5 //
#define LCD_D3 6 //

#define LCD_D4 LCD_D0 //
#define LCD_D5 LCD_D1 //
#define LCD_D6 LCD_D2 // 
#define LCD_D7 LCD_D3 // 

#define LCD_RS 1 // 
//#define LCD_RW  
#define LCD_E  2 // 
//#define LCD_LED 3
#define LCD_CS PB2//strobe


// LCD display buffer ####
#define LCD_LINE1 0
#define LCD_LINE2 1
#define LCD_LINES 2
#define LCD_LINE_LEN 16

// LCD commands macroses ####
// Commands
// The commands for HD44780 chip are:

//    Function set (8-bit interface, 2 lines, 5*7 Pixels): 0x38
#define lcd_set_8_bit_2_lines_5x7_pxs() lcd_send_cmd(0x38)
//    Function set (8-bit interface, 1 line, 5*7 Pixels): 0x30
#define lcd_set_8_bit_1_lines_5x7_pxs() lcd_send_cmd(0x30)
//    Function set (4-bit interface, 2 lines, 5*7 Pixels): 0x28
#define lcd_set_4_bit_2_lines_5x7_pxs() lcd_send_cmd(0x28)
//    Function set (4-bit interface, 1 line, 5*7 Pixels): 0x20
#define lcd_set_4_bit_1_lines_5x7_pxs() lcd_send_cmd(0x20)
//    Scroll display one character right (all lines): 0x1E
#define lcd_scroll_one_r() lcd_send_cmd(0x1E)
//    Scroll display one character left (all lines): 0x18
#define lcd_scroll_one_l() lcd_send_cmd(0x18)
//    Home (move cursor to top/left character position):0x02
#define lcd_home() lcd_send_cmd(0x02)
//    Move cursor one character left: 0x10
#define lcd_move_one_l() lcd_send_cmd(0x10)
//    Move cursor one character right: 0x14
#define lcd_move_one_r() lcd_send_cmd(0x14)
//    Turn on visible underline cursor: 0x0E
#define lcd_turnon_ucurs() lcd_send_cmd(0x0E)
//    Turn on visible blinking-block cursor: 0x0F
#define lcd_turnon_bbcurs() lcd_send_cmd(0x0F)
//    Make cursor invisible: 0x0C
#define lcd_make_curs_invis() lcd_send_cmd(0x0C)
//    Blank the display (without clearing): 0x08
#define lcd_blink_disp() lcd_send_cmd(0x08)
//    Restore the display (with cursor hidden): 0x0C
#define lcd_restore_disp() lcd_send_cmd(0x0C)
//    Clear Screen: 0x01
#define lcd_clear() lcd_send_cmd(0x01)
//    Set cursor position (DDRAM address): 0x80+ addr
#define lcd_set_curs_pos(address) lcd_send_cmd(0x80+address)
//    Set pointer in character-generator RAM (CG RAM address): 0x40+ addr 
#define lcd_set_cgram_ptr(address) lcd_send_cmd(0x40+address)

//    Entry mode set: 0x04, 0x05, 0x06, 0x07
//    i.e. : %000001IS
//    where
//    I : 0 = Dec Cursor    1 = Inc Cursor
//    S : 0 = Cursor Move   1 = Display Shift

// To send a command: set R/W pin to 0 (write), set RS pin to 0 (command selected), put the command to data bus D0-D7.
// Set E pin to 1 then to 0 (remember: data is transferred only on the high to low transition of this signal).
// To send data: set R/W pin to 0 (write), set RS pin to 1 (data selected), put the data to bus D0-D7, rise E and then back to 0.


void lcd_init(void);
void lcd_send(uint8_t lcd_reg);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_puts(const char *line);
void lcd_put_ln1(char *line);
void lcd_put_ln2(char *line);
void lcd_draw(void);

#endif // HD44780_H

