// ORBOptronixDevice.h -*- C++ -*-

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

#include "Device.h"


////////////////////////////////////////


///
/// @file ORBOptronixDevice.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class ORBOptronixDevice provides...
///
class ORBOptronixDevice : public Device
{
public:
	ORBOptronixDevice( void );
	ORBOptronixDevice( libusb_device *dev );
	ORBOptronixDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~ORBOptronixDevice( void );

	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

	void captureData( void );
protected:
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );
	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

private:

};

} // namespace USB

