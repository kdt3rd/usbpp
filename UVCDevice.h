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
#include "Stream.h"
#include "Control.h"
#include <string>
#include <vector>
#include <functional>


////////////////////////////////////////


///
/// @file UVCDevice.h
///
/// @author Kimball Thurston
///

namespace USB
{

// strictly speaking, this is a tiered thing where formats have
// frames, but collapse here since the primary interest is frames and
// formats are just collections
struct FrameDefinition
{
	uint8_t format_index;
	uint8_t frame_index;
	uint8_t interface;

	ImageBuffer::Format format;

	int width;
	int height;
	int bytesPerLine;
	int bytesPerPixel;

	uint32_t defaultFrameInterval;
	bool variableFrameInterval;
	std::vector<uint32_t> availableIntervals;
};

///
/// @brief Class UVCDevice provides a simple UVC video interface...
///
class UVCDevice : public Device
{
public:
	typedef std::function<void (const std::shared_ptr<ImageBuffer> &imgBuf)> ImageReceivedCallback;

	static std::shared_ptr<Device> factory( libusb_device *dev, const struct libusb_device_descriptor &desc );

	UVCDevice( void );
	UVCDevice( libusb_device *dev );
	UVCDevice( libusb_device *dev, const struct libusb_device_descriptor &desc );
	virtual ~UVCDevice( void );

	bool supportsROI( void ) { return mySupportsROI; }
	void setBinning( int b );
	void getROI( ROI &roi );
	void setROI( ROI &roi );

	size_t getCurrentFormat( void ) const { return myCurrentFrame; }
	const std::vector<FrameDefinition> &formats( void ) const { return myFormats; }

	void setImageCallback( const ImageReceivedCallback &cb = ImageReceivedCallback() );
	// if for some reason, video frame requested isn't possible, it
	// returns the resulting frame chosen
	void startVideo( size_t &frameIdx );
	void stopVideo( void );

	VideoStream &getVideoStream( void ) { return myVidStream; }

	size_t getNumControls( void ) const { return myControls.size(); };
	Control &control( size_t i ) { return (*myControls[i]); }
	Control &control( const std::string &name );

protected:
	virtual void closeHandle( void );
	virtual bool handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer );

	void handleVideoTransfer( libusb_transfer *xfer );
	void fillFrame( uint8_t *buf, int buflen );

	virtual bool wantInterface( const struct libusb_interface_descriptor &iface );

	virtual std::shared_ptr<AsyncTransfer> createTransfer( const struct libusb_endpoint_descriptor &epDesc );

	virtual void parseInterface( uint8_t inum, const struct libusb_interface_descriptor &ifacedesc );

	void addFormats( const uint8_t iface, const unsigned char *buffer, int buflen );
	bool addFormat( uint8_t &fmtidx, int &bpp, ImageBuffer::Format &fmt, const unsigned char *buffer, int buflen );
	void addFrameUncompressed( uint8_t iface, uint8_t fmtidx, int bpp, ImageBuffer::Format fmt, const unsigned char *buffer, int buflen );
	void addFrameFrameBased( uint8_t iface, uint8_t fmtidx, int bpp, ImageBuffer::Format fmt, const unsigned char *buffer, int buflen );

	void addControls( const uint8_t iface, const unsigned char *buffer, int buflen );
	void parseControls( const uint8_t iface, const uint16_t terminal,
						const uint8_t type,
						const uint8_t *bmControls, int bControlSize );

	virtual void dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iface );

	void dumpControls( std::ostream &os, const uint8_t type, const uint8_t *bmControls, int bControlSize );
	void dumpStandardControls( std::ostream &os, const unsigned char *buffer, int buflen, bool skipIfNoCS = false );
	void dumpFormats( std::ostream &os, const unsigned char *buffer, int buflen );
	void dumpInputFormats( std::ostream &os, const unsigned char *buffer, int buflen );
	void dumpOutputFormats( std::ostream &os, const unsigned char *buffer, int buflen );

	void dumpFormatUncompressed( std::ostream &os, const unsigned char *buffer, int buflen, int &bpp );
	void dumpFrameUncompressed( std::ostream &os, const unsigned char *buffer, int buflen, int bpp );

	void dumpFormatFrameBased( std::ostream &os, const unsigned char *buffer, int buflen, int &bpp );
	void dumpFrameFrameBased( std::ostream &os, const unsigned char *buffer, int buflen, int bpp );

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
		uint32_t dwClockFrequency; // UVC 1.1
		uint8_t bmFramingInfo;
		uint8_t bPreferredVersion;
		uint8_t bMinVersion;
		uint8_t bMaxVersion;
		uint8_t bUsage; // UVC 1.5
		uint8_t bBitDepthLuma;
		uint8_t bmSettings;
		uint8_t bMaxNumberOfRefFramesPlus1;
		uint16_t bmRateControlModes;
		uint16_t bmLayoutPerStream[4];
	};

	struct PayloadHeader
	{
		uint8_t bLength;
		uint8_t bPayloadFlags;
		uint32_t dwPresentationTime;
		uint16_t wSOFToken;
		uint32_t dwSourceTimeClock;
	};
#pragma pack(pop)
	inline size_t getProbeLen( void ) const
	{
		if ( myUVCVersion >= 0x0110 )
		{
			if ( myUVCVersion < 0x0150 )
				return 34;
			return sizeof(UVCProbe);
		}
		return 26;
	}
	void probeVideo( UVCProbe &p, size_t &len );
	size_t getCurrentSetup( const UVCProbe &devInfo );
	void dumpProbe( const char *tag, const UVCProbe &p, size_t len );

	int myControlInterface = 0;
	int myCameraInterface = 1;
	uint16_t myUVCVersion = 0;
	std::mutex myCBMutex;
	ImageReceivedCallback myImageCB;

	uint8_t myControlEndPoint = 0;
	uint8_t myVideoEndPoint = 0;
	int myVideoInterface = 0;
	uint8_t myVideoXferMode = 0;
	uint8_t myISOSyncType = 0;
	uint8_t myISOUsageType = 0;
	uint8_t myExtensionUnit[16] = { 0 };

	std::vector<FrameDefinition> myFormats;
	size_t myCurrentFrame = 0;

	std::vector<std::shared_ptr<AsyncTransfer>> myVideoTransfers;
	VideoStream myVidStream;

	static const int roiOFFSET_X = 0;
	static const int roiOFFSET_Y = 1;
	static const int roiWIDTH = 2;
	static const int roiHEIGHT = 3;
	static const int roiBINNING = 4;
	static const int roiNUM_ROI = 5;
	std::shared_ptr<Control> myROIControls[roiNUM_ROI];
	bool mySupportsROI = false;

	std::shared_ptr<ImageBuffer> myWorkImage;

	int myLastFID = -1;

	std::vector<std::shared_ptr<Control>> myControls;

private:
	UVCDevice( const UVCDevice & ) = delete;
	UVCDevice &operator=( const UVCDevice & ) = delete;
};

} // namespace usbpp

#endif // _usbpp_UVCDevice_h_


