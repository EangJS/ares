#include "gpio_manager.h"
#include <fcntl.h>
#include <stdio.h>
#include <tinyara/gpio.h>
#include <unistd.h>

#define LED_PIN      52
#define LED_ON       1
#define LED_OFF      0
#define MAX_GPIO_PIN 60

/* ******************************************************************************* */
/*                           Public Variable Defnitions                            */
/* ******************************************************************************* */

/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static void gpio_write( int port, int value );
static int gpio_read( int port );
static void gpio_flash( int pin );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

static void gpio_write( int port, int value )
{
    char str[ 4 ];
    static char devpath[ 16 ];
    snprintf( devpath, 16, "/dev/gpio%d", port );
    int fd = open( devpath, O_RDWR );
    if ( fd < 0 )
    {
        printf( "fd open fail\n" );
        return;
    }

    ioctl( fd, GPIOIOC_SET_DIRECTION, GPIO_DIRECTION_OUT );
    if ( write( fd, str, snprintf( str, 4, "%d", value != 0 ) + 1 ) < 0 )
    {
        printf( "write error\n" );
    }

    close( fd );
}

static int gpio_read( int port )
{
    char buf[ 4 ];
    char devpath[ 16 ];
    snprintf( devpath, 16, "/dev/gpio%d", port );
    int fd = open( devpath, O_RDWR );
    if ( fd < 0 )
    {
        printf( "fd open fail\n" );
        return -1;
    }

    ioctl( fd, GPIOIOC_SET_DIRECTION, GPIO_DIRECTION_IN );
    if ( read( fd, buf, sizeof( buf ) ) < 0 )
    {
        printf( "read error\n" );
        close( fd );
        return -1;
    }
    close( fd );

    return buf[ 0 ] == '1';
}

void gpio_flash( int pin )
{
    gpio_write( pin, LED_ON );
    usleep( 1000 );
    gpio_write( pin, LED_OFF );
}

int gpio_runnable( int argc, char *argv[] )
{
    while ( 1 )
    {
        gpio_flash( LED_PIN );
        usleep( 100000 );
        gpio_flash( LED_PIN );
        sleep( 1 );
    }
    return 0;
}
