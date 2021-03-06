/* Lab 4 - ENGR331
* Authors: Alex Bourdage, Sophie Woessner
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
#define TICKER_RATE_MS 400
// The delay required between commands. Started at 20ms, but found that 10ms works and is more responsive.
#define CMD_DELAY_MS 10
#define DATA_DELAY_MS 4


/*******************************
 * STATIC FUNCTION PROTOTYPES
 *******************************
 */

static void LCD_putNibble(uint8_t nibble);
static void tim6_delay(void);
// END Functions


/*******************************
 * tim6_delay(void)
 * Inputs: NONE
 * Outputs: NONE
 * Based on PSC=0 and ARR=16000; 
 * we get delay of approximately 1ms
 *******************************
 */
static void tim6_delay(void){
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
	delay(CMD_DELAY_MS);
	LCD_putNibble(0x03);
	delay(CMD_DELAY_MS);
	LCD_putNibble(0x03);
	delay(CMD_DELAY_MS);
	LCD_putNibble(0x02);
	delay(CMD_DELAY_MS);

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
 * LCD_write_char()
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
    delay(DATA_DELAY_MS);
}


/*******************************
 * LCD_write_string()
 * Inputs: unsigned character data (8-bit) and display state while writing
 * Outputs: NONE
 * writes the string to LCD.
 *
 *******************************
 */
void LCD_write_string(char *message, write_type_t write_type)
{
	char string_buffer[LINE_WIDTH_CHARS + 1];
    int len = (int)strlen(message);
	int i = 0;
	
	// Clear any contents and put cursor back at the beginning of top row
	LCD_send_cmd(LCD_CMD_CLEAR_DISPLAY);

	// Copy the message into the buffer
	strncpy(string_buffer, message, LINE_WIDTH_CHARS);

	// Make sure the buffer has a null terminator
	string_buffer[LINE_WIDTH_CHARS] = '\0';

	if (write_type == OFF_WHILE_WRITING)
	{
		// Turn off display if displaying all at once
		LCD_send_cmd(LCD_CMD_DISPLAY_ON_OFF);
	}
	
	// Send the message to the LCD one character at a time
	while (string_buffer[i] != '\0' && i <= LINE_WIDTH_CHARS)
	{
		LCD_write_char(string_buffer[i]);
		i++;
		// Delay before next character
	}	
	

	if (write_type == OFF_WHILE_WRITING)
	{
		// Turn display back on if it was turned off
		LCD_send_cmd(LCD_CMD_DISPLAY_ON_OFF | DISPLAY_MODE_ON | DISPLAY_MODE_CURSOR | DISPLAY_MODE_BLINK);
	}

	// If the string is longer than the display is wide, show ticker style
	if (len > DISPLAY_WIDTH_CHARS)
	{
		// Wait one second before scrolling starts
        delay(1000);
		// Set the number of shifts to display whole message
		int max_shifts = (len - DISPLAY_WIDTH_CHARS) < (LINE_WIDTH_CHARS - DISPLAY_WIDTH_CHARS) ? (len - DISPLAY_WIDTH_CHARS) : (LINE_WIDTH_CHARS - DISPLAY_WIDTH_CHARS);
		int shift_count = 0;

		// Shift ticker style
		while (shift_count < max_shifts)
		{
            // Shift two times and delay twice as long for better readability
			LCD_send_cmd(LCD_CMD_CURSOR_DISPLAY_SHIFT | SHIFT_OPERATION);
            LCD_send_cmd(LCD_CMD_CURSOR_DISPLAY_SHIFT | SHIFT_OPERATION);            
			shift_count += 2;
			delay(TICKER_RATE_MS * 2);
		}
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
   

	// Set EN = LOW
	gpio_pin_clear(GPIOD, EN);
}


void LCD_send_cmd(uint8_t cmd)
{

	// STEP 2: Set RS pin LOW to send instructions
	gpio_pin_clear(GPIOD, RS);

	// Send upper 4 bits
	LCD_putNibble(cmd >> 4);
	
	// Send lower four bits
	LCD_putNibble(cmd & 0x0F);
	
	// Set RS back to high, which is the default state
	gpio_pin_set(GPIOD, RS);
	
	// Allow time for command to be processed
	delay(CMD_DELAY_MS);
}





