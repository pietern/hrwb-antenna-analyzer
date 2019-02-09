#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "hd44780u.h"
#include "task.h"

struct mode {
  PGM_P name;
};

const char mode_swr_min[] PROGMEM = "SWR min";
const char mode_band_start[] PROGMEM = "band start";
const char mode_band_stop[] PROGMEM = "band stop";
const char mode_band_mid[] PROGMEM = "band mid";

const struct mode modes[] PROGMEM = {
  {
    .name = mode_swr_min,
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

  // Start/stop of band
  uint32_t start;
  uint32_t stop;
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
    .start = 1600000,
    .stop = 2000000,
  },
  {
    .name = band_80m,
    .fa = 2000000,
    .fb = 5000000,
    .start = 3500000,
    .stop = 4000000,
  },
  {
    .name = band_60m,
    .fa = 5000000,
    .fb = 6000000,
    .start = 5332000,
    .stop = 5405000,
  },
  {
    .name = band_40m,
    .fa = 6000000,
    .fb = 8000000,
    .start = 7000000,
    .stop = 7300000,
  },
  {
    .name = band_30m,
    .fa = 9000000,
    .fb = 11000000,
    .start = 10100000,
    .stop = 10150000,
  },
  {
    .name = band_20m,
    .fa = 13000000,
    .fb = 16000000,
    .start = 14000000,
    .stop = 14350000,
  },
  {
    .name = band_17m,
    .fa = 17000000,
    .fb = 19000000,
    .start = 18068000,
    .stop = 18168000,
  },
  {
    .name = band_15m,
    .fa = 20000000,
    .fb = 23000000,
    .start = 21000000,
    .stop = 21450000,
  },
  {
    .name = band_12m,
    .fa = 24000000,
    .fb = 26000000,
    .start = 24890000,
    .stop = 24990000,
  },
  {
    .name = band_10m,
    .fa = 28000000,
    .fb = 30000000,
    .start = 28000000,
    .stop = 29700000,
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
  int8_t ok = (cur != *prev && *prev);
  *prev = cur;
  return ok;
}

uint16_t time_substr(uint16_t t2, uint16_t t1) {
  if (t2 > t1) {
    return t2 - t1;
  } else {
    return t2 + (UINT16_MAX - t1) + 1;
  }
}

void control_task(void* unused) {
  lcd_init();

  uint16_t time_button = task_msec();
  uint16_t time_tick;
  uint8_t idle = 0;
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

    if (!idle && mode_button) {
      cli();
      mode_index = (mode_index + 1) % (sizeof(modes) / sizeof(modes[0]));
      memcpy_P(&mode_cur, &modes[mode_index], sizeof(struct mode));
      sei();
    }

    if (!idle && band_button) {
      cli();
      band_index = (band_index + 1) % (sizeof(bands) / sizeof(bands[0]));
      memcpy_P(&band_cur, &bands[band_index], sizeof(struct band));
      sei();
    }

    // Refresh LCD with mode/band selection if any button was pressed.
    if (mode_button || band_button) {
      lcd_show_mode_band();
      time_button = task_msec();
      idle = 0;
    }

    // Switch to idle mode when last button press is >= 1 second ago.
    if (!idle && time_substr(task_msec(), time_button) >= 1000) {
      idle = 1;
      time_tick = time_substr(task_msec(), 250);
    }

    // Refresh LCD if enough time has passed since previous refresh.
    if (idle && time_substr(task_msec(), time_tick) >= 250) {
      time_tick = task_msec();
      lcd_clear_display();
      lcd_puts("Idle!");
    }

    task_sleep(0);
  }
}

void setup() {
  // Turn off Pro Micro RX LED.
  DDRB |= _BV(PB0);
  PORTB |= _BV(PB0);

  // Turn off Pro Micro TX LED.
  DDRD |= _BV(PD5);
  PORTD |= _BV(PD5);

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
