// UVCDevice.cpp -*- C++ -*-

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

#include "UVCDevice.h"
#include "Util.h"
#include "Logger.h"

// cheating for now, need to have our own constants to work on the mac
//#include <linux/usb/video.h>
//#include <linux/v4l2-controls.h>
#include "uvc_constants.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Logger.h"


////////////////////////////////////////


#define USB_TYPE_MASK			(0x03 << 5)
#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE		0x04
#define USB_DT_ENDPOINT			0x05

#define USB_DT_CS_DEVICE		(USB_TYPE_CLASS | USB_DT_DEVICE)
#define USB_DT_CS_CONFIG		(USB_TYPE_CLASS | USB_DT_CONFIG)
#define USB_DT_CS_STRING		(USB_TYPE_CLASS | USB_DT_STRING)
#define USB_DT_CS_INTERFACE		(USB_TYPE_CLASS | USB_DT_INTERFACE)
#define USB_DT_CS_ENDPOINT		(USB_TYPE_CLASS | USB_DT_ENDPOINT)

static const uint8_t UVC_GUID_FORMAT_YUY2[16] = { 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_UYVY[16] = { 'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_Y800[16] = { 'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_Y8[16] = { 'Y',  '8',  ' ',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_Y16[16] = { 'Y',  '1',  '6',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_BA81[16] = { 'B',  'A',  '8',  '1', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };
static const uint8_t UVC_GUID_FORMAT_GRBG[16] = { 'G',  'R',  'B',  'G', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 };

static const uint8_t TIS_EXTENSION_BLOCK_SKYRIS_NEWER[16] = { 0x0a, 0xba, 0x49, 0xde, 0x5c, 0x0b, 0x49, 0xd5, 0x8f, 0x71, 0x0b, 0xe4, 0x0f, 0x94, 0xa6, 0x7a };
static const uint8_t TIS_EXTENSION_BLOCK_NEXIMAGE[16] = { 0x26, 0x52, 0x21, 0x5a, 0x89, 0x32, 0x56, 0x41, 0x89, 0x4a, 0x5c, 0x55, 0x7c, 0xdf, 0x96, 0x64 };

// Get queries for the NexImage5:
// 'Known Input Terminal' iface 0 terminal 1 unit 4 (0x04) length 4 value : 0x0000007f (127) [range: 1 - 2500]
// 'Known Input Terminal' iface 0 terminal 1 unit 17 (0x11) length 1 value : 0x00 (0)
// 'Hidden Input Terminal' iface 0 terminal 1 unit 32 (0x20) length 4 value : 0x000159f6 (88566) [range: 0 - 255]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 33 (0x21) length 1 value : 0x00 (0) [range: 0 - 255]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 34 (0x22) length 2 value : 0x0000 (0) [range: 0 - 65535]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 36 (0x24) length 4 value : 0x01312d00 (20000000) [range: 5000000 - 42000000]
//      -> Pixel Clock
// 'Hidden Input Terminal' iface 0 terminal 1 unit 37 (0x25) length 2 value : 0x0a20 (2592) [range: 4 - 2592]
//      -> Partial Scan Width
// 'Hidden Input Terminal' iface 0 terminal 1 unit 38 (0x26) length 2 value : 0x0798 (1944) [range: 4 - 1944]
//      -> Partial Scan Height
// 'Hidden Input Terminal' iface 0 terminal 1 unit 39 (0x27) length 4 value : 0x000903e8 (590824) [range: 0 - 4294967295]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 40 (0x28) length 2 value : 0x0000 (0)
// 'Hidden Input Terminal' iface 0 terminal 1 unit 41 (0x29) length 2 value : 0x0000 (0)
// 'Hidden Input Terminal' iface 0 terminal 1 unit 42 (0x2a) length 1 value : 0x01 (1) [range: 1 - 4]
//      -> Binning
// 'Hidden Input Terminal' iface 0 terminal 1 unit 43 (0x2b) length 1 value : 0x00 (0) [range: 1 - 4]
//      -> Software Trigger
// 'Hidden Input Terminal' iface 0 terminal 1 unit 45 (0x2d) length 2 value : 0x007c (124) [range: 0 - 65535]
//      -> Firmware Revision
// 'Hidden Input Terminal' iface 0 terminal 1 unit 46 (0x2e) length 1 value : 0x00 (0) [range: 0 - 1]
//      -> GP out
// 'Hidden Input Terminal' iface 0 terminal 1 unit 54 (0x36) length 1 value : 0x01 (1) [range: 1 - 4]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 55 (0x37) length 1 value : 0x01 (1) [range: 1 - 4]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 64 (0x40) length 1 value : 0x48 (72) [range: 0 - 255]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 66 (0x42) length 1 value : 0x00 (0) [range: 0 - 255]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 67 (0x43) length 2 value : 0x0000 (0) [range: 0 - 65535]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 68 (0x44) length 2 value : 0x0026 (38) [range: 0 - 65535]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 69 (0x45) length 2 value : 0x0000 (0) [range: 0 - 65535]
// 'Hidden Input Terminal' iface 0 terminal 1 unit 80 (0x50) length 4 value : 0x000000f3 (243) [range: 0 - 4294967295]

// 'Known Processing Unit' iface 0 terminal 3 unit 2 (0x02) length 2 value : 0x0080 (128) [range: 0 - 255]
// 'Known Processing Unit' iface 0 terminal 3 unit 3 (0x03) length 2 value : 0x0010 (16) [range: 0 - 31]
// 'Known Processing Unit' iface 0 terminal 3 unit 4 (0x04) length 2 value : 0x0010 (16) [range: 4 - 63]
// 'Known Processing Unit' iface 0 terminal 3 unit 5 (0x05) length 2 value : 0x0001 (1) [range: 0 - 2]
// 'Known Processing Unit' iface 0 terminal 3 unit 7 (0x07) length 2 value : 0x0010 (16) [range: 0 - 31]
// 'Known Processing Unit' iface 0 terminal 3 unit 8 (0x08) length 2 value : 0x0000 (0) [range: 0 - 15]
// 'Known Processing Unit' iface 0 terminal 3 unit 9 (0x09) length 2 value : 0x0020 (32) [range: 0 - 63]
// 'Known Processing Unit' iface 0 terminal 3 unit 12 (0x0c) length 2 value : 0x0000 (0) [range: 0 - 120]
// 'Hidden Processing Unit' iface 0 terminal 3 unit 32 (0x20) length 2 value : 0x0002 (2) [range: 0 - 2]
// 'Hidden Processing Unit' iface 0 terminal 3 unit 33 (0x21) length 2 value : 0x0003 (3) [range: 0 - 16]
// 'Hidden Processing Unit' iface 0 terminal 3 unit 34 (0x22) length 2 value : 0x0010 (16) [range: 0 - 65535]

// 'Vendor Extension Unit' iface 0 terminal 4 unit 1 (0x01) length 1 value : 0x01 (1)
// 'Vendor Extension Unit' iface 0 terminal 4 unit 2 (0x02) length 1 value : 0x01 (1)
// 'Vendor Extension Unit' iface 0 terminal 4 unit 3 (0x03) length 1 value : 0x01 (1)
// 'Vendor Extension Unit' iface 0 terminal 4 unit 4 (0x04) length 1 value : 0x01 (1)
// 'Vendor Extension Unit' iface 0 terminal 4 unit 5 (0x05) length 1 value : 0x01 (1)

// Get Queries for the Skyris 445 b

// 'Known Input Terminal' iface 0 terminal 1 unit 4 (0x04) length 4 value : 0x00000032 (50) [range: 1 - 300000]
// 'Known Processing Unit' iface 0 terminal 2 unit 4 (0x04) length 2 value : 0x00ae (174) [range: 174 - 1023]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 24 (0x18) length 4 value : 0x00000000 (0) [range: 0 - 3039]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 25 (0x19) length 9 value : { 44 fd ff fd 48 fb 00 fa 46 }
// 'Vendor Extension Unit' iface 0 terminal 3 unit 27 (0x1b) length 4 value : 0x00001388 (5000) [range: 100 - 30000000]

// experimental new camera

// 'Known Input Terminal' iface 0 terminal 1 unit 4 (0x04) length 4 value : 0x0000000a (10) [range: 1 - 10000]
// 'Known Processing Unit' iface 0 terminal 2 unit 4 (0x04) length 2 value : 0x0020 (32) [range: 32 - 255]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 16 (0x10) length 4 value : 0x00000001 (1) [range: 1 - 2]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 17 (0x11) length 4 value : 0x00000500 (1280) [range: 96 - 1280]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 18 (0x12) length 4 value : 0x000003c0 (960) [range: 96 - 960]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 19 (0x13) length 4 value : 0x00000001 (1)
// ERROR: Seems to just error? (unit 20): Unable to query value
// 'Vendor Extension Unit' iface 0 terminal 3 unit 21 (0x15) length 4 value : 0x00000000 (0) [range: 0 - 1184]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 22 (0x16) length 4 value : 0x00000000 (0) [range: 0 - 864]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 23 (0x17) length 1 value : 0x01 (1)
// 'Vendor Extension Unit' iface 0 terminal 3 unit 24 (0x18) length 4 value : 0x00000000 (0) [range: 0 - 901]
// 'Vendor Extension Unit' iface 0 terminal 3 unit 25 (0x19) length 9 value : { 4f f7 fa f7 54 f5 fb f4 51 }
// 'Vendor Extension Unit' iface 0 terminal 3 unit 27 (0x1b) length 4 value : 0x000003e8 (1000) [range: 100 - 1000000]



////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


std::shared_ptr<Device>
UVCDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::make_shared<UVCDevice>( dev, desc );
}


////////////////////////////////////////


UVCDevice::UVCDevice( void )
{
}


////////////////////////////////////////


UVCDevice::UVCDevice( libusb_device *dev )
		: Device( dev )
{
}


////////////////////////////////////////


UVCDevice::UVCDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: Device( dev, desc )
{
}


////////////////////////////////////////


UVCDevice::~UVCDevice( void )
{
}


////////////////////////////////////////


void
UVCDevice::closeHandle( void )
{
	stopVideo();

	for ( size_t i = 0; i < roiNUM_ROI; ++i )
		myROIControls[i].reset();

	myControls.clear();
	myFormats.clear();
	Device::closeHandle();
}


////////////////////////////////////////


void
UVCDevice::setBinning( int b )
{
	if ( myROIControls[roiBINNING] )
	{
		myVidStream.clear();

		std::unique_lock<std::mutex> myCBMutex;
		b = myROIControls[roiBINNING]->set( b );
		myROIControls[roiBINNING]->coalesce();

		const FrameDefinition &curFrame = myFormats[myCurrentFrame];

		ROI roi;
		roi.x = 0;
		if ( myROIControls[roiOFFSET_X] )
			roi.x = myROIControls[roiOFFSET_X]->get();
		roi.y = 0;
		if ( myROIControls[roiOFFSET_Y] )
			roi.y = myROIControls[roiOFFSET_Y]->get();
		roi.w = curFrame.width;
		if ( myROIControls[roiWIDTH] )
			roi.w = myROIControls[roiWIDTH]->get();
		roi.h = curFrame.height;
		if ( myROIControls[roiHEIGHT] )
			roi.h = myROIControls[roiHEIGHT]->get();

		roi.x /= b;
		roi.y /= b;
		roi.w /= b;
		roi.h /= b;

		myVidStream.reset( roi.w, roi.h, curFrame.bytesPerLine, curFrame.bytesPerPixel, curFrame.format, 3 );
		myVidStream.setROI( roi );
	}
}


////////////////////////////////////////


void
UVCDevice::getROI( ROI &roi )
{
	const FrameDefinition &curFrame = myFormats[myCurrentFrame];
	roi.x = 0;
	roi.y = 0;
	roi.w = curFrame.width;
	roi.h = curFrame.height;

	if ( myROIControls[roiOFFSET_X] )
		roi.x = myROIControls[roiOFFSET_X]->get();
	if ( myROIControls[roiOFFSET_Y] )
		roi.y = myROIControls[roiOFFSET_Y]->get();
	if ( myROIControls[roiWIDTH] )
		roi.w = myROIControls[roiWIDTH]->get();
	if ( myROIControls[roiHEIGHT] )
		roi.h = myROIControls[roiHEIGHT]->get();
}


////////////////////////////////////////


void
UVCDevice::setROI( ROI &roi )
{
	myVidStream.clear();
	std::unique_lock<std::mutex> myCBMutex;

	if ( mySupportsROI )
	{
		if ( myROIControls[roiOFFSET_X] )
			roi.x = myROIControls[roiOFFSET_X]->set( roi.x );
		if ( myROIControls[roiOFFSET_Y] )
			roi.y = myROIControls[roiOFFSET_Y]->set( roi.y );
		if ( myROIControls[roiWIDTH] )
			roi.w = myROIControls[roiWIDTH]->set( roi.w );
		if ( myROIControls[roiHEIGHT] )
			roi.h = myROIControls[roiHEIGHT]->set( roi.h );

		for ( int i = 0; i < roiNUM_ROI; ++i )
			myROIControls[i]->coalesce();

		if ( myROIControls[roiOFFSET_X] )
			roi.x = myROIControls[roiOFFSET_X]->update();
		if ( myROIControls[roiOFFSET_Y] )
			roi.y = myROIControls[roiOFFSET_Y]->update();
		if ( myROIControls[roiWIDTH] )
			roi.w = myROIControls[roiWIDTH]->update();
		if ( myROIControls[roiHEIGHT] )
			roi.h = myROIControls[roiHEIGHT]->update();
	}
	
	const FrameDefinition &curFrame = myFormats[myCurrentFrame];
//	myVidStream.reset( curFrame.width, curFrame.height, curFrame.bytesPerLine, curFrame.bytesPerPixel, curFrame.format, 3 );
	myVidStream.reset( curFrame.width, curFrame.height, curFrame.bytesPerLine, curFrame.bytesPerPixel, curFrame.format, 3 );
	myVidStream.setROI( roi );
	myLastFID = -1;

	std::cout << "roi set to " << roi.x << ", " << roi.y << " " << roi.w << "x" << roi.h << std::endl;
}


////////////////////////////////////////


void
UVCDevice::setImageCallback( const ImageReceivedCallback &cb )
{
	std::unique_lock<std::mutex> myCBMutex;
	myImageCB = cb;
}


////////////////////////////////////////


void
UVCDevice::startVideo( size_t &frameIndex )
{
	info() << "startVideo( " << frameIndex << " )" << send;
	stopVideo();

	if ( frameIndex >= myFormats.size() )
	{
		warning() << "Invalid frame index " << frameIndex << send;
		if ( myFormats.empty() )
			throw std::runtime_error( "No formats to chose from" );
		frameIndex = 0;
	}

	// NB: every time this is called it toggles streaming, so only
	// call as appropriate
	// Start streaming interface...
	check_error( libusb_set_interface_alt_setting( myHandle, myVideoInterface, 0 ) );
//	check_error( libusb_clear_halt( myHandle, (myVideoEndPoint & 0xF) ) );

	const FrameDefinition &chosenFrame = myFormats[frameIndex];

	UVCProbe setInfo;
	memset( &setInfo, 0, sizeof(UVCProbe) );
	size_t len = getProbeLen();

	setInfo.bmHint |= (1 << 0); // frame interval at D0
	setInfo.bFormatIndex = chosenFrame.format_index;
	setInfo.bFrameIndex = chosenFrame.frame_index;
	setInfo.dwFrameInterval = chosenFrame.defaultFrameInterval;
	setInfo.dwMaxVideoFrameSize = chosenFrame.bytesPerLine * chosenFrame.height;

	ControlTransfer probe( myContext );
	uint8_t reqType = LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;

	probe.fill( myHandle,
				endpoint_out(0) | reqType, UVC_SET_CUR,
				( UVC_VS_PROBE_CONTROL << 8 ), 1,
				len, &setInfo );
	int err = probe.submitAndWait();
	if ( err != len )
	{
		error() << "Negotiating stream rate: " << err << " " << libusb_strerror( libusb_error(err) ) << send;
		
	}

	UVCProbe getInfo;
	probe.fill( myHandle,
				endpoint_in(0) | reqType, UVC_GET_CUR,
				( UVC_VS_PROBE_CONTROL << 8 ), 1,
				sizeof(UVCProbe), &getInfo );
	int getLen = probe.submitAndWait();
	dumpProbe( "Response from set", getInfo, getLen );

	size_t curfrm = getCurrentSetup( getInfo );
	if ( curfrm != frameIndex )
	{
		warning() << "Unable to change frame format" << send;
		frameIndex = curfrm;
	}

	const FrameDefinition &curFrame = myFormats[curfrm];

	ROI roi;
	roi.x = 0;
	roi.y = 0;
	roi.w = curFrame.width;
	roi.h = curFrame.height;
	setROI( roi );
	myLastFID = -1;

	probe.fill( myHandle,
				endpoint_out(0) | reqType, UVC_SET_CUR,
				( UVC_VS_COMMIT_CONTROL << 8 ), 1,
				getLen, &getInfo );
	err = probe.submitAndWait();

	if ( err != getLen )
	{
		error() << "Committing stream rate: " << err << " " << libusb_strerror( libusb_error(err) ) << send;
		throw std::runtime_error( "Error starting video" );
	}

	// Give things time to coalesce
	usleep( 100000 );

	info() << "Creating video stream..." << send;

	size_t vidFrameSize = getInfo.dwMaxVideoFrameSize;
	size_t bulkSize = getInfo.dwMaxPayloadTransferSize;
	if ( bulkSize == 0 )
		bulkSize = vidFrameSize;

	if ( myVideoXferMode == LIBUSB_TRANSFER_TYPE_BULK )
	{
		size_t nXfersNeeded = ( vidFrameSize + bulkSize - 1 ) / bulkSize;

		// make sure we have a few frames ready to go...
		if ( nXfersNeeded < 2 )// && myUVCVersion > 0x0100 )
			nXfersNeeded = 2;

		for ( size_t i = 0; i < nXfersNeeded; ++i )
		{
			myVideoTransfers.push_back( std::make_shared<BulkTransfer>( myContext, bulkSize ) );
		}
	}
	else if ( myVideoXferMode == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS )
	{
		int maxPacketSize = libusb_get_max_iso_packet_size( myDevice, myVideoEndPoint );
		int numPackets = ( vidFrameSize + maxPacketSize - 1 ) / maxPacketSize;

		myVideoTransfers.push_back( std::make_shared<ISOTransfer>( myContext, numPackets, vidFrameSize, maxPacketSize ) );
	}
	else
	{
		error() << "Unknown video xfer mode" << send;
	}

	if ( ! myVideoTransfers.empty() )
	{
		// init everything prior to submitting all at once
		for ( auto &s: myVideoTransfers )
		{
			s->init( myHandle, myVideoEndPoint );
			s->setCallback( std::bind( &UVCDevice::handleVideoTransfer, this, std::placeholders::_1 ) );
		}

		// now start them all
		for ( auto &s: myVideoTransfers )
			s->submit();
	}
}


////////////////////////////////////////


void
UVCDevice::stopVideo( void )
{
	std::unique_lock<std::mutex> myCBMutex;

	myLastFID = -1;
	myVidStream.clear();
	myWorkImage.reset();

	if ( myVideoTransfers.empty() )
		return;

	for ( auto &s: myVideoTransfers )
	{
		s->cancel();
		s->wait();
	}
	myVideoTransfers.clear();

	// NB: every time this is called it toggles streaming, so only
	// call as appropriate
	libusb_set_interface_alt_setting( myHandle, myVideoInterface, 0 );
}


////////////////////////////////////////


Control &
UVCDevice::control( const std::string &name )
{
	for ( auto &x: myControls )
	{
		if ( x->name() == name )
			return (*x);
	}

	throw std::runtime_error( "Unable to find control '" + name + "'" );
}


////////////////////////////////////////


bool
UVCDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
	error() << "Why are we in Device::handleEvent" << send;
	return true;
}


////////////////////////////////////////


void
UVCDevice::handleVideoTransfer( libusb_transfer *xfer )
{
	uint8_t *buf = xfer->buffer;
	int buflen = xfer->actual_length;
	AsyncTransfer *transfer = reinterpret_cast<AsyncTransfer *>( xfer->user_data );

	if ( xfer->status == LIBUSB_TRANSFER_COMPLETED )
		fillFrame( buf, buflen );

	if ( transfer )
		transfer->submit();
}


////////////////////////////////////////


void
UVCDevice::fillFrame( uint8_t *buf, int buflen )
{
	if ( buflen <= 0 )
		return;

	if ( myVidStream.off() )
		return;

	bool newFrame = false;
	bool isEOF = false;

	const PayloadHeader *hdr = reinterpret_cast<const PayloadHeader *>( buf );
	if ( hdr->bLength == 2 || hdr->bLength == 12 )
	{
		uint8_t status = hdr->bPayloadFlags;

		int fid = ( status & UVC_STREAM_FID );
		if ( fid != myLastFID )
		{
			newFrame = true;
			myLastFID = fid;
		}

		if ( ( status & UVC_STREAM_EOF ) != 0 )
			isEOF = true;

		buf += hdr->bLength;
		buflen -= hdr->bLength;
	}
	else
	{
		error() << "Unknown image data header size: " << int(hdr->bLength) << send;
		return;
	}

	std::unique_lock<std::mutex> myCBMutex;

	if ( newFrame )
	{
		if ( myWorkImage )
		{
			if ( ! myWorkImage->empty() )
			{
				if ( myImageCB )
					myImageCB( myWorkImage );
				else
					myVidStream.put( myWorkImage );

				myWorkImage = myVidStream.get();
			}
		}
		else
		{
			myWorkImage = myVidStream.get();
		}
	}

	while ( myWorkImage && buflen > 0 )
	{
		int curLeft = buflen;
		if ( myWorkImage->addData( buf, curLeft ) || isEOF )
		{
			if ( myImageCB )
				myImageCB( myWorkImage );
			else
				myVidStream.put( myWorkImage );

			myWorkImage = myVidStream.get();
			// stop processing buffer at this point...
			if ( curLeft > 0 )
			{
//				warning() << "Skipping rest of buffer -- at eof (" << curLeft << " bytes left)" << send;
			}
			break;
		}
		buf += buflen - curLeft;
		buflen = curLeft;
	}
}


////////////////////////////////////////


bool
UVCDevice::wantInterface( const struct libusb_interface_descriptor &iface )
{
	return true;
}


////////////////////////////////////////


std::shared_ptr<AsyncTransfer>
UVCDevice::createTransfer( const struct libusb_endpoint_descriptor &epDesc )
{
	if ( epDesc.bEndpointAddress == myVideoEndPoint )
	{
		myVideoXferMode = ( epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK );
		myISOSyncType = ( epDesc.bmAttributes & LIBUSB_ISO_SYNC_TYPE_MASK );
		myISOUsageType = ( epDesc.bmAttributes & LIBUSB_ISO_USAGE_TYPE_MASK );
	}

	return std::shared_ptr<AsyncTransfer>();
}


////////////////////////////////////////


void
UVCDevice::parseInterface( uint8_t inum, const struct libusb_interface_descriptor &iFaceDesc )
{
	if ( iFaceDesc.bInterfaceClass == 0x0e && iFaceDesc.bNumEndpoints > 0 )
		addFormats( inum, iFaceDesc.extra, iFaceDesc.extra_length );
	else
	{
		addControls( inum, iFaceDesc.extra, iFaceDesc.extra_length );
		if ( iFaceDesc.bNumEndpoints == 0 )
			myControlEndPoint = 0;
		else
		{
			warning() << "Need to find the control endpoint" << send;
		}
	}
}


////////////////////////////////////////


void
UVCDevice::addFormats( const uint8_t iface, const unsigned char *buffer, int buflen )
{
	if ( buflen <= 2 )
		return;

	if ( buffer[2] != UVC_VS_INPUT_HEADER )
		return;

	if ( buflen < UVC_DT_INPUT_HEADER_SIZE(0, 0) )
		return;

	const uvc_input_header_descriptor *inpDesc = reinterpret_cast<const uvc_input_header_descriptor *>( buffer );

	myVideoInterface = iface;
	myVideoEndPoint = inpDesc->bEndpointAddress;

	buflen -= buffer[0];
	buffer += buffer[0];

	uint8_t fmtIdx = 0;
	int bpp = 0;
	ImageBuffer::Format fmt = ImageBuffer::Format::UNKNOWN;
	while ( buflen > 2 && buffer[1] == USB_DT_CS_INTERFACE )
	{
		const uvc_descriptor_header *cur = reinterpret_cast<const uvc_descriptor_header *>( buffer );
		int nFmtInfo = 0;
		bool skipToNextFmt = false;
		switch ( cur->bDescriptorSubType )
		{
			case UVC_VS_FORMAT_UNCOMPRESSED:
			case UVC_VS_FORMAT_FRAME_BASED:
				skipToNextFmt = false;
				if ( ! addFormat( fmtIdx, bpp, fmt, buffer, buflen ) )
					skipToNextFmt = true;
				break;
			case UVC_VS_FRAME_UNCOMPRESSED:
				if ( ! skipToNextFmt )
					addFrameUncompressed( iface, fmtIdx, bpp, fmt, buffer, buflen );
				break;
			case UVC_VS_FRAME_FRAME_BASED:
				if ( ! skipToNextFmt )
					addFrameFrameBased( iface, fmtIdx, bpp, fmt, buffer, buflen );
				break;

			case UVC_VS_STILL_IMAGE_FRAME:
				// skip these for now...
			case UVC_VS_COLORFORMAT:
				// color format seems to not be specified, ignore for now
			default:
				break;
		}
		buflen -= buffer[0];
		buffer += buffer[0];
	}
}


////////////////////////////////////////


bool
UVCDevice::addFormat( uint8_t &fmtidx, int &bpp, ImageBuffer::Format &fmtType, const unsigned char *buffer, int buflen )
{
	const uvc_format_uncompressed *fmt = reinterpret_cast<const uvc_format_uncompressed *>( buffer );

	fmtidx = fmt->bFormatIndex;
	bpp = fmt->bBitsPerPixel / 8;

	bool ok = true;
	if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_YUY2 ) )
	{
		info() << "Format claims to be YUY2, but is it?" << send;
		//nfmt.frameFmt = FrameFormat::YUY2;
		fmtType = ImageBuffer::Format::BAYER_GRBG;
	}
	else if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_UYVY ) )
	{
		info() << "Format claims to be UYVY, but is it?" << send;
		//nfmt.frameFmt = FrameFormat::UYVY;
		fmtType = ImageBuffer::Format::BAYER_GRBG;
	}
	else if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_BA81 ) )
	{
		fmtType = ImageBuffer::Format::BAYER_BGGR;
	}
	else if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_GRBG ) )
	{
		fmtType = ImageBuffer::Format::BAYER_GRBG;
	}
	else if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_Y800 ) ||
			  matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_Y8 ) )
	{
		fmtType = ImageBuffer::Format::MONO_8;
		// hrm, the skyris seems to lie about it being monochrome
		// let's skip this 8 bit one since it's the same as the normal
		// bayer
		if ( getProductName().find( "SKYRIS" ) != std::string::npos )
			return false;
	}
	else if ( matchGUID( fmt->guidFormat, UVC_GUID_FORMAT_Y16 ) )
	{
		fmtType = ImageBuffer::Format::MONO_16;
		// hrm, the skyris seems to lie about it being monochrome
		// leave the 16-bit
		if ( getProductName().find( "SKYRIS" ) != std::string::npos )
			fmtType = ImageBuffer::Format::BAYER_BGGR;
	}
	else
	{
		dumpGUID( std::cout, "UNKNOWN GUID", fmt->guidFormat, true );
		ok = false;
	}

	return ok;
}


////////////////////////////////////////


void
UVCDevice::addFrameUncompressed( uint8_t iface, uint8_t fmtidx, int bpp, ImageBuffer::Format frmfmt, const unsigned char *buffer, int buflen )
{
	const uvc_frame_uncompressed *fmt = reinterpret_cast<const uvc_frame_uncompressed *>( buffer );


	FrameDefinition f;
	f.format_index = fmtidx;
	f.frame_index = fmt->bFrameIndex;
	f.interface = iface;
	f.format = frmfmt;
	f.width = int(fmt->wWidth);
	int bpl = bpp * f.width;
	if ( myUVCVersion <= 0x0100 )
	{
		f.width *= bpp;
		bpp = 1;
	}
	else
	{
		bpl = fmt->dwFrameInterval[fmt->bFrameIntervalType];
	}
	f.height = int(fmt->wHeight);
	f.bytesPerLine = bpl;
	f.bytesPerPixel = bpp;
	f.defaultFrameInterval = fmt->dwDefaultFrameInterval;

	if ( fmt->bFrameIntervalType == 0 )
	{
		if ( fmt->bLength < 34 )
		{
			error() << "Invalid uncompressed frame specification: interval type 0, expecting frame length at least 34, got " << int(fmt->bLength) << send;
		}

		f.variableFrameInterval = true;
		f.availableIntervals.resize( 3 );
		f.availableIntervals[0] = fmt->dwFrameInterval[0];
		f.availableIntervals[1] = fmt->dwFrameInterval[1];
		f.availableIntervals[2] = fmt->dwFrameInterval[2];
	}
	else
	{
		f.variableFrameInterval = false;
		f.availableIntervals.resize( size_t(fmt->bFrameIntervalType) );
		for ( size_t i = 0; i < f.availableIntervals.size(); ++i )
			f.availableIntervals[i] = fmt->dwFrameInterval[i];
	}

	myFormats.push_back( f );
}


////////////////////////////////////////


void
UVCDevice::addFrameFrameBased( uint8_t iface, uint8_t fmtidx, int bpp, ImageBuffer::Format frmfmt, const unsigned char *buffer, int buflen )
{
	const uvc_frame_frame_based *fmt = reinterpret_cast<const uvc_frame_frame_based *>( buffer );

	FrameDefinition f;
	f.format_index = fmtidx;
	f.frame_index = fmt->bFrameIndex;
	f.interface = iface;
	f.format = frmfmt;
	f.width = int(fmt->wWidth);
	f.height = int(fmt->wHeight);
	f.bytesPerLine = fmt->dwBytesPerLine;
	f.bytesPerPixel = bpp;
	f.defaultFrameInterval = fmt->dwDefaultFrameInterval;
	if ( fmt->bFrameIntervalType == 0 )
	{
		if ( fmt->bLength < 34 )
		{
			error() << "Invalid uncompressed frame specification: interval type 0, expecting frame length at least 34, got " << int(fmt->bLength) << send;
		}

		f.variableFrameInterval = true;
		f.availableIntervals.resize( 3 );
		f.availableIntervals[0] = fmt->dwFrameInterval[0];
		f.availableIntervals[1] = fmt->dwFrameInterval[1];
		f.availableIntervals[2] = fmt->dwFrameInterval[2];
	}
	else
	{
		f.variableFrameInterval = false;
		f.availableIntervals.resize( size_t(fmt->bFrameIntervalType) );
		for ( size_t i = 0; i < f.availableIntervals.size(); ++i )
			f.availableIntervals[i] = fmt->dwFrameInterval[i];
	}

	myFormats.push_back( f );
}


////////////////////////////////////////


void
UVCDevice::addControls( const uint8_t iface, const unsigned char *buffer, int buflen )
{
	while ( buflen > 2 )
	{
		const uvc_descriptor_header *desc = reinterpret_cast<const uvc_descriptor_header *>( buffer );

		if ( desc->bDescriptorType == USB_DT_CS_INTERFACE )
		{
			switch ( desc->bDescriptorSubType )
			{
				case UVC_VC_HEADER:
					if ( buflen >= UVC_DT_HEADER_SIZE(0) )
					{
						const uvc_header_descriptor *header = reinterpret_cast<const uvc_header_descriptor *>( desc );
						myUVCVersion = header->bcdUVC;
					}
					break;
				case UVC_VC_INPUT_TERMINAL:
					if ( buflen >= UVC_DT_INPUT_TERMINAL_SIZE )
					{
						const uvc_input_terminal_descriptor *itd = reinterpret_cast<const uvc_input_terminal_descriptor *>( desc );
						switch ( itd->wTerminalType )
						{
							case UVC_ITT_CAMERA:
							{
								const uvc_camera_terminal_descriptor *ctd = reinterpret_cast<const uvc_camera_terminal_descriptor *>( itd );
								if ( ctd->bControlSize )
									parseControls( iface, itd->bTerminalID,
												   ctd->bDescriptorSubType,
												   ctd->bmControls,
												   ctd->bControlSize );
								break;
							}
							default:
								warning() << "Unhandled terminal type parsing input terminal controls" << send;
								break;
						}
					}
					break;
				case UVC_VC_PROCESSING_UNIT:
					if ( buflen >= UVC_DT_PROCESSING_UNIT_SIZE(0) )
					{
						const uvc_processing_unit_descriptor *pud = reinterpret_cast<const uvc_processing_unit_descriptor *>( desc );
						if ( pud->bControlSize )
							parseControls( iface, pud->bUnitID,
										   pud->bDescriptorSubType,
										   pud->bmControls,
										   pud->bControlSize );
					}
					break;
				case UVC_VC_EXTENSION_UNIT:
					if ( buflen >= UVC_DT_EXTENSION_UNIT_SIZE(0, 0) )
					{
						const uvc_extension_unit_descriptor *eud = reinterpret_cast<const uvc_extension_unit_descriptor *>( desc );
						int nControls = eud->bNumControls;
						int nPins = eud->bNrInPins;
						int ctrlSize = eud->baSourceID[nPins];
						memcpy( myExtensionUnit, eud->guidExtensionCode, sizeof(myExtensionUnit) );
						if ( ctrlSize )
							parseControls( iface,
										   eud->bUnitID,
										   eud->bDescriptorSubType,
										   eud->baSourceID + nPins + 1,
										   ctrlSize );
					}
					break;
				default:
					break;
			}
		}

		buflen -= desc->bLength;
		buffer += desc->bLength;
	}
}


////////////////////////////////////////


void
UVCDevice::parseControls( const uint8_t iface, const uint16_t terminal,
						  const uint8_t type,
						  const uint8_t *bmControls, int bControlSize )
{
#if 1
	Control fkCtrl;
	uint8_t ctrlLen;
	for ( int i = 1; i < 255; ++i )
	{
		uint16_t unit = i;

		const char *tag = nullptr;
		switch ( type )
		{
			case UVC_VC_INPUT_TERMINAL:
				if ( unit <= UVC_CT_REGION_OF_INTEREST_CONTROL )
					tag = "Known Input Terminal";
				else
					tag = "Hidden Input Terminal";
				break;
			case UVC_VC_PROCESSING_UNIT:
				if ( unit <= UVC_PU_CONTRAST_AUTO_CONTROL )
					tag = "Known Processing Unit";
				else
					tag = "Hidden Processing Unit";
				break;
			case UVC_VC_EXTENSION_UNIT:
				tag = "Vendor Extension Unit";
				break;
			default:
				tag = "Unknown Unit Test";
				break;
		}
		fkCtrl.init( tag, myHandle, myControlEndPoint, unit, iface, terminal, myContext );
		if ( fkCtrl.valid() )
			info() << fkCtrl << send;
	}
#endif
	for ( int i = 0; i < bControlSize * 8; ++i )
	{
		if ( test_bit( bmControls, i ) == 0 )
			continue;

		int roiControlIdx = -1;
		uint8_t unit = 0;
		const char *name = NULL;
		switch ( type )
		{
			case UVC_VC_INPUT_TERMINAL:
				switch ( i )
				{
					case 0: name = "Scanning Mode"; unit = UVC_CT_SCANNING_MODE_CONTROL; break;
					case 1: name = "AE Mode"; unit = UVC_CT_AE_MODE_CONTROL; break;
					case 2: name = "AE Priority"; unit = UVC_CT_AE_PRIORITY_CONTROL; break;
					case 3: name = "Exposure"; unit = UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL; break;
					case 4: name = "Exposure Time Relative"; unit = UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL; break;
					case 5: name = "Focus"; unit = UVC_CT_FOCUS_ABSOLUTE_CONTROL; break;
					case 6: name = "Focus Relative"; unit = UVC_CT_FOCUS_RELATIVE_CONTROL; break;
					case 7: name = "Iris Absolute"; unit = UVC_CT_IRIS_ABSOLUTE_CONTROL; break;
					case 8: name = "Iris Relative"; unit = UVC_CT_IRIS_RELATIVE_CONTROL; break;
					case 9: name = "Zoom Absolute"; unit = UVC_CT_ZOOM_ABSOLUTE_CONTROL; break;
					case 10: name = "Zoom Relative"; unit = UVC_CT_ZOOM_RELATIVE_CONTROL; break;
					case 11: name = "Pan / Tilt"; unit = UVC_CT_PANTILT_ABSOLUTE_CONTROL; break;
					case 12: name = "Pan / Tilt Relative"; unit = UVC_CT_PANTILT_RELATIVE_CONTROL; break;
					case 13: name = "Roll"; unit = UVC_CT_ROLL_ABSOLUTE_CONTROL; break;
					case 14: name = "Roll Relative"; unit = UVC_CT_ROLL_RELATIVE_CONTROL; break;
					case 15:
					case 16:
						// reserved
						break;
					case 17: name = "Focus, Auto"; unit = UVC_CT_FOCUS_AUTO_CONTROL; break;
					case 18: name = "Privacy Control"; unit = UVC_CT_PRIVACY_CONTROL; break;
					case 19: name = "Focus, Simple"; unit = UVC_CT_FOCUS_SIMPLE_CONTROL; break;
					case 20: name = "Window"; unit = UVC_CT_WINDOW_CONTROL; break;
					case 21: name = "Region of Interest"; unit = UVC_CT_REGION_OF_INTEREST_CONTROL; break;
					default:
						name = "Unknown Camera Control";
						unit = i - 1;
						break;
				}
				break;
			case UVC_VC_PROCESSING_UNIT:
				switch ( i )
				{
					case 0: name = "Brightness"; unit = UVC_PU_BRIGHTNESS_CONTROL; break;
					case 1: name = "Contrast"; unit = UVC_PU_CONTRAST_CONTROL; break;
					case 2: name = "Hue"; unit = UVC_PU_HUE_CONTROL; break;
					case 3: name = "Saturation"; unit = UVC_PU_SATURATION_CONTROL; break;
					case 4: name = "Sharpness"; unit = UVC_PU_SHARPNESS_CONTROL; break;
					case 5: name = "Gamma"; unit = UVC_PU_GAMMA_CONTROL; break;
					case 6: name = "White Balance Temperature"; unit = UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL; break;
					case 7: name = "White Balance Component"; unit = UVC_PU_WHITE_BALANCE_COMPONENT_CONTROL; break;
					case 8: name = "Backlight Compensation"; unit = UVC_PU_BACKLIGHT_COMPENSATION_CONTROL; break;
					case 9: name = "Gain"; unit = UVC_PU_GAIN_CONTROL; break;
					case 10: name = "Power Line Frequency"; unit = UVC_PU_POWER_LINE_FREQUENCY_CONTROL; break;
					case 11: name = "Hue, Auto"; unit = UVC_PU_HUE_AUTO_CONTROL; break;
					case 12: name = "Auto White Balance Temperature"; unit = UVC_PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL; break;
					case 13: name = "Auto White Balance Component"; unit = UVC_PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL; break;
					case 14: name = "Digital Multiplier"; unit = UVC_PU_DIGITAL_MULTIPLIER_CONTROL; break;
					case 15: name = "Digital Multiplier Limit"; unit = UVC_PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL; break;
					case 16: name = "Analog Video Standard"; unit = UVC_PU_ANALOG_VIDEO_STANDARD_CONTROL; break;
					case 17: name = "Analog Lock Status"; unit = UVC_PU_ANALOG_LOCK_STATUS_CONTROL; break;
					case 18: name = "Contrast, Auto"; unit = UVC_PU_CONTRAST_AUTO_CONTROL; break;
					default:
						break;
				}
				break;
			case UVC_VC_EXTENSION_UNIT:
				// don't know what all these are yet, but try querying them
				// anyway...
				name = "Unknown Vendor Control";
				unit = i + 1;
				if ( matchGUID( myExtensionUnit, TIS_EXTENSION_BLOCK_SKYRIS_NEWER ) )
				{
					switch ( i )
					{
						case 15: name = "Binning"; roiControlIdx = roiBINNING; break;
						case 16: name = "ROI Width"; roiControlIdx = roiWIDTH; break;
						case 17: name = "ROI Height"; roiControlIdx = roiHEIGHT; break;
						case 18: name = "Some Sort of Fixed Flag?"; break;
						case 19: name = "Seems to just error?"; break;
						case 20: name = "ROI Offset X"; roiControlIdx = roiOFFSET_X; break;
						case 21: name = "ROI Offset Y"; roiControlIdx = roiOFFSET_Y; break;
						case 22: name = "Some kind of flag?"; break;
						case 23: name = "Unknown?"; break;
						case 24: name = "Fixed high-ish number (UART?) has size of 9"; break;
						case 26: name = "Unknown 2?"; break;
						default:
							name = "Unknown Skyris Extension Control";
							break;
					}
				}
				else if ( matchGUID( myExtensionUnit, TIS_EXTENSION_BLOCK_NEXIMAGE ) )
				{
					name = "Unknown NexImage Extension Control";

					// hidden controls...
					if ( unit == UVC_CT_TIS_PARTIAL_SCAN_WIDTH )
					{
						name = "Partial Scan Width";
						roiControlIdx = roiWIDTH;
					}
					else if ( unit == UVC_CT_TIS_PARTIAL_SCAN_HEIGHT )
					{
						name = "Partial Scan Height";
						roiControlIdx = roiHEIGHT;
					}
					else if ( unit == UVC_CT_TIS_BINNING )
					{
						name = "Binning";
						roiControlIdx = roiBINNING;
					}
				}
				break;

			default:
				break;
		}

		if ( name && unit )
		{
			std::shared_ptr<Control> newCtrl( new Control );
			bool skipped = true;
			try
			{
				newCtrl->init( name, myHandle, myControlEndPoint, unit, iface, terminal, myContext );
				
				if ( newCtrl->valid() )
				{
					info() << "Added " << (*newCtrl) << send;
					myControls.push_back( newCtrl );
					if ( roiControlIdx >= 0 )
					{
						mySupportsROI = true;
						myROIControls[roiControlIdx] = newCtrl;
					}

					skipped = false;
				}
			}
			catch ( ... )
			{
			}

			if ( skipped )
			{
				info() << "Skipping Control " << i << " ('" << name << "') due to errors" << send;
			}
		}
	}
}


////////////////////////////////////////


void
UVCDevice::dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iFaceDesc )
{
	if ( iFaceDesc.bInterfaceClass == 0x0e && iFaceDesc.bNumEndpoints > 0 )
	{
		const unsigned char *controlBuf = iFaceDesc.extra;
		int buflen = int(iFaceDesc.extra_length);
		dumpFormats( os, controlBuf, buflen );
	}
	else if ( iFaceDesc.extra_length > 0 )
	{
		const unsigned char *controlBuf = iFaceDesc.extra;
		int buflen = int(iFaceDesc.extra_length);
		dumpStandardControls( os, controlBuf, buflen );
	}
}


////////////////////////////////////////


void
UVCDevice::dumpControls( std::ostream &os, const uint8_t type, const uint8_t *bmControls, int bControlSize )
{
	for ( int i = 0; i < bControlSize * 8; ++i )
	{
		if ( test_bit( bmControls, i ) == 0 )
			continue;

		const char *name = NULL;
		switch ( type )
		{
			case UVC_VC_INPUT_TERMINAL:
				switch ( i )
				{
					case 0: name = "Scanning Mode"; break;
					case 1: name = "AE Mode"; break;
					case 2: name = "AE Priority"; break;
					case 3: name = "Exposure Time Absolute"; break;
					case 4: name = "Exposure Time Relative"; break;
					case 5: name = "Focus Absolute"; break;
					case 6: name = "Focus Relative"; break;
					case 7: name = "Iris Absolute"; break;
					case 8: name = "Iris Relative"; break;
					case 9: name = "Zoom Absolute"; break;
					case 10: name = "Zoom Relative"; break;
					case 11: name = "Pan / Tilt Absolute"; break;
					case 12: name = "Pan / Tilt Relative"; break;
					case 13: name = "Roll Absolute"; break;
					case 14: name = "Roll Relative"; break;
					case 15: name = "<Reserved>"; break;
					case 16: name = "<Reserved>"; break;
					case 17: name = "Focus, Auto"; break;
					case 18: name = "Privacy Control"; break;
					case 19: name = "Focus, Simple"; break;
					case 20: name = "Window"; break;
					case 21: name = "Region of Interest"; break;
					case 22: name = "<Reserved>"; break;
					case 23: name = "<Reserved>"; break;
					default:
						name = "<Unknown Input Terminal>"; break;
						break;
				}
				break;
			case UVC_VC_PROCESSING_UNIT:
				switch ( i )
				{
					case 0: name = "Brightness"; break;
					case 1: name = "Contrast"; break;
					case 2: name = "Hue"; break;
					case 3: name = "Saturation"; break;
					case 4: name = "Sharpness"; break;
					case 5: name = "Gamma"; break;
					case 6: name = "White Balance Temperature"; break;
					case 7: name = "White Balance Component"; break;
					case 8: name = "Backlight Compensation"; break;
					case 9: name = "Gain"; break;
					case 10: name = "Power Line Frequency"; break;
					case 11: name = "Hue, Auto"; break;
					case 12: name = "White Balance Temperature, Auto"; break;
					case 13: name = "White Balance Component, Auto"; break;
					case 14: name = "Digital Multiplier"; break;
					case 15: name = "Digital Multiplier Limit"; break;
					case 16: name = "Analog Video Standard"; break;
					case 17: name = "Analog Lock Status"; break;
					case 18: name = "Contrast, Auto"; break;
					case 19: name = "<Reserved> (Pixel Clock?)"; break;
					case 20: name = "<Reserved>"; break;
					case 21: name = "<Reserved>"; break;
					case 22: name = "<Reserved>"; break;
					case 23: name = "<Reserved>"; break;
					default:
						name = "<Unknown Processing Unit>"; break;
						break;
				}
				break;
			case UVC_VC_EXTENSION_UNIT:
				name = "Unknown Vendor Extension";
				break;

			default:
				name = "Unknown Block Type Control";
				break;
		}
		if ( name )
			os << "             index: " << i << ' ' << name << std::endl;
	}
}


////////////////////////////////////////


void
UVCDevice::dumpStandardControls( std::ostream &os, const unsigned char *buffer, int buflen, bool skipIfNoCS )
{
	while ( buflen > 2 )
	{
		const uvc_descriptor_header *desc = reinterpret_cast<const uvc_descriptor_header *>( buffer );

		if ( desc->bDescriptorType != USB_DT_CS_INTERFACE )
		{
			os << "Skipping non DT_CS_INTERFACE info: " << std::hex << desc->bDescriptorType << std::dec << std::endl;
			buflen -= desc->bLength;
			buffer += desc->bLength;
			continue;
		}

		switch ( desc->bDescriptorSubType )
		{
			case UVC_VC_HEADER:
				if ( buflen >= UVC_DT_HEADER_SIZE(0) )
				{
					const uvc_header_descriptor *header = reinterpret_cast<const uvc_header_descriptor *>( desc );
					int n = header->bInCollection;
					os << "        UVC_VC_HEADER:\n";
					dumpHex( os, "           uvc_version", header->bcdUVC, true );
					dumpHex( os, "           total length", header->wTotalLength, true );
					dumpHex( os, "           clock freq", header->dwClockFrequency, true );
					os << "           num interfaces: " << n << std::endl;
					os << "            ";
					for ( int i = 0; i < n; ++i )
						os << ' ' << int(header->baInterfaceNr[i]);
					os << std::endl;
				}
				else
				{
					os << "        UVC_VC_HEADER: invalid header size: " << buflen << std::endl;
				}
				break;
			case UVC_VC_INPUT_TERMINAL:
				if ( buflen >= UVC_DT_INPUT_TERMINAL_SIZE )
				{
					const uvc_input_terminal_descriptor *itd = reinterpret_cast<const uvc_input_terminal_descriptor *>( desc );
					os << "        UVC_VC_INPUT_TERMINAL:\n";
					dumpHex( os, "           ID", itd->bTerminalID );
					dumpHex( os, "           type", itd->wTerminalType );
					dumpHex( os, "           assoc terminal", itd->bAssocTerminal );
					if ( itd->iTerminal != 0 )
					{
						os << "           name: " << int(itd->iTerminal) << " '" << pullString( itd->iTerminal ) << "'" << std::endl;
					}

					switch ( itd->wTerminalType )
					{
						case UVC_ITT_VENDOR_SPECIFIC:
							os << "           -- VENDOR SPECIFIC --" << std::endl;
							break;
						case UVC_ITT_CAMERA:
						{
							const uvc_camera_terminal_descriptor *ctd = reinterpret_cast<const uvc_camera_terminal_descriptor *>( itd );
							os << "           UTT CAMERA INFO" << std::endl;
							dumpHex( os, "           objective focal length min", ctd->wObjectiveFocalLengthMin, true );
							dumpHex( os, "           objective focal length max", ctd->wObjectiveFocalLengthMax, true );
							dumpHex( os, "           ocular focal length", ctd->wOcularFocalLength, true );
							if ( ctd->bControlSize )
							{
								os << "           controls: " << int(ctd->bControlSize) << "bytes\n";
								dumpControls( os, desc->bDescriptorSubType, ctd->bmControls, ctd->bControlSize );
							}
							break;
						}
						case UVC_ITT_MEDIA_TRANSPORT_INPUT:
						{
							os << "           MEDIA TRANSPORT:" << std::endl;
							int n = buflen >= 9 ? buffer[8] : 0;
							if ( n > 0 )
							{
								os << "             controls: " << n << "bytes\n";
								dumpControls( os, desc->bDescriptorSubType, &buffer[9], n );
							}
							break;
						}
						default:
							dumpHex( os, "           UNKNOWN terminal type", itd->wTerminalType );
							break;
					}
				}
				else
				{
					os << "        UVC_VC_INPUT_TERMINAL: invalid header size: " << buflen << std::endl;
				}
				break;
			case UVC_VC_OUTPUT_TERMINAL:
				if ( buflen >= UVC_DT_OUTPUT_TERMINAL_SIZE )
				{
					const uvc_output_terminal_descriptor *otd = reinterpret_cast<const uvc_output_terminal_descriptor *>( desc );
					os << "        UVC_VC_OUTPUT_TERMINAL:\n";
					dumpHex( os, "           ID", otd->bTerminalID );
					dumpHex( os, "           type", otd->wTerminalType );
					dumpHex( os, "           assoc terminal", otd->bAssocTerminal );
					dumpHex( os, "           source ID", otd->bSourceID );
					if ( otd->iTerminal != 0 )
					{
						os << "           name: " << int(otd->iTerminal) << " '" << pullString( otd->iTerminal ) << "'" << std::endl;
					}
				}
				else
				{
					os << "        UVC_VC_OUTPUT_TERMINAL: invalid header size: " << buflen << std::endl;
				}
				break;
			case UVC_VC_SELECTOR_UNIT:
				if ( buflen >= UVC_DT_SELECTOR_UNIT_SIZE(0) )
				{
					const uvc_selector_unit_descriptor *sud = reinterpret_cast<const uvc_selector_unit_descriptor *>( desc );
					os << "        UVC_VC_SELECTOR_UNIT:\n";
					dumpHex( os, "           ID", sud->bUnitID );
					dumpHex( os, "           bNrInPins", sud->bNrInPins );
					switch ( sud->bNrInPins )
					{
						case 0:
							break;
						case 1:
							dumpHex( os, "            baSourceID", sud->baSourceID[0] );
							break;
						case 2:
							dumpHex( os, "            baSourceID", get_unaligned_le16( sud->baSourceID ) );
							break;
						case 4:
							dumpHex( os, "            baSourceID", get_unaligned_le32( sud->baSourceID ) );
							break;
						default:
							os << "           Unhandled selector size " << int(sud->bNrInPins) << std::endl;
							break;
					}
					uint8_t iSelNameID = sud->baSourceID[sud->bNrInPins];
					if ( iSelNameID != 0 )
					{
						os << "           name: " << int(iSelNameID) << " '" << pullString( iSelNameID ) << "'" << std::endl;
					}
				}
				else
				{
					os << "        UVC_VC_OUTPUT_TERMINAL: invalid header size: " << buflen << std::endl;
				}
				break;
			case UVC_VC_PROCESSING_UNIT:
				if ( buflen >= UVC_DT_PROCESSING_UNIT_SIZE(0) )
				{
					const uvc_processing_unit_descriptor *pud = reinterpret_cast<const uvc_processing_unit_descriptor *>( desc );
					os << "        UVC_VC_PROCESSING_UNIT:\n";
					dumpHex( os, "           ID", pud->bUnitID );
					dumpHex( os, "           Source ID", pud->bSourceID );
					dumpHex( os, "           Max Multiplier", pud->wMaxMultiplier, true );
					uint8_t procNameID = pud->bmControls[pud->bControlSize];
					if ( procNameID != 0 )
					{
						os << "           Name: " << int(procNameID) << " '" << pullString( procNameID ) << "'" << std::endl;
					}

					if ( pud->bLength > UVC_DT_PROCESSING_UNIT_SIZE(pud->bControlSize) )
					{
						// 1.5 has video standards
						dumpHex( os, "           Video Standards", pud->bmControls[pud->bControlSize+1] );
					}

					os << "           controls: " << int(pud->bControlSize) << " bytes\n";
					dumpControls( os, desc->bDescriptorSubType, pud->bmControls, pud->bControlSize );

				}
				else
				{
					os << "        UVC_VC_PROCESSING_UNIT: invalid header size: " << buflen << std::endl;
				}
				break;

			case UVC_VC_EXTENSION_UNIT:
				if ( buflen >= UVC_DT_EXTENSION_UNIT_SIZE(0, 0) )
				{
					const uvc_extension_unit_descriptor *eud = reinterpret_cast<const uvc_extension_unit_descriptor *>( desc );
					os << "        UVC_VC_EXTENSION_UNIT:\n";
					dumpHex( os, "           ID", eud->bUnitID );
					dumpGUID( os, "           ID", eud->guidExtensionCode );
					dumpHex( os, "           Num Controls", eud->bNumControls, true );
					dumpHex( os, "           Num In Pins", eud->bNrInPins, true );
					int nControls = eud->bNumControls;
					int nPins = eud->bNrInPins;
					int ctrlSize = eud->baSourceID[nPins];
					os << "           Control Size: " << ctrlSize << std::endl;
					for ( int i = 0; i < nPins; ++i )
					{
						os << "           Input[" << i;
						dumpHex( os, "]", eud->baSourceID[i] );
					}
					uint8_t nameID = eud->baSourceID[nPins + ctrlSize + 1];
					if ( nameID != 0 )
					{
						os << "           Name: " << int(nameID) << " '" << pullString( nameID ) << "'" << std::endl;
					}
					if ( ctrlSize > 0 )
					{
						os << "           controls: " << ctrlSize << " bytes\n";;
						dumpControls( os, desc->bDescriptorSubType, eud->baSourceID + nPins + 1, ctrlSize );
					}
				}
				else
				{
					os << "        UVC_VC_PROCESSING_UNIT: invalid header size: " << buflen << std::endl;
				}
				break;

			default:
				dumpHex( os, "        unknown UVC control tag", buffer[2] );
				break;
		}

		buflen -= buffer[0];
		buffer += buffer[0];
	}
}

void
UVCDevice::dumpFormats( std::ostream &os, const unsigned char *buffer, int buflen )
{
	if ( buflen <= 2 )
	{
		os << "ERROR, no stream format descriptors?" << std::endl;
		return;
	}
	switch ( buffer[2] )
	{
		case UVC_VS_OUTPUT_HEADER:
			os << "        VIDEO OUTPUT" << std::endl;
			dumpOutputFormats( os, buffer, buflen );
			break;
		case UVC_VS_INPUT_HEADER:
			os << "        VIDEO CAPTURE" << std::endl;
			dumpInputFormats( os, buffer, buflen );
			break;
		default:
			os << "ERROR, invalid header descriptor" << std::endl;
			return;
	}
}

void
UVCDevice::dumpOutputFormats( std::ostream &os, const unsigned char *buffer, int buflen )
{
	if ( buflen < UVC_DT_OUTPUT_HEADER_SIZE(0, 0) )
	{
		os << "ERROR, invalid output header size: " << buflen << std::endl;
	}

	const uvc_output_header_descriptor *outpDesc = reinterpret_cast<const uvc_output_header_descriptor *>( buffer );
	os << "        Num Formats: " << int(outpDesc->bNumFormats) << std::endl;
	os << "        Total Length: " << int(outpDesc->wTotalLength) << std::endl;
	dumpHex( os, "        Endpoint Address", outpDesc->bEndpointAddress );
	dumpHex( os, "        Terminal Link", outpDesc->bTerminalLink );
	dumpHex( os, "        ControlSize", outpDesc->bTerminalLink );
	os << "        <ADD PARSING OF OUTPUT CONTROLS>" << std::endl;
}

void
UVCDevice::dumpInputFormats( std::ostream &os, const unsigned char *buffer, int buflen )
{
	if ( buflen < UVC_DT_INPUT_HEADER_SIZE(0, 0) )
	{
		os << "ERROR, invalid input header size: " << buflen << std::endl;
		return;
	}

	const uvc_input_header_descriptor *inpDesc = reinterpret_cast<const uvc_input_header_descriptor *>( buffer );

	int numFormats = inpDesc->bNumFormats; // p
	int ctrlSize = inpDesc->bControlSize; // n

	os << "        Video Formats: " << numFormats << std::endl;
	os << "        Control Size: " << ctrlSize << std::endl;
	if ( buflen < UVC_DT_INPUT_HEADER_SIZE(numFormats, ctrlSize) )
	{
		os << "ERROR, invalid input header size: " << buflen << " vs " << int(UVC_DT_INPUT_HEADER_SIZE(numFormats, ctrlSize)) << std::endl;
		return;
	}

	dumpHex( os, "        Endpoint Address: ", inpDesc->bEndpointAddress );
	dumpHex( os, "        Total Length: ", inpDesc->wTotalLength, true );
	dumpHex( os, "        bmInfo: ", inpDesc->bmInfo );
	if ( ( inpDesc->bmInfo & 0x1 ) )
		os << "         -> Dynamic Format Change Supported" << std::endl;
	dumpHex( os, "        bTerminalLink: ", inpDesc->bTerminalLink );
	os << "        Still Capture Method: " << int(inpDesc->bStillCaptureMethod) << ' ';
	switch ( inpDesc->bStillCaptureMethod )
	{
		case 0: os << "None"; break;
		case 1: os << "Method 1"; break;
		case 2: os << "Method 2"; break;
		case 3: os << "Method 3"; break;
		default:
			os << "<Unknown>";
			break;
	}
	os << "\n        Trigger Support: " << int(inpDesc->bTriggerSupport) << ' ';
	switch ( inpDesc->bTriggerSupport )
	{
		case 0: os << "Not Supported"; break;
		case 1: os << "Supported"; break;
		default:
			os << "<Unknown>";
			break;
	}
	if ( inpDesc->bTriggerUsage )
	{
		if ( inpDesc->bTriggerUsage )
			os << "\n        Trigger Usage: <Initiator>";
		else
			os << "\n        Trigger Usage: <General Purpose>";
	}
	os << std::endl;

	
	for ( int i = 0; i < numFormats; ++i )
	{
		const uint8_t *bmControls = inpDesc->bmaControls + i * ctrlSize;

		os << "        Controls Format Index " << i << ":" << std::endl;
		int nbits = 0;
		for ( int b = 0; b < ctrlSize * 8; ++b )
		{
			if ( test_bit( bmControls, b ) == 0 )
				continue;
			++nbits;
			switch ( b )
			{
				case 0: os << "          wKeyFrameRate" << std::endl; break;
				case 1: os << "          wPFrameRate" << std::endl; break;
				case 2: os << "          wCompQuality" << std::endl; break;
				case 3: os << "          wCompWindowSize" << std::endl; break;
				case 4: os << "          Generate Key Frame" << std::endl; break;
				case 5: os << "          Update Frame Segment" << std::endl; break;
				default:
					os << "          <Reserved>" << std::endl;
					break;
			}
		}

		if ( nbits == 0 )
			os << "          <None>" << std::endl;
	}

	buflen -= buffer[0];
	buffer += buffer[0];

	int curbpp = 1;
	while ( buflen > 2 && buffer[1] == USB_DT_CS_INTERFACE )
	{
		const uvc_descriptor_header *cur = reinterpret_cast<const uvc_descriptor_header *>( buffer );
		switch ( cur->bDescriptorSubType )
		{
			case UVC_VS_FORMAT_UNCOMPRESSED:
				os << "        UNCOMPRESSED FORMAT:\n";
				dumpFormatUncompressed( os, buffer, buflen, curbpp );
				break;
			case UVC_VS_FORMAT_MJPEG:
				os << "        MJPEG FORMAT: <need printer>\n";
				break;
			case UVC_VS_FORMAT_FRAME_BASED:
				os << "        FRAME BASED FORMAT:\n";
				dumpFormatFrameBased( os, buffer, buflen, curbpp );
				break;
			case UVC_VS_FORMAT_DV:
				os << "        DV FORMAT: <need printer>\n";
				break;
			case UVC_VS_FORMAT_MPEG2TS:
				os << "        MPEG2TS FORMAT: <need printer>\n";
				break;
			case UVC_VS_FORMAT_STREAM_BASED:
				os << "        STREAM BASED FORMAT: <need printer>\n";
				break;
			case UVC_VS_FRAME_UNCOMPRESSED:
				os << "          FRAME UNCOMPRESSED:\n";
				dumpFrameUncompressed( os, buffer, buflen, curbpp );
				break;
			case UVC_VS_FRAME_MJPEG:
				os << "          FRAME MJPEG: <need printer>\n";
				break;
			case UVC_VS_FRAME_FRAME_BASED:
				os << "          FRAME BASED FRAME:\n";
				dumpFrameFrameBased( os, buffer, buflen, curbpp );
				break;
			case UVC_VS_COLORFORMAT:
				os << "        COLOR MATCHING DESCRIPTOR:\n";
				dumpHex( os, "          Color Primaries", buffer[3] );
				if ( buffer[0] > 4 )
					dumpHex( os, "          Transfer Characteristics", buffer[4] );
				if ( buffer[0] > 5 )
					dumpHex( os, "          Matrix Coefficients", buffer[5] );
				break;
			case UVC_VS_STILL_IMAGE_FRAME:
				os << "          STILL IMAGE FRAME INFO:\n";
				dumpFrameUncompressed( os, buffer, buflen, curbpp );
				break;
			case UVC_VS_INPUT_HEADER:
				os << "        INPUT HEADER: <need printer>\n";
				break;
			case UVC_VS_OUTPUT_HEADER:
				os << "        OUTPUT HEADER: <need printer>\n";
				break;
			default:
				dumpHex( os, "unknown USB_DT_CS_INTERFACE tag", buffer[2] );
				break;
		}
		buflen -= buffer[0];
		buffer += buffer[0];
	}
}


////////////////////////////////////////


void
UVCDevice::dumpFormatUncompressed( std::ostream &os, const unsigned char *buffer, int buflen, int &bpp )
{
	const uvc_format_uncompressed *fmt = reinterpret_cast<const uvc_format_uncompressed *>( buffer );

	bpp = fmt->bBitsPerPixel / 8;
	dumpHex( os, "          Format IDX", fmt->bFormatIndex );
	dumpHex( os, "          Bits Per Pixel", fmt->bBitsPerPixel, true );
	dumpHex( os, "          Num Frame Descriptors", fmt->bNumFrameDescriptors, true );
	dumpGUID( os, "          Format GUID", fmt->guidFormat, true );
	dumpHex( os, "          Default Frame Idx", fmt->bDefaultFrameIndex, true );
	dumpHex( os, "          Aspect Ratio X", fmt->bAspectRatioX, true );
	dumpHex( os, "          Aspect Ratio Y", fmt->bAspectRatioY, true );
	dumpHex( os, "          Interlace Flags", fmt->bmInterfaceFlags );
	dumpHex( os, "          Copy Protect", fmt->bCopyProtect );
}


////////////////////////////////////////


void
UVCDevice::dumpFrameUncompressed( std::ostream &os, const unsigned char *buffer, int buflen, int bpp )
{
	const uvc_frame_uncompressed *fmt = reinterpret_cast<const uvc_frame_uncompressed *>( buffer );
	dumpHex( os, "            Frame IDX", fmt->bFrameIndex );
	dumpHex( os, "            Capabilities", fmt->bmCapabilities );
	os << "            Width: (" << int(fmt->wWidth) << " x " << bpp << ") " << int(fmt->wWidth) * bpp << std::endl;
	os << "            Height: " << int(fmt->wHeight) << std::endl;
	dumpHex( os, "            Min Bit Rate", fmt->dwMinBitRate, true );
	dumpHex( os, "            Max Bit Rate", fmt->dwMaxBitRate, true );
	dumpHex( os, "            Max Video Buffer Size", fmt->dwMaxVideoFrameBufferSize, true );
	dumpHex( os, "            Default Frame Interval", fmt->dwDefaultFrameInterval, true );
	dumpHex( os, "            Frame Interval Type", fmt->bFrameIntervalType );
	if ( fmt->bFrameIntervalType == 0 )
	{
		dumpHex( os, "            Min Frame Interval", fmt->dwFrameInterval[0] );
		dumpHex( os, "            Max Frame Interval", fmt->dwFrameInterval[1] );
		dumpHex( os, "            Frame Interval Step", fmt->dwFrameInterval[2] );
	}
	else
	{
		for ( uint8_t i = 0; i < fmt->bFrameIntervalType; ++i )
		{
			os << "            Frame Interval[" << int(i);
			dumpHex( os, "]", fmt->dwFrameInterval[i], true );
		}
	}
}


////////////////////////////////////////


void
UVCDevice::dumpFormatFrameBased( std::ostream &os, const unsigned char *buffer, int buflen, int &bpp )
{
	const uvc_format_frame_based *fmt = reinterpret_cast<const uvc_format_frame_based *>( buffer );

	bpp = fmt->bBitsPerPixel / 8;
	dumpHex( os, "          Format IDX", fmt->bFormatIndex );
	dumpHex( os, "          Bits Per Pixel", fmt->bBitsPerPixel, true );
	dumpHex( os, "          Num Frame Descriptors", fmt->bNumFrameDescriptors, true );
	dumpGUID( os, "          Format GUID", fmt->guidFormat, true );
	dumpHex( os, "          Default Frame Idx", fmt->bDefaultFrameIndex, true );
	dumpHex( os, "          Aspect Ratio X", fmt->bAspectRatioX, true );
	dumpHex( os, "          Aspect Ratio Y", fmt->bAspectRatioY, true );
	dumpHex( os, "          Interlace Flags", fmt->bmInterfaceFlags );
	dumpHex( os, "          Copy Protect", fmt->bCopyProtect );
	dumpHex( os, "          Variable Size", fmt->bVariableSize );
}


////////////////////////////////////////


void
UVCDevice::dumpFrameFrameBased( std::ostream &os, const unsigned char *buffer, int buflen, int bpp )
{
	const uvc_frame_frame_based *fmt = reinterpret_cast<const uvc_frame_frame_based *>( buffer );
	dumpHex( os, "            Frame IDX", fmt->bFrameIndex );
	dumpHex( os, "            Capabilities", fmt->bmCapabilities );
	os << "            Width: (" << int(fmt->wWidth) << " x " << bpp << ") " << int(fmt->wWidth) * bpp << std::endl;
	os << "            Height: " << int(fmt->wHeight) << std::endl;
	dumpHex( os, "            Min Bit Rate", fmt->dwMinBitRate, true );
	dumpHex( os, "            Max Bit Rate", fmt->dwMaxBitRate, true );
	dumpHex( os, "            Default Frame Interval", fmt->dwDefaultFrameInterval );
	dumpHex( os, "            Frame Interval Type", fmt->bFrameIntervalType );
	dumpHex( os, "            Bytes Per Line", fmt->dwBytesPerLine, true );
	if ( fmt->bFrameIntervalType == 0 )
	{
		dumpHex( os, "            Min Frame Interval", fmt->dwFrameInterval[0] );
		dumpHex( os, "            Max Frame Interval", fmt->dwFrameInterval[1] );
		dumpHex( os, "            Frame Interval Step", fmt->dwFrameInterval[2] );
	}
	else
	{
		for ( uint8_t i = 0; i < fmt->bFrameIntervalType; ++i )
		{
			os << "            Frame Interval[" << int(i);
			dumpHex( os, "]", fmt->dwFrameInterval[i], true );
		}
	}
}


////////////////////////////////////////


void
UVCDevice::probeVideo( UVCProbe &p, size_t &len )
{
	memset( &p, 0, sizeof(UVCProbe) );
	len = getProbeLen();

	uint8_t reqType = endpoint_in(0) | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
	uint16_t idx = 1;
	int err = libusb_control_transfer( myHandle, reqType, UVC_GET_CUR,
									   (UVC_VS_PROBE_CONTROL << 8),
									   idx,
									   (unsigned char *)&p, len, 0 );
	if ( err <= 26 )
	{
		error() << "Unable to probe device settings: " << err << send;
	}
	else
		len = err;
}


////////////////////////////////////////


void
UVCDevice::dumpProbe( const char *tag, const UVCProbe &p, size_t len )
{
	std::cout << tag << ":\n";
	dumpHex( std::cout, "  bmHint", p.bmHint );
	dumpHex( std::cout, "  bFormatIndex", p.bFormatIndex );
	dumpHex( std::cout, "  bFrameIndex", p.bFrameIndex );
	dumpHex( std::cout, "  dwFrameInterval", p.dwFrameInterval );
	dumpHex( std::cout, "  wKeyFrameRate", p.wKeyFrameRate );
	dumpHex( std::cout, "  wPFrameRate", p.wPFrameRate );
	dumpHex( std::cout, "  wCompQuality", p.wCompQuality );
	dumpHex( std::cout, "  wCompWindowSize", p.wCompWindowSize );
	dumpHex( std::cout, "  wDelay", p.wDelay );
	dumpHex( std::cout, "  dwMaxVideoFrameSize", p.dwMaxVideoFrameSize, true );
	dumpHex( std::cout, "  dwMaxPayloadTransferSize", p.dwMaxPayloadTransferSize, true );
	if ( len > 26 )
	{
		dumpHex( std::cout, "  dwClockFrequency", p.dwClockFrequency, true );
		dumpHex( std::cout, "  bmFramingInfo", p.bmFramingInfo, true );
		dumpHex( std::cout, "  bPreferredVersion", p.bPreferredVersion, true );
		dumpHex( std::cout, "  bMinVersion", p.bMinVersion, true );
		dumpHex( std::cout, "  bMaxVersion", p.bMaxVersion, true );
	}
	if ( len > 34 )
	{
		dumpHex( std::cout, "  bUsage", p.bUsage );
		dumpHex( std::cout, "  bBitDepthLuma", p.bBitDepthLuma, true );
		dumpHex( std::cout, "  bmSettings", p.bmSettings );
		dumpHex( std::cout, "  bMaxNumberOfRefFramesPlus1", p.bMaxNumberOfRefFramesPlus1, true );
		dumpHex( std::cout, "  bmRateControlModes", p.bmRateControlModes );
		dumpHex( std::cout, "  bmLayoutPerStream[0]", p.bmLayoutPerStream[0] );
		dumpHex( std::cout, "  bmLayoutPerStream[1]", p.bmLayoutPerStream[1] );
		dumpHex( std::cout, "  bmLayoutPerStream[2]", p.bmLayoutPerStream[2] );
		dumpHex( std::cout, "  bmLayoutPerStream[3]", p.bmLayoutPerStream[3] );
	}
}


////////////////////////////////////////


size_t
UVCDevice::getCurrentSetup( const UVCProbe &devInfo )
{
	uint8_t curFmtIdx = devInfo.bFormatIndex;
	uint8_t curFrmIdx = devInfo.bFrameIndex;

	if ( curFmtIdx == 0 || curFrmIdx == 0 )
		throw std::runtime_error( "Invalid setup" );

	bool found = false;
	for ( size_t f = 0; f < myFormats.size(); ++f )
	{
		const FrameDefinition &frm = myFormats[f];
		if ( frm.format_index == curFmtIdx && frm.frame_index == curFrmIdx )
		{
			myCurrentFrame = f;
			return f;
		}
	}

	throw std::runtime_error( "Unable to find current format / frame pair" );
}


////////////////////////////////////////


} // usbpp



