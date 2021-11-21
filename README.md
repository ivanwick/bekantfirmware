# BEKANT Firmware

![Position memory diagram](https://github.com/ivanwick/bekantfirmware/wiki/images/diagram.png)

Control firmware for IKEA BEKANT adjustable-height desk with lower and upper memory positions. Can be flashed onto the OEM controller, without changing any hardware.

Pre-program a sitting and standing position for your desk and move between them with a single button press.

## Build & Flash

Downloadable firmware images are built in MPLAB X with the XC8 compiler. The microcontroller (PIC16LF1938) can be programmed through ICSP using a PICKit or other PIC programmer.

[Latest Release](https://github.com/ivanwick/bekantfirmware/releases/latest)

## Documentation

[Project Wiki](https://github.com/ivanwick/bekantfirmware/wiki/)

[Installation Guide](https://github.com/ivanwick/bekantfirmware/wiki/Installation-Guide)

## User's Guide

| Gesture | Action |
| ------- | ------ |
| <kbd>△</kbd> | Move up  |
| <kbd>△</kbd> + <kbd>▽</kbd> | Move up to upper memory position  |
| <kbd>▽</kbd> | Move down |
| <kbd>▽</kbd> + <kbd>△</kbd> | Move down to lower memory position  |
| <kbd>△</kbd><kbd>▽</kbd> (hold 3 sec) | Save current position |

"+" means press the first button, and while holding it, press the second button, then release both. Just like <kbd>Ctrl</kbd> + <kbd>X</kbd>.

Automatic movement to a stored position can be canceled by pressing either button or cutting power.

For the *Save* gesture, press both buttons at the same time and hold for 3 seconds until the legs click. The firmware will overwrite either the lower or upper stored position, whichever is closer to the current position. The table should not be moving during the hold, otherwise one of the other gestures was triggered. Both buttons must be pressed within the software debounce interval.

## Memory Positions

Memory positions are stored in the PIC EEPROM. These are 16-bit little-endian integers for the encoder values of the motorized table legs.

| Offset | Length | Default | Description |
| ------ | ------ | ------- | ----------- |
| 0x00   | 2 bytes | 0x0636 | Lower position encoder value, default about 70cm |
| 0x02   | 2 bytes | 0x1600 | Upper position encoder value, default about 110cm |

## Disclaimer

Use at your own risk.

The real reason that position memory is not built into the stock firmware is probably for safety and liability. Requiring a human to hold a button while the table is moving keeps a human in the control loop.

Ensure that the path of the table is clear during all movement.

## References
 1. <a name="1">https://web.archive.org/web/20190116092248/https://blog.rnix.de/hacking-ikea-bekant/</a>
 2. <a name="2">https://github.com/trainman419/bekant</a>
 3. <a name="3">https://github.com/robin7331/IKEA-Hackant</a>
 4. <a name="4">https://github.com/diodenschein/TableMem</a>
 5. <a name="5">https://en.wikipedia.org/wiki/Local_Interconnect_Network</a>
