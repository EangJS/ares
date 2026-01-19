#include <inttypes.h>
#include <stdio.h>
#include <tinyara/config.h>

#include "lcd_runner.h"

void start_pm_test( uint32_t timeout )
{
    static char timeout_str[ 11 ]; // uint32_t max: 10 digits + '\0'

    snprintf( timeout_str, sizeof( timeout_str ), "%" PRIu32, timeout );

    char *argv[] = { "power", "start", "-t", timeout_str, NULL };

    int argc = 4;

    power_main( argc, argv );
}

int task_start_power_management( int argc, char *argv[] )
{
    start_pm_test( 2000 );

    return 0;
}
