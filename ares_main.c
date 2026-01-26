#include "task_manager.h"
#include <stdio.h>
#include <tinyara/config.h>

/* ******************************************************************************* */
/*                           Public Function Defnitions                            */
/* ******************************************************************************* */

#ifdef CONFIG_BUILD_KERNEL
int main( int argc, FAR char *argv[] )
#else
int ares_main( int argc, char *argv[] )
#endif
{
    task_create( "Ares", 254, 2048, run_tasks, NULL );
    return 0;
}
