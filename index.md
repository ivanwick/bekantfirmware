## BEKANT Firmware

The IKEA BEKANT adjustable-height sit/stand desk is controlled by a minimal two-button interface: hold one button to move up, hold the other button to move down. Releasing the button stops moving the desk, which means you have to hold a button while the table steadily moves into position, perhaps while you watch for it to line up with a mark you made on the wall at the perfect height.

If you switch the height of the desk often, standing there holding a button gives you plenty of time to wonder why a position memory feature wasn't originally built in. A single button press should be able to move it to a pre-programmed height.

Several IKEA hackers with the same thoughts have reverse engineered the signaling protocol, built replacement hardware, and added more automation to the the control system. But these implementations have all either treated the OEM control board as a black box or replaced it outright, meaning their modifications all require new hardware.

This project is a reimplementation of the BEKANT control firmware which can be flashed onto the OEM microcontroller, on its OEM circuit board, adding position memory without changing any hardware.

## Build & Flash

The firmware image was built in MPLAB X with the XC8 compiler. The microcontroller (PIC16LF1938) can be programmed through ICSP using a PICKit or other PIC programmer.

## User's Guide

| Button | Action |
| ------ | ------ |
| <kbd>△</kbd> (hold) | Move up  |
| <kbd>△</kbd> + <kbd>▽</kbd> | Move to upper memory position  |
| <kbd>▽</kbd> (hold) | Move down |
| <kbd>▽</kbd> + <kbd>△</kbd> | Move to lower memory position  |

"+" means press the first button, and while holding it, press the second button, then release both. Just like <kbd>Ctrl</kbd> + <kbd>X</kbd>.

Automatic movement to a memory position can be canceled by pressing either button or cutting power.

### Memory Positions

Memory positions are stored in the PIC EEPROM. These are 16-bit little-endian integers for the presumed encoder values of the motorized table legs.

| Offset | Length | Default | Description |
| ------ | ------ | ------- | ----------- |
| 0x00   | 2 bytes | 0x0636 | Lower position encoder value |
| 0x02   | 2 bytes | 0x1600 | Upper position encoder value |

## Disclaimer

Use at your own risk.

# References
 1. <a name="1">https://web.archive.org/web/20190116092248/https://blog.rnix.de/hacking-ikea-bekant/</a>
 2. <a name="2">https://github.com/trainman419/bekant</a>
 3. <a name="3">https://github.com/robin7331/IKEA-Hackant</a>
 4. <a name="4">https://github.com/diodenschein/TableMem</a>
 5. <a name="5">https://en.wikipedia.org/wiki/Local_Interconnect_Network</a>
