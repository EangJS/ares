#include <stdio.h>
#include <tinyara/config.h>

#include "lcd_runner.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <tinyara/config.h>
#include <tinyara/lcd/lcd_dev.h>
#include <tinyara/rtc.h>

#define LCD_DEV_PATH "/dev/lcd%d"
#define LCD_DEV_PORT 0

#define RED 0xF800
#define WHITE 0xFFFF
#define BLACK 0x0000
#define GREEN 0x07E0
#define BLUE 0x00F8
#define SIZE 40

#define COLINDEX 10
#define ROWINDEX 10
#define NOPIXELS 200
static int xres;
static int yres;
static bool g_terminate;

#ifdef CONFIG_EXAMPLE_LCD_FPS_TEST
#define EXAMPLE_LCD_FPS_TEST CONFIG_EXAMPLE_LCD_FPS_TEST
#else
#define EXAMPLE_LCD_FPS_TEST 5000
#endif
// RGB565 helper macro
#define RGB565( r, g, b )                                                      \
    ( ( ( r & 0x1F ) << 11 ) | ( ( g & 0x3F ) << 5 ) | ( b & 0x1F ) )

static int prepare_frame_buffer( struct lcddev_area_s *area,
                                 uint16_t color,
                                 int xres,
                                 int yres )
{
    size_t len;
    len = xres * yres * 2 + 1;
    uint8_t *lcd_data = (uint8_t *)malloc( len );
    if ( lcd_data == NULL )
    {
        printf( "malloc failed for lcd data : %d\n", len );
        return ERROR;
    }

    for ( int i = 0; i < len - 1; i += 2 )
    {
        lcd_data[ i + 1 ] = ( color & 0xFF00 ) >> 8;
        lcd_data[ i ] = color & 0x00FF;
    }

    area->planeno = 0;
    area->row_start = 0;
    area->row_end = yres - 1;
    area->col_start = 0;
    area->col_end = xres - 1;
    area->stride = 2 * xres;
    area->data = lcd_data;

    return OK;
}

static void release_frame_buffer( struct lcddev_area_s *area )
{
    free( area->data );
}

static void frame_change_test( void )
{
    int fd = 0;
    int ret;
    char port[ 20 ] = { '\0' };
    struct lcddev_area_s area_red;
    struct lcddev_area_s area_blue;
    struct fb_videoinfo_s vinfo;
    snprintf( port,
              sizeof( port ) / sizeof( port[ 0 ] ),
              LCD_DEV_PATH,
              LCD_DEV_PORT );
    fd = open( port, O_RDWR | O_SYNC, 0666 );
    if ( fd < 0 )
    {
        printf( "ERROR: STRESS TEST, Failed to open lcd port : %s error:%d\n",
                port,
                get_errno() );
        return;
    }
    if ( ioctl( fd, LCDDEVIO_GETVIDEOINFO, (unsigned long)(uintptr_t)&vinfo ) <
         0 )
    {
        printf( "Fail to call LCD GETVIDEOINFO(errno %d)", get_errno() );
        close( fd );
        return;
    }
    xres = vinfo.xres;
    yres = vinfo.yres;
    ret = prepare_frame_buffer( &area_red, RED, xres, yres );
    if ( ret != OK )
    {
        printf( "ERROR: prepare_frame_buffer failed\n" );
        close( fd );
        return;
    }
    ret = prepare_frame_buffer( &area_blue, BLUE, xres, yres );
    if ( ret != OK )
    {
        printf( "ERROR: prepare_frame_buffer failed\n" );
        close( fd );
        release_frame_buffer( &area_red );
        return;
    }

    bool is_red = true;
    for ( int i = 0; i < 64; i++ )
    {
        if ( is_red )
        {
            if ( ioctl( fd,
                        LCDDEVIO_PUTAREA,
                        (unsigned long)(uintptr_t)&area_red ) < 0 )
            {
                printf( "ERROR: PUTAREA ioctl failed, errno: %d\n",
                        get_errno() );
                goto cleanup;
            }
            is_red = false;
        }
        else
        {
            if ( ioctl( fd,
                        LCDDEVIO_PUTAREA,
                        (unsigned long)(uintptr_t)&area_blue ) < 0 )
            {
                printf( "ERROR: PUTAREA ioctl failed, errno: %d\n",
                        get_errno() );
                goto cleanup;
            }
            is_red = true;
        }
        usleep( 100000 ); /* Sleep for 100ms */
    }
cleanup:
    close( fd );
    release_frame_buffer( &area_red );
    release_frame_buffer( &area_blue );
}

void set_lcd_power( int power )
{
    power_test( power );
}

int task_display_lcd( int argc, char *argv[] )
{
    while ( 1 )
    {
        frame_change_test();
        sleep( 5 );
        set_lcd_power( 0 );
        sleep( 5 );
        set_lcd_power( 100 );
        sleep( 3 );
    }
}