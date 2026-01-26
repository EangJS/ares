#include "lcd_runner.h"
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <tinyara/config.h>

/* ******************************************************************************* */
/*                           Public Variable Declarations                          */
/* ******************************************************************************* */

volatile bool lcd_on = true;

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

void set_lcd_power( int power )
{
    power_test( power );
}

int task_power_lcd( int argc, char *argv[] )
{
    while ( 1 )
    {
        sleep( 5 );

        // Turn off LCD
        lcd_on = false;
        set_lcd_power( 0 );

        sleep( 5 );

        // Turn on LCD
        set_lcd_power( 100 );
        lcd_on = true;

        sleep( 5 );
    }
}
