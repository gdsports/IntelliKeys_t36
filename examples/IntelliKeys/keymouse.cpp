/*
 * Minimal demonstration of translating touches to keyboard and mouse events.
 * This is designed for the QWERT USB overlay.
 */

#include <Arduino.h>
#include <USBHost_t36.h>
#include <intellikeys.h>
#include "keymouse.h"

/*
 * The native touch resolution is 24x24. For this example, each virtual button
 * is 2 native columns by 3 rows. This array represents the 8 rows of 12
 * virtual buttons. The elements are incremented with each native touch press
 * and decremented with each native touch release. When the count goes from 0
 * to 1, press action (see membrane_actions[]) is performed. When the count
 * goes from 1 to 0, the release action is performed.
 */
static uint8_t membrane[8][12];

static const uint16_t membrane_actions[8][12] = {
	// Top row = 0
	KEY_ESC, 		// [0,0]
	KEY_TAB, 		// [1,0]
	KEY_TILDE,		// [2,0]
	KEY_NUM_LOCK,	// [3,0]
	0,				// [4,0]
	KEY_INSERT,		// [5,0]
	KEY_HOME,		// [6,0]
	KEY_END,		// [7,0]
	0,				// [8,0]
	KEY_PAGE_UP,	// [9,0]
	KEY_PAGE_DOWN,	// [10,0]
	KEY_DELETE,		// [11,0]
	// row = 1
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	// row = 2
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_MINUS,
	KEY_EQUAL,
	// row = 3
	KEY_Q,
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_T,
	KEY_Y,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_BACKSPACE,
	KEY_BACKSPACE,
	// row = 4
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	0,
	0,
	0,
	// row = 5
	KEY_Z,
	KEY_X,
	KEY_C,
	KEY_V,
	KEY_B,
	KEY_N,
	KEY_M,
	KEY_SEMICOLON,
	KEY_QUOTE,
	0,
	0,
	0,
	// row = 6
	KEY_CAPS_LOCK,
	MODIFIERKEY_SHIFT,
	MODIFIERKEY_SHIFT,
	KEY_SPACE,
	KEY_SPACE,
	KEY_SPACE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	0,
	0,
	0,
	// row = 7
	MODIFIERKEY_CTRL,
	MODIFIERKEY_ALT,
	MODIFIERKEY_GUI,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_ENTER,
	0,
	0,
	0,
};

enum mouse_actions {
	MOUSE_MOVE_NW, MOUSE_MOVE_N, MOUSE_MOVE_NE,
	MOUSE_MOVE_W,  MOUSE_CLICK,  MOUSE_MOVE_E,
	MOUSE_MOVE_SW, MOUSE_MOVE_S, MOUSE_MOVE_SE,
	MOUSE_DOUBLE_CLICK, MOUSE_RIGHT_CLICK, MOUSE_PRESS
};

static const uint16_t membrane_actions_mouse[8][12] = {
	// Top row = 0
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// row = 1
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// row = 2
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// row = 3
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	// row = 4, mousepad
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	MOUSE_MOVE_NW,	// mouse move NW
	MOUSE_MOVE_N,	// mouse move N
	MOUSE_MOVE_NE,	// mouse move NE
	// row = 5, mousepad
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	MOUSE_MOVE_W,	// mouse move W
	MOUSE_CLICK,	// mouse click
	MOUSE_MOVE_E,	// mouse move E
	// row = 6, mousepad
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	MOUSE_MOVE_SW,	// mouse move SW
	MOUSE_MOVE_S,	// mouse move S
	MOUSE_MOVE_SE,	// mouse move SE
	// row = 7, mousepad
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	MOUSE_DOUBLE_CLICK,	// mouse double click
	MOUSE_RIGHT_CLICK,	// mouse right click
	MOUSE_PRESS,	// mouse press/release
};

#define MOUSE_MOVE	(10)
static bool num_lock=false;
static bool caps_lock=false;
static uint8_t shift_lock=0;	//0=off,1=on next key,2=lock on
static uint8_t ctrl_lock=0;	//0=off,1=on next key,2=lock on
static uint8_t alt_lock=0;	//0=off,1=on next key,2=lock on
static uint8_t gui_lock=0;	//0=off,1=on next key,2=lock on

void clear_membrane(void)
{
	memset((void *)membrane, 0, sizeof(membrane));
	if (num_lock) Keyboard.press(KEY_NUM_LOCK);
	if (caps_lock) Keyboard.press(KEY_CAPS_LOCK);
	num_lock = caps_lock = false;
	shift_lock = ctrl_lock = alt_lock = gui_lock = 0;
	Mouse.release(MOUSE_ALL);
	Keyboard.releaseAll();
}

void process_membrane_release(IntelliKeys &ikey, int x, int y)
{
	uint8_t row, col;
	uint16_t keycode, mousecode;
	col = x / 2;
	row = y / 3;
	membrane[row][col]--;
	if (membrane[row][col] < 0) membrane[row][col] = 0;
	if (membrane[row][col] != 0) return;
	keycode = membrane_actions[row][col];
	if (keycode) {
		switch (keycode) {
			case MODIFIERKEY_SHIFT:
				/* fall through */
			case MODIFIERKEY_ALT:
				/* fall through */
			case MODIFIERKEY_CTRL:
				/* fall through */
			case MODIFIERKEY_GUI:
				/* do nothing */
				break;
			default:
				Keyboard.release(keycode);
				break;
		}
	}
	else {
		mousecode = membrane_actions_mouse[row][col];
		if (mousecode) {
			switch (mousecode) {
				case MOUSE_MOVE_NW:
					break;
				case MOUSE_MOVE_N:
					break;
				case MOUSE_MOVE_NE:
					break;
				case MOUSE_MOVE_W:
					break;
				case MOUSE_CLICK:
					break;
				case MOUSE_MOVE_E:
					break;
				case MOUSE_MOVE_SW:
					break;
				case MOUSE_MOVE_S:
					break;
				case MOUSE_MOVE_SE:
					break;
				case MOUSE_DOUBLE_CLICK:
					break;
				case MOUSE_RIGHT_CLICK:
					break;
				case MOUSE_PRESS:
					// Mouse.?
					break;
				default:
					break;
			}
		}
	}
}

void process_locking(IntelliKeys &ikey, uint8_t &lock_status, uint8_t LED, uint16_t keycode)
{
	switch (lock_status) {
		case 0:
			lock_status = 1;
			ikey.setLED(LED, 1);
			break;
		case 1:
			lock_status = 2;
			ikey.setLED(LED, 1);
			break;
		case 2:
			/* fall through */
		default:
			lock_status = 0;
			Keyboard.release(keycode);
			break;
	}
}

void process_membrane_press(IntelliKeys &ikey, int x, int y)
{
	uint8_t row, col;
	uint16_t keycode, mousecode;
	col = x / 2;
	row = y / 3;
	Serial.printf("col,row (%d,%d) membrane %d\n", col, row, membrane[row][col]);
	if (membrane[row][col] == 0) {
		keycode = membrane_actions[row][col];
		if (keycode) {
			Keyboard.press(keycode);
			switch (keycode) {
				case KEY_CAPS_LOCK:
					caps_lock = !caps_lock;
					ikey.setLED(IntelliKeys::IK_LED_CAPS_LOCK, caps_lock);
					break;
				case KEY_NUM_LOCK:
					num_lock = !num_lock;
					ikey.setLED(IntelliKeys::IK_LED_NUM_LOCK, num_lock);
					break;
				case MODIFIERKEY_SHIFT:
					process_locking(ikey, shift_lock, IntelliKeys::IK_LED_SHIFT, keycode);
					if (shift_lock == 0) ikey.setLED(IntelliKeys::IK_LED_SHIFT, 0);
					break;
				case MODIFIERKEY_ALT:
					process_locking(ikey, alt_lock, IntelliKeys::IK_LED_ALT, keycode);
					if (alt_lock == 0) ikey.setLED(IntelliKeys::IK_LED_ALT, 0);
					break;
				case MODIFIERKEY_CTRL:
					process_locking(ikey, ctrl_lock, IntelliKeys::IK_LED_CTRL_CMD, keycode);
					if (!ctrl_lock && !gui_lock) {
						ikey.setLED(IntelliKeys::IK_LED_CTRL_CMD, 0);
					}
					break;
				case MODIFIERKEY_GUI:
					process_locking(ikey, gui_lock, IntelliKeys::IK_LED_CTRL_CMD, keycode);
					if (!ctrl_lock && !gui_lock) {
						ikey.setLED(IntelliKeys::IK_LED_CTRL_CMD, 0);
					}
					break;
				default:
					if (shift_lock == 1) {
						shift_lock = 0;
						ikey.setLED(IntelliKeys::IK_LED_SHIFT, 0);
						Keyboard.release(MODIFIERKEY_SHIFT);
					}
					if (alt_lock == 1) {
						alt_lock = 0;
						ikey.setLED(IntelliKeys::IK_LED_ALT, 0);
						Keyboard.release(MODIFIERKEY_ALT);
					}
					if (ctrl_lock == 1) {
						ctrl_lock = 0;
						if (!ctrl_lock && !gui_lock) {
							ikey.setLED(IntelliKeys::IK_LED_CTRL_CMD, 0);
						}
						Keyboard.release(MODIFIERKEY_CTRL);
					}
					if (gui_lock == 1) {
						gui_lock = 0;
						if (!ctrl_lock && !gui_lock) {
							ikey.setLED(IntelliKeys::IK_LED_CTRL_CMD, 0);
						}
						Keyboard.release(MODIFIERKEY_GUI);
					}
					break;
			}
		}
		else {
			mousecode = membrane_actions_mouse[row][col];
			if (mousecode) {
				switch (mousecode) {
					case MOUSE_MOVE_NW:
						Mouse.move(-MOUSE_MOVE, -MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_MOVE_N:
						Mouse.move(0, -MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_MOVE_NE:
						Mouse.move(MOUSE_MOVE, -MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_MOVE_W:
						Mouse.move(-MOUSE_MOVE, 0, 0, 0);
						break;
					case MOUSE_CLICK:
						Mouse.click();
						break;
					case MOUSE_MOVE_E:
						Mouse.move(MOUSE_MOVE, 0, 0, 0);
						break;
					case MOUSE_MOVE_SW:
						Mouse.move(MOUSE_MOVE, -MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_MOVE_S:
						Mouse.move(0, MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_MOVE_SE:
						Mouse.move(MOUSE_MOVE, MOUSE_MOVE, 0, 0);
						break;
					case MOUSE_DOUBLE_CLICK:
						break;
					case MOUSE_RIGHT_CLICK:
						break;
					case MOUSE_PRESS:
						break;
					default:
						break;
				}
			}
		}
	}
	membrane[row][col]++;
}
