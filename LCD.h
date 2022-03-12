#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#include "stm32f4xx.h"

#define DISPLAY_WIDTH_CHARS 16

/*******************************
 * FUNCTION PROTOTYPES
 *******************************
 */
// Delay Functions
void tim6_delay(void);
void delay(int ms);

// LCD related functions
void LCD_port_init(void);
void LCD_init(void);
void place_lcd_cursor(uint8_t lineno);
void LCD_send_cmd(uint8_t cmd);
void LCD_write_char(unsigned char data);
void LCD_write_string(char *message, int delay_ms);
void LCD_clear_display(void);
// END Functions


#endif // _LCD_H_