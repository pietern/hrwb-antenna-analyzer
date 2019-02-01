#include <avr/io.h>

#include "hd44780u.h"
#include "task.h"

typedef enum {
  INSTRUCTION = 0,
  DATA        = 1,
} mode_t;

static void lcd_yield_usec(int16_t usec) {
  uint16_t t1;
  uint16_t t2;
  uint16_t dt;

  t1 = task_usec();
  for (;;) {
    task_yield();
    t2 = task_usec();

    if (t2 > t1) {
      dt = t2 - t1;
    } else {
      dt = t2 + (UINT16_MAX - t1) + 1;
    }

    if (dt >= usec) {
      break;
    }

    usec -= dt;
    t1 = t2;
  }
}

// Return Nth bit.
#define BIT(v, n) (((v) >> (n)) & 0x1)

static void __write4(uint8_t b) {
  // Clear and set data pins.
  PORTB &= ~(_BV(PB6) | _BV(PB2) | _BV(PB3) | _BV(PB1));
  PORTB |= (BIT(b, 3) << PB6);
  PORTB |= (BIT(b, 2) << PB2);
  PORTB |= (BIT(b, 1) << PB3);
  PORTB |= (BIT(b, 0) << PB1);
  // Trigger rising edge on enable pin.
  PORTB &= ~_BV(PB5);
  lcd_yield_usec(1);
  PORTB |= _BV(PB5);
  lcd_yield_usec(1);
  PORTB &= ~_BV(PB5);
}

static void lcd_send(mode_t m, uint8_t b) {
  // Clear and set instruction bit.
  PORTB &= ~_BV(PB4);
  PORTB |= (m << PB4);
  // Write byte in 2 steps
  __write4(b >> 4);
  lcd_yield_usec(37);
  __write4(b >> 0);
  lcd_yield_usec(37);
}

void lcd_init(void) {
  // Configure all LCD related pins as output pins.
  DDRB |= 0b01111110;
  PORTB &= ~(_BV(PB4) | _BV(PB5) | _BV(PB6) | _BV(PB2) | _BV(PB3) | _BV(PB1));

  // Wait 10ms after power up.
  lcd_yield_usec(10000);

  // Initialize for 4-bit data transfer.
  __write4(0b0011);
  lcd_yield_usec(4100);
  __write4(0b0011);
  lcd_yield_usec(100);
  __write4(0b0011);
  lcd_yield_usec(37);
  __write4(0b0010);
  lcd_yield_usec(37);

  // Function set: 4-bit data, 2 display lines, 5x8 font
  lcd_send(INSTRUCTION, 0b00101000);
  lcd_yield_usec(37);

  // Display control: on, no cursor, no blink
  lcd_send(INSTRUCTION, 0b00001100);
  lcd_yield_usec(37);

  // Entry mode set: increment cursor by 1
  lcd_send(INSTRUCTION, 0b00000110);
  lcd_yield_usec(37);

  // Clear and initialize cursor
  lcd_clear_display();
  lcd_return_home();
}

void lcd_clear_display(void) {
  lcd_send(INSTRUCTION, 0b00000001);
  lcd_yield_usec(1520);
}

void lcd_return_home(void) {
  lcd_send(INSTRUCTION, 0b00000010);
  lcd_yield_usec(1520);
}

void lcd_putc(char c) {
  lcd_send(DATA, c);
  lcd_yield_usec(37);
}

void lcd_puts(const char *buf, int8_t len) {
  int8_t i;

  for (i = 0; i < len; i++) {
    lcd_send(DATA, buf[i]);
    lcd_yield_usec(37);
  }

  // The LCD advances to the next line after character 40.
  for (; i < 40; i++) {
    lcd_send(INSTRUCTION, 0b00010100);
    lcd_yield_usec(37);
  }
}
