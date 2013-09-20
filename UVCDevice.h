// UVCDevice.h -*- C++ -*-

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
#ifndef _usbpp_UVCDevice_h_
#define _usbpp_UVCDevice_h_ 1

#include "Device.h"

////////////////////////////////////////


///
/// @file UVCDevice.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class UVCDevice provides a simple UVC video interface...
///
class UVCDevice : public Device
{
public:
	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

	UVCDevice( void );
	UVCDevice( libusb_device *dev );
	UVCDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~UVCDevice( void );

	void queryValues( void );

	void startVideo( void );

protected:
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );

	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

	virtual std::shared_ptr<AsyncTransfer> createTransfer( const struct libusb_endpoint_descriptor &epDesc );

	virtual void dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iface );

private:
	void dumpControls( std::ostream &os, const uint8_t type, const uint8_t *bmControls, int bControlSize );
	void dumpStandardControls( std::ostream &os, const unsigned char *buffer, int buflen );
	void dumpStandardControlsAndFormats( std::ostream &os, const unsigned char *buffer, int buflen );

private:

#pragma pack(push,1)

	struct UVCProbe
	{
		uint16_t bmHint;
		uint8_t bFormatIndex;
		uint8_t bFrameIndex;
		uint32_t dwFrameInterval;
		uint16_t wKeyFrameRate;
		uint16_t wPFrameRate;
		uint16_t wCompQuality;
		uint16_t wCompWindowSize;
		uint16_t wDelay;
		uint32_t dwMaxVideoFrameSize;
		uint32_t dwMaxPayloadTransferSize;
		uint32_t dwClockFrequency;
		uint8_t bmFramingInfo;
		uint8_t bPreferredVersion;
		uint8_t bMinVersion;
		uint8_t bMaxVersion; // UVC 1.1
	};

#pragma pack(pop)

	int myControlInterface = 0;
	int myCameraInterface = 1;
	UVCProbe myDeviceInfo;
	uint16_t myUVCVersion = 0;
};

} // namespace usbpp

#endif // _usbpp_UVCDevice_h_


