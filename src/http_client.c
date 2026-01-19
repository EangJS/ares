#include <protocols/webclient.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define REQUEST_URL "http://example.com/"
#define REQUEST_BUFFER_SIZE ( 1024 * 4 )

int http_client( void )
{
    struct http_client_request_t request;
    struct http_keyvalue_list_t headers;
    struct http_client_response_t response;
    struct http_client_ssl_config_t *ssl_config = NULL;

    memset( &request, 0, sizeof( request ) );
    request.method = WGET_MODE_GET;
    request.url = REQUEST_URL;
    request.buflen = REQUEST_BUFFER_SIZE;
    request.encoding = CONTENT_LENGTH;

    http_keyvalue_list_init( &headers );
    request.headers = &headers;
    if ( http_client_response_init( &response ) < 0 )
    {
        printf( "fail to response init\n" );
        goto release_out;
    }
    if ( http_client_send_request( &request, ssl_config, &response ) )
    {
        printf( "fail to send request\n" );
        goto release_out;
    }
    printf( "Response Status: %d\n", response.status );
    printf( "%s\n", response.entity );

release_out:
    http_client_response_release( &response );
    http_keyvalue_list_release( &headers );

    return NULL;
}
