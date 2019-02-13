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

## Bootloader

See the `./bootloader` directory for a modified version of the LUFA
USB CDC (Communications Device Class) bootloader. This implements the
`avr109` protocol for programming the processor. The only modification
is that it requires the **mode** and **band** buttons to be pressed to
enter the bootloader. If they are not, it will immediately jump to the
main program.

To flash it (this overwrites all flash memory):

``` shell
cd ./bootloader
make BootloaderCDC.hex
avrdude -p atmega32u4 -c atmelice_isp -P usb -U flash:w:BootloaderCDC.hex
```

Now you can run the bootloader by pressing reset **while pressing**
the mode and band buttons. The on-board LEDs will flash and a new USB
device will show up on your computer.

Then, to program the antenna analyzer firmware, run:

``` shell
make main.hex
avrdude -p atmega32u4 -c avr109 -P /dev/ttyACM0 -U flash:w:main.hex
```

Ensure that the `BOOTRST` is enabled. This makes the processor ALWAYS
run the bootloader at boot and is required if you're using the Arduino
Pro Micro form factor. Namely, this form factor doesn't have the
`HWBE` (Hardware Boot Enable) pin broken out, which can be used as an
alternative to force the processor to run the bootloader. If you don't
enable the `BOOTRST` fuse, the processor will never run the bootloader
again after being programmed once.

For fuse settings, you can use http://www.engbedded.com/fusecalc/ or
refer to the ATmega32U4 data sheet.

To read fuse settings:

``` shell
avrdude -p atmega32u4 -c atmelice_isp -P usb
```

To write fuse settings (for example):

``` shell
avrdude -p atmega32u4 -c atmelice_isp -P usb -U hfuse:w:0xd8:m
```
