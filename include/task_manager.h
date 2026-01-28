#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <tinyara/config.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>

#include "task_define.h"
#include "sound_runner.h"
#include "lcd_runner.h"
#include "wifi_runner.h"
#include "fs_runner.h"
#include "uart_runner.h"
#include "pm_runner.h"
#include "monitor.h"
#include "lcd_drawer.h"
#include "netstress_manager.h"

/*============================================================
 *  Public Function Prototypes
 *============================================================*/

/**
 * @brief Run all defined tasks
 *
 * This function creates and starts all tasks defined in the task table.
 */
int run_tasks( int argc, char *argv[] );

#ifdef __cplusplus
}
#endif

#endif /* TASK_MANAGER_H */
