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

#include "TangentWaveDevice.h"
#include "Logger.h"
#include "Util.h"
#include <unistd.h>
#include <math.h>


////////////////////////////////////////


namespace
{

static USB::TangentButton butMap[] =
{
	USB::TangentButton::ENCODER_LEFT_1,
	USB::TangentButton::ENCODER_LEFT_2,
	USB::TangentButton::ENCODER_LEFT_3,
	USB::TangentButton::ENCODER_CENTER_1,
	USB::TangentButton::ENCODER_CENTER_2,
	USB::TangentButton::ENCODER_CENTER_3,
	USB::TangentButton::ENCODER_RIGHT_1,
	USB::TangentButton::ENCODER_RIGHT_2,
	USB::TangentButton::ENCODER_RIGHT_3,
	USB::TangentButton::ALT,
	USB::TangentButton::UNDER_DISPLAY_LEFT_1,
	USB::TangentButton::UNDER_DISPLAY_LEFT_2,
	USB::TangentButton::UNDER_DISPLAY_LEFT_3,
	USB::TangentButton::OPEN_CIRCLE_ENCODER_1,
	USB::TangentButton::CLOSE_CIRCLE_ENCODER_1,
	USB::TangentButton::UNDER_DISPLAY_CENTER_1,
	USB::TangentButton::UNDER_DISPLAY_CENTER_2,
	USB::TangentButton::UNDER_DISPLAY_CENTER_3,
	USB::TangentButton::OPEN_CIRCLE_ENCODER_2,
	USB::TangentButton::CLOSE_CIRCLE_ENCODER_2,
	USB::TangentButton::UNDER_DISPLAY_RIGHT_1,
	USB::TangentButton::UNDER_DISPLAY_RIGHT_2,
	USB::TangentButton::UNDER_DISPLAY_RIGHT_3,
	USB::TangentButton::OPEN_CIRCLE_ENCODER_3,
	USB::TangentButton::CLOSE_CIRCLE_ENCODER_3,
	USB::TangentButton::UP,
	USB::TangentButton::DOWN,
	USB::TangentButton::F7,
	USB::TangentButton::F8,
	USB::TangentButton::F9,
	USB::TangentButton::F4,
	USB::TangentButton::F5,
	USB::TangentButton::F6,
	USB::TangentButton::F1,
	USB::TangentButton::F2,
	USB::TangentButton::F3,
	USB::TangentButton::JUMP_LEFT,
	USB::TangentButton::JUMP_RIGHT,
	USB::TangentButton::PLAY_REVERSE,
	USB::TangentButton::PLAY_STOP,
	USB::TangentButton::PLAY_FORWARD
};

static USB::TangentEncoder encMap[] =
{
	USB::TangentEncoder::ENCODER_TOP_LEFT_1,
	USB::TangentEncoder::ENCODER_TOP_LEFT_2,
	USB::TangentEncoder::ENCODER_TOP_LEFT_3,
	USB::TangentEncoder::ENCODER_TOP_CENTER_1,
	USB::TangentEncoder::ENCODER_TOP_CENTER_2,
	USB::TangentEncoder::ENCODER_TOP_CENTER_3,
	USB::TangentEncoder::ENCODER_TOP_RIGHT_1,
	USB::TangentEncoder::ENCODER_TOP_RIGHT_2,
	USB::TangentEncoder::ENCODER_TOP_RIGHT_3,
	USB::TangentEncoder::ENCODER_LEFT,
	USB::TangentEncoder::ENCODER_CENTER,
	USB::TangentEncoder::ENCODER_RIGHT,
	USB::TangentEncoder::JOG_SHUTTLE
};

static USB::TangentBall ballMap[] =
{
	USB::TangentBall::LEFT,
	USB::TangentBall::CENTER,
	USB::TangentBall::RIGHT,
};

static std::map<USB::TangentButton, std::string> butNames = 
{
	{ USB::TangentButton::ENCODER_LEFT_1, "ENCODER_LEFT_1" },
	{ USB::TangentButton::ENCODER_LEFT_2, "ENCODER_LEFT_2" },
	{ USB::TangentButton::ENCODER_LEFT_3, "ENCODER_LEFT_3" },
	{ USB::TangentButton::ENCODER_CENTER_1, "ENCODER_CENTER_1" },
	{ USB::TangentButton::ENCODER_CENTER_2, "ENCODER_CENTER_2" },
	{ USB::TangentButton::ENCODER_CENTER_3, "ENCODER_CENTER_3" },
	{ USB::TangentButton::ENCODER_RIGHT_1, "ENCODER_RIGHT_1" },
	{ USB::TangentButton::ENCODER_RIGHT_2, "ENCODER_RIGHT_2" },
	{ USB::TangentButton::ENCODER_RIGHT_3, "ENCODER_RIGHT_3" },
	{ USB::TangentButton::UNDER_DISPLAY_LEFT_1, "UNDER_DISPLAY_LEFT_1" },
	{ USB::TangentButton::UNDER_DISPLAY_LEFT_2, "UNDER_DISPLAY_LEFT_2" },
	{ USB::TangentButton::UNDER_DISPLAY_LEFT_3, "UNDER_DISPLAY_LEFT_3" },
	{ USB::TangentButton::UNDER_DISPLAY_CENTER_1, "UNDER_DISPLAY_CENTER_1" },
	{ USB::TangentButton::UNDER_DISPLAY_CENTER_2, "UNDER_DISPLAY_CENTER_2" },
	{ USB::TangentButton::UNDER_DISPLAY_CENTER_3, "UNDER_DISPLAY_CENTER_3" },
	{ USB::TangentButton::UNDER_DISPLAY_RIGHT_1, "UNDER_DISPLAY_RIGHT_1" },
	{ USB::TangentButton::UNDER_DISPLAY_RIGHT_2, "UNDER_DISPLAY_RIGHT_2" },
	{ USB::TangentButton::UNDER_DISPLAY_RIGHT_3, "UNDER_DISPLAY_RIGHT_3" },
	{ USB::TangentButton::ALT, "ALT" },
	{ USB::TangentButton::OPEN_CIRCLE_ENCODER_1, "OPEN_CIRCLE_ENCODER_1" },
	{ USB::TangentButton::CLOSE_CIRCLE_ENCODER_1, "CLOSE_CIRCLE_ENCODER_1" },
	{ USB::TangentButton::OPEN_CIRCLE_ENCODER_2, "OPEN_CIRCLE_ENCODER_2" },
	{ USB::TangentButton::CLOSE_CIRCLE_ENCODER_2, "CLOSE_CIRCLE_ENCODER_2" },
	{ USB::TangentButton::OPEN_CIRCLE_ENCODER_3, "OPEN_CIRCLE_ENCODER_3" },
	{ USB::TangentButton::CLOSE_CIRCLE_ENCODER_3, "CLOSE_CIRCLE_ENCODER_3" },
	{ USB::TangentButton::UP, "UP" },
	{ USB::TangentButton::DOWN, "DOWN" },
	{ USB::TangentButton::F1, "F1" },
	{ USB::TangentButton::F2, "F2" },
	{ USB::TangentButton::F3, "F3" },
	{ USB::TangentButton::F4, "F4" },
	{ USB::TangentButton::F5, "F5" },
	{ USB::TangentButton::F6, "F6" },
	{ USB::TangentButton::F7, "F7" },
	{ USB::TangentButton::F8, "F8" },
	{ USB::TangentButton::F9, "F9" },
	{ USB::TangentButton::JUMP_RIGHT, "JUMP_RIGHT" },
	{ USB::TangentButton::JUMP_LEFT, "JUMP_LEFT" },
	{ USB::TangentButton::PLAY_FORWARD, "PLAY_FORWARD" },
	{ USB::TangentButton::PLAY_STOP, "PLAY_STOP" },
	{ USB::TangentButton::PLAY_REVERSE, "PLAY_REVERSE" }
};

static std::map<USB::TangentEncoder, std::string> encNames = 
{
	{ USB::TangentEncoder::ENCODER_TOP_LEFT_1, "ENCODER_TOP_LEFT_1" },
	{ USB::TangentEncoder::ENCODER_TOP_LEFT_2, "ENCODER_TOP_LEFT_2" },
	{ USB::TangentEncoder::ENCODER_TOP_LEFT_3, "ENCODER_TOP_LEFT_3" },
	{ USB::TangentEncoder::ENCODER_LEFT, "ENCODER_LEFT" },
	{ USB::TangentEncoder::ENCODER_TOP_CENTER_1, "ENCODER_TOP_CENTER_1" },
	{ USB::TangentEncoder::ENCODER_TOP_CENTER_2, "ENCODER_TOP_CENTER_2" },
	{ USB::TangentEncoder::ENCODER_TOP_CENTER_3, "ENCODER_TOP_CENTER_3" },
	{ USB::TangentEncoder::ENCODER_CENTER, "ENCODER_CENTER" },
	{ USB::TangentEncoder::ENCODER_TOP_RIGHT_1, "ENCODER_TOP_RIGHT_1" },
	{ USB::TangentEncoder::ENCODER_TOP_RIGHT_2, "ENCODER_TOP_RIGHT_2" },
	{ USB::TangentEncoder::ENCODER_TOP_RIGHT_3, "ENCODER_TOP_RIGHT_3" },
	{ USB::TangentEncoder::ENCODER_RIGHT, "ENCODER_RIGHT" },
	{ USB::TangentEncoder::JOG_SHUTTLE, "JOG_SHUTTLE" }
};

static std::map<USB::TangentBall, std::string> ballNames = 
{
	{ USB::TangentBall::LEFT, "LEFT" },
	{ USB::TangentBall::CENTER, "CENTER" },
	{ USB::TangentBall::RIGHT, "RIGHT" }
};

}


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


TangentWaveDevice::TangentWaveDevice( void )
{
	myEventLoopThread = std::thread( &TangentWaveDevice::eventLoop, this );
}


////////////////////////////////////////


TangentWaveDevice::TangentWaveDevice( libusb_device *dev )
		: HIDDevice( dev )
{
	// per their doc requested via support, standard HID device
	// with an output report. output report has a command ID and
	// variable length message
	// two commands are supported:
	// 0x00 set text
	// 0x01 set custom character bitmap
	static_assert( sizeof(PanelEvent) == 26, "Invalid structure layout for panel event" );
	myEventLoopThread = std::thread( &TangentWaveDevice::eventLoop, this );
}


////////////////////////////////////////


TangentWaveDevice::TangentWaveDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: HIDDevice( dev, desc )
{
	myEventLoopThread = std::thread( &TangentWaveDevice::eventLoop, this );
}


////////////////////////////////////////


TangentWaveDevice::~TangentWaveDevice( void )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );
	myEventLoopRunning = false;
	myEventNotify.notify_all();
	lk.unlock();
	myEventLoopThread.join();
}

////////////////////////////////////////

void
TangentWaveDevice::eventLoop( void )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );

	while ( myEventLoopRunning )
	{
		while ( ! myEvents.empty() )
		{
			PanelEvent event = myEvents.front();
			myEvents.pop_front();
			for ( size_t i = 0; i < kNumEncoders; ++i )
			{
				if ( event.encoders[i] != 0 )
				{
//					std::cout << "Encoder " << i + 1 << " (" << encNames[encMap[i]] << "): delta " << int(event.encoders[i]) << std::endl;
					auto x = myEncoderCallbacks.find( encMap[i] );
					if ( x != myEncoderCallbacks.end() )
					{
						for ( auto f: x->second )
						{
							lk.unlock();
							f( int(event.encoders[i]) );
							lk.lock();
						}
					}
				}
			
			}
			for ( size_t i = 0; i < kNumBalls; ++i )
			{
				if ( event.balls[i].dx != 0 || event.balls[i].dy != 0 )
				{
					float ang = atan2f( float(event.balls[i].dy), float(event.balls[i].dx) );
					ang *= 180.F / M_PI;

					float mag = sqrtf( float(event.balls[i].dy)*float(event.balls[i].dy) + float(event.balls[i].dx)*float(event.balls[i].dx) );
//					std::cout << "Ball " << i + 1 << " (" << ballNames[ballMap[i]] << "): dx " << int(event.balls[i].dx) << " dy " << int(event.balls[i].dy) << " angle " << ang << " mag " << mag << std::endl;
					auto x = myBallCallbacks.find( ballMap[i] );
					if ( x != myBallCallbacks.end() )
					{
						for ( auto f: x->second )
						{
							lk.unlock();
							f( int(event.balls[i].dx), int(event.balls[i].dy), ang, mag );
							lk.lock();
						}
					}
				}
			}

			for ( size_t i = 0; i < kNumButtons; ++i )
			{
				size_t byteOff = i / 8;
				size_t bit = (i % 8);
				bool newState = ( event.buttons[byteOff] >> bit ) & 0x1;
				bool oldState = ( myLastPanelEvent.buttons[byteOff] >> bit ) & 0x1;
				if ( newState != oldState )
				{
//					std::cout << "Button " << (i+1) << " (" << butNames.size() << ' ' << sizeof(butMap)/sizeof(TangentButton) << ' ' << butNames[butMap[i]] << ")" << (newState ? " DOWN" : " UP") << std::endl;
					auto x = myButtonCallbacks.find( butMap[i] );
					if ( x != myButtonCallbacks.end() )
					{
						for ( auto f: x->second )
						{
							lk.unlock();
							f( newState );
							lk.lock();
						}
					}
				}
			}
			myLastPanelEvent = event;
		}

		if ( myEventLoopRunning )
			myEventNotify.wait( lk );
	}
}


////////////////////////////////////////


void
TangentWaveDevice::resetCallbacks( void )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );
	myButtonCallbacks.clear();
	myEncoderCallbacks.clear();
	myBallCallbacks.clear();
}


////////////////////////////////////////


void
TangentWaveDevice::addButtonCallback( TangentButton bt, const std::function<void (bool)> &f )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );
	myButtonCallbacks[bt].push_back( f );
}


////////////////////////////////////////


void
TangentWaveDevice::addEncoderCallback( TangentEncoder e, const std::function<void (int)> &f )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );
	myEncoderCallbacks[e].push_back( f );
}


////////////////////////////////////////


void
TangentWaveDevice::addBallCallback( TangentBall b, const std::function<void (int, int, float, float)> &f )
{
	std::unique_lock<std::mutex> lk( myCallbackMutex );
	myBallCallbacks[b].push_back( f );
}


////////////////////////////////////////


void
TangentWaveDevice::clearText( void )
{
	std::string txt( 32, ' ' );
	for ( uint8_t panel = 0; panel < 3; ++panel )
		for ( uint8_t line = 0; line < 5; ++line )
			setText( panel, line, 0, txt );
}


////////////////////////////////////////


void
TangentWaveDevice::setText( uint8_t panel, uint8_t line, uint8_t pos,
							const std::string &text )
{
	// byte:
	// 0 - command
	// 1 - display ID (0 - 2)
	// 2 - line (0 - 4)
	// 3 - pos (0 - 31)
	// 4 - length (1 - 32)
	// 5-37 - text
	// command is 0x00 for text write
	uint8_t data[40];

	data[0] = 0x0;
	data[1] = panel;
	data[2] = line;
	data[3] = pos;
	data[4] = static_cast<uint8_t>( std::min( size_t(32), text.size() ) );
	for ( uint8_t i = 0; i < data[4]; ++i )
		data[i+5] = text[i];

	int messageSize = 5 + int(data[4]);
	InterruptTransfer xfr( myContext, myTextSetSize );
	xfr.fill( myHandle, endpoint_out( myTextSetEndpoint ),
			  data, messageSize );
	xfr.submit();
	// make sure we don't send more than 1 text request per the
	// interval so the device isn't swamped
	usleep( myTextInterval );

	int transferred = xfr.wait();
	if ( transferred < messageSize )
		std::cout << "setText request to transfer " << messageSize << ", transferred " << transferred << std::endl;
}


////////////////////////////////////////


void
TangentWaveDevice::setCustomCharacter( uint8_t charID, uint8_t bitmap[13] )
{
	// byte:
	// 0 - command
	// 1 - char id (0 - 0x1F)
	// 2-14 - data
	uint8_t data[40];

	data[0] = 0x01;
	data[1] = charID;
	for ( uint8_t i = 0; i < 13; ++i )
		data[i+2] = bitmap[i];

	int messageSize = 15;
	InterruptTransfer xfr( myContext, myTextSetSize );
	xfr.fill( myHandle, endpoint_out( myTextSetEndpoint ),
			  data, messageSize );
	xfr.submit();
	// make sure we don't send more than 1 text request per the
	// interval so the device isn't swamped
	usleep( myTextInterval );

	int transferred = xfr.wait();
	if ( transferred < messageSize )
		std::cout << "setText request to transfer " << messageSize << ", transferred " << transferred << std::endl;
}


////////////////////////////////////////


std::shared_ptr<Device>
TangentWaveDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::make_shared<TangentWaveDevice>( dev, desc );
}


////////////////////////////////////////


bool
TangentWaveDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
	if ( ! buf )
	{
		std::cout << "Unexpected null buffer: len " << buflen << std::endl;
		return true;
	}
	if ( ( buflen % int(sizeof(PanelEvent)) ) != 0 )
	{
		std::cout << "Unexpected event size: " << buflen << std::endl;
		return true;
	}

	int nEvents = buflen / int(sizeof(PanelEvent));
	for ( int e = 0; e < nEvents; ++e )
	{
		const PanelEvent *eventPtr = reinterpret_cast<const PanelEvent *>( buf + sizeof(PanelEvent) * e );
		std::unique_lock<std::mutex> lk( myCallbackMutex );
		myEvents.push_back( *eventPtr );
		myEventNotify.notify_one();
	}
	return true;
}


////////////////////////////////////////


bool
TangentWaveDevice::wantInterface( const struct libusb_interface_descriptor &iface )
{
	int nep = int(iface.bNumEndpoints);
	for ( int ep = 0; ep < nep; ++ep )
	{
		const struct libusb_endpoint_descriptor &epDesc = iface.endpoint[ep];
		bool isOutput = ( epDesc.bEndpointAddress & 0x80 ) == LIBUSB_ENDPOINT_OUT;
		if ( isOutput )
		{
			myTextSetEndpoint = epDesc.bEndpointAddress;
			myTextSetSize = epDesc.wMaxPacketSize;
			myTextInterval = int(epDesc.bInterval) * 1000;
			libusb_transfer_type tt = static_cast<libusb_transfer_type>( epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK );
			if ( tt != LIBUSB_TRANSFER_TYPE_INTERRUPT )
				throw std::runtime_error( "Expecting interrupt transfer for text set endpoint" );
		}
	}

	return true;
}


////////////////////////////////////////


} // USB



