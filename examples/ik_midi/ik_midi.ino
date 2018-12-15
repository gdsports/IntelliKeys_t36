/*
 * Demonstrate the use of the Teensy 3.6 IntelliKeys (IK) USB host driver.
 * Translate IK touch events into USB MIDI note on and off. The Teensy 3.6 has
 * one USB host and one USB device port so it plugs in between the IK and the
 * computer.
 *
 * Set the USB Type to MIDI. Do not use Serial + MIDI because this does not
 * work on Android.
 *
 * This was tested using an Android tablet with a USB OTG to host cable.
 * FluidSynth MIDI Synthesizer is a free download from Google Play.
 * 
 * Android tablet - USB OTG cable - USB micro cable - Teensy 3.6 - IntelliKeys
 */

#include <USBHost_t36.h>
#include <intellikeys.h>

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
IntelliKeys ikey1(myusb);

#define AllSoundOff			(120)
#define ResetAllControllers	(121)
#define AllNotesOff			(123)

// the MIDI channel number to send messages
const int channel = 1;
// Lowest MIDI note on an 88 key piano
const int MIDI_LOW = 21;

/*
 * The native touch resolution is 24x24. For this example, each virtual button
 * is 2 native columns by 3 rows. This array represents the 8 rows of 12
 * virtual buttons. The elements are incremented with each native touch press
 * and decremented with each native touch release. When the count goes from 0
 * to 1, press action (see membrane_actions[]) is performed. When the count
 * goes from 1 to 0, the release action is performed.
 */
uint8_t membrane[8][12];

/*
 * The top left corner corresponds to the left most piano key. This plays MIDI
 * note 21 also known as A0 also known as note A, octave 0 also known as
 * frequency 27.5 Hertz.
 */

void IK_press(int x, int y)
{
  Serial.printf("membrane press (%d,%d)\n", x, y);
  uint8_t row = y / 3;
  uint8_t col = x / 2;
  if (membrane[row][col] == 0) {
	uint8_t midi_note_number = (row*12) + col + MIDI_LOW;
    Serial.printf("MIDI note %d On\n", midi_note_number);
	usbMIDI.sendNoteOn(midi_note_number, 127, channel);
  }
  membrane[row][col]++;
}

void IK_release(int x, int y)
{
  Serial.printf("membrane release (%d,%d)\n", x, y);
  uint8_t row = y / 3;
  uint8_t col = x / 2;
  membrane[row][col]--;
  if (membrane[row][col] < 0) membrane[row][col] = 0;
  if (membrane[row][col] != 0) return;
  uint8_t midi_note_number = (row*12) + col + MIDI_LOW;
  Serial.printf("MIDI note %d Off\n", midi_note_number);
  usbMIDI.sendNoteOff(midi_note_number, 0, channel);
}

void IK_switch(int switch_number, int switch_state)
{
  Serial.printf("switch[%d] = %d\n", switch_number, switch_state);
  if (switch_state == 0) return;
  switch (switch_number) {
    case 1:
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
	memset((void *)membrane, 0, sizeof(membrane));
	usbMIDI.sendControlChange(AllNotesOff, 0, channel);
	usbMIDI.sendControlChange(AllSoundOff, 0, channel);
	usbMIDI.sendControlChange(ResetAllControllers, 0, channel);
  }
}

char mySN[IK_EEPROM_SN_SIZE+1];	//+1 for terminating NUL

void IK_get_SN(uint8_t SN[IK_EEPROM_SN_SIZE])
{
  memcpy(mySN, SN, IK_EEPROM_SN_SIZE);
  mySN[IK_EEPROM_SN_SIZE] = '\0';
  Serial.printf("Serial Number = %s\n", mySN);
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
}

void loop() {
  myusb.Task();

  // MIDI Controllers should discard incoming MIDI messages.
  while (usbMIDI.read()) {
    // ignore incoming messages
  }
}
