// ORBOptronixDevice.cpp -*- C++ -*-

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

#include "ORBOptronixDevice.h"


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


ORBOptronixDevice::ORBOptronixDevice( void )
{
}


////////////////////////////////////////


ORBOptronixDevice::ORBOptronixDevice( libusb_device *dev )
		: Device( dev )
{
}


////////////////////////////////////////


ORBOptronixDevice::ORBOptronixDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: Device( dev, desc )
{
}


////////////////////////////////////////


ORBOptronixDevice::~ORBOptronixDevice( void )
{
}


////////////////////////////////////////


std::shared_ptr<Device>
ORBOptronixDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::shared_ptr<Device>( new ORBOptronixDevice( dev, desc ) );
}


////////////////////////////////////////


void
ORBOptronixDevice::captureData( void )
{
	std::cout << "Sending RST to device..." << std::endl;
	std::vector<uint8_t> cmd;
	cmd.push_back( '*' );
	cmd.push_back( 'R' );
	cmd.push_back( 'S' );
	cmd.push_back( 'T' );
	cmd.push_back( '\n' );
	BulkTransfer rst( myContext, 64 );
	rst.send( myHandle,
			  endpoint_in(0) | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
			  &cmd[0], cmd.size() );
	std::cout << "Sent..." << std::endl;
}


////////////////////////////////////////


bool
ORBOptronixDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
	std::cout << "ORBOptronixDevice::handleEvent( " << endpoint << ", " << buflen << " )" << std::endl;
	if ( buflen > 0 )
	{
		std::cout << "DATA: '" << reinterpret_cast<char *>( buf ) << "'" << std::endl;
	}
	return true;
}


////////////////////////////////////////


bool
ORBOptronixDevice::wantInterface( const struct libusb_interface_descriptor &iface )
{
	return true;
}


////////////////////////////////////////


} // usbpp



