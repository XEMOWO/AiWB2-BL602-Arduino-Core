/*
 * main.c — SDK boot entry for the Arduino core.
 *
 * The Ai-Thinker SDK's bfl_main() (bl602 component) runs main() as a FreeRTOS
 * task once the scheduler is up. We forward straight into arduino_main()
 * (defined in main.cpp), which spawns the dedicated setup()/loop() task.
 *
 * Living in the core (not the sketch/variant) means both the SDK-make path and
 * the Arduino-IDE path get their main() from the same place — like ESP32's core.
 * SDK test apps therefore must NOT define their own main().
 */
extern void arduino_main(void);

void main(void)
{
    arduino_main();
}
