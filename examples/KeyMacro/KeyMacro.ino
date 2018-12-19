/*
 * Demonstrate triggering keyboard macros using the switch 1 and 2 inputs.
 */

#include <USBHost_t36.h>
#include <intellikeys.h>
#include <keymouse_play.h>
#include <SD.h>
#include <SPI.h>

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
IntelliKeys ikey1(myusb);
keymouse_play keyplay;

const int chipSelect = BUILTIN_SDCARD;

bool Connected = false;

// Default key macros for switch 1 and 2. They can be overriden by
// keymacros.txt stored on SD card.
// Windows
// GUI key is the Windows logo key
const char google[] =
  "GUI-KEY_R ~100 'chrome' SPACE 'https://www.google.com/' ENTER";
const char youtube[] =
  "GUI-KEY_R ~100 'chrome' SPACE 'https://www.youtube.com/' ENTER";

char *macros[2] = {(char *)youtube, (char *)google};

void IK_press(int x, int y)
{
  Serial.printf("membrane press (%d,%d)\n", x, y);
}

void IK_release(int x, int y)
{
  Serial.printf("membrane release (%d,%d)\n", x, y);
}

void IK_switch(int switch_number, int switch_state)
{
  Serial.printf("switch[%d] = %d\n", switch_number, switch_state);
  if (switch_state == 0) return;
  switch (switch_number) {
    case 1:
      keyplay.start(macros[0]);
      break;
    case 2:
      keyplay.start(macros[1]);
      break;
  }
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
}

void setup()
{
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
  ikey1.onOnOffSwitch(IK_onoff);

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  File MacroFile;
  MacroFile = SD.open("keymacro.txt");
  if (MacroFile) {
    char buf[1024];
    Serial.println("Open keymacro.txt");
    for (size_t idx = 0; idx < sizeof(macros)/sizeof(macros[0]); idx++) {
      if (!MacroFile.available()) break;
      int bytesIn = MacroFile.readBytesUntil('\n', buf, sizeof(buf)-1);
      if (bytesIn > 0) {
        buf[bytesIn] = '\0';
        char *buf_right_size = strdup(buf);
        macros[idx] = buf_right_size;
        Serial.printf("macro[%d]=%s\n", idx, buf_right_size);
      }
    }
    MacroFile.close();
    Serial.println("Close keymacro.txt");
  }
}

void loop()
{
  myusb.Task();
  keyplay.loop();
}
