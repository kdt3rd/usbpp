// HIDDevice.h -*- C++ -*-

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
#ifndef _usbpp_HIDDevice_h_
#define _usbpp_HIDDevice_h_ 1

#include "Device.h"


////////////////////////////////////////


///
/// @file HIDDevice.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class HIDDevice provides...
///
class HIDDevice : public Device
{
public:
	HIDDevice( void );
	HIDDevice( libusb_device *dev );
	HIDDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~HIDDevice( void );

	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

protected:
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );
	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

	virtual void parseInterface( uint8_t inum, const struct libusb_interface_descriptor &ifacedesc );

	virtual void dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iface );

private:
	void parseReport( const uint8_t *reportDesc, size_t rSize );
	void dumpReport( std::ostream &os, const uint8_t *reportDesc, size_t rSize );

#pragma pack(push, 1)
	struct Descriptor
	{
		uint8_t bDescriptorType;
		uint16_t wDescriptorLength;
	};
	struct HID_info
	{
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint16_t bcdHID;
		uint8_t bCountryCode;
		uint8_t bNumDescriptors;
		Descriptor bDescriptors[1];
	};
#pragma pack(pop)
};

} // namespace usbpp

#endif // _usbpp_HIDDevice_h_


