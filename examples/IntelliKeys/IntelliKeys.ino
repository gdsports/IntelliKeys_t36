/*
 * Demonstrate the use of the Teensy 3.6 IntelliKeys (IK) USB host driver.
 * Translate IK touch and switch events into USB keyboard and mouse inputs. The
 * Teensy 3.6 has one USB host and one USB device port so it plugs in between
 * the IK and the computer.
 */

#include <USBHost_t36.h>
#include <intellikeys.h>
#include <keymouse_play.h>  // https://github.com/gdsports/keymouse_t3
#include "keymouse.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
IntelliKeys ikey1(myusb);
keymouse_play keyplay;

elapsedMillis beepTime;
bool Connected = false;

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

const char keysequence[] =
  "GUI-KEY_R ~100 'chrome' SPACE 'https://www.google.com/' ENTER";

void IK_switch(int switch_number, int switch_state)
{
  Serial.printf("switch[%d] = %d\n", switch_number, switch_state);
  if (switch_state == 0) return;
  switch (switch_number) {
    case 1:
      keyplay.start(keysequence);
      break;
    case 2:
      break;
  }
}

void IK_sensor(int sensor_number, int sensor_value)
{
  Serial.printf("sensor[%d] = %d\n", sensor_number, sensor_value);
}

void IK_version(int major, int minor)
{
  Serial.printf("IK firmware Version %d.%d\n", major, minor);
}

void IK_connect(void)
{
  Serial.println("IK connect");
  Connected = true;
}

void IK_disconnect(void)
{
  Serial.println("IK disconnect");
  Connected = false;
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

char mySN[IK_EEPROM_SN_SIZE+1];	//+1 for terminating NUL

void IK_get_SN(uint8_t SN[IK_EEPROM_SN_SIZE])
{
  memcpy(mySN, SN, IK_EEPROM_SN_SIZE);
  mySN[IK_EEPROM_SN_SIZE] = '\0';
  Serial.printf("Serial Number = %s\n", mySN);
}

void IK_correct_membrane(int x, int y)
{
  Serial.printf("correct membrane (%d,%d)\n", x, y);
}

void IK_correct_switch(int switch_number, int switch_state)
{
  Serial.printf("correct switch[%d] = %d\n", switch_number, switch_state);
}

void IK_correct_done(void)
{
  Serial.println("correct done");
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
  ikey1.onSerialNum(IK_get_SN);
  ikey1.onCorrectMembrane(IK_correct_membrane);
  ikey1.onCorrectSwitch(IK_correct_switch);
  ikey1.onCorrectDone(IK_correct_done);
}

void loop() {
  myusb.Task();
  keyplay.loop();
  if (Connected && beepTime > 30000) {
    //ikey1.sound(1000, 100, 500);
    beepTime = 0;
  }
}
