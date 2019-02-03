#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "hd44780u.h"
#include "task.h"

struct mode {
  PGM_P name;
};

const char mode_swrmin[] PROGMEM = "SWR min";
const char mode_swr15[] PROGMEM = "SWR <=1.5";
const char mode_swr20[] PROGMEM = "SWR <=2.0";
const char mode_band_start[] PROGMEM = "band start";
const char mode_band_stop[] PROGMEM = "band stop";
const char mode_band_mid[] PROGMEM = "band mid";

const struct mode modes[] PROGMEM = {
  {
    .name = mode_swrmin,
  },
  {
    .name = mode_swr15,
  },
  {
    .name = mode_swr20,
  },
  {
    .name = mode_band_start,
  },
  {
    .name = mode_band_stop,
  },
  {
    .name = mode_band_mid,
  },
};

struct band {
  PGM_P name;

  // Start/stop of sweep
  uint32_t fa;
  uint32_t fb;
};

const char band_160m[] PROGMEM = "160m";
const char band_80m[] PROGMEM = "80m";
const char band_60m[] PROGMEM = "60m";
const char band_40m[] PROGMEM = "40m";
const char band_30m[] PROGMEM = "30m";
const char band_20m[] PROGMEM = "20m";
const char band_17m[] PROGMEM = "17m";
const char band_15m[] PROGMEM = "15m";
const char band_12m[] PROGMEM = "12m";
const char band_10m[] PROGMEM = "10m";

const struct band bands[] PROGMEM = {
  {
    .name = band_160m,
    .fa = 1500000,
    .fb = 2300000,
  },
  {
    .name = band_80m,
    .fa = 2000000,
    .fb = 5000000,
  },
  {
    .name = band_60m,
    .fa = 5000000,
    .fb = 6000000,
  },
  {
    .name = band_40m,
    .fa = 6000000,
    .fb = 8000000,
  },
  {
    .name = band_30m,
    .fa = 9000000,
    .fb = 11000000,
  },
  {
    .name = band_20m,
    .fa = 13000000,
    .fb = 16000000,
  },
  {
    .name = band_17m,
    .fa = 17000000,
    .fb = 19000000,
  },
  {
    .name = band_15m,
    .fa = 20000000,
    .fb = 23000000,
  },
  {
    .name = band_12m,
    .fa = 24000000,
    .fb = 26000000,
  },
  {
    .name = band_10m,
    .fa = 28000000,
    .fb = 30000000,
  },
};

// Current mode.
uint8_t mode_index = 0;
struct band mode_cur;

// Current band.
uint8_t band_index = 0;
struct band band_cur;

// Show mode/band selection on LCD display.
void lcd_show_mode_band() {
  lcd_clear_display();
  lcd_setline(0);
  lcd_puts("Mode: ");
  lcd_puts_P(mode_cur.name);
  lcd_setline(1);
  lcd_puts("Band: ");
  lcd_puts_P(band_cur.name);
}

// Returns if the state has changed and the new state is true-ish.
// This is the equivalent of a keydown event.
int8_t button_check(uint8_t* prev, uint8_t cur) {
  int8_t ok = (cur != *prev && cur);
  *prev = cur;
  return ok;
}

void control_task(void* unused) {
  lcd_init();

  uint8_t mode_button_prev = 0;
  uint8_t band_button_prev = 0;
  memcpy_P(&mode_cur, &modes[mode_index], sizeof(struct mode));
  memcpy_P(&band_cur, &bands[band_index], sizeof(struct band));
  lcd_show_mode_band();

  while (1) {
    // XOR to negate. Pulled low means it is pressed.
    uint8_t buttons = ~PINF;
    int8_t mode_button = button_check(&mode_button_prev, buttons & _BV(PINF5));
    int8_t band_button = button_check(&band_button_prev, buttons & _BV(PINF4));

    if (mode_button) {
      cli();
      mode_index = (mode_index + 1) % (sizeof(modes) / sizeof(modes[0]));
      memcpy_P(&mode_cur, &modes[mode_index], sizeof(struct mode));
      sei();
    }

    if (band_button) {
      cli();
      band_index = (band_index + 1) % (sizeof(bands) / sizeof(bands[0]));
      memcpy_P(&band_cur, &bands[band_index], sizeof(struct band));
      sei();
    }

    if (mode_button || band_button) {
      lcd_show_mode_band();
    }

    task_sleep(0);
  }
}

void setup() {
  DDRB = _BV(PB0); // Pro Micro RX LED
  DDRD = _BV(PD5); // Pro Micro TX LED

  // Mode button (PF5) is an input.
  DDRF &= ~_BV(DDF5);

  // Band button (PF4) is an input.
  DDRF &= ~_BV(DDF4);
}

int main() {
  setup();
  task_init();
  task_create(control_task, 0);
  task_start();
}
