#include "ota_manager.h"

#define MINUTES( x ) ( ( x ) * 60 )

static volatile uint8_t ota_in_progress = 0;

static void run_kernel_update( void )
{
    binary_update_same_version_test();
    return;
}

uint8_t is_ota_in_progress( void )
{
    return ota_in_progress == 1 ? 1 : 0;
}

int ota_manager_runnable( int argc, char *argv[] )
{
    wait_for_wifi();

    while ( 1 )
    {
        printf( "\n==== OTA Manager Task Start Kernel Update Test ====\n" );
        ota_in_progress = 1;
        run_kernel_update();
        ota_in_progress = 0;
        printf( "==== OTA OK ====\n" );
        sleep( MINUTES( 10 ) );
    }
    
    return 0;
}
