
#include "LCD.h"
#include "lab_gpio.h"
#include "stm32f4xx.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define DEBOUNCE_ITERS 20
#define COUNT_INCREMENT_TICKS 100000
#define INCREMENTAL_DELAY_TICKS 50000
#define UPPERCASE_LOWERCASE_OFFSET 0x20

typedef enum {
  DISPLAY_OFF = 0,
  DISPLAY_AT_ONCE,
  DISPLAY_REVERSE,
  DISPLAY_INCREMENTAL,
  DISPLAY_CAP_VOWELS,
  DISPLAY_DECIMAL_NUMBER
} LCD_state_t;

static void reverse_string(char *str);
static void handle_state_transition(LCD_state_t state);
static void capitalize_vowels(char *str);
static bool is_lower_case_vowel(char letter);
static void handle_state_actions(LCD_state_t state, int timer_ticks);

static char *message = "Hello engr331 this is a long message!";
static int count = 0;
static int count_changed = 1;
static int incremental_char_pos = 0;

static bool is_lower_case_vowel(char letter)
{
    char vowels[7] = "aeiouy";
    
    for (int i = 0; i < 6; i++)
    {
        if (letter == vowels[i])
        {
            return true;
        }
    }
    return false;
        
}

static void capitalize_vowels(char str[])
{
    int len = (int)strlen(str);

    for (int i = 0; i < len; i++)
    {
        if (is_lower_case_vowel(str[i]))
        {
            str[i] = str[i] - UPPERCASE_LOWERCASE_OFFSET;
        }
    }
}

static void reverse_string(char str[])
{
  int len = (int)strlen(str);
    char temp;
    
    for (int i = 0; i < len/2; i++)
    {
        temp = str[i];
        str[i] = str[len - i -1];
        str[len - i - 1] = temp;
    }
}

static void handle_state_actions(LCD_state_t state, int timer_ticks)
{
    switch(state)
    {
        case DISPLAY_DECIMAL_NUMBER:
        {
            if (count_changed)
            {
                handle_state_transition(DISPLAY_DECIMAL_NUMBER);
                count_changed = 0;
            }
						break;
        }

        case DISPLAY_INCREMENTAL:
        {   
            int len = (int)strlen(message);
            if (timer_ticks % INCREMENTAL_DELAY_TICKS == 0)
            {   
                if (incremental_char_pos == 0)
                {
                    LCD_clear_display();
                    LCD_place_cursor(15);
                }
                LCD_write_char(message[incremental_char_pos]);
                incremental_char_pos = (incremental_char_pos + 1) % len; 
            }
						break;
        }

        default:
            break;
    }
}

static void handle_state_transition(LCD_state_t state)
{
    switch (state)
    {
        case DISPLAY_OFF:
        {
            LCD_clear_display();
            break;
        }

        case DISPLAY_AT_ONCE:
        {
            LCD_write_string(message, OFF_WHILE_WRITING);
            break;
        }

        case DISPLAY_REVERSE:
        {
            char buffer[DISPLAY_WIDTH_CHARS + 1];
            strncpy(buffer, message, DISPLAY_WIDTH_CHARS);
            reverse_string(buffer);
            LCD_write_string(buffer, OFF_WHILE_WRITING);
            break;
        }
        
        case DISPLAY_INCREMENTAL:
        {
            // Ticker mode, so put cursor on right side of the screen
            LCD_place_cursor(15);
            // Change modes to shift left
            LCD_send_cmd(LCD_CMD_SET_ENTRY_MODE | ENTRY_MODE_CURSOR_DIRECTION_RIGHT | ENTRY_MODE_SHIFT_ENABLE);
            break;
        }

        case DISPLAY_CAP_VOWELS:
        {
            char buffer[DISPLAY_WIDTH_CHARS + 1] = {0};
            strncpy(buffer, message, DISPLAY_WIDTH_CHARS);
            capitalize_vowels(buffer);
            LCD_write_string(buffer, OFF_WHILE_WRITING);
            break;
        }

        case DISPLAY_DECIMAL_NUMBER:
        {
            char buffer[DISPLAY_WIDTH_CHARS + 1];
            sprintf(buffer, "Count: %d", count);
            LCD_write_string(buffer, ON_WHILE_WRITING);
            break;
        }
    }   
}


/*******************************
 * START OF ACTUAL CODE
 *******************************
 */

/*******************************
 * main()
 * Inputs: NONE
 * Outputs: NONE
 * Main function. Goals:
 * Initialize LCD Port
 * Initialize LCD
 * Place Cursor on Line 1
 * Write character string to Line 1
 * Place Cursor on Line 2
 * Write character string to Line 2
 *******************************
 */

int main(void) 
{
  int timer_ticks = 0;
  int button_high_count = 0;
  int button_clicked = 0;
  LCD_state_t application_state = DISPLAY_OFF;
  LCD_port_init();
  LCD_init();
  user_button_init();
  LCD_clear_display();


  while(1)
  {
    timer_ticks++;

    if (timer_ticks % COUNT_INCREMENT_TICKS == 0)
    {
      count++;
      count_changed = 1;
    }
    if (gpio_pin_get_level(GPIOA, USER_BTN) == 1)
    {

      button_high_count++;
      if (button_high_count == DEBOUNCE_ITERS)
      {
        button_clicked = 1;
      }
    }
    else
    {
      button_high_count = 0;
    }

  

        if (button_clicked)
        {
            LCD_clear_display();
            switch(application_state)
            {
                case DISPLAY_OFF:
                    application_state = DISPLAY_AT_ONCE;
                    break;

                case DISPLAY_AT_ONCE:
                    application_state = DISPLAY_REVERSE;
                    break;
                case DISPLAY_REVERSE:
                    application_state = DISPLAY_INCREMENTAL;
                    break;

                case DISPLAY_INCREMENTAL:
                    application_state = DISPLAY_CAP_VOWELS;
                
                    // Clean up for next state
                    // Set entry mode and place cursor back at left side
                    LCD_send_cmd(LCD_CMD_SET_ENTRY_MODE | ENTRY_MODE_CURSOR_DIRECTION_RIGHT);
                    LCD_send_cmd(LCD_CMD_RETURN_HOME);
                    incremental_char_pos = 0;
                    
                    break;

                case DISPLAY_CAP_VOWELS:
                    application_state = DISPLAY_DECIMAL_NUMBER;
                    break;

                case DISPLAY_DECIMAL_NUMBER:
                    application_state = DISPLAY_OFF;
                    break;
            }
            handle_state_transition(application_state);
            button_clicked = 0;
        }
        handle_state_actions(application_state, timer_ticks);
    }
}