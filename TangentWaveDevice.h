//
// Copyright (c) 2014 Kimball Thurston
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

#include "HIDDevice.h"


////////////////////////////////////////



namespace USB
{

///
/// @brief Class TangentWaveDevice provides...
///
class TangentWaveDevice : public HIDDevice
{
public:
	TangentWaveDevice( void );
	TangentWaveDevice( libusb_device *dev );
	TangentWaveDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~TangentWaveDevice( void );

	// panel 0 - 2
	// line 0 - 4
	// pos 0 - 31
	// text 32 chars max
	//  0 - 0x1F are custom characters
	//  0x20 - 0x7F are ascii
	//  0x7F - 0xFF display inverted
	void setText( uint8_t panel, uint8_t line, uint8_t pos,
				  const std::string &text );
	// charID is 0 - 0x1F
	// font is 8x13, each byte of data maps to one line of character bitmap
	void setCustomCharacter( uint8_t charID, uint8_t data[13] );

	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

protected:
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );

	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

private:
#pragma pack(push, 1)
	static const size_t kNumButtons = 41;
	static const size_t kNumEncoders = 13;
	static const size_t kNumBalls = 3;
	struct RollerBall
	{
		int8_t dx;
		int8_t dy;
	};

	struct PanelEvent
	{
		// buttons
		uint8_t buttons[6];  // 6
		int8_t encoders[13]; // 13 bytes
		RollerBall balls[3]; // 6 bytes
		uint8_t status; // 1 byte, reserved for firmware update
	};
#pragma pack(pop)
	uint8_t myTextSetEndpoint = 0;
	uint8_t myTextSetSize = 0;
	int myTextInterval = 5 * 1000;
	PanelEvent myLastPanelEvent = {0};
};

} // namespace USB



