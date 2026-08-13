/*
 * main.cpp — Arduino entry glue.
 *
 * The SDK calls main() (as a FreeRTOS task, see bfl_main.c), which calls
 * arduino_main(). We spawn a dedicated task that runs setup() once and then
 * loop() forever, so the user sketch gets its own stack budget.
 */
#include <FreeRTOS.h>
#include <task.h>

#include "Arduino.h"
#include "Schedule.h"

#ifndef ARDUINO_LOOP_TASK_STACK
#define ARDUINO_LOOP_TASK_STACK 4096
#endif
#ifndef ARDUINO_LOOP_TASK_PRIO
#define ARDUINO_LOOP_TASK_PRIO 15
#endif

static void arduino_loop_task(void *arg)
{
    (void)arg;

    setup();
    for (;;) {
        loop();
        /* ESP8266: the core's loop() wrapper drains scheduled functions after
         * the sketch's loop() returns. Ticker/attach_scheduled callbacks land
         * here. */
        run_scheduled_functions();
        run_scheduled_recurrent_functions();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

extern "C" void arduino_main(void)
{
    xTaskCreate(arduino_loop_task, "arduinoLoop", ARDUINO_LOOP_TASK_STACK, NULL,
                ARDUINO_LOOP_TASK_PRIO, NULL);
    /* main() may return now; the loop task keeps running. */
}
