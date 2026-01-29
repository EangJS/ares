#include "common.h"
#include "wifi_runner.h"
#include <stdio.h>
#include <tinyara/config.h>

#define IPV4 1

static void* tcp_nettest_server( void *arg )
{
    while ( 1 )
    {
        tcp_server_thread( 10 );
        printf( "================== Received 10 TCP Packets ==================\n" );
        sleep( 10 );
    }

    return NULL;
}

static void* tcp_nettest_client( void *arg )
{
    while ( 1 )
    {
        char *argv[] = { "nettest", "2", "tcp", SERVER_IP, "5555", "10", "3" };
        int argc = 7;

        nettest_main( argc, argv );
        sleep( 10 );
    }

    return NULL;
}

static void* udp_nettest_server( void *arg )
{
    while ( 1 )
    {
        udp_server_thread( 10 );
        printf( "================== Received 10 UDP Packets ==================\n" );
        sleep( 10 );
    }

    return NULL;
}

static void* udp_nettest_client( void *arg )
{
    while ( 1 )
    {
        char *argv[] = { "nettest", "2", "udp", SERVER_IP, "5555", "10", "3" };
        int argc = 7;

        nettest_main( argc, argv );
        sleep( 10 );
    }

    return NULL;
}

int task_start_netstress( int argc, char *argv[] )
{
    wait_for_wifi();

    pthread_t tcp_nettest_server_thread,
        udp_nettest_server_thread,
        tcp_nettest_client_thread,
        udp_nettest_client_thread;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, 8192 );

    pthread_create( &tcp_nettest_server_thread, &attr, tcp_nettest_server, NULL );
    pthread_create( &udp_nettest_server_thread, &attr, udp_nettest_server, NULL );
    pthread_create( &tcp_nettest_client_thread, &attr, tcp_nettest_client, NULL );
    pthread_create( &udp_nettest_client_thread, &attr, udp_nettest_client, NULL );

    pthread_join( tcp_nettest_server_thread, NULL );
    pthread_join( udp_nettest_server_thread, NULL );
    pthread_join( tcp_nettest_client_thread, NULL );
    pthread_join( udp_nettest_client_thread, NULL );

    return 0;
}
