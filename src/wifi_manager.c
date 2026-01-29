#include "http_client.h"
#include "wifi_runner.h"
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/stat.h>
#include <tinyara/config.h>
#include <pthread.h>

/* ******************************************************************************* */
/*                           Macro Defnitions                                      */
/* ******************************************************************************* */

#define XIAOMI_SSID "xiaomi_test"
#define XIAOMI_PASSWORD "1234567890"
#define XIAOMI_AUTH "wpa2_aes"
#define MQ_NAME "/time_status_mq"
#define WIFI_CONNECTED_BIT 0x01

/* ******************************************************************************* */
/*                           Public Variable Declarations                          */
/* ******************************************************************************* */
mqd_t time_status_mq;
pthread_mutex_t wifi_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wifi_cond = PTHREAD_COND_INITIALIZER;
int wifi_status = 0;

/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static void init_wifi( void );
static void connect_wifi( void );
static void get_wifi_info( void );
static int mq_init( void );
static void wifi_set_ready( void );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

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

static void get_wifi_info( void )
{
    wait_for_wifi();

    char *argv[] = { "wm_test", "info", NULL };
    int argc = 2;

    wm_test_main( argc, argv );

    return;
}

static int mq_init( void )
{
    struct mq_attr attr = { .mq_flags = 0, .mq_maxmsg = 1, .mq_msgsize = sizeof( char * ), .mq_curmsgs = 0 };

    time_status_mq = mq_open( MQ_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0644, &attr );
    if ( time_status_mq == (mqd_t)-1 )
    {
        return -1;
    }
    return 0;
}

static void wifi_set_ready( void )
{
    pthread_mutex_lock(&wifi_mutex);
    wifi_status |= WIFI_CONNECTED_BIT;
    pthread_cond_broadcast(&wifi_cond);
    pthread_mutex_unlock(&wifi_mutex);
}

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

void wait_for_wifi( void )
{
    pthread_mutex_lock(&wifi_mutex);

    while (!(wifi_status & WIFI_CONNECTED_BIT))
    {
        pthread_cond_wait(&wifi_cond, &wifi_mutex);
    }

    pthread_mutex_unlock(&wifi_mutex);
}

int wifi_runnable( int argc, char *argv[] )
{
    init_wifi();
    sleep( 3 );
    connect_wifi();
    sleep( 5 );
    wifi_set_ready();
    if ( mq_init() < 0 )
    {
        perror( "mq_init failed" );
        return -1;
    }

    while ( 1 )
    {
        get_wifi_info();
        char *time_str = (char *)malloc( 64 );
        http_client( time_str );

        char *old_msg;
        while ( mq_receive( time_status_mq, (char *)&old_msg, sizeof( old_msg ), NULL ) >= 0 )
        {
            free( old_msg );
        }

        if ( mq_send( time_status_mq, (char *)&time_str, sizeof( time_str ), 0 ) < 0 )
        {
            perror( "mq_send failed" );
            free( time_str );
        }

        sleep( 10 );
    }
}
