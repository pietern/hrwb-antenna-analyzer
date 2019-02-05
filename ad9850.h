#ifndef _AD9850_H
#define _AD9850_H

#include <stdint.h>

/*
 * Analog Devices AD9850 driver.
 *
 * D7 (Data Input) to digital pin 3 (PD0)
 * W_CLK (Word Load Clock) to digital pin 5 (PC6)
 * FQ_UD (Frequency Update) to digital pin 4 (PD4)
 * RESET (Reset) to digital pin 2 (PD1)
 */

void dds_init(void);

void dds_reset(void);

void dds_set_freq(uint32_t hz);

#endif
