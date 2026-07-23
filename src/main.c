#include <libdragon.h>

#include "boot/boot.h"
#include "menu/menu.h"


int main (void) {
    boot_params_t boot_params;

    debugf("Main: entering menu_run\n");
    menu_run(&boot_params);
    debugf("Main: menu_run returned\n");

    disable_interrupts();

    debugf("Main: calling boot()\n");
    boot(&boot_params);

    assertf(false, "Unexpected return from 'boot' function");

    while (true) {
        // Shouldn't get here
    }
}
