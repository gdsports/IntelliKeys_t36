/*
 * Demonstrate triggering keyboard macros using the switch 1 and 2 inputs.
 * The touch area is divided into a 2 row by 3 column grid for a total of
 * 6 large touch buttons/pads.
 *
 * The key presses for the 2 switch inputs and the 6 touch inputs are
 * determined by a file stored on a micro SD card named "keymacro.txt".
 * The first 2 lines are for the switch inputs. The following lines are
 * for the touch pads.
 *
 * Line 1: Switch input 1
 * Line 2: Switch input 2
 * Line 3: Touch row 1, col 1
 * Line 4: Touch row 1, col 2
 * Line 5: Touch row 1, col 3
 * Line 6: Touch row 2, col 1
 * Line 7: Touch row 2, col 2
 * Line 8: Touch row 2, col 3
 *
 * Extra lines are ignored.
 *
 * Samples lines to launch chrome to various URLs using Windows.
 *
 * GUI-KEY_R ~100 'chrome' SPACE 'https://www.youtube.com/' ENTER
 * GUI-KEY_R ~100 'chrome' SPACE 'https://www.google.com/' ENTER
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

/*
 * The native touch resolution is 24x24. For this example, each virtual button
 * is COLS native columns by ROWS rows. This array represents the ROWS rows of
 * COLS virtual buttons. The elements are incremented with each native touch
 * press and decremented with each native touch release. When the count goes
 * from 0 to 1, the corresponding key macro starts.
 */

#define ROW_DIV (12)
#define COL_DIV (8)
#define ROWS    (24/ROW_DIV)
#define COLS    (24/COL_DIV)
uint8_t membrane[COLS][ROWS];

// Default key macros for switch 1 and 2. They can be overriden by
// keymacros.txt stored on SD card.
// Windows
// GUI key is the Windows logo key
const char google[] =
  "GUI-KEY_R ~100 'chrome' SPACE 'https://www.google.com/' ENTER";
const char youtube[] =
  "GUI-KEY_R ~100 'chrome' SPACE 'https://www.youtube.com/' ENTER";

// Key macro strings. [0] = switch 1, [1] = switch 2, [...] touch keys
char *macros[2+(ROWS*COLS)] = {(char *)youtube, (char *)google};

void IK_press(int x, int y)
{
  Serial.printf("membrane press (%d,%d)\n", x, y);
  uint8_t row = y / ROW_DIV;
  uint8_t col = x / COL_DIV;
  if (membrane[col][row] == 0) {
    uint8_t  macro_index = (row*COLS) + col + 2;
    Serial.printf("x=%d col=%d y=%d row=%d idx=%d\n", x, col, y, row,
        macro_index);
    keyplay.start(macros[macro_index]);
  }
  membrane[col][row]++;
}

void IK_release(int x, int y)
{
  Serial.printf("membrane release (%d,%d)\n", x, y);
  uint8_t row = y / ROW_DIV;
  uint8_t col = x / COL_DIV;
  membrane[col][row]--;
  if (membrane[col][row] < 0) membrane[col][row] = 0;
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
  Keyboard.releaseAll();
  Mouse.release(MOUSE_ALL);
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
      if (MacroFile.available()) {
        int bytesIn = MacroFile.readBytesUntil('\n', buf, sizeof(buf)-1);
        if (bytesIn > 0) {
          buf[bytesIn] = '\0';
          char *buf_right_size = strdup(buf);
          macros[idx] = buf_right_size;
          Serial.printf("macro[%d]=%s\n", idx, buf_right_size);
        }
      }
      else {
        macros[idx] = NULL;
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
