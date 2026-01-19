#include <stdio.h>
#include <tinyara/config.h>

#include "http_client.h"
#include "wifi_runner.h"

#define XIAOMI_SSID "xiaomi_test"
#define XIAOMI_PASSWORD "1234567890"
#define XIAOMI_AUTH "wpa2_aes"

static void init_wifi( void )
{
    char *argv[] = { "wm_test", "start", NULL };
    int argc = 2;

    wm_test_main( argc, argv );

    return;
}

static void connect_wifi( void )
{
    char *argv[] = { "wm_test", "join", XIAOMI_SSID, XIAOMI_AUTH, XIAOMI_PASSWORD, NULL };
    int argc = 5;

    wm_test_main( argc, argv );

    return;
}

int wifi_runnable( int argc, char *argv[] )
{
    init_wifi();
    sleep( 5 );
    connect_wifi();

    while ( 1 )
    {
        http_client();
        sleep( 5 );
    }
}
