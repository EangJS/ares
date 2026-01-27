#include "fs_runner.h"
#include <fcntl.h>
#include <pthread.h>
#include <tinyara/arch.h>

/* ******************************************************************************* */
/*                           Macro Defnitions                                      */
/* ******************************************************************************* */

#define KB( x ) ( ( x ) * 1024 )
#define WRITE_SIZE KB( 16 )
#define DUMP_BUFFER( n, buf )                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        for ( int i = 0; i < ( n ); i++ )                                                                              \
        {                                                                                                              \
            printf( "%02x ", buf[ i ] );                                                                               \
        }                                                                                                              \
        printf( "\n" );                                                                                                \
    } while ( 0 )

/* ******************************************************************************* */
/*                           Private Variable Defnitions                           */
/* ******************************************************************************* */

static pthread_mutex_t g_mutex_fs_ready;

/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static uint8_t verify_buffer( uint8_t *buf, size_t len );
static void fs_init( void );
static void *task_write_fs( void *arg );
static void *task_read_fs( void *arg );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

static void fs_init( void )
{
    pthread_mutex_init( &g_mutex_fs_ready, NULL );
    pthread_mutex_lock( &g_mutex_fs_ready );
}

static void delete_file( const char *path )
{
    int ret = unlink( path );
    if ( ret < 0 )
    {
        printf( "Failed to delete file %s\n", path );
    }
}

static void *task_write_fs( void *arg )
{
    char *path = (char *)arg;
    uint8_t buf[ WRITE_SIZE ];
    for ( int i = 0; i < WRITE_SIZE; i++ )
    {
        buf[ i ] = i % 256;
    }

    int first_run = 1;

    while ( 1 )
    {
        if ( first_run )
        {
            pthread_mutex_unlock( &g_mutex_fs_ready );
            first_run = 0;
        }
        pthread_mutex_lock( &g_mutex_fs_ready );
        int fd = open( path, O_RDWR | O_CREAT );
        if ( fd >= 0 )
        {
            write( fd, buf, sizeof( buf ) );
            close( fd );
        }
        else
        {
            printf( "Writer: open failed for %s\n", path );
        }
        pthread_mutex_unlock( &g_mutex_fs_ready );
        sleep( 5 );
    }
    return NULL;
}

static void *task_read_fs( void *arg )
{
    char *path = (char *)arg;
    char buf[ WRITE_SIZE + 1 ];

    while ( 1 )
    {
        pthread_mutex_lock( &g_mutex_fs_ready );

        int fd = open( path, O_RDONLY );
        if ( fd < 0 )
        {
            printf( "Reader: open failed for %s\n", path );
            pthread_mutex_unlock( &g_mutex_fs_ready );
            sleep( 5 );
            continue;
        }

        int n = read( fd, buf, WRITE_SIZE );
        if ( n > 0 )
        {
            uint8_t result = verify_buffer( (uint8_t *)buf, n );
            if ( result == 0 )
            {
                printf( "Reader: Data verified successfully (%d bytes)\n", n );
            }
            else
            {
                printf( "Reader: Data verification failed\n" );
            }
            printf( "\n" );
            delete_file( path );
        }
        else
        {
            printf( "Reader: read failed for %s\n", path );
        }

        close( fd );

        pthread_mutex_unlock( &g_mutex_fs_ready );
        sleep( 5 );
    }
    return NULL;
}

static uint8_t verify_buffer( uint8_t *buf, size_t len )
{
    for ( size_t i = 0; i < len; i++ )
    {
        if ( buf[ i ] != ( i % 256 ) )
        {
            printf( "Data mismatch at index %zu: expected %u, got %u\n", i, (unsigned char)( i % 256 ), buf[ i ] );
            return -1;
        }
    }
    return 0;
}

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

int fs_task_main( int argc, char *argv[] )
{
    char *path = ( argv[ 1 ] != NULL ) ? argv[ 1 ] : MNT_PATH;

    fs_init();

    pthread_t writer_thread, reader_thread;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, WRITE_SIZE * 2 );

    pthread_create( &writer_thread, &attr, task_write_fs, path );
    pthread_create( &reader_thread, &attr, task_read_fs, path );

    pthread_join( writer_thread, NULL );
    pthread_join( reader_thread, NULL );

    return 0;
}
