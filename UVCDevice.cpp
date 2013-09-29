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

// cheating for now, need to have our own constants to work on the mac
//#include <linux/usb/video.h>
//#include <linux/v4l2-controls.h>
#include "uvc_constants.h"
#include <stdio.h>


////////////////////////////////////////


#define USB_DT_CS_INTERFACE ((0x01 << 5) | 0x04)


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


std::shared_ptr<Device>
UVCDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::shared_ptr<Device>( new UVCDevice( dev, desc ) );
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


template <typename T>
inline void
spitValue( libusb_device_handle *handle, uint16_t unit, int interface, const char *tag )
{
	// libusb_control_transfer( handle, request_type, request, wvalue, windex
	//                          data, wlength, timeout )
	// usb_control_msg( dev, pipe, request, requesttype, value, index, data,
	//                  size, timeout )
	// __uvc_query_ctrl( dev, query, unit, intfnum, cs, data, size, timeout )
	//    -> usb_control_msg( dev->udev, pipe, query, type, cs << 8,
	//                        unit << 8 | intfnum, data, size, timeout )
	// uvc_query_ctrl( dev, query, unit, intfnum, cs, data, size )

	// uvc_query_ctrl( chain->dev, UVC_GET_CUR, ctrl->entity->id,
	//                 chain->dev->intfnum, ctrl->info.selector,
	//                 data, size )
	// ctrl->entity->id = V4L2_CID_GAIN
	// ctrl->dev->intfnum = 0
	// ctrl->info.selector = UVC_PU_GAIN_CONTROL
	T val = T(0);
	T mn = T(0);
	T mx = T(0);
	uint16_t value = unit << 8;
	int x = libusb_control_transfer( handle, 0xa1, UVC_GET_CUR, value,
									 1 << 8 | interface,
									 (unsigned char *)(&val),
									 sizeof(val), 1000 );
	int y = libusb_control_transfer( handle, 0xa1, UVC_GET_MIN, value,
									 1 << 8 | interface,
									 (unsigned char *)(&mn),
									 sizeof(mn), 1000 );
	int z = libusb_control_transfer( handle, 0xa1, UVC_GET_MAX, value,
									 1 << 8 | interface,
									 (unsigned char *)(&mx),
									 sizeof(mx), 1000 );
	if ( x == sizeof(val) && x == y && x == y )
	{
		std::cout << tag << ": " << int(val) << " (" << int(mn) << " - " << int(mx) << ")" << std::endl;
	}
	else
	{
		std::cout << tag << ": ERROR(" << x << ", " << y << ", " << z << "): "
				  << libusb_strerror( libusb_error(x) ) << std::endl;
	}
}


////////////////////////////////////////


void
UVCDevice::queryValues( void )
{
	spitValue<uint16_t>( myHandle, UVC_PU_GAIN_CONTROL, 0, "Gain" );
	spitValue<uint16_t>( myHandle, UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL, 0, "Absolute Exposure Time" );
	spitValue<uint16_t>( myHandle, UVC_PU_BRIGHTNESS_CONTROL, 1, "Brightness" );
	spitValue<uint8_t>( myHandle, UVC_PU_HUE_CONTROL, 1, "Hue" );
	spitValue<uint16_t>( myHandle, UVC_PU_CONTRAST_CONTROL, 1, "Contrast" );
	// can only set these
//	spitValue<uint16_t>( myHandle, UVC_CT_EXPOSURE_TIME_RELATIVE_CONTROL, 0, "Relative Exposure Time" );
//	spitValue<uint16_t>( myHandle, UVC_PU_DIGITAL_MULTIPLIER_CONTROL, 1, "Digital Multiplier" );
	spitValue<uint32_t>( myHandle, 0x24, 0, "Pixel Clock" );

}


////////////////////////////////////////


void
UVCDevice::startVideo( void )
{
	stopEventHandling();

	uint16_t gainControl = 127;
	int err = libusb_control_transfer( myHandle, 0x21, UVC_SET_CUR,
									   UVC_PU_GAIN_CONTROL << 8,
									   1 << 8 | 0,
									   (unsigned char *)(&gainControl),
									   sizeof(gainControl), 1000 );
	std::cout << "setting gain control: " << err << std::endl;

	// Just for now, do the bulk transfer default setting...
	check_error( libusb_set_interface_alt_setting( myHandle, 1, 0 ) );

	size_t len = sizeof(myDeviceInfo);
//	if ( myUVCVersion < 0x0110 )
//		len = len - 8;

	err = libusb_control_transfer( myHandle, 0xa1, UVC_GET_CUR,
								   (UVC_VS_PROBE_CONTROL << 8),
								   1,
								   (unsigned char *)&myDeviceInfo, len, 0 );
	if ( err > 0 )
	{
		std::cout << "Device Probe:\n";
		dumpHex( std::cout, "bmHint", myDeviceInfo.bmHint );
		dumpHex( std::cout, "bFormatIndex", myDeviceInfo.bFormatIndex );
		dumpHex( std::cout, "bFrameIndex", myDeviceInfo.bFrameIndex );
		dumpHex( std::cout, "dwFrameInterval", myDeviceInfo.dwFrameInterval );
		dumpHex( std::cout, "wKeyFrameRate", myDeviceInfo.wKeyFrameRate );
		dumpHex( std::cout, "wPFrameRate", myDeviceInfo.wPFrameRate );
		dumpHex( std::cout, "wCompQuality", myDeviceInfo.wCompQuality );
		dumpHex( std::cout, "wCompWindowSize", myDeviceInfo.wCompWindowSize );
		dumpHex( std::cout, "wDelay", myDeviceInfo.wDelay );
		dumpHex( std::cout, "dwMaxVideoFrameSize", myDeviceInfo.dwMaxVideoFrameSize, true );
		dumpHex( std::cout, "dwMaxPayloadTransferSize", myDeviceInfo.dwMaxPayloadTransferSize, true );
		if ( err > 26 )
		{
			dumpHex( std::cout, "dwClockFrequency", myDeviceInfo.dwClockFrequency, true );
			dumpHex( std::cout, "bmFramingInfo", myDeviceInfo.bmFramingInfo, true );
			dumpHex( std::cout, "bPreferredVersion", myDeviceInfo.bPreferredVersion, true );
			dumpHex( std::cout, "bMinVersion", myDeviceInfo.bMinVersion, true );
			dumpHex( std::cout, "bMaxVersion", myDeviceInfo.bMaxVersion, true );
			
		}
	}
	else
	{
		std::cerr << "Unable to probe device settings: " << err << std::endl;
	}

	std::cerr << "Attempting to set probe control: " << err << std::endl;
	err = libusb_control_transfer( myHandle, 0x21, UVC_SET_CUR,
									   (UVC_VS_PROBE_CONTROL << 8),
									   1,
									   (unsigned char *)&myDeviceInfo, len, 0 );
	std::cerr << "Request to set probe control: " << err << std::endl;

	err = libusb_control_transfer( myHandle, 0x21, UVC_SET_CUR,
								   (UVC_VS_COMMIT_CONTROL << 8),
								   1,
								   (unsigned char *)&myDeviceInfo, len, 0 );
	std::cerr << "Request to commit control: " << err << std::endl;

	std::cerr << "Starting event handling..." << std::endl;
	startEventHandling();
}


////////////////////////////////////////


bool
UVCDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
//	std::cout << "event on endpoint: " << endpoint << ", size " << buflen << std::endl;
	static bool first = true;
	if ( buflen == 5038850 && buf[0] == 2 )
	{
		uint8_t status = buf[1];
		static const char *bitNames[] = { "FID", "EOF", "PTS", "SCR", "RES", "STI", "ERR", "EOH" };
		std::cout << "Status:";
		for ( int b = 0; b < 8; ++b )
		{
			if ( status & (1 << b) )
				std::cout << ' ' << bitNames[b];
		}
		std::cout << std::endl;

		if ( first )
		{
			FILE *xxx = fopen( "foo.pgm", "wb" );
			if ( xxx )
			{
				fwrite( "P5\n2592 1944 255\n", 17, 1, xxx );
				fwrite( buf + 2, 5038848, 1, xxx );
				fclose( xxx );
			}
			first = false;
		}
	}
	return true;
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
	uint8_t epAddr = ( epDesc.bEndpointAddress & ~0x80 );
	if ( epAddr == 2 )
	{
		std::shared_ptr<AsyncTransfer> xfer;
		libusb_transfer_type tt = static_cast<libusb_transfer_type>( epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK );

		if ( tt == LIBUSB_TRANSFER_TYPE_BULK )
		{
//			xfer.reset( new BulkTransfer( 512 ) ); // 0x004e0000
			xfer.reset( new BulkTransfer( 0x004e0000 ) );
			xfer->init( myHandle, epDesc );
			return xfer;
		}
	}

	return Device::createTransfer( epDesc );
}


////////////////////////////////////////


void
UVCDevice::dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iFaceDesc )
{
	if ( iFaceDesc.bInterfaceClass == 0x0e )
	{
		const unsigned char *controlBuf = iFaceDesc.extra;
		int buflen = int(iFaceDesc.extra_length);
		dumpStandardControlsAndFormats( os, controlBuf, buflen );
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
		if ( test_bit( bmControls, i ) == 0 && i != 19 )
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
					case 17: name = "Focus Auto"; break;
					case 18: name = "Privacy Control"; break;
					default:
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
					case 11: name = "Hue Auto"; break;
					case 12: name = "White Balance Temperature Auto"; break;
					case 13: name = "White Balance Component Auto"; break;
					case 14: name = "Digital Multiplier"; break;
					case 15: name = "Digital Multiplier Limit"; break;
					case 16: name = "Analog Video Standard"; break;
					case 17: name = "Analog Lock Status"; break;
					case 19: name = "Pixel Clock"; break;
					default:
						break;
				}
				break;
			default:
				break;
		}
		if ( name )
			os << "      control index: " << i << ' ' << name << std::endl;
	}
}


////////////////////////////////////////


void
UVCDevice::dumpStandardControls( std::ostream &os, const unsigned char *buffer, int buflen )
{
	while ( buflen > 2 )
	{
		if ( buffer[1] != USB_DT_CS_INTERFACE )
		{
			os << "Skipping non DT_CS_INTERFACE info: " << std::hex << buffer[1] << std::dec << std::endl;
			goto next_descriptor;
		}

		uint16_t type;
		uint8_t id;
		unsigned int num_pads;
		unsigned int extra_size;

		int n, p, len;
		switch ( buffer[2] )
		{
			case UVC_VC_HEADER:
				if ( buflen >= 12 )
				{
					n = buffer[11];
					myUVCVersion = get_unaligned_le16( &buffer[3] );
					os << "UVC_VC_HEADER:\n";
					dumpHex( os, "   uvc_version", myUVCVersion );
					os << "   clock freq: " << get_unaligned_le32( &buffer[7] ) << std::endl;
					os << "   interfaces: count " << n;
					for ( int i = 0; i < n; ++i )
						os << ' ' << int(buffer[12+i]);
					os << std::endl;
				}
				break;
			case UVC_VC_INPUT_TERMINAL:
				type = get_unaligned_le16( &buffer[4] );
				if ( type == UVC_ITT_CAMERA )
				{
					n = buflen >= 15 ? buffer[14] : 0;
					len = 15;
				}
				else if ( type == UVC_ITT_MEDIA_TRANSPORT_INPUT )
				{
					n = buflen >= 9 ? buffer[8] : 0;
					p = buflen >= 10 + n ? buffer[9 + n] : 0;
					len = 10;
				}
				outputEntity( os, "VC_INPUT_TERMINAL", type, buffer[3], 1, n + p );
				if ( type == UVC_ITT_CAMERA )
					dumpControls( os, buffer[2], &buffer[15], n );
				else
					dumpControls( os, buffer[2], &buffer[9], n );

				break;
			case UVC_VC_OUTPUT_TERMINAL:
				outputEntity( os, "VC_OUTPUT_TERMINAL", get_unaligned_le16( &buffer[4] ), buffer[3], 1, 0 );
				dumpHex( os, "    baSourceID", buffer[7] );
				if ( buffer[8] != 0 )
				{
					os << "    name: " << int(buffer[8]) << " '" << pullString( buffer[8] ) << "'" << std::endl;
				}
				break;
			case UVC_VC_SELECTOR_UNIT:
				p = buflen >= 5 ? buffer[4] : 0;
				outputEntity( os, "VC_SELECTOR_UNIT", buffer[2], buffer[3], p + 1, 0 );

				switch ( p )
				{
					case 1:
						dumpHex( os, "    baSourceID", buffer[5] );
						break;
					case 2:
						dumpHex( os, "    baSourceID", get_unaligned_le16( &buffer[5] ) );
						break;
					case 4:
						dumpHex( os, "    baSourceID", get_unaligned_le32( &buffer[5] ) );
						break;
					default:
							
						break;
				}

				if ( buffer[5 + p] != 0 )
				{
					os << "    name: " << int(buffer[5+p]) << " '" << pullString( buffer[5+p] ) << "'" << std::endl;
				}
				break;
			case UVC_VC_PROCESSING_UNIT:
				n = buflen >= 8 ? buffer[7] : 0;
				p = myUVCVersion >= 0x0110 ? 10 : 9;

				outputEntity( os, "VC_PROCESSING_UNIT", buffer[2], buffer[3], 2, n );
				dumpHex( os, "    baSourceID", buffer[4] );
				os << "    max multiplier: " << int(get_unaligned_le16(&buffer[5])) << std::endl;
				os << "    control size: " << int(buffer[7]) << std::endl;
				if ( myUVCVersion >= 0x0110 )
					dumpHex( os, "    vid std", buffer[9+n] );
				if ( buffer[8+n] != 0 )
				{
					os << "    name: " << int(buffer[8+n]) << " '" << pullString( buffer[8+n] ) << "'" << std::endl;
				}
				dumpControls( os, buffer[2], &buffer[8], n );
				break;
			case UVC_VC_EXTENSION_UNIT:
				p = buflen >= 22 ? buffer[21] : 0;
				n = buflen >= 24 + p ? buffer[22+p] : 0;
				outputEntity( os, "VC_EXTENSION_UNIT", buffer[2], buffer[3], p + 1, n );
				dumpHex( os, "  guid code", &buffer[4], 16 );
				os << "  num source ids: " << p << std::endl;
				if ( buffer[23+p+n] != 0 )
				{
					os << "    name: " << int(buffer[23+p+n]) << " '" << pullString( buffer[23+p+n] ) << "'" << std::endl;
				}
//					dumpControls( os, buffer[2], &buffer[23+p], n );
				break;
			default:
				dumpHex( os, "unknown UVC tag", buffer[2] );
				break;
		}
	  next_descriptor:
		buflen -= buffer[0];
		buffer += buffer[0];
	}
}

void
UVCDevice::dumpStandardControlsAndFormats( std::ostream &os, const unsigned char *buffer, int buflen )
{
	dumpStandardControls( os, buffer, buflen );
	while ( buflen > 2 && buffer[1] != USB_DT_CS_INTERFACE )
	{
		buflen -= buffer[0];
		buffer += buffer[0];
	}

	if ( buflen <= 2 )
	{
		os << "ERROR, no stream format descriptors?" << std::endl;
		return;
	}
	size_t size = 0;
	switch ( buffer[2] )
	{
		case UVC_VS_OUTPUT_HEADER:
			os << "VIDEO OUTPUT" << std::endl;
			size = 9;
			break;
		case UVC_VS_INPUT_HEADER:
			os << "VIDEO CAPTURE" << std::endl;
			size = 13;
			break;
		default:
			os << "ERROR, invalid header descriptor" << std::endl;
			return;
	}

	int p = buflen >= 4 ? buffer[3] : 0;
	int n = buflen >= size ? buffer[size - 1] : 0;
	if ( buflen < size + p * n )
	{
		os << "ERROR: interface header descriptor is invalid" << std::endl;
		return;
	}

	os << "Video Formats: " << p << std::endl;
	dumpHex( os, "Endpoint Address: ", buffer[6] );
	dumpHex( os, "bmInfo: ", buffer[7] );
	dumpHex( os, "bTerminalLink: ", buffer[8] );
	dumpHex( os, "bStillCaptureMethod: ", buffer[9] );
	dumpHex( os, "bTriggerSupport: ", buffer[10] );
	dumpHex( os, "bTriggerUsage: ", buffer[11] );
	os << "bControlSize: " << n << std::endl;

	buflen -= buffer[0];
	buffer += buffer[0];

	while ( buflen > 2 && buffer[1] == USB_DT_CS_INTERFACE )
	{
		int nFmtInfo = 0;
		switch ( buffer[2] )
		{
			case UVC_VS_FORMAT_UNCOMPRESSED:
				os << "  UNCOMPRESSED: ";
				nFmtInfo = 27;
				break;
			case UVC_VS_FORMAT_MJPEG:
				os << "  MJPEG: ";
				break;
			case UVC_VS_FORMAT_FRAME_BASED:
				os << "  FORMAT FRAME: ";
				nFmtInfo = 28;
				break;
			case UVC_VS_FORMAT_DV:
				os << "  DV: ";
				break;
			case UVC_VS_FORMAT_MPEG2TS:
				os << "  MPEG2TS: ";
				break;
			case UVC_VS_FORMAT_STREAM_BASED:
				os << "  STREAM BASE: ";
				break;
			case UVC_VS_FRAME_UNCOMPRESSED:
				os << "  FRAME UNCOMPRESSED: intervals " << int(buffer[25]);
				break;
			case UVC_VS_FRAME_MJPEG:
				os << "  FRAME MJPEG: intervals " << int(buffer[25]);
				break;
			case UVC_VS_FRAME_FRAME_BASED:
				os << "  FRAME FRAME: intervals " << int(buffer[21]);
				break;
			case UVC_VS_COLORFORMAT:
				dumpHex( os, "  colorspace", buffer[3] );
				break;
			case UVC_VS_STILL_IMAGE_FRAME:
				os << "  STILL IMAGE FRAME INFO" << std::endl;
				break;
			default:
				dumpHex( os, "unknown tag", buffer[2] );
				break;
		}
		if ( nFmtInfo > 0 )
		{
			os << "bpp: " << int(buffer[21]) << std::endl;
			buflen -= buffer[0];
			buffer += buffer[0];
			while ( buflen > 2 && buffer[1] == USB_DT_CS_INTERFACE &&
					buffer[2] == UVC_VS_FRAME_UNCOMPRESSED )
			{
				os << std::endl;
				dumpHex( os, "       frameIndex", buffer[3] );
				dumpHex( os, "       bmCapabilities", buffer[3] );
				os << "       width: " << int(get_unaligned_le16( &buffer[5] ) ) << " * 2: "  << int(get_unaligned_le16( &buffer[5] ))*2 << std::endl;
				os << "       height: " << int(get_unaligned_le16( &buffer[7] ) ) << std::endl;
				os << "       min bitrate: " << get_unaligned_le32( &buffer[9] ) << std::endl;
				os << "       max bitrate: " << get_unaligned_le32( &buffer[13] ) << std::endl;
				os << "       max vid fb size: " << get_unaligned_le32( &buffer[17] ) << std::endl;
				os << "       default frame interval: " << get_unaligned_le32( &buffer[21] ) << std::endl;
				dumpHex( os, "       interval type", buffer[25] );
				buflen -= buffer[0];
				buffer += buffer[0];
			}
		}
		os << std::endl;
		buflen -= buffer[0];
		buffer += buffer[0];
	}
}


////////////////////////////////////////


} // usbpp



