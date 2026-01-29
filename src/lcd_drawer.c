#include "lcd_drawer.h"
#include "lcd_runner.h"
#include "lvgl.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <tinyara/config.h>
#include <tinyara/lcd/lcd_dev.h>
#include <tinyara/rtc.h>
#include <tinyara/timer.h>

/* ******************************************************************************* */
/*                           Private Macro Defnitions                           */
/* ******************************************************************************* */

#define LCD_DEV_PATH "/dev/lcd%d"
#define LCD_DEV_PORT 0
#define LCD_W CONFIG_LCD_YRES
#define LCD_H CONFIG_LCD_XRES
#define BYTES_PER_PIXEL 2
#define IMG_NAME crabpower
#define RUNNING_STATE 0
#define OTA_STATE 1

/* ******************************************************************************* */
/*                           Public Variable Declarations                          */
/* ******************************************************************************* */

extern bool lcd_on;
extern mqd_t time_status_mq;

/* ******************************************************************************* */
/*                           Private Variable Declarations                         */
/* ******************************************************************************* */

static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
LV_IMG_DECLARE( IMG_NAME );
static int lcd_fd = -1;
static lv_display_t *disp;
static lv_obj_t *price_label = NULL;
static lv_anim_t* animation;


/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static void lcd_flush_cb( lv_display_t *display, const lv_area_t *area, uint8_t *px_map );
static void lv_hal_init( void );
static void lv_port_disp_init( void );
static void anim_x_cb( void *var, int32_t v );
static void anim_size_cb( void *var, int32_t v );
static void lcd_draw( void );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

static void lcd_flush_cb( lv_display_t *display, const lv_area_t *area, uint8_t *px_map )
{
    struct lcddev_area_s lcd_area;

    lcd_area.planeno = 0;
    lcd_area.row_start = area->y1;
    lcd_area.row_end = area->y2;
    lcd_area.col_start = area->x1;
    lcd_area.col_end = area->x2;
    lcd_area.stride = ( area->x2 - area->x1 + 1 ) * BYTES_PER_PIXEL;
    lcd_area.data = px_map;

    ioctl( lcd_fd, LCDDEVIO_PUTAREA, (unsigned long)(uintptr_t)&lcd_area );

    lv_display_flush_ready( display );
}

static void lv_hal_init( void )
{
    struct fb_videoinfo_s vinfo;
    char port[ 20 ];

    /* Open LCD device */
    snprintf( port, sizeof( port ), LCD_DEV_PATH, LCD_DEV_PORT );
    lcd_fd = open( port, O_RDWR | O_SYNC );
    if ( lcd_fd < 0 )
    {
        printf( "ERROR: Failed to open LCD: %d\n", get_errno() );
        return;
    }

    if ( ioctl( lcd_fd, LCDDEVIO_GETVIDEOINFO, (unsigned long)(uintptr_t)&vinfo ) < 0 )
    {
        printf( "ERROR: GETVIDEOINFO failed\n" );
        close( lcd_fd );
        lcd_fd = -1;
        return;
    }

    printf( "LCD resolution: %dx%d\n", vinfo.xres, vinfo.yres );
}

static void lv_port_disp_init( void )
{
    lv_init();

    disp = lv_display_create( LCD_W, LCD_H );

    size_t buf_pixels = LCD_W * LCD_H;
    buf1 = malloc( buf_pixels * sizeof( lv_color_t ) );
    buf2 = malloc( buf_pixels * sizeof( lv_color_t ) );
    if ( !buf1 || !buf2 )
    {
        printf( "LVGL buffer alloc failed\n" );
        return;
    }

    lv_display_set_buffers( disp, buf1, buf2, buf_pixels * sizeof( lv_color_t ), LV_DISPLAY_RENDER_MODE_DIRECT );

    lv_display_set_flush_cb( disp, lcd_flush_cb );
}

static void anim_x_cb( void *var, int32_t v )
{
    lv_obj_set_x( (lv_obj_t *)var, v );
}

static void anim_size_cb( void *var, int32_t v )
{
    lv_obj_set_size( (lv_obj_t *)var, v, v );
}

static void lcd_draw( void )
{
    // --- Animated Image ---
    lv_obj_t *img = lv_img_create( lv_scr_act() );
    lv_img_set_src( img, &IMG_NAME );
    lv_obj_align( img, LV_ALIGN_LEFT_MID, 10, 0 );

    static lv_anim_t a;
    lv_anim_init( &a );
    lv_anim_set_var( &a, img );
    lv_anim_set_exec_cb( &a, anim_x_cb );
    lv_coord_t start_x = 10;
    lv_coord_t end_x = LCD_W - lv_obj_get_width( img ) - 10;
    lv_anim_set_values( &a, start_x, end_x );
    lv_anim_set_time( &a, 2000 );
    lv_anim_set_path_cb( &a, lv_anim_path_linear );
    lv_anim_set_playback_time( &a, 2000 );
    lv_anim_set_repeat_count( &a, LV_ANIM_REPEAT_INFINITE );
    animation = lv_anim_start( &a );

    // --- Static Stock Price Label ---
    price_label = lv_label_create( lv_scr_act() );
    lv_obj_align( price_label, LV_ALIGN_TOP_RIGHT, -10, 10 );
    lv_obj_set_style_text_font( price_label, &lv_font_montserrat_14, 0 );
}

static void resume_animation( void )
{
    lv_anim_resume( animation );
}

static void pause_animation( void )
{
    lv_anim_pause( animation );
}

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

int task_draw_lcd( int argc, char *argv[] )
{
    set_lcd_power( 100 );
    lv_hal_init();
    lv_port_disp_init();
    lcd_draw();
    uint32_t i = 0;
    uint8_t current_state = RUNNING_STATE;

    while ( 1 )
    {
        if ( lcd_on )
        {
            lv_timer_handler();
        }
        if ( ( i % 200 ) == 0 )
        {
            if ( is_ota_in_progress() )
            {
                if ( current_state != OTA_STATE )
                {
                    current_state = OTA_STATE;
                    pause_animation();
                }
                lv_label_set_text( price_label, "OTA In Progress" );
                i++;
                sleep( 3 );
            }
            else
            {
                if ( current_state != RUNNING_STATE )
                {
                    current_state = RUNNING_STATE;
                    resume_animation();
                }
                char *time_str;
                if ( mq_receive( time_status_mq, (char *)&time_str, sizeof( time_str ), NULL ) > 0 )
                {
                    lv_label_set_text( price_label, time_str );
                    free( time_str );
                }
            }
        }
        i++;

        usleep( 5000 );
    }

    return 0;
}

int lvgl_tick_timer_init( void )
{
    int fd = open( "/dev/timer0", O_RDONLY );
    if ( fd < 0 )
    {
        printf( "Failed to open /dev/timer0\n" );
        return -1;
    }

    struct timer_notify_s notify;
    notify.arg = NULL;
    notify.pid = (pid_t)getpid();

    if ( ioctl( fd, TCIOC_NOTIFICATION, &notify ) < 0 )
    {
        printf( "Failed to set timer notification\n" );
        close( fd );
        return -1;
    }

    int period_us = 5000;
    if ( ioctl( fd, TCIOC_SETTIMEOUT, (unsigned long)period_us ) < 0 )
    {
        printf( "Failed to set timer interval\n" );
        close( fd );
        return -1;
    }

    if ( ioctl( fd, TCIOC_START, 0 ) < 0 )
    {
        printf( "Failed to start timer\n" );
        close( fd );
        return -1;
    }
    return fd;
}

int task_lvgl_tick( int argc, char *argv[] )
{
    int fd = lvgl_tick_timer_init();
    while ( 1 )
    {
        fin_wait();
        lv_tick_inc( 5 );
    }
    /* Should never reach here */
    ioctl( fd, TCIOC_STOP, 0 );
    close( fd );
    return 0;
}
