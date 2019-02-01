#include <avr/io.h>

#include "task.h"

// Toggle both LEDs every 500ms
void blink_task(void* unused) {
  while (1) {
    task_sleep(500);
    PORTB ^= _BV(PB0);
    PORTD ^= _BV(PD5);
  }
}

void setup() {
  DDRB = _BV(PB0); // Pro Micro RX LED
  DDRD = _BV(PD5); // Pro Micro TX LED
}

int main() {
  setup();
  task_init();
  task_create(blink_task, 0);
  task_start();
}
