// SpacePilotDevice.h -*- C++ -*-

//
// Copyright (c) 2012 Kimball Thurston
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#pragma once
#ifndef _usbpp_SpacePilotDevice_h_
#define _usbpp_SpacePilotDevice_h_ 1

#include <array>
#include <vector>
#include "Device.h"


////////////////////////////////////////


///
/// @file SpacePilotDevice.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class SpacePilotDevice provides...
///
class SpacePilotDevice : public Device
{
public:
	static const uint16_t VendorID = 0x046d;
	static const uint16_t ProductID = 0xc629;

	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

	enum WhichButton
	{
		LCD_UP_ARROW, // byte 0, bit 7
		LCD_DOWN_ARROW, // byte 0, bit 6
		LCD_RIGHT_ARROW, // byte 0, bit 4
		LCD_LEFT_ARROW, // byte 0, bit 5
		LCD_OK_BUTTON, // byte 0, bit 3
		LCD_SQUARE_BUTTON, // byte 0, bit 2
		LCD_BACK_BUTTON, // byte 0, bit 1
		LCD_TOGGLE_SQUARE_BUTTON, // byte 0, bit 0
		LCD_TOGGLE_DISPLAY, // byte 1, bit 1
		SPACENAV_FIT, // byte 0, bit 0
		SPACENAV_MENU, // byte 0, bit 1
		SPACENAV_ESC, // byte 2, bit 6
		SPACENAV_SHIFT, // byte 3, bit 0
		SPACENAV_CTRL, // byte 3, bit 1
		SPACENAV_ALT, // byte 2, bit 7
		SPACENAV_1, // byte 1, bit 4
		SPACENAV_2, // byte 1, bit 5
		SPACENAV_3, // byte 1, bit 6
		SPACENAV_4, // byte 1, bit 7
		SPACENAV_5, // byte 2, bit 0
		SPACENAV_PLUS, // byte 3, bit 5
		SPACENAV_MINUS, // byte 3, bit 6
		SPACENAV_SNAP_SCALE, // byte 3, bit 4
		SPACENAV_SNAP_TRANSLATE, // byte 3, bit 3
		SPACENAV_SNAP_ROTATION, // byte 3, bit 2
		SPACENAV_ROTATE_OBJ, // byte 1, bit 0
		SPACENAV_TRANSLATE_OBJ, // byte 0, bit 2
		SPACENAV_F_OBJ, // byte 0, bit 5
		SPACENAV_B_OBJ, // byte 0, bit 4
		SPACENAV_ISO1, // byte 1, bit 2
	};

	SpacePilotDevice( void );
	SpacePilotDevice( libusb_device *dev );
	SpacePilotDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~SpacePilotDevice( void );

	void setLCD( uint8_t r, uint8_t g, uint8_t b );
	void setLCD( const uint8_t *rgbdata, int w, int h );
	void setLCD( const std::string &imagefilename );

protected:
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );

	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

private:
	void handleLCDButton( uint8_t *buf );
	void handleSpaceNav( int report, uint8_t *buf, int buflen );

	size_t fillLCDData( void );
	void loadLCD( void );

	std::array<bool, 8> myLCDButtonState;
	std::array<bool, 21> mySpaceNavButtonState;
	int myLCDEndpoint = 0;
	std::vector<uint16_t> myLCDData;
};

} // namespace usbpp

#endif // _usbpp_SpacePilotDevice_h_


