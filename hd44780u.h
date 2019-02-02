#ifndef _HD44780U_H
#define _HD44780U_H

#include <stdint.h>

/*
 * Hitachi HD44780U LCD controller driver.
 *
 * LCD RS pin to digital pin 8 (PB4)
 * LCD Enable pin to digital pin 9 (PB5)
 * LCD D4 pin to digital pin 15 (PB1)
 * LCD D5 pin to digital pin 14 (PB3)
 * LCD D6 pin to digital pin 16 (PB2)
 * LCD D7 pin to digital pin 10 (PB6)
 */

// Initialize LCD.
void lcd_init(void);

// Clear LCD display.
void lcd_clear_display(void);

// Return cursor to origin of LCD.
void lcd_return_home(void);

// Set cursor to beginning of line.
void lcd_setline(int8_t line);

// Write character to LCD.
void lcd_putc(char c);

// Write buffer to LCD.
void lcd_puts(const char *buf);

// Write PGM buffer to LCD.
void lcd_puts_P(const char *buf);

#endif
