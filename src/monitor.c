#include <errno.h>
#include <fcntl.h>

#include <tinyara/fs/ioctl.h>
#include <tinyara/mminfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <tinyara/config.h>
#if defined( CONFIG_BUILD_PROTECTED ) && defined( CONFIG_MM_KERNEL_HEAP )
#include <errno.h>
#include <fcntl.h>

#include <tinyara/fs/ioctl.h>
#include <tinyara/mminfo.h>
#endif

#define MAX( a, b ) ( ( a ) > ( b ) ? ( a ) : ( b ) )

static int get_heap_info( struct mallinfo *out )
{
    if ( !out )
    {
        return -1;
    }

    int fd = open( MMINFO_DRVPATH, O_RDONLY );
    if ( fd < 0 )
    {
        printf( "Open %s fail, errno %d\n", MMINFO_DRVPATH, errno );
        return -1;
    }

    int ret = ioctl( fd, MMINFOIOC_HEAP, (unsigned long)out );
    close( fd );

    if ( ret != OK )
    {
        printf( "Mminfo ioctl fail, ret %d\n", ret );
        return -1;
    }

    return 0;
}

int monitor_task( int argc, char *argv[] )
{
    struct mallinfo heap;
    uint32_t max_memory_used = 0;
    while ( 1 )
    {
        get_heap_info( &heap );
        max_memory_used = MAX( max_memory_used, heap.uordblks );
        printf( "Used Memory: %d/%d bytes MAX:%d\n",
                heap.uordblks,
                heap.arena,
                max_memory_used );
        sleep( 1 );
    }
}
