#include "lcd_runner.h"
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <tinyara/config.h>

#define RUNNING_STATE 0
#define OTA_STATE 1

/* ******************************************************************************* */
/*                           Public Variable Declarations                          */
/* ******************************************************************************* */

volatile bool lcd_on = true;
extern sem_t ota_complete_semaphore;

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

void set_lcd_power( int power )
{
    if ( power > 0 )
    {
        lcd_on = true;
    }
    else
    {
        lcd_on = false;
    }
    power_test( power );
}

void await_ota_completion( void )
{
    sem_wait(&ota_complete_semaphore);
}

uint8_t update_state( uint8_t current_state )
{
    if ( is_ota_in_progress() )
    {
        if ( current_state != OTA_STATE )
        {
            current_state = OTA_STATE;
            await_ota_completion();
        }
    }
    else
    {
        if ( current_state != RUNNING_STATE )
        {
            current_state = RUNNING_STATE;
        }
    }
    return current_state;
}

void toggle_lcd_power( void )
{
    if ( lcd_on )
    {
        set_lcd_power( 0 );
    }
    else
    {
        set_lcd_power( 100 );
    }
}

int task_power_lcd( int argc, char *argv[] )
{
    uint8_t state = RUNNING_STATE;
    while ( 1 )
    {
        state = update_state( state );
        switch ( state )
        {
            case RUNNING_STATE:
                sleep( 10 );
                toggle_lcd_power();
                break;
            case OTA_STATE:
                break;
            default:
                break;
        }
    }
}
