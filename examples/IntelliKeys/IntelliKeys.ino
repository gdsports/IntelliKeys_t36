/*
 * Demonstrate the use of the Teensy 3.6 IntelliKeys (IK) USB host driver.
 * Translate IK touch and switch events into USB keyboard and mouse inputs. The
 * Teensy 3.6 has one USB host and one USB device port so it plugs in between
 * the IK and the computer.
 */

#include <USBHost_t36.h>
#include <intellikeys.h>
#include "keymouse.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
IntelliKeys ikey1(myusb);

elapsedMillis beepTime;

void IK_press(int x, int y)
{
  Serial.printf("membrane press (%d,%d)\n", x, y);
  process_membrane_press(ikey1, x, y);
}

void IK_release(int x, int y)
{
  Serial.printf("membrane release (%d,%d)\n", x, y);
  process_membrane_release(ikey1, x, y);
}

void IK_switch(int switch_number, int switch_state)
{
  Serial.printf("switch[%d] = %d\n", switch_number, switch_state);
  if (switch_state == 0) return;
  switch (switch_number) {
    case 1:
	  // On Ubuntu Linux 18.04, run Chromium (also known as Chrome) and open
	  // a tab to Google. ALT-F2 opens a 1 line command window.
      Keyboard.press(MODIFIERKEY_ALT);
      Keyboard.press(KEY_F2);
      delay(10);
      Keyboard.releaseAll();
      delay(1000);
      Keyboard.print("chromium-browser");
      delay(100);
      Keyboard.press(KEY_ENTER);
      delay(10);
      Keyboard.releaseAll();
      delay(1000);
      Keyboard.print("https://google.com\n");
      break;
    case 2:
      break;
  }
}

void IK_sensor(int sensor_number, int sensor_value)
{
  //Serial.printf("sensor[%d] = %d\n", sensor_number, sensor_value);
}

void IK_version(int major, int minor)
{
  Serial.printf("IK firmware Version %d.%d\n", major, minor);
}

void IK_connect(void)
{
  Serial.println("IK connect");
}

void IK_disconnect(void)
{
  Serial.println("IK disconnect");
}

void IK_onoff(int onoff)
{
  Serial.printf("On/Off switch = %d\n", onoff);
  if (onoff == 0) {
	clear_membrane();
	ikey1.setLED(IntelliKeys::IK_LED_SHIFT, 0);
	ikey1.setLED(IntelliKeys::IK_LED_CAPS_LOCK, 0);
	ikey1.setLED(IntelliKeys::IK_LED_MOUSE, 0);
	ikey1.setLED(IntelliKeys::IK_LED_ALT, 0);
	ikey1.setLED(IntelliKeys::IK_LED_CTRL_CMD, 0);
	ikey1.setLED(IntelliKeys::IK_LED_NUM_LOCK, 0);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) ; // wait for Arduino Serial Monitor
  Serial.println("IntelliKeys USB Test");
  myusb.begin();

  ikey1.begin();
  ikey1.onConnect(IK_connect);
  ikey1.onDisconnect(IK_disconnect);
  ikey1.onMembranePress(IK_press);
  ikey1.onMembraneRelease(IK_release);
  ikey1.onSwitch(IK_switch);
  ikey1.onSensor(IK_sensor);
  ikey1.onVersion(IK_version);
  ikey1.onOnOffSwitch(IK_onoff);
}

void loop() {
  myusb.Task();
  if (beepTime > 30000) {
    //ikey1.sound(1000, 100, 500);
    beepTime = 0;
  }
}
