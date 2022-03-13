/* Lab 4 - ENGR331
 * The goal of this lab is help you figure out
 * (on your own) how to interface to a peripheral
 * that you have never used before based on the
 * documentation you find (either datasheet or other
 * available sources/examples online).
 * ENJOY!!! (and I really mean it!)
 */
#include "LCD.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lab_gpio.h"
#include "stm32f4xx.h" 

/*******************************
 * LCD pins connections to PORTD
 *******************************
 */
#define RS 7
#define EN 6 

#define DB7 3
#define DB6 2
#define DB5 1
#define DB4 0

#define INIT_DELAY_MS 100


/*******************************
 * FUNCTION PROTOTYPES
 *******************************
 */

static void LCD_putNibble(uint8_t nibble);
// END Functions


/*******************************
 * tim6_delay(void)
 * Inputs: NONE
 * Outputs: NONE
 * Based on PSC=0 and ARR=21000; 
 * we get delay of approximately 1.33ms
 *******************************
 */
void tim6_delay(void){
	// enable APB1 bus clock
	RCC->APB1ENR|=RCC_APB1ENR_TIM6EN;
	//TIM6 prescaler set at default to 0 for now
	TIM6->PSC=0; // prescalar
	TIM6->ARR = 16000;  //auto reload register 
	TIM6->CNT=0;   //clear counter register
	TIM6->CR1|=TIM_CR1_CEN;
	//WHEN COUNTER IS DONE THE TIM6_SR REG UIF FLAG IS SET
	while(TIM6->SR==0);
	TIM6->SR=0; //CLEAR uIF FLAG
}

/*******************************
 * delay(int ms)
 * Inputs: delay in milliseconds
 * Outputs: NONE
 * An approximate delay because  
 * call of tim6_delay() creates about 1.33ms
 *******************************
 */
void delay(int ms)
{
	int i;
	for (i=ms; i>0; i--)
	{
		tim6_delay();
	}
}


/*******************************
 * LCD_port_init()
 * Inputs: NONE
 * Outputs: NONE
 * Port Initialization
 * Refer to the #define statements at top to
 * see what ports are used to connect
 * the STMicro Board with the HD44780 LCD driver
 * Set appropriate pins as digital input/outputs
 *******************************
 */
void LCD_port_init()
{
	//STEP 1: Enable GPIOD in RCC AHB1ENR register
	gpio_clock_enable(RCC_AHB1ENR_GPIODEN_Pos);

	//STEP 2: Set MODER of GPIOD Pins 7, 6, 3, 2, 1 & 0 as outputs
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE7_Pos, GPIO_MODER_MODER7_0);
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE6_Pos, GPIO_MODER_MODER6_0);
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE3_Pos, GPIO_MODER_MODER3_0);
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE2_Pos, GPIO_MODER_MODER2_0);
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE1_Pos, GPIO_MODER_MODER1_0);
	gpio_pin_set_mode(GPIOD, GPIO_MODER_MODE0_Pos, GPIO_MODER_MODER0_0);

	//STEP 3: Set OTYPER of GPIOD Pins 7, 6, 3, 2, 1 & 0 as push-pull
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT7_Pos, 0x00);
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT6_Pos, 0x00);
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT3_Pos, 0x00);
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT2_Pos, 0x00);
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT1_Pos, 0x00);
	gpio_set_output_type(GPIOD, GPIO_OTYPER_OT0_Pos, 0x00);
	
	gpio_pin_clear(GPIOD, EN);
	
	//Done with LCD port Initialization
}

/*******************************
 * LCD_init()
 * Inputs: NONE
 * Outputs: NONE
 * LCD Initialization
 * Read the manual carefully
 * We are doing initialization by instruction
 * Don't rush it.
 *******************************
 */

void LCD_init()
{
	// STEP 1: Wait for 100ms for power-on-reset to take effect
	delay(INIT_DELAY_MS);

	// STEP 2: Set RS pin LOW to send instructions
	gpio_pin_clear(GPIOD, RS);

	// Send instructions using following format:
	// Set EN=HIGH; Send 4-bit instruction; Set EN=low; delay 20ms;

	// STEP 3a-3d: Set 4-bit mode (takes a total of 4 steps)
	LCD_putNibble(0x03);
	delay(20);
	LCD_putNibble(0x03);
	delay(20);
	LCD_putNibble(0x03);
	delay(20);
	LCD_putNibble(0x02);
	delay(20);

	// STEP 4: Set 2 line display -- treats 16 char as 2 lines
	LCD_send_cmd(LCD_CMD_FUNCTION_SET | TWO_LINE);
	
	// STEP 5: Set DISPLAY to OFF
	LCD_send_cmd(LCD_CMD_DISPLAY_ON_OFF);

	// STEP 6: CLEAR DISPLAY
	LCD_send_cmd(LCD_CMD_CLEAR_DISPLAY);

	// STEP 7: SET ENTRY MODE - Auto increment; no scrolling
	LCD_send_cmd(LCD_CMD_SET_ENTRY_MODE | ENTRY_MODE_CURSOR_DIRECTION_RIGHT);

	// STEP 8: Set Display to ON with Cursor and Blink.
	LCD_send_cmd(LCD_CMD_DISPLAY_ON_OFF | DISPLAY_MODE_ON | DISPLAY_MODE_CURSOR | DISPLAY_MODE_BLINK);
}

/*******************************
 * place_lcd_cursor()
 * Inputs: unsigned character
 * Outputs: NONE
 * sets Cursor position to
 * Line 1, character 1 (hex address 0x80)
 * or Line 2, character 1 (hex addres 0xC0)
 *
 *******************************
 */

void LCD_place_cursor(uint8_t address)
{
    int cur_address = 0x00;
    LCD_send_cmd(LCD_CMD_RETURN_HOME);
    
    while (cur_address < address)
    {
        LCD_send_cmd(LCD_CMD_CURSOR_DISPLAY_SHIFT | MOVE_RIGHT);
        cur_address++;
    }
}


void LCD_clear_display(void)
{
	LCD_send_cmd(LCD_CMD_CLEAR_DISPLAY);
}



/*******************************
 * LCD_write()
 * Inputs: unsigned character data (8-bit)
 * Outputs: NONE
 * writes the character to LCD.
 *
 *******************************
 */

void LCD_write_char(unsigned char data)
{
	LCD_putNibble(data >> 4);
	LCD_putNibble(data & 0x0F);
}

/**
* NOTE: It is up to the caller to clear the LCD before writing to the screen
* if desired.
*/
void LCD_write_string(char *message, write_type_t write_type)
{
	char string_buffer[DISPLAY_WIDTH_CHARS + 1];
	int i = 0;
	
	LCD_send_cmd(0x02);
	strncpy(string_buffer, message, 16);
	string_buffer[DISPLAY_WIDTH_CHARS] = '\0';

	if (write_type == OFF_WHILE_WRITING)
	{
		// Turn off display if displaying all at once
		LCD_send_cmd(0x08);
	}
	
	while (string_buffer[i] != '\0' && i < 17)
	{
		LCD_write_char(string_buffer[i]);
		i++;
		// Delay before next character
	}	
	

	if (write_type == OFF_WHILE_WRITING)
	{
		// Turn display back on if it was turned off
		LCD_send_cmd(0x0F);
	}

}

static void LCD_putNibble(uint8_t nibble)
{
	// Send instructions using following format:
	// Set EN=HIGH; Send 4-bit instruction; Set EN=low; delay 20ms;
	gpio_pin_set(GPIOD, EN);

	// Set pins for nibble
	gpio_pin_set_level(GPIOD, DB7, (bool)(nibble & (1 << 3)));
	gpio_pin_set_level(GPIOD, DB6, (bool)(nibble & (1 << 2)));
	gpio_pin_set_level(GPIOD, DB5, (bool)(nibble & (1 << 1)));
	gpio_pin_set_level(GPIOD, DB4, (bool)(nibble & (1 << 0)));
	
	delay(4);

	// Set EN = LOW
	gpio_pin_clear(GPIOD, EN);
	
	delay(4);
}


void LCD_send_cmd(uint8_t cmd)
{

	// STEP 2: Set RS pin LOW to send instructions
	gpio_pin_clear(GPIOD, RS);

	LCD_putNibble(cmd >> 4);
	
	LCD_putNibble(cmd & 0x0F);
	
	gpio_pin_set(GPIOD, RS);
	
	delay(20);
}





