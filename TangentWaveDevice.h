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
#include <map>
#include <list>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>


////////////////////////////////////////


namespace USB
{

enum class TangentButton
{
	ENCODER_LEFT_1,
	ENCODER_LEFT_2,
	ENCODER_LEFT_3,
	ENCODER_CENTER_1,
	ENCODER_CENTER_2,
	ENCODER_CENTER_3,
	ENCODER_RIGHT_1,
	ENCODER_RIGHT_2,
	ENCODER_RIGHT_3,
	UNDER_DISPLAY_LEFT_1,
	UNDER_DISPLAY_LEFT_2,
	UNDER_DISPLAY_LEFT_3,
	UNDER_DISPLAY_CENTER_1,
	UNDER_DISPLAY_CENTER_2,
	UNDER_DISPLAY_CENTER_3,
	UNDER_DISPLAY_RIGHT_1,
	UNDER_DISPLAY_RIGHT_2,
	UNDER_DISPLAY_RIGHT_3,
	ALT,
	OPEN_CIRCLE_ENCODER_1,
	CLOSE_CIRCLE_ENCODER_1,
	OPEN_CIRCLE_ENCODER_2,
	CLOSE_CIRCLE_ENCODER_2,
	OPEN_CIRCLE_ENCODER_3,
	CLOSE_CIRCLE_ENCODER_3,
	UP,
	DOWN,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	JUMP_RIGHT,
	JUMP_LEFT,
	PLAY_FORWARD,
	PLAY_STOP,
	PLAY_REVERSE
};

enum class TangentEncoder
{
	ENCODER_TOP_LEFT_1,
	ENCODER_TOP_LEFT_2,
	ENCODER_TOP_LEFT_3,
	ENCODER_LEFT,
	ENCODER_TOP_CENTER_1,
	ENCODER_TOP_CENTER_2,
	ENCODER_TOP_CENTER_3,
	ENCODER_CENTER,
	ENCODER_TOP_RIGHT_1,
	ENCODER_TOP_RIGHT_2,
	ENCODER_TOP_RIGHT_3,
	ENCODER_RIGHT,
	JOG_SHUTTLE
};

enum class TangentBall
{
	LEFT,
	CENTER,
	RIGHT
};


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

	// clears out all callbacks
	void resetCallbacks( void );

	// passed true when button goes down, false when button goes up
	void addButtonCallback( TangentButton bt, const std::function<void (bool)> &f );

	// passed the delta for that encoder
	void addEncoderCallback( TangentEncoder e, const std::function<void (int)> &f );

	// passed the deltax and deltay for that ball and the angle and magnitude that corresponds to
	void addBallCallback( TangentBall b, const std::function<void (int, int, float, float)> &f );

	// clears all the text on all the panels
	void clearText( void );
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
	void eventLoop( void );

#pragma pack(push, 1)
	static const size_t kNumButtons = 41;
	static const size_t kNumEncoders = 13;
	static const size_t kNumBalls = 3;
	struct RollerBall
	{
		int8_t dx = 0;
		int8_t dy = 0;
	};

	struct PanelEvent
	{
		// buttons
		uint8_t buttons[6] = {0};  // 6
		int8_t encoders[13] = {0}; // 13 bytes
		RollerBall balls[3]; // 6 bytes
		uint8_t status = 0; // 1 byte, reserved for firmware update
	};
#pragma pack(pop)
	uint8_t myTextSetEndpoint = 0;
	uint8_t myTextSetSize = 0;
	int myTextInterval = 5 * 1000;
	PanelEvent myLastPanelEvent;
	std::mutex myCallbackMutex;
	std::condition_variable myEventNotify;
	bool myEventLoopRunning = true;
	std::thread myEventLoopThread;
	std::list<PanelEvent> myEvents;

	std::map<TangentButton, std::vector<std::function<void (bool)> > > myButtonCallbacks;
	std::map<TangentEncoder, std::vector<std::function<void (int)> > > myEncoderCallbacks;
	std::map<TangentBall, std::vector<std::function<void (int, int, float, float)> > > myBallCallbacks;
};

} // namespace USB



