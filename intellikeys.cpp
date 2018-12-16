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


#include <Arduino.h>
#include "USBHost_t36.h"  // Read this header first for key info
#include "intellikeysdefs.h"
#include "intellikeys.h"

#define IK_VID          0x095e
#define IK_PID_FWLOAD   0x0100  // Firmware load required
#define IK_PID_RUNNING  0x0101  // Firmware running

#define debug_print   USBHost::print_
#define debug_println USBHost::println_

#define ENABLE_SERIALPRINTF   0

#if ENABLE_SERIALPRINTF
#undef debug_printf
#define debug_printf(...) Serial.printf(__VA_ARGS__); Serial.write("\r\n")
#else
#undef debug_printf
#define debug_printf(...)
#endif


void IntelliKeys::init()
{
	contribute_Pipes(mypipes, sizeof(mypipes)/sizeof(Pipe_t));
	contribute_Transfers(mytransfers, sizeof(mytransfers)/sizeof(Transfer_t));
	contribute_String_Buffers(mystring_bufs, sizeof(mystring_bufs)/sizeof(strbuf_t));
	driver_ready_for_device(this);
}

bool IntelliKeys::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
	if (type != 1) return false;
	debug_println("IntelliKeys claim this=", (uint32_t)this, HEX);
	if (dev->idVendor != IK_VID) return false;
	if (dev->idProduct == IK_PID_FWLOAD) {
		debug_println("found IntelliKeys, need FW load, pid=", dev->idProduct, HEX);
		IK_state = 2;
		return true;
	}
	if (dev->idProduct != IK_PID_RUNNING) return false;
	debug_println("found IntelliKeys, pid=", dev->idProduct, HEX);
	memset((void*)rxpipe, 0, sizeof(rxpipe));
	txpipe = NULL;
	const uint8_t *p = descriptors;
	const uint8_t *end = p + len;
	int descriptorLength = p[0];
	int descriptorType = p[1];
	if (descriptorLength < 9 || descriptorType != 4) return false;
	p += descriptorLength;
	while (p < end) {
		descriptorLength = p[0];
		if (p + descriptorLength > end) return false; // reject if beyond end of data
		descriptorType = p[1];
		if (descriptorType == 5) { // 5 = endpoint
			bool epIN = (p[2] & 0xF0) == 0x80;	// true=IN, false=OUT
			uint8_t epAddr = p[2] & 0x0F;
			uint8_t epType = p[3] & 0x03;
			uint16_t epSize = p[4] | (p[5] << 8);
			if (epType == 3) {	// Interrupt
				if (!epIN && epAddr == 0x02) { // OUT
					txpipe = new_Pipe(dev, epType, epAddr, 0, epSize);
				} else if (epIN) {	// IN
					if (epAddr == 1 || epAddr == 3 || epAddr == 4) {
						uint8_t idx = mapEpAddr2Index[epAddr-1];
						rxpipe[idx] = new_Pipe(dev, epType, epAddr, 1, epSize);
					}
				}
			}
		}
		p += descriptorLength;
	}
	if (rxpipe[0] && rxpipe[1] && rxpipe[2] && txpipe) {
		rxpipe[0]->callback_function = rx_callback1;
		rxpipe[1]->callback_function = rx_callback3;
		rxpipe[2]->callback_function = rx_callback4;
		txpipe->callback_function = tx_callback;
		txhead = 0;
		txtail = 0;
		//rxhead = 0;
		//rxtail = 0;
		memset(txbuffer, 0, sizeof(txbuffer));
		first_update = true;
		txready = true;
		updatetimer.start(500000);
		for (int i = 0; i < 3; i++) {
			queue_Data_Transfer(rxpipe[i], rxpacket[i], 64, this);
			rxlen[i] = 0;
		}
		do_polling = false;
		start();
		return true;
	}
	return false;
}

inline int IntelliKeys::PostCommand(uint8_t *command)
{
	return write(command, IK_REPORT_LEN);
}

int IntelliKeys::setLED(uint8_t number, uint8_t value)
{
	uint8_t command[IK_REPORT_LEN] = {IK_CMD_LED,number,value,0,0,0,0,0};
	return PostCommand(command);
}

int IntelliKeys::sound(int freq, int duration, int volume)
{
	uint8_t report[IK_REPORT_LEN] = {IK_CMD_TONE,0,0,0,0,0,0,0};

	//  set parameters and blow
	report[1] = freq;
	report[2] = volume;
	report[3] = duration/10;
	return PostCommand(report);
}


int IntelliKeys::get_version(void) {
	uint8_t command[IK_REPORT_LEN] = {IK_CMD_GET_VERSION,0,0,0,0,0,0,0};
	version_done = false;
	return PostCommand(command);
}

int IntelliKeys::get_all_sensors(void) {
	uint8_t command[IK_REPORT_LEN] = {IK_CMD_ALL_SENSORS,0,0,0,0,0,0,0};
	memset(sensorStatus, 255, sizeof(sensorStatus));
	return PostCommand(command);
}

void IntelliKeys::get_eeprom(void)
{
	uint8_t report[IK_REPORT_LEN] = {IK_CMD_EEPROM_READBYTE,0,0x1F,0,0,0,0,0};
	uint8_t pending = 0;

	if (eeprom_period > 64) {
		debug_println("get_eeprom");
		eeprom_period = 0;
		for (uint8_t i=0; i < sizeof(eeprom_t); i++) {
			if (!eeprom_valid[i]) {
				report[1] = 0x80 + i;
				PostCommand(report);
				if (pending++ > 8) break;
			}
		}
		if (pending == 0) {
			eeprom_all_valid = true;
			// Get sensor status events because eeprom_data.sensorBlack and White
			// now have valid data.
			get_all_sensors();

			if (on_SN_callback) (*on_SN_callback)(eeprom_data.serialnumber);
		}
	}
}

void IntelliKeys::clear_eeprom()
{
	eeprom_all_valid = false;
	memset(eeprom_valid, 0, sizeof(eeprom_valid));
}

void IntelliKeys::start()
{
	uint8_t command[IK_REPORT_LEN] = {0};

	debug_println("start");
	command[0] = IK_CMD_INIT;
	command[1] = 0;  //  interrupt event mode
	PostCommand(command);

	command[0] = IK_CMD_SCAN;
	command[1] = 1;	//  enable
	PostCommand(command);

	delay(250);

	get_all_sensors();

	get_version();

	clear_eeprom();

	if (connect_callback) (*connect_callback)();
}

void IntelliKeys::disconnect()
{
	updatetimer.stop();
	//txtimer.stop();

	if (disconnect_callback) (*disconnect_callback)();
}


void IntelliKeys::rx_callback1(const Transfer_t *transfer)
{
	if (!transfer->driver) return;
	((IntelliKeys *)(transfer->driver))->rx_data(0, transfer);
}

void IntelliKeys::rx_callback3(const Transfer_t *transfer)
{
	if (!transfer->driver) return;
	((IntelliKeys *)(transfer->driver))->rx_data(1, transfer);
}

void IntelliKeys::rx_callback4(const Transfer_t *transfer)
{
	if (!transfer->driver) return;
	((IntelliKeys *)(transfer->driver))->rx_data(2, transfer);
}

void IntelliKeys::tx_callback(const Transfer_t *transfer)
{
	if (!transfer->driver) return;
	((IntelliKeys *)(transfer->driver))->tx_data(transfer);
}

void IntelliKeys::rx_data(uint8_t idx, const Transfer_t *transfer)
{
	uint32_t len = transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF);
	//print_hexbytes(transfer->buffer, len);
	if (len < 1 || len > 64) {
		queue_Data_Transfer(rxpipe[idx], rxpacket[idx], 64, this);
		rxlen[idx] = 0;
	} else {
		rxlen[idx] = len;
		// TODO: should someday use EventResponder to call from yield()
	}
}

void IntelliKeys::tx_data(const Transfer_t *transfer)
{
	uint8_t *p = (uint8_t *)transfer->buffer;
	//debug_print("tx_data, len=", *(p-1));
	//debug_print(", tail=", (p-1) - txbuffer);
	//debug_println(", tail=", txtail);
	uint32_t tail = txtail;
	uint8_t size = *(p-1);
	tail += size + 1;
	if (tail >= sizeof(txbuffer)) tail -= sizeof(txbuffer);
	txtail = tail;
	//debug_println("new tail=", tail);
	txready = true;
	transmit();
	//txtimer.start(8000);
	// adjust tail...
	// start timer if more data to send
}


size_t IntelliKeys::write(const void *data, const size_t size)
{
	//debug_print("write ", size);
	//debug_print(" bytes: ");
	//print_hexbytes(data, size);
	if (size > 64) return 0;
	uint32_t head = txhead;
	if (++head >= sizeof(txbuffer)) head = 0;
	uint32_t remain = sizeof(txbuffer) - head;
	if (remain < size + 1) {
		// not enough space at end of buffer
		txbuffer[head] = 0xFF;
		head = 0;
	}
	uint32_t avail;
	do {
		uint32_t tail = txtail;
		if (head > tail) {
			avail = sizeof(txbuffer) - head + tail;
		} else {
			avail = tail - head;
		}
	} while (avail < size + 1); // wait for space in buffer
	txbuffer[head] = size;
	memcpy(txbuffer + head + 1, data, size);
	txhead = head + size;
	//debug_print("head=", txhead);
	//debug_println(", tail=", txtail);
	//print_hexbytes(txbuffer, 60);
	NVIC_DISABLE_IRQ(IRQ_USBHS);
	transmit();
	NVIC_ENABLE_IRQ(IRQ_USBHS);
	return size;
}

void IntelliKeys::transmit()
{
	if (!txready) return;
	uint32_t head = txhead;
	uint32_t tail = txtail;
	if (head == tail) {
		//debug_println("no data to transmit");
		return; // no data to transit
	}
	if (++tail >= sizeof(txbuffer)) tail = 0;
	uint32_t size = txbuffer[tail];
	//debug_print("tail=", tail);
	//debug_println(", tx size=", size);
	if (size == 0xFF) {
		txtail = 0;
		tail = 0;
		size = txbuffer[0];
		//debug_println("tx size=", size);
	}
	//txtail = tail + size;
	queue_Data_Transfer(txpipe, txbuffer + tail + 1, size, this);
	//txtimer.start(8000);
	txready = false;
}

void IntelliKeys::timer_event(USBDriverTimer *whichTimer)
{
#if 1
	if (whichTimer == &updatetimer) {
		updatetimer.start(250000);
		if (first_update) {
			//ResetSystem();
			first_update = false;
		} else {
			do_polling = true;
		}
		//debug_println("ant update timer");
	}
	/* else if (whichTimer == &txtimer) {
	   debug_println("ant tx timer");
	//txtimer.stop(); // TODO: why is this needed?
	txready = true;
	transmit();
	} */
#endif
}

void IntelliKeys::ezusb_8051Reset(uint8_t resetBit)
{
	static uint8_t reg_value;
	debug_println("Ezusb_8051Reset");
	reg_value = resetBit;
	mk_setup(IK_setup, 0x40, ANCHOR_LOAD_INTERNAL, CPUCS_REG, 0, 1);
	queue_Control_Transfer(device, &IK_setup, &reg_value, this);
}

static PINTEL_HEX_RECORD pHex;
static uint8_t pHexBuf[MAX_INTEL_HEX_RECORD_LENGTH];

int IntelliKeys::ezusb_DownloadIntelHex(bool internal)
{
	while (pHex->Type == 0) {
		debug_print("pHex="); debug_println((uint32_t)pHex, HEX);
		if (INTERNAL_RAM(pHex->Address) == internal) {
			debug_print("pHex->Address="); debug_println((uint32_t)pHex->Address, HEX);
			mk_setup(IK_setup, 0x40,
					(internal)?ANCHOR_LOAD_INTERNAL:ANCHOR_LOAD_EXTERNAL,
					pHex->Address, 0, pHex->Length);
			memcpy(pHexBuf, pHex->Data, pHex->Length);
			queue_Control_Transfer(device, &IK_setup, pHexBuf, this);
			pHex++;
			return 0;
		}
		pHex++;
	}
	return 1;
}

void IntelliKeys::IK_firmware_load()
{
	while (1) {
		switch (IK_state)
		{
			case 1:
				return;
				// Firmware load
			case 2: // set interface(0, 0)
				debug_println("set interface(0,0)");
				mk_setup(IK_setup, 1, 11, 0, 0, 0);
				queue_Control_Transfer(device, &IK_setup, NULL, this);
				IK_state = 3;
				return;
			case 3:
				ezusb_8051Reset(1);
				pHex = (PINTEL_HEX_RECORD)loader;
				IK_state = 4;
				return;
			case 4:
				debug_println("IKfl=4");
				// Download external records first
				if (ezusb_DownloadIntelHex(false) == 0) return;
				pHex = (PINTEL_HEX_RECORD)loader;
				IK_state = 6;
				break;
			case 5:
				debug_println("IKfl=5");
				// unused
				return;
			case 6:
				debug_println("IKfl=6");
				// Download internal records
				if (ezusb_DownloadIntelHex(true) == 0) return;
				IK_state = 8;
				break;
			case 7:
				debug_println("IKfl=7");
				// unused
				return;
			case 8:
				debug_println("IKfl=8");
				ezusb_8051Reset(0);
				pHex = (PINTEL_HEX_RECORD)firmware;
				IK_state = 9;
				return;
			case 9:
				debug_println("IKfl=9");
				// Download external records first
				if (ezusb_DownloadIntelHex(false) == 0) return;
				IK_state = 11;
				break;
			case 10:
				debug_println("IKfl=10");
				// unused
				return;
			case 11:
				debug_println("IKfl=11");
				ezusb_8051Reset(1);
				pHex = (PINTEL_HEX_RECORD)firmware;
				IK_state = 12;
				return;
			case 12:
				debug_println("IKfl=12");
				// Download internal records
				if (ezusb_DownloadIntelHex(true) == 0) return;
				ezusb_8051Reset(0);
				IK_state = 0;
				return;
			default:
				return;
		}
	}
}

void IntelliKeys::control(const Transfer_t *transfer)
{
	debug_println("control callback (IntelliKeys)");
	print_hexbytes(transfer->buffer, transfer->length);
	// To decode hex dump to human readable HID report summary:
	//   http://eleccelerator.com/usbdescreqparser/
	uint32_t mesg = transfer->setup.word1;
	debug_println("  mesg = ", mesg, HEX);
	if (IK_state != 1) IK_firmware_load();
}

void IntelliKeys::sensorUpdate(int sensor, int value)
{
	int midpoint = 150;

	if (eeprom_all_valid) {
		midpoint = (50*eeprom_data.sensorBlack[sensor] +
			50*eeprom_data.sensorWhite[sensor]) / 100;
	}
	int sensorOn = (value > midpoint);
	if (sensorStatus[sensor] != sensorOn) {
		if (sensor_callback) (*sensor_callback)(sensor, sensorOn);
		sensorStatus[sensor] = sensorOn;
	}
}

void IntelliKeys::handleEvents(const uint8_t *rxpacket, size_t len)
{
	if ((rxpacket == NULL) || (len == 0)) return;

	switch (*rxpacket) {
		case IK_EVENT_ACK:
			//debug_println("IK_EVENT_ACK");
			break;
		case IK_EVENT_MEMBRANE_PRESS:
			debug_printf("IK_EVENT_MEMBRANE_PRESS=(%d,%d)", rxpacket[1], rxpacket[2]);
			if(membrane_press_callback) (*membrane_press_callback)(rxpacket[1], rxpacket[2]);
			break;
		case IK_EVENT_MEMBRANE_RELEASE:
			debug_printf("IK_EVENT_MEMBRANE_RELEASE=(%d,%d)", rxpacket[1], rxpacket[2]);
			if (membrane_release_callback) (*membrane_release_callback)(rxpacket[1], rxpacket[2]);
			break;
		case IK_EVENT_SWITCH:
			debug_printf("IK_EVENT_SWITCH switch[%d]=%d", rxpacket[1], rxpacket[2]);
			if (switch_callback) (*switch_callback)(rxpacket[1], rxpacket[2]);
			break;
		case IK_EVENT_SENSOR_CHANGE:
			//debug_printf("IK_EVENT_SENSOR_CHANGE sensor[%d]=%d", rxpacket[1], rxpacket[2]);
			sensorUpdate(rxpacket[1], rxpacket[2]);
			break;
		case IK_EVENT_VERSION:
			debug_printf("IK_EVENT_VERSION= %d.%d", rxpacket[1], rxpacket[2]);
			if (!version_done && version_callback) (*version_callback)(rxpacket[1], rxpacket[2]);
			version_done = true;
			break;
		case IK_EVENT_EEPROM_READ:
			debug_println("IK_EVENT_EEPROM_READ");
			break;
		case IK_EVENT_ONOFFSWITCH:
			debug_printf("IK_EVENT_ONOFFSWITCH= %d", rxpacket[1]);
			if (on_off_callback) {
				if (rxpacket[1]) get_all_sensors();
				(*on_off_callback)(rxpacket[1]);
			}
			break;
		case IK_EVENT_NOMOREEVENTS:
			debug_println("IK_EVENT_NOMOREEVENTS");
			break;
		case IK_EVENT_MEMBRANE_REPEAT:
			debug_println("IK_EVENT_MEMBRANE_REPEAT");
			break;
		case IK_EVENT_SWITCH_REPEAT:
			debug_println("IK_EVENT_SWITCH_REPEAT");
			break;
		case IK_EVENT_CORRECT_MEMBRANE:
			debug_println("IK_EVENT_CORRECT_MEMBRANE");
			break;
		case IK_EVENT_CORRECT_SWITCH:
			debug_println("IK_EVENT_CORRECT_SWITCH");
			break;
		case IK_EVENT_CORRECT_DONE:
			debug_println("IK_EVENT_CORRECT_DONE");
			break;
		case IK_EVENT_EEPROM_READBYTE:
			debug_print("IK_EVENT_EEPROM_READBYTE ");
			{
				uint8_t idx = rxpacket[2] - 0x80;
				uint8_t *p = (uint8_t *)&eeprom_data;
				p[idx] = rxpacket[1];
				eeprom_valid[idx] = true;
			}
			break;
		case IK_EVENT_DEVICEREADY:
			debug_println("IK_EVENT_DEVICEREADY");
			break;
		case IK_EVENT_AUTOPILOT_STATE:
			debug_println("IK_EVENT_AUTOPILOT_STATE");
			break;
		case IK_EVENT_DELAY:
			debug_println("IK_EVENT_DELAY");
			break;
		case IK_EVENT_ALL_SENSORS:
			debug_println("IK_EVENT_ALL_SENSORS");
			break;
		default:
			debug_printf("Unknown event code=%d\n", *rxpacket);
			break;
	}
}

void IntelliKeys::Task()
{
	if (IK_state == 2) IK_firmware_load();

	for (int i = 0; i < 3; i++) {
		uint32_t len = rxlen[i];
		if (len) {
			if (i == 0) handleEvents(rxpacket[i], len);
			NVIC_DISABLE_IRQ(IRQ_USBHS);
			queue_Data_Transfer(rxpipe[i], rxpacket[i], 64, this);
			rxlen[i] = 0;
			NVIC_ENABLE_IRQ(IRQ_USBHS);
		}
	}

	if (!eeprom_all_valid) get_eeprom();
}

void IntelliKeys::begin()
{
}
