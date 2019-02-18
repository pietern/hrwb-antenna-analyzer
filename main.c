#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "ad9850.h"
#include "hd44780u.h"
#include "task.h"

#define MIN(a, b) (((a) > (b)) ? (b) : (a));
#define MAX(a, b) (((a) < (b)) ? (b) : (a));

struct mode {
  PGM_P name;
};

const char mode_swr_min[] PROGMEM = "SWR min";
const char mode_band_start[] PROGMEM = "band start";
const char mode_band_stop[] PROGMEM = "band stop";
const char mode_band_mid[] PROGMEM = "band mid";
const char mode_band_edge[] PROGMEM = "band edge";

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
  {
    .name = mode_band_edge,
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

// The sweep task writes its output directly to this buffer.
char lcd_buffer[2][17];

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

// Returns if the state has changed and the previous state was true-ish.
// This is the equivalent of a key-up event.
int8_t button_check(uint8_t* prev, uint8_t cur) {
  int8_t ok = (cur != *prev && *prev);
  *prev = cur;
  return ok;
}

// Subtract two uint16_t values, taking overflows into account.
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
      time_tick = time_substr(task_msec(), 500);
    }

    // Refresh LCD if enough time has passed since previous refresh.
    if (idle && time_substr(task_msec(), time_tick) >= 500) {
      time_tick = task_msec();
      lcd_clear_display();
      lcd_setline(0);
      lcd_puts(lcd_buffer[0]);
      lcd_setline(1);
      lcd_puts(lcd_buffer[1]);
    }

    task_sleep(0);
  }
}

// Pointer to task waiting for ADC conversion.
task_t* task_adc = 0;

// Wake up task waiting for ADC conversion.
ISR(ADC_vect) {
  task_wakeup(task_adc);
}

uint16_t adc_sample(uint8_t port) {
  uint8_t adcl, adch;

  // Select port.
  ADMUX = _BV(REFS0) | port;

  // ADC Start Conversion.
  ADCSRA |= (_BV(ADSC) | _BV(ADIE));

  // Wait for ADC Conversion Complete interrupt.
  task_adc = task_current();
  task_suspend(0);

  // Load and combine result.
  adcl = ADCL;
  adch = ADCH;
  return (adch << 8) | adcl;
}

uint16_t vswr_sample() {
  uint32_t fwd;
  uint32_t rev;
  uint32_t vswr;

  // Forward power (channel A1, port ADC6).
  fwd = adc_sample(6);

  // Reverse power (channel A0, port ADC7).
  rev = adc_sample(7);

  // Compute integer VSWR (in thousandths).
  vswr = (1000 * (fwd + rev)) / (fwd - rev);
  if (vswr > 0xffff) {
    vswr = 0xffff;
  }

  return vswr;
}

uint16_t vswr_at_frequency(uint32_t hz, uint8_t delay) {
  dds_set_freq(hz);
  task_sleep(delay);
  return vswr_sample();
}

uint16_t avg_vswr_at_frequency(uint32_t hz, uint8_t n) {
  uint32_t sum = 0;
  uint8_t i;

  // Configure frequency and let settle.
  dds_set_freq(hz);
  task_sleep(20);

  // Take multiple measurements.
  for (i = 0; i < n; i++) {
    sum += vswr_sample();
  }

  // Compute average.
  return sum / n;
}

uint32_t round_step_size(uint32_t step_size) {
  uint32_t base = 1;
  while ((step_size / 10) > 0) {
    uint8_t remainder = step_size % 10;
    base *= 10;
    step_size /= 10;
    if (remainder) {
      step_size++;
    }
  }

  // Turn 600 into 1000.
  if (step_size > 5) {
    return base * 10;
  }

  // Turn 201 into 500.
  if (step_size > 2) {
    return base * 5;
  }

  // Turn 200 into 200, 100 into 100.
  return base * step_size;
}

void sweep_swr_min(struct band band) {
  uint16_t min_vswr = UINT16_MAX;
  uint32_t min_hz = 0;
  uint32_t start;
  uint32_t stop;
  uint32_t step_size;

  // Sweep entire band.
  //
  // Use zero delay when computing VSWR for a frequency. This will not
  // result in an accurate reading but is good enough to find a rough
  // frequency where the VSWR is minimal.
  //
  start = band.fa;
  stop = band.fb;
  step_size = round_step_size((stop - start) / 100);
  for (uint32_t hz = start; hz < stop; hz += step_size) {
    uint16_t vswr = vswr_at_frequency(hz, 0);
    if (vswr < min_vswr) {
      min_vswr = vswr;
      min_hz = hz;
    }
  }

  // Sweep minimum range.
  //
  // Use 10ms delay when computing VSWR for a frequency. This results
  // in a more accurate reading than before, because the power levels
  // are given a chance to settle before the ADC conversion.
  //
  start = min_hz - step_size;
  stop = min_hz + step_size;
  step_size = round_step_size((stop - start) / 20);
  for (uint32_t hz = start; hz < stop; hz += step_size) {
    uint16_t vswr = vswr_at_frequency(hz, 10);
    if (vswr < min_vswr) {
      min_vswr = vswr;
      min_hz = hz;
    }
  }

  snprintf(
    lcd_buffer[0],
    sizeof(lcd_buffer[0]),
    "Freq: %2lu.%06lu",
    min_hz / 1000000,
    min_hz % 1000000);
  snprintf(
    lcd_buffer[1],
    sizeof(lcd_buffer[1]),
    "SWR: %2u.%03u",
    min_vswr / 1000,
    min_vswr % 1000);
}

void sweep_band_position(struct band band, uint32_t hz) {
  uint16_t vswr;

  vswr = avg_vswr_at_frequency(hz, 20);

  snprintf(
    lcd_buffer[0],
    sizeof(lcd_buffer[0]),
    "Freq: %2lu.%06lu",
    hz / 1000000,
    hz % 1000000);
  snprintf(
    lcd_buffer[1],
    sizeof(lcd_buffer[1]),
    "SWR: %2u.%03u",
    vswr / 1000,
    vswr % 1000);
}

void sweep_band_edges(struct band band) {
  uint16_t low;
  uint16_t mid;
  uint16_t high;

  low = avg_vswr_at_frequency(band.start, 20);
  low = MIN(9999, low);
  mid = avg_vswr_at_frequency((band.start + band.stop) / 2, 20);
  mid = MIN(9999, mid);
  high = avg_vswr_at_frequency(band.stop, 20);
  high = MIN(9999, high);

  snprintf(
    lcd_buffer[0],
    sizeof(lcd_buffer[0]),
    "A     B     C");
  snprintf(
    lcd_buffer[1],
    sizeof(lcd_buffer[1]),
    "%1u.%2u  %1u.%2u  %1u.%2u",
    (low / 1000),
    (low % 1000) / 10,
    (mid / 1000),
    (mid % 1000) / 10,
    (high / 1000),
    (high % 1000) / 10);
}

void sweep_task(void* unused) {
  // ADC Enable.
  ADCSRA = _BV(ADEN);

  // Prescaler at 128 to turn 16 MHz into 125 KHz.
  ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);

  // Ports ADC6 and ADC7 are inputs.
  DDRF &= ~_BV(DDF7);
  DDRF &= ~_BV(DDF6);
  PORTF &= ~_BV(PF7);
  PORTF &= ~_BV(PF6);

  dds_init();
  dds_reset();

  while (1) {
    switch (mode_index) {
    case 0:
      sweep_swr_min(band_cur);
      break;
    case 1:
      sweep_band_position(band_cur, band_cur.start);
      break;
    case 2:
      sweep_band_position(band_cur, band_cur.stop);
      break;
    case 3:
      sweep_band_position(band_cur, (band_cur.start + band_cur.stop) / 2);
      break;
    case 4:
      sweep_band_edges(band_cur);
      break;
    }
    task_yield();
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
  task_create(sweep_task, 0);
  task_start();
}
