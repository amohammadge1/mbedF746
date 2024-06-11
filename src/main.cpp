#include <mbed.h>
#include <threadLvgl.h>
#include <cstdio>
#include "lvgl.h"
#include "simon_game.h"

// Thread for LVGL
ThreadLvgl threadLvgl(30);

int main() {
    // Initialize LVGL
    threadLvgl.lock();
    lv_init(); // Ensure LVGL is initialized
    // Initialize display and input drivers here if necessary
    threadLvgl.unlock();

    // Start the Simon game
    app_main();

    // Main loop
    while (1) {
        // Update LVGL
        threadLvgl.lock();
        lv_timer_handler();
        threadLvgl.unlock();

        // Sleep for a short period to allow other tasks to run
        ThisThread::sleep_for(5ms);
    }
}
