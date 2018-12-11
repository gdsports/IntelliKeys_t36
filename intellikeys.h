/* USB Intellikeys driver
 * Copyright 2018 gdsports625@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

class IntelliKeys: public USBDriver {
public:
	IntelliKeys(USBHost &host) : /* txtimer(this),*/  updatetimer(this) { init(); }
	void begin();
	int setLED(uint8_t number, uint8_t value);
	int sound(int freq, int duration, int volume);
	int get_version(void);
	void onMembranePress(void (*function)(int x, int y)) {
		membrane_press_callback = function;
	}
	void onMembraneRelease(void (*function)(int x, int y)) {
		membrane_release_callback = function;
	}
	void onSwitch(void (*function)(int switch_number, int switch_state)) {
		switch_callback = function;
	}
	void onSensor(void (*function)(int sensor_number, int sensor_value)) {
		sensor_callback = function;
	}
	void onVersion(void (*function)(int major, int minor)) {
		version_callback = function;
	}
	void onConnect(void (*function)(void)) {
		connect_callback = function;
	}
	void onDisconnect(void (*function)(void)) {
		disconnect_callback = function;
	}
	void onOnOffSwitch(void (*function)(int switch_status)) {
		on_off_callback = function;
	}

protected:
	virtual void Task();
	virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
	virtual void disconnect();
	virtual void timer_event(USBDriverTimer *whichTimer);
	virtual void control(const Transfer_t *transfer);
private:
	void (*membrane_press_callback)(int x, int y);
	void (*membrane_release_callback)(int x, int y);
	void (*switch_callback)(int switch_number, int switch_state);
	void (*sensor_callback)(int sensor_number, int sensor_value);
	void (*version_callback)(int major, int minor);
	void (*connect_callback)(void);
	void (*disconnect_callback)(void);
	void (*on_off_callback)(int switch_status);
	int PostCommand(uint8_t *command);
	static void rx_callback1(const Transfer_t *transfer);
	static void rx_callback3(const Transfer_t *transfer);
	static void rx_callback4(const Transfer_t *transfer);
	static void tx_callback(const Transfer_t *transfer);
	void rx_data(uint8_t idx, const Transfer_t *transfer);
	void tx_data(const Transfer_t *transfer);
	void init();
	size_t write(const void *data, const size_t size);
	int read(void *data, const size_t size);
	void transmit();
	void IK_firmware_load();
	void ezusb_8051Reset(uint8_t resetBit);
	int ezusb_DownloadIntelHex(bool internal);
	void start();
	void handleEvents(const uint8_t *rxpacket, size_t len);
public:
	enum IK_LEDS {
		IK_LED_SHIFT=1,
		IK_LED_CAPS_LOCK=4,
		IK_LED_MOUSE=7,
		IK_LED_ALT=2,
		IK_LED_CTRL_CMD=5,
		IK_LED_NUM_LOCK=8
	};

private:
	Pipe_t mypipes[4] __attribute__ ((aligned(32)));
	Transfer_t mytransfers[5] __attribute__ ((aligned(32)));
	strbuf_t mystring_bufs[1];
	//USBDriverTimer txtimer;
	USBDriverTimer updatetimer;
	Pipe_t *rxpipe[3];
	Pipe_t *txpipe;
	bool first_update;
	setup_t IK_setup;
	uint8_t txbuffer[256];
	uint8_t rxpacket[3][64];
	volatile uint16_t txhead;
	volatile uint16_t txtail;
	volatile bool     txready;
	volatile uint8_t  rxlen[3];
	volatile bool     do_polling;
	volatile uint8_t  IK_state;
	const uint8_t mapEpAddr2Index[4] = {0, 0, 1, 2};
};
