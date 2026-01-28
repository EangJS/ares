#include "task_manager.h"

/* ******************************************************************************* */
/*                           Macro Defnitions                                      */
/* ******************************************************************************* */

#define TASK_COUNT ( sizeof( task_table ) / sizeof( task_table[ 0 ] ) )
#define TASK_DEFINE( name, priority, stack_size, run, arg, cpu ) { name, priority, stack_size, run, arg, cpu },
#define CPU0_AFFINITY 0
#define CPU1_AFFINITY 1

/* ******************************************************************************* */
/*                           Private Variable Defnitions                           */
/* ******************************************************************************* */

static char *fs_mnt_argv[] = { MNT_PATH, NULL };
static char *fs_mnt0_argv[] = { MNT0_PATH, NULL };
static const task_t task_table[] = {
#ifdef CONFIG_LCD
    TASK_DEFINE( "lvgl_tick"  , SCHED_PRIORITY_DEFAULT + 10, 1024 , task_lvgl_tick             , NULL        , CPU1_AFFINITY )
    TASK_DEFINE( "lvgl_drawer", SCHED_PRIORITY_DEFAULT + 9 , 65536, task_draw_lcd              , NULL        , CPU1_AFFINITY )
    TASK_DEFINE( "lcd_power"  , SCHED_PRIORITY_DEFAULT     , 4096 , task_power_lcd             , NULL        , CPU0_AFFINITY )
#endif /* CONFIG_LCD */
    TASK_DEFINE( "wifi"       , SCHED_PRIORITY_DEFAULT     , 16384, wifi_runnable              , NULL        , CPU0_AFFINITY )
    TASK_DEFINE( "fs_mnt"     , SCHED_PRIORITY_DEFAULT     , 8192 , fs_task_main               , fs_mnt_argv , CPU0_AFFINITY )
    TASK_DEFINE( "fs_mnt0"    , SCHED_PRIORITY_DEFAULT     , 8192 , fs_task_main               , fs_mnt0_argv, CPU0_AFFINITY )
    TASK_DEFINE( "uart_rx"    , SCHED_PRIORITY_DEFAULT     , 8192 , uart_runnable              , NULL        , CPU0_AFFINITY )
    TASK_DEFINE( "power"      , SCHED_PRIORITY_DEFAULT     , 4096 , task_start_power_management, NULL        , CPU0_AFFINITY )
    TASK_DEFINE( "monitor"    , SCHED_PRIORITY_DEFAULT     , 8192 , monitor_task               , NULL        , CPU0_AFFINITY )
    TASK_DEFINE( "audio"      , SCHED_PRIORITY_DEFAULT     , 65536, task_play_sound            , NULL        , CPU0_AFFINITY )
    TASK_DEFINE( "netstress"  , SCHED_PRIORITY_DEFAULT     , 65536, task_start_netstress       , NULL        , CPU0_AFFINITY )
};

/* ******************************************************************************* */
/*                           Private Function Declarations                         */
/* ******************************************************************************* */

static void set_affinity( int cpu );

/* ******************************************************************************* */
/*                           Private Function Defnitions                           */
/* ******************************************************************************* */

static void set_affinity( int cpu )
{
    cpu_set_t cpu_set;

    CPU_ZERO( &cpu_set );
    CPU_SET( cpu, &cpu_set );
    if ( sched_setaffinity( 0, sizeof( cpu_set_t ), &cpu_set ) != 0 )
    {
        return;
    }
}

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

int run_tasks( int argc, char *argv[] )
{
    for ( size_t i = 0; i < TASK_COUNT; i++ )
    {
        set_affinity( task_table[ i ].cpu_affinity );
        int pid = task_create( task_table[ i ].name,
                               task_table[ i ].priority,
                               task_table[ i ].stack_size,
                               task_table[ i ].run,
                               task_table[ i ].arg );
        if ( pid < 0 )
        {
            return -1;
        }
        printf( "Running (PID:%d) %s (prio=%d)\n", pid, task_table[ i ].name, task_table[ i ].priority );
    }
    return 0;
}
