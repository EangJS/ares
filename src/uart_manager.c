#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinyara/config.h>
#include <unistd.h>
#ifndef CONFIG_DISABLE_POLL
#include <poll.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

/* ******************************************************************************* */
/*                           Macro Defnitions                                      */
/* ******************************************************************************* */

#ifndef CONFIG_EXAMPLES_UART_LOOPBACK_PORT
#define CONFIG_EXAMPLES_UART_LOOPBACK_PORT 2
#endif

#define UART_DEV_PATH "/dev/ttyS%d"
#define UART_DEV_PATH_MAX_LEN 12
#define UART_POLL_TIMEOUT_MS 10000

#define TEST_STR "1234567890abcdefghijklmnopqrstuvwxyz"
#define TEST_STR_LEN 37

/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static int uart_rx_loop( pthread_addr_t *arg );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

static int uart_rx_loop( pthread_addr_t *arg )
{
    int fd = 0;
    char port[ UART_DEV_PATH_MAX_LEN ] = { '\0' };
    ssize_t ret_size;
    int remain_size;
    char *read_ptr;
    int rx_test_count = 0;
    char read_buf[ TEST_STR_LEN ];
    int port_num = (int)arg;

    snprintf( port, UART_DEV_PATH_MAX_LEN, UART_DEV_PATH, port_num );

    printf( "UART RX THREAD START [Port: %s]\n", port );

    fd = open( port, O_RDWR | O_SYNC, 0666 );
    if ( fd < 0 )
    {
        printf( "ERROR: Failed to open %s Rx UART(%d):\n", port, get_errno() );
        return -1;
    }

    while ( 1 )
    {
        rx_test_count++;
        read_ptr = (char *)&read_buf;
        remain_size = TEST_STR_LEN;

        while ( 0 < remain_size )
        {

#ifndef CONFIG_DISABLE_POLL
            struct pollfd fds[ 1 ];
            fds[ 0 ].fd = fd;
            fds[ 0 ].events = POLLIN;
            if ( poll( fds, 1, UART_POLL_TIMEOUT_MS ) < 0 )
            {
                printf( "Fail to poll(%d):\n", get_errno() );
                close( fd );
                return -1;
            }
            if ( !( fds[ 0 ].revents & POLLIN ) )
            {
                printf( "RESULT(%d): FAILED (Timeout)\n", rx_test_count );
                continue;
            }
#endif
            ret_size = read( fd, (void *)read_ptr, remain_size );
            remain_size -= ret_size;
            read_ptr += ret_size;
        }
        printf( "RECEIVE(%d): %s\n", rx_test_count, read_buf );
        if ( strncmp( read_buf, TEST_STR, TEST_STR_LEN ) == 0 )
        {
            printf( "RESULT(%d): PASSED\n", rx_test_count );
        }
        else
        {
            printf( "RESULT(%d): FAILED (It does not match)\n", rx_test_count );
        }
        sleep( 3 );
    }

    close( fd );
    return 0;
}

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

int uart_runnable( int argc, char *argv[] )
{
    pthread_t rx_tid;
    int port_num = CONFIG_EXAMPLES_UART_LOOPBACK_PORT;

    if ( argc == 2 )
    {
        port_num = atoi( argv[ 1 ] );
    }

    printf( "######################### UART loopback test START #########################\n" );

    if ( pthread_create( &rx_tid, NULL, (pthread_startroutine_t)uart_rx_loop, (void *)port_num ) < 0 )
    {
        printf( "Fail to create rx pthread(%d):\n", get_errno() );
        return -1;
    }
    pthread_join( rx_tid, NULL );
    return 0;
}
