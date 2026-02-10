#include "ota_manager.h"
#include <semaphore.h>

#define MINUTES( x ) ( ( x ) * 60 )

static volatile uint8_t ota_in_progress = 0;
sem_t ota_complete_semaphore;
extern volatile bool lcd_on;
static void run_kernel_update( void )
{
    binary_update_same_version_test();
    return;
}

uint8_t is_ota_in_progress( void )
{
    return ota_in_progress == 1 ? 1 : 0;
}

int ota_manager_runnable( int argc, char *argv[] )
{
    wait_for_wifi();
    sem_init(&ota_complete_semaphore, 0, 0);

while (1) {
    /* Drain semaphore */
    while (sem_trywait(&ota_complete_semaphore) == 0);

    ota_in_progress = 1;
    sleep(10); // Give some time to prepare for OTA
    power_test(100);
    lcd_on = true;
    printf("OTN START......\n");
    run_kernel_update();
    printf("OTN DONE......\n");
    ota_in_progress = 0;
    power_test(0);
    lcd_on = false;
    sem_post(&ota_complete_semaphore);

    sleep(MINUTES(15));

}
    
    return 0;
}
