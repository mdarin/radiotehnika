/* * * * *
 * Library for Hitachi HD44780 chip or a different one compatible with the HD44780 
 * via I2C PCF8574 expander.
 * 
 * Version: 0.1
 * Date: РЎР±. СЏРЅРІ. 25 18:15:45 VOLT 2014
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
#include "hd44780.h"

static char display[LCD_LINES][LCD_LINE_LEN] = {0}; // display buffer

void lcd_init(void)
{
  //===========================
  // lcd initialization start  
  
  init_spi_master(); 

  //Power ON

  //Wait for more than 30 ms 
  //after Vdd rises to 4.5 v 
  _delay_ms(50);

  uint8_t lcd_reg = 0; // обнулить регистр
  
  // warn reset
  //RS R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0  
  // 0  0   0   0   1   1   X   X   X   X
  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D5) | (1 << LCD_D4);
  lcd_reg |= (1 << LCD_E);
  lcd_send(lcd_reg);

  lcd_reg &= ~(1 << LCD_E);  
  lcd_send(lcd_reg);

  _delay_ms(5); // больше 4.1 мс

  lcd_reg = 0;
 
  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D5) | (1 << LCD_D4);
  lcd_reg |= (1 << LCD_E);
  lcd_send(lcd_reg);

  lcd_reg &= ~(1 << LCD_E); // reset E strobe
  lcd_send(lcd_reg);

  _delay_us(200); // больше 100 мкс

  lcd_reg = 0;
  
  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D5) | (1 << LCD_D4);
  lcd_reg |= (1 << LCD_E);
  lcd_send(lcd_reg);
  
  lcd_reg &= ~(1 << LCD_E); // reset E strobe
  lcd_send(lcd_reg);
  
  _delay_us(200);

//Function set
//N =
//  0 1-line mode
//  1 2-line mode
//F =
//  0 display off
//  1 display on
//RS R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0  
// 0  0   0   0   1   0   X   X   X   X
// 0  0   0   0   1   0   X   X   X   X
// 0  0   X   X   X   X   N   F   X   X
  lcd_reg = 0;
  

  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D5);
  lcd_reg |= (1 << LCD_E);
  
  lcd_send(lcd_reg);

  lcd_reg &= ~(1 << LCD_E);
  lcd_send(lcd_reg);

  _delay_us(200);
  
  lcd_reg = 0;
  

  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D5);
  lcd_reg |= (1 << LCD_E);
  lcd_send(lcd_reg);


  lcd_reg &= ~(1 << LCD_E);
  lcd_send(lcd_reg);  

  _delay_us(200);

  lcd_reg = 0;
  

  lcd_reg &= ~(1 << LCD_RS);
  lcd_reg |= (1 << LCD_D3) | (1 << LCD_D2);
  lcd_reg |= (1 << LCD_E);
  lcd_send(lcd_reg);

  lcd_reg &= ~(1 << LCD_E);
  lcd_send(lcd_reg);
    
  //lcd_set_4_bit_1_lines_5x7_pxs();
  //lcd_set_4_bit_2_lines_5x7_pxs();

  _delay_ms(50); //Wait for more than 39 ms


//Display ON/OFF Control
//D =
//  0 display off
//  1 display on
//C =
//  0 cursor off
//  1 cursor on
//B =
//  0 blink off
//  1 blink on
//RS R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// 0  0   0   0   0   0   X   X   X   X  hi nible
// 0  0   X   X   X   X   1   D   C   B  lo nible
// display ON with blinking cursor
  lcd_send_cmd(0x0F); // ????????? ????????? ????????????? ? ??????????
  
//Wait for more than 39 ms
  _delay_ms(50); 

//Display Clear
//RS R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// 0  0   0   0   0   0   X   X   X   X
// 0  0   X   X   X   X   0   0   0   1
  lcd_send_cmd(0x01); 

//Wait for more than 1.53 ms
  _delay_ms(50); 

//Entry Mode Set
//I/D =
//  0 decrement mode
//  1 increment mode
//SH = 
//  0 entire shift off
//  1 entire shift on
//RS R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
// 0  0   0   0   0   0   X   X   X   X
// 0  0   X   X   X   X   0   1  I/D  SH
  lcd_send_cmd(0x06); // ?????????? ????????? ?????????????

//Initialization end
  //============================
  return;
}


//------------------------------------------------------------
void lcd_send(uint8_t lcd_reg) 
{
  spi_transfer(lcd_reg); // send data via SPI
  _delay_us(1); // wait // если не будет работаь то ставить 50 us
  PORTB |= (1 << LCD_CS); //strobe shift reg CS to Hi
  PORTB &= ~(1 << LCD_CS); // CS to Lo
  return;
}
//------------------------------------------------------------
void lcd_send_cmd(uint8_t cmd)
{
  //uart_puts("cmd:");
  //uart_putbin_byte(cmd);
  //uart_puts("\r\n");
  
  uint8_t nibble_hi = cmd & 0xF0;
  nibble_hi >>= 1;
  uint8_t nibble_lo = cmd << 4;
  nibble_lo >>= 1;
  
  // debug REMOVE!!!
  //uart_puts("hi:");
  //uart_putbin_byte(nibble_hi);
  //uart_puts("\r\n");
  //uart_puts("lo:");
  //uart_putbin_byte(nibble_lo);
  //uart_puts("\r\n");

  uint8_t lcd_reg = 0; // reset lcd_reg

  lcd_reg |= (1 << LCD_E); // rise E
  lcd_reg |= nibble_hi;
  
  lcd_send(lcd_reg); // hi nibble

  //__DEBUG
  //uart_puts("reghi:");
  //uart_putbin_byte(lcd_reg);
  //uart_puts("\r\n");
 
  lcd_reg &= ~(1 << LCD_E); // strobe E
  
  lcd_send(lcd_reg); // 

  _delay_us(200);

  lcd_reg = 0; // reset lcd_reg

  lcd_reg |= (1 << LCD_E); // rise E
  lcd_reg |= nibble_lo;

  lcd_send(lcd_reg); // lo
  
  //__DEBUG
  //uart_puts("reglo:");
  //uart_putbin_byte(lcd_reg);
  //uart_puts("\r\n");
  
  lcd_reg &= ~(1 << LCD_E); // strobe E
  
  lcd_send(lcd_reg); 

  _delay_us(200);
  return;
}
//------------------------------------------------------------
void lcd_send_data(uint8_t data)
{
  //uart_puts("D:");
  //uart_putbin_byte(data);
  //uart_puts("\r\n");
  
  uint8_t nibble_hi = data & 0xF0;
  nibble_hi >>= 1;
  uint8_t nibble_lo = data << 4;
  nibble_lo >>= 1;
  
  // debug REMOVE!!!
  //uart_puts("hi:");
  //uart_puthex_byte(nibble_hi);
  //uart_puts("\r\n");
  //uart_puts("lo:");
  //uart_puthex_byte(nibble_lo);
  //uart_puts("\r\n");

  uint8_t lcd_reg = 0; // reset lcd_reg  

  lcd_reg |= (1 << LCD_RS);
  lcd_reg |= (1 << LCD_E); // rise E
  lcd_reg |= nibble_hi;
  
  lcd_send(lcd_reg); // hi nibble

  //__DEBUG
  //uart_puts("reghi:");
  //uart_putbin_byte(lcd_reg);
  //uart_puts("\r\n");

  lcd_reg &= ~(1 << LCD_E); // reset E strobe

  lcd_send(lcd_reg);

  _delay_us(200);

  lcd_reg = 0; // reset lcd_reg

  lcd_reg |= (1 << LCD_RS);
  lcd_reg |= (1 << LCD_E); // rise E
  lcd_reg |= nibble_lo;

  lcd_send(lcd_reg); // lo nibble

  //__DEBUG
  //uart_puts("reglo:");
  //uart_putbin_byte(lcd_reg);
  //uart_puts("\r\n");

  lcd_reg &= ~(1 << LCD_E); // reset E strobe 
  
  lcd_send(lcd_reg);
  
  _delay_us(200);

  return;
}
//------------------------------------------------------------
void lcd_putc(const char c)
{
  lcd_send_data(c);	
  return;
}
//------------------------------------------------------------
void lcd_puts(const char *line) 
{
  if (NULL != line) {
    uint8_t len = strlen(line);
    if (len > 0) {
	  for (uint8_t c = 0; c < len; c++) {
	    lcd_putc(line[c]);
	  }
    }
  }
  return;
}
//------------------------------------------------------------
void lcd_put_ln1(char *line)
{
  lcd_clear();
  _delay_ms(3);
  strncpy(&(display[LCD_LINE1][0]), line, LCD_LINE_LEN);
  lcd_draw();
  return;
}
//------------------------------------------------------------
void lcd_put_ln2(char *line)
{
  strncpy(&(display[LCD_LINE2][0]), line, LCD_LINE_LEN);  
  lcd_draw();
  return;
}
//------------------------------------------------------------
void lcd_draw(void)
{
  // clear LCD display
  lcd_clear();
  _delay_ms(3);
  // out first line
  lcd_puts(&(display[LCD_LINE1][0]));
  // out second line
  lcd_send_cmd(0xC0);
  lcd_puts(&(display[LCD_LINE2][0]));
  return;
}
