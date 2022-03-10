#include "LCD.h"
#include "stm32f4xx.h"


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

    LCD_port_init();
    LCD_init();

    while(1);
    
    return 0;
}