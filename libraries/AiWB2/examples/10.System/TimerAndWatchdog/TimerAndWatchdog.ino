/*
  TimerAndWatchdog — hardware timer + watchdog + RTC + chip info.

  timer_begin(period_us, cb) fires cb every period. cb runs in ISR context:
  keep it short, no Serial/delay inside.
  watchdogEnable(ms) arms the BL602 WDT — call watchdogFeed() before the
  timeout or the chip resets. rtc_set_time()/rtc_get_time() keep a Unix
  timestamp (keeps counting across resets while its clock source is alive).
*/
#include <Arduino.h>

volatile uint32_t ticks = 0;
void onTimer(void) {
  ticks++;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.printf("chip id 0x%08lX  cpu %lu MHz  heap %lu\n",
                (unsigned long)esp_get_chip_id(),
                (unsigned long)getCpuFrequencyMhz(),
                (unsigned long)esp_get_free_heap_size());

  rtc_set_time(1700000000UL);        /* seed the RTC (Unix epoch) */
  timer_begin(1000000, onTimer);     /* 1 s periodic tick */
  timer_start();
  watchdogEnable(5000);              /* must feed within 5 s */
}

void loop() {
  watchdogFeed();                    /* pet the dog */
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.printf("ticks=%lu  rtc=%lu\n",
                  (unsigned long)ticks, (unsigned long)rtc_get_time());
  }
  delay(10);
}
