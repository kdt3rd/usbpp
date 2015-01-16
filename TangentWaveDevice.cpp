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


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


TangentWaveDevice::TangentWaveDevice( void )
{
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
}


////////////////////////////////////////


TangentWaveDevice::TangentWaveDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: HIDDevice( dev, desc )
{
}


////////////////////////////////////////


TangentWaveDevice::~TangentWaveDevice( void )
{
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
	if ( buflen != int(sizeof(PanelEvent)) )
	{
		std::cout << "Unexpected event size: " << buflen << std::endl;
		return true;
	}

	const PanelEvent *eventPtr = reinterpret_cast<const PanelEvent *>( buf );
	const PanelEvent &event = *(eventPtr);
	for ( size_t i = 0; i < kNumEncoders; ++i )
	{
		if ( event.encoders[i] != 0 )
			std::cout << "Encoder " << i + 1 << ": delta " << int(event.encoders[i]) << std::endl;
	}
	for ( size_t i = 0; i < kNumBalls; ++i )
	{
		if ( event.balls[i].dx != 0 || event.balls[i].dy != 0 )
			std::cout << "Ball " << i + 1 << ": dx " << int(event.balls[i].dx) << " dy " << int(event.balls[i].dy) << std::endl;
	}

	for ( size_t i = 0; i < kNumButtons; ++i )
	{
		size_t byteOff = i / 8;
		size_t bit = (i % 8);
		bool newState = ( event.buttons[byteOff] >> bit ) & 0x1;
		bool oldState = ( myLastPanelEvent.buttons[byteOff] >> bit ) & 0x1;
		if ( newState != oldState )
		{
			std::cout << "Button " << (i+1) << (newState ? " DOWN" : " UP") << std::endl;
		}
	}
	myLastPanelEvent = event;
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



