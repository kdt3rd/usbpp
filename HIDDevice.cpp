// HIDDevice.cpp -*- C++ -*-

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

#include "HIDDevice.h"
#include "Util.h"
#include "Logger.h"
#include <unistd.h>


////////////////////////////////////////


namespace
{

static const char *
globalTagName( uint8_t gTag )
{
	switch ( (gTag >> 4) )
	{
		case 0x0: return "Usage Page";
		case 0x1: return "Logical Minimum";
		case 0x2: return "Logical Maximum";
		case 0x3: return "Physical Minimum";
		case 0x4: return "Physical Maximum";
		case 0x5: return "Unit Exponent";
		case 0x6: return "Unit";
		case 0x7: return "Report Size";
		case 0x8: return "Report ID";
		case 0x9: return "Report Count";
		case 0xa: return "Push";
		case 0xb: return "Pop";
	}
	return "Reserved";
}

static const char *
localTagName( uint8_t lTag )
{
	switch ( (lTag >> 4) )
	{
		case 0x0: return "Usage";
		case 0x1: return "Usage Minimum";
		case 0x2: return "Usage Maximum";
		case 0x3: return "Designator Index";
		case 0x4: return "Designator Minimum";
		case 0x5: return "Designator Maximum";
		case 0x7: return "String Index";
		case 0x8: return "String Minimum";
		case 0x9: return "String Maximum";
		case 0xa: return "Report Count";
	}
	return "Reserved";
}

static void
extractAndReportValue( std::ostream &os, const char *prefix, const uint8_t *data, uint8_t len )
{
	switch ( len )
	{
		case 0: os << prefix << " (no data)" << std::endl; break;
		case 1: USB::dumpHex( os, prefix, data[0], true ); break;
		case 2: USB::dumpHex( os, prefix, *( reinterpret_cast<const uint16_t *>( data ) ), true ); break;
		case 4: USB::dumpHex( os, prefix, *( reinterpret_cast<const uint32_t *>( data ) ), true ); break;
		default:
			USB::dumpHexRaw( os, prefix, data, len );
			break;
	}
}

} // empty namespace


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


HIDDevice::HIDDevice( void )
{
}


////////////////////////////////////////


HIDDevice::HIDDevice( libusb_device *dev )
		: Device( dev )
{
}


////////////////////////////////////////


HIDDevice::HIDDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: Device( dev, desc )
{
}


////////////////////////////////////////


HIDDevice::~HIDDevice( void )
{
}


////////////////////////////////////////


std::shared_ptr<Device>
HIDDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::make_shared<HIDDevice>( dev, desc );
}


////////////////////////////////////////


bool
HIDDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
	std::cout << "HIDDevice::handleEvent( " << endpoint << ", " << buflen << " )" << std::endl;
	dumpHexBuf( std::cout, " -> ", buf, buflen, false );
	return true;
}


////////////////////////////////////////


bool
HIDDevice::wantInterface( const struct libusb_interface_descriptor &iface )
{
	return true;
}


////////////////////////////////////////


void
HIDDevice::parseInterface( uint8_t inum, const struct libusb_interface_descriptor &iFaceDesc )
{
	if ( iFaceDesc.extra_length < static_cast<int>( sizeof(HID_info) ) )
		return;

	const HID_info *hidInfo = reinterpret_cast<const HID_info *>( iFaceDesc.extra );
	for ( uint8_t i = 0; i < hidInfo->bNumDescriptors; ++i )
	{
		const Descriptor &desc = hidInfo->bDescriptors[i];

		size_t getBytes = 0;

		switch ( desc.bDescriptorType )
		{
			case LIBUSB_DT_DEVICE:
			case LIBUSB_DT_CONFIG:
			case LIBUSB_DT_STRING:
			case LIBUSB_DT_INTERFACE:
			case LIBUSB_DT_ENDPOINT:
			case LIBUSB_DT_BOS:
			case LIBUSB_DT_DEVICE_CAPABILITY:
			case LIBUSB_DT_HID:
			case LIBUSB_DT_PHYSICAL:
			case LIBUSB_DT_HUB:
			case LIBUSB_DT_SUPERSPEED_HUB:
			case LIBUSB_DT_SS_ENDPOINT_COMPANION:
				break;
			case LIBUSB_DT_REPORT:
				getBytes = desc.wDescriptorLength;
				break;
			default:
				std::cerr << "WARNING: Unknown descriptor type" << std::endl;
				break;
		}

		if ( getBytes )
		{
			std::vector<uint8_t> descriptorInfo( getBytes + 1 );
			uint8_t reqType = endpoint_in( iFaceDesc.bInterfaceNumber ) | LIBUSB_RECIPIENT_INTERFACE;
			int err = libusb_control_transfer(
				myHandle, reqType, LIBUSB_REQUEST_GET_DESCRIPTOR,
				(desc.bDescriptorType << 8) | iFaceDesc.bInterfaceNumber,
				0, (unsigned char *)descriptorInfo.data(), getBytes, 1000 );
			if ( err == static_cast<int>( getBytes ) )
			{
				if ( LIBUSB_DT_REPORT == desc.bDescriptorType )
					parseReport( descriptorInfo.data(), getBytes );
				else
					throw std::logic_error( "Unknown descriptor type being retrieved" );
			}
			else if ( err != LIBUSB_ERROR_IO )
			{
				error() << "Unable to get descriptor for " << int(i) << ": err " << err << " getBytes " << getBytes << send;
				throw std::runtime_error( "Unable to extract HID descriptor" );
			}
		}
	}
}


////////////////////////////////////////


void
HIDDevice::dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iFaceDesc )
{
	if ( iFaceDesc.extra_length < static_cast<int>( sizeof(HID_info) ) )
	{
		os << "Invalid HID interface descriptor" << std::endl;
		return;
	}

	const HID_info *hidInfo = reinterpret_cast<const HID_info *>( iFaceDesc.extra );
	dumpHex( os, "            HID Length", hidInfo->bLength, true );
	dumpHex( os, "            HID Descriptor Type", hidInfo->bDescriptorType );
	dumpHex( os, "            HID Class Spec", hidInfo->bcdHID );
	dumpHex( os, "            HID Country Code", hidInfo->bCountryCode );
	dumpHex( os, "            HID Num Descriptors", hidInfo->bNumDescriptors, true );
	for ( uint8_t i = 0; i < hidInfo->bNumDescriptors; ++i )
	{
		os << "              Descriptor " << int(i) << '\n';
		const Descriptor &desc = hidInfo->bDescriptors[i];
		dumpHex( os, "                Type", desc.bDescriptorType );
		dumpHex( os, "                Length", desc.wDescriptorLength, true );

		size_t getBytes = 0;

		switch ( desc.bDescriptorType )
		{
			case LIBUSB_DT_DEVICE: os << "                  {Device Descriptor}\n"; break;
			case LIBUSB_DT_CONFIG: os << "                  {Config Descriptor}\n"; break;
			case LIBUSB_DT_STRING:
				os << "                  {String Descriptor}\n";
				getBytes = desc.wDescriptorLength;
				break;
			case LIBUSB_DT_INTERFACE: os << "                  {Interface Descriptor}\n"; break;
			case LIBUSB_DT_ENDPOINT: os << "                  {Endpoint Descriptor}\n"; break;
			case LIBUSB_DT_BOS: os << "                  {BOS Descriptor}\n"; break;
			case LIBUSB_DT_DEVICE_CAPABILITY: os << "                  {Device Capability Descriptor}\n"; break;
			case LIBUSB_DT_HID: os << "                  {HID Descriptor}\n"; break;
			case LIBUSB_DT_REPORT:
				os << "                  {Report Descriptor}\n";
				getBytes = desc.wDescriptorLength;
				break;
			case LIBUSB_DT_PHYSICAL: os << "                  {Physical Descriptor}\n"; break;
			case LIBUSB_DT_HUB: os << "                  {Hub Descriptor}\n"; break;
			case LIBUSB_DT_SUPERSPEED_HUB: os << "                  {SuperSpeed Hub Descriptor}\n"; break;
			case LIBUSB_DT_SS_ENDPOINT_COMPANION: os << "                  {SS Endpoint Companion}\n"; break;
			default: os << "                  {UNKNOWN DESCRIPTOR}\n"; break;
		}

		if ( getBytes )
		{
			std::vector<uint8_t> descriptorInfo( getBytes + 1 );
			uint8_t reqType = endpoint_in( iFaceDesc.bInterfaceNumber ) | LIBUSB_RECIPIENT_INTERFACE;
			int err = libusb_control_transfer(
				myHandle, reqType, LIBUSB_REQUEST_GET_DESCRIPTOR,
				(desc.bDescriptorType << 8) | iFaceDesc.bInterfaceNumber,
				0, (unsigned char *)descriptorInfo.data(), getBytes, 1000 );

			if ( err == static_cast<int>( getBytes ) )
			{
				if ( LIBUSB_DT_STRING == desc.bDescriptorType )
				{
					descriptorInfo[getBytes] = '\0';
					os << "                  \"" << reinterpret_cast<const char *>( descriptorInfo.data() ) << "\"\n";
				}
				else if ( LIBUSB_DT_REPORT == desc.bDescriptorType )
				{
					dumpReport( os, descriptorInfo.data(), getBytes );
				}
			}
			else
				throw std::runtime_error( "Unable to extract HID descriptor" );
		}
	}
}


////////////////////////////////////////


void
HIDDevice::parseReport( const uint8_t *reportDesc, size_t rSize )
{
}


////////////////////////////////////////


void
HIDDevice::dumpReport( std::ostream &os, const uint8_t *reportDesc, size_t rSize )
{
	size_t i = 0;

	while ( i < rSize )
	{
		uint8_t key = reportDesc[i];
		uint8_t bSize = key & 0x03;
		uint8_t bType = key & 0x0c;
		uint8_t bTag = key & 0xf0;

		uint8_t key_size = 1;
		uint8_t data_len = 0;

		if ( (key & 0xf0) == 0xf0 )
		{
			// long item, length is next item
			if ( (i + 2) < rSize )
				data_len = reportDesc[i + 1];
			else
				throw std::runtime_error( "Invalid long item in report descriptor" );

			key_size = 3;
			bTag = reportDesc[i + 2];
		}
		else
		{
			switch ( bSize )
			{
				case 0:
				case 1:
				case 2:
					data_len = bSize;
					break;
				case 3:
					data_len = 4;
					break;
				default:
					break;
			}
		}

		switch ( bType )
		{
			case 0x0: // main
				switch ( bTag )
				{
					case 0x80: // input
						extractAndReportValue( os, "                  [MAIN] Input", reportDesc + i + key_size, data_len );
						break;
					case 0x90: // output
						extractAndReportValue( os, "                  [MAIN] Output", reportDesc + i + key_size, data_len );
						break;
					case 0xa0: // collection
						extractAndReportValue( os, "                  [MAIN] Collection", reportDesc + i + key_size, data_len );
						break;
					case 0xb0: // feature
						extractAndReportValue( os, "                  [MAIN] Feature", reportDesc + i + key_size, data_len );
						break;
					case 0xc0: // end collection
						break;
					default:
						dumpHex( os, "                  [MAIN] tag", bTag, true );
						dumpHex( os, "                  [MAIN] type", bType, true );
						dumpHex( os, "                  [MAIN] len", data_len, true );
						break;
				}
				break;
			case 0x4: // global
				dumpHexNamed( os, "                  [Global] Tag", bTag, true, false, globalTagName( bTag ) );
				extractAndReportValue( os, " value", reportDesc + i + key_size, data_len );
				break;
			case 0x8: // local
				dumpHexNamed( os, "                  [Local] Tag", bTag, true, false, localTagName( bTag ) );
				extractAndReportValue( os, " value", reportDesc + i + key_size, data_len );
				break;
			case 0xc: // reserved
				dumpHex( os, "                  [RESERVED] Tag", bTag, true );
				break;
			default:
				dumpHex( os, "                  [UNKNOWN TYPE] Type", bType, true );
				dumpHex( os, "                  [UNKNOWN TYPE] Tag", bTag, true );
				break;
		}
		i += data_len + key_size;
	}
}


////////////////////////////////////////


} // usbpp



