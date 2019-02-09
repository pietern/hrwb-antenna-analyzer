#include <avr/io.h>

#include "ad9850.h"
#include "task.h"

void dds_init(void) {
  // Configure pins as output pins
  DDRD |= _BV(PD0) | _BV(PD1) | _BV(PD4);
  DDRC |= _BV(PC6);
  // Zero all pins
  PORTD &= ~(_BV(PD0) | _BV(PD1) | _BV(PD4));
  PORTC &= ~(_BV(PC6));
}

void dds_reset(void) {
  // Toggle the reset pin.
  // Minimum RESET with is 5 CLKIN cycles per the datasheet.
  // CLKIN runs at 125 MHz, or ~8 cycles per processor cycle.
  PORTD |= _BV(PD1);
  PORTD &= ~_BV(PD1);
}

void dds_set_freq(uint32_t hz) {
  int i;

  // Scale goes from 0 to 125 MHz.
  uint32_t f = (hz * (uint64_t)UINT32_MAX) / 125000000;

  // Bit bang frequency number (LSB to MSB).
  for (i = 0; i < 32; i++) {
    PORTD = (PORTD & ~_BV(PD0)) | ((f & 0x1) << PD0);
    PORTC |= _BV(PC6);
    PORTC &= ~_BV(PC6);
    f >>= 1;
  }

  // Control bits, power down bit, and phase bits can all be zero.
  PORTD &= ~_BV(PD0);
  for (i = 0; i < 8; i++) {
    PORTC |= _BV(PC6);
    PORTC &= ~_BV(PC6);
  }

  // Toggle the frequency update pin.
  PORTD |= _BV(PD4);
  PORTD &= ~_BV(PD4);
}
