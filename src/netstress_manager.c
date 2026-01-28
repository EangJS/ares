#include "common.h"
#include <stdio.h>
#include <tinyara/config.h>

extern uint8_t is_wifi_connected;

static void *tcp_nettest_server( void *arg )
{
    while ( 1 )
    {
        tcp_server_thread( 10 );
        printf("================== Received 10 TCP Packets ==================\n");
        sleep( 1 );
    }
    // char *argv[] = { "nettest", "1", "tcp", "0", TCP_LISTEN_PORT, "0" };
    // int argc = 6;

    // nettest_main( argc, argv );

    return NULL;
}

// static void tcp_nettest_client( char *ip_addr )
// {
//     char *argv[] = { "nettest", "2", "tcp", ip_addr, TCP_CLIENT_PORT, "0", "1" };
//     int argc = 7;

//     nettest_main( argc, argv );

//     return;
// }

static void *udp_nettest_server( void *arg )
{
    while ( 1 )
    {
        udp_server_thread( 10 );
        printf("================== Received 10 UDP Packets ==================\n");
        sleep( 1 );
    }
    // char *argv[] = { "nettest", "1", "udp", "0", UDP_LISTEN_PORT, "0" };
    // int argc = 6;

    // nettest_main( argc, argv );

    return NULL;
}

// static void udp_nettest_client( char *ip_addr )
// {
//     char *argv[] = { "nettest", "2", "udp", ip_addr, UDP_CLIENT_PORT, "0", "1" };
//     int argc = 7;

//     nettest_main( argc, argv );

//     return;
// }

int task_start_netstress( int argc, char *argv[] )
{
    while ( !is_wifi_connected )
    {
        printf( "WiFi not connected yet, wait 3 seconds\n" );
        sleep( 3 );
    }

    pthread_t tcp_nettest_server_thread, udp_nettest_server_thread;
    //, tcp_nettest_client_thread, udp_nettest_client_thread;
    pthread_attr_t attr;

    pthread_attr_init( &attr );
    pthread_attr_setstacksize( &attr, 8192 );

    pthread_create( &tcp_nettest_server_thread, &attr, tcp_nettest_server, NULL );
    pthread_create( &udp_nettest_server_thread, &attr, udp_nettest_server, NULL );

    pthread_join( tcp_nettest_server_thread, NULL );
    pthread_join( udp_nettest_server_thread, NULL );

    return 0;
}
