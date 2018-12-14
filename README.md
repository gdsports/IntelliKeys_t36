# IntelliKeys_t36
IntelliKeys USB library for the Teensy 3.6 USB host library

![Teensy 3.6 with IntelliKeys board](./images/teensy36ikey.jpg)

Demonstrate the use of the [Teensy
3.6](https://www.pjrc.com/store/teensy36.html) IntelliKeys (IK) USB host
driver. Translate IK touch and switch events into USB keyboard and mouse
actions. The Teensy 3.6 has one USB host port and one USB device port so it
plugs in between the IK and the computer.

For example, when the IK 'A' key is pressed, the computer receives a USB
keyboard 'A'. As far as the computer knows, it is connected to a USB keyboard
and mouse composite device. For debugging, USB serial is also included. USB
joystick is just along for the ride.

The modifier keys (SHIFT, CTRL, ALT, CMD) work in latching mode. Pressing and
releasing the SHIFT key followed by pressing the 'A' key sends a capital 'A'
to the computer. Pressing and releasing the SHIFT key twice in a row turns on
the SHIFT lock. All keys are shifted until the SHIFT key is is pressed and
released one more time. The other modifier keys work the same way.

The lower right corner mouse pad sends USB mouse actions but is not fully
implemented.

The two audio jacks on the left side are for Assistive Technology switches.
Switch 1 opens Chrome to Google on Linux systems. See the example for how it
does this.

The On/Off switch at the top resets the board if it behaves strangely. It does
not control power to the board.

The 8051 firmware is extracted from the
[OpenIKeys](https://github.com/ATMakersOrg/OpenIKeys) project.

The IK EEPROM holds the device serial number. Use the onSerialNum function to
get the serial number. See the example for details. The EEPROM also holds
calibration values(?) for the overlay sensors.

## Hardware components

* PJRC [Teensy 3.6](https://www.pjrc.com/store/teensy36.html)
* PJRC [USB host cable](https://www.pjrc.com/store/cable_usb_host_t36.html)

The Teensy 3.6 is used because it has two USB ports so it can act as a USB
translator.

Note only the 5 pin header for the USB host cable must be soldered on. All the
other pins are not used so there is no need to solder on the other headers.
The micro SD card slot may be used later to load tables mapping touch locations
to keyboard and mouse actions. This will allow the mapping to be changed
without using the Arduino IDE to rebuild the code.

## Development environment

* Install Arduino IDE 1.8.8. Download and follow the instructions at arduino.cc.
* Install Teenysduino 1.45. Download and follow the instructions at pjrc.com.
* Install this library in the Arduino library directory.
* Build and upload the example included with this library. Set the Board to
Teensy 3.6 and the USB Type to "Serial + Keyboard + Mouse + Joystick".

## TODO

* ~~EEPROM operations?~~
* CORRECT operations?
* Load tables from micro SD card
* Modify onSensors so it returns 0 or 1 instead of raw sensor values. Use
  the EEPROM calibration values.
