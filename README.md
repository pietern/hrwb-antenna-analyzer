# Antenna analyzer firmware

Firmware for the [K6BEZ Antenna Analyzer][1] (also see [this page][2]).

This board uses the ATmega32U4 microcontroller.

The firmware uses [avr-tasks][3] to run separate tasks for user
interaction (buttons and the LCD) and measurements.

## Why

The original firmware is not very responsive to button presses (it can take
many seconds before there is feedback to pressing a button) and
doesn't hook up the mode button. I wanted to fix these issues, learn
about the AD9850 chip, and have some fun hacking it all up.

## Usage

Run `make` to build. Use `avrdude` to flash the firmware.

If you have the ATmega32U4 connected to an Atmel-ICE programmer:
(beware, this will overwrite everything, including any boot loader you
may be using):

``` shell
avrdude -p atmega32u4 -c atmelice_isp -P usb -U flash:w:main.hex
```

If you have a boot loader installed on your ATmega32U4 (e.g. the one
from Sparkfun), it likely uses the `avr109` protocol, and you can run:

``` shell
avrdude -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U flash:w:main.hex
```

[1]: https://github.com/HamRadio360/Antenna-Analyzer
[2]: https://www.hamradioworkbench.com/k6bez-antenna-analyzer.html
[3]: https://github.com/pietern/avr-tasks
