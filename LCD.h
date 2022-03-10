#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#include "stm32f4xx.h"

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
void LCD_write(uint8_t data);
void place_lcd_cursor(uint8_t lineno);
void LCD_send_cmd(uint8_t cmd);
// END Functions


#endif // _LCD_H_