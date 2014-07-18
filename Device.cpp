// Device.cpp -*- C++ -*-

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

#include "Device.h"
#include <iomanip>
#include <locale>
// libstdc++ doesn't have codecvt stuff yet :(
#if defined(__GLIBCXX__) && (__GLIBCXX__ <= 20150101)
# define USE_OLD_UCHAR
#endif

#ifdef USE_OLD_UCHAR
#include <uchar.h>
#include <ctype.h>
#else
#include <cwchar>
#include <codecvt>
#endif
#include <string.h>
#include "Util.h"
#include "Logger.h"
#include <functional>


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


Device::Device( void )
{
}


////////////////////////////////////////


Device::Device( libusb_device *dev )
		: myDevice( dev )
{
	check_error( libusb_get_device_descriptor( myDevice, &myDescriptor ) );
}


////////////////////////////////////////


Device::Device( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: myDevice( dev ), myDescriptor( desc )
{
}


////////////////////////////////////////


Device::~Device( void )
{
}


////////////////////////////////////////


void
Device::setDevice( libusb_device *dev )
{
	closeHandle();
	myInputs.clear();
	myDevice = dev;
	check_error( libusb_get_device_descriptor( myDevice, &myDescriptor ) );
}


////////////////////////////////////////


void
Device::setDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	closeHandle();
	myInputs.clear();
	myDevice = dev;
	myDescriptor = desc;
}


////////////////////////////////////////


void
Device::claimInterfaces( void )
{
	openHandle();
	for ( size_t i = 0, N = myConfigs.size(); i != N; ++i )
	{
		struct libusb_config_descriptor &conf = *(myConfigs[i]);
		for ( size_t iface = 0; iface < conf.bNumInterfaces; ++iface )
		{
			const struct libusb_interface &curIface = conf.interface[iface];
			int nset = curIface.num_altsetting;
			for ( int curset = 0; curset < nset; ++curset )
			{
				const struct libusb_interface_descriptor &iFaceDesc = curIface.altsetting[curset];
				uint8_t inum = iFaceDesc.bInterfaceNumber;
				try
				{
					if ( wantInterface( iFaceDesc ) )
					{
						check_error( libusb_claim_interface( myHandle, inum ) );

						parseInterface( inum, iFaceDesc );
						myClaimedInterfaces.push_back( inum );

						int nep = int(iFaceDesc.bNumEndpoints);
						for ( int ep = 0; ep < nep; ++ep )
						{
							std::shared_ptr<AsyncTransfer> xfer = createTransfer( iFaceDesc.endpoint[ep] );
							if ( xfer )
								myInputs.push_back( xfer );
						}
					}
				}
				catch ( usb_error &e )
				{
					int err = e.getUSBError();
					libusb_error ec = (libusb_error)( err );
					error() << "Unable to claim interface " << int(inum) << ": "
							  << libusb_error_name( err ) << " - "
							  << libusb_strerror( ec ) << send;
				}
				catch ( std::exception &e )
				{
					error() << "Unable to claim interface " << int(inum) << ": " << e.what() << send;
				}
				catch ( ... )
				{
					error() << "Unable to claim interface " << int(inum) << send;
				}
			}
		}
	}
}


////////////////////////////////////////


void
Device::startEventHandling( void )
{
	for ( std::shared_ptr<AsyncTransfer> &xfer: myInputs )
	{
		xfer->setCallback( std::bind( &Device::dispatchEvent, this, std::placeholders::_1 ) );
		xfer->submit();
	}
}


////////////////////////////////////////


void
Device::stopEventHandling( void )
{
	for ( std::shared_ptr<AsyncTransfer> &xfer: myInputs )
	{
		xfer->cancel();
		xfer->wait();
	}
}


////////////////////////////////////////


void
Device::shutdown( void )
{
	closeHandle();
	myInputs.clear();
}


////////////////////////////////////////


void
Device::setLanguageID( uint16_t id )
{
	if ( id == 0 )
	{
		unsigned char tmpbuf[256];
		openHandle();
		int r = libusb_get_string_descriptor( myHandle, 0, 0, tmpbuf, 256 );
		if ( r < 0 )
		{
			error() << "Retrieving language IDs: " << libusb_strerror( (enum libusb_error)(r) ) << send;
			myLangID = 0x0409;
			return;
		}
		myLangID = uint16_t(tmpbuf[2]) | (uint16_t(tmpbuf[3]) << 8);
	}
	else
		myLangID = id;
}


////////////////////////////////////////


void
Device::dumpInfo( std::ostream &os )
{
	openHandle();

	if ( ! myDevice )
		return;

	if ( myLangID == 0 )
		setLanguageID();

	dumpHex( os, "           Vendor ID", myDescriptor.idVendor );
	dumpHex( os, "          Product ID", myDescriptor.idProduct );
	os <<        "        Manufacturer: '" << myManufacturer << "'\n";
	os <<        "             Product: '" << myProduct << "'\n";
	os <<        "       Serial Number: '" << mySerialNumber << "'\n";
	dumpHex( os, "      Release Number", myDescriptor.bcdDevice );
	dumpHex( os, "            USB spec", myDescriptor.bcdUSB );
	dumpHex( os, "        Device Class", myDescriptor.bDeviceClass );
	dumpHex( os, "     Device SubClass", myDescriptor.bDeviceSubClass );
	dumpHex( os, "     Device Protocol", myDescriptor.bDeviceProtocol );
	os <<        "Max Packet Size[ep0]: " << int(myDescriptor.bMaxPacketSize0) << '\n';
	dumpHex( os, "    Current Language", myLangID );
	os <<        "      Configurations: " << int(myDescriptor.bNumConfigurations) << '\n';
	if ( myBOS )
	{
		dumpHex( os, "      BOS Descr Type", myBOS->bDescriptorType );
		dumpHex( os, "        Num Dev Caps", myBOS->bNumDeviceCaps, true );
		for ( uint8_t i = 0; i < myBOS->bNumDeviceCaps; ++i )
		{
			libusb_bos_dev_capability_descriptor *curDesc = myBOS->dev_capability[i];
			os << "          Dev Cap " << int(i) << ": ";
			dumpHex( os, " desc type", curDesc->bDescriptorType, false, false );
			dumpHex( os, " dev type", curDesc->bDevCapabilityType, false, false );
			size_t n = curDesc->bLength - LIBUSB_DT_DEVICE_CAPABILITY_SIZE;
			dumpHexRaw( os, " data", curDesc->dev_capability_data, n );
		}
	}

	if ( myUSB2Ext )
		dumpHex( os, "      USB 2.0 Attribs", myUSB2Ext->bmAttributes );

	if ( mySSDesc )
	{
		dumpHex( os, "      SS Attribs", mySSDesc->bmAttributes );
		dumpHex( os, "      SpeedSupported", mySSDesc->wSpeedSupported, true );
		if ( ( mySSDesc->wSpeedSupported & LIBUSB_LOW_SPEED_OPERATION ) != 0 )
			os << "        LOW_SPEED" << std::endl;
		if ( ( mySSDesc->wSpeedSupported & LIBUSB_FULL_SPEED_OPERATION ) != 0 )
			os << "        FULL_SPEED" << std::endl;
		if ( ( mySSDesc->wSpeedSupported & LIBUSB_HIGH_SPEED_OPERATION ) != 0 )
			os << "        HIGH_SPEED" << std::endl;
		if ( ( mySSDesc->wSpeedSupported & LIBUSB_SUPER_SPEED_OPERATION ) != 0 )
			os << "        SUPER_SPEED" << std::endl;

		dumpHex( os, "      FunctionalitySupport", mySSDesc->bFunctionalitySupport );
		dumpHex( os, "      U1 Device Exit Latency", mySSDesc->bU1DevExitLat );
		dumpHex( os, "      U2 Device Exit Latency", mySSDesc->bU2DevExitLat );
	}

	mySpeed = libusb_get_device_speed( myDevice );
	switch ( mySpeed )
	{
		case LIBUSB_SPEED_LOW:
			os << "    *** DEVICE OPERATING AT LOW SPEED (1.5MBit/s) **" << std::endl;
			break;
		case LIBUSB_SPEED_FULL:
			os << "    *** DEVICE OPERATING AT FULL SPEED (12MBit/s) **" << std::endl;
			break;
		case LIBUSB_SPEED_HIGH:
			os << "    *** DEVICE OPERATING AT HIGH SPEED (480MBits/s) **" << std::endl;
			break;
		case LIBUSB_SPEED_SUPER:
			os << "    *** DEVICE OPERATING AT SUPER SPEED (5000MBit/s) **" << std::endl;
			break;
		case LIBUSB_SPEED_UNKNOWN:
		default:
			os << "    <- UNKNOWN DEVICE SPEED -> "  << mySpeed << std::endl;
			break;
	}

	for ( size_t i = 0, N = myConfigs.size(); i != N; ++i )
	{
		struct libusb_config_descriptor &conf = *(myConfigs[i]);
		os << "Config " << i << ":\n";
		os << "  Value: " << std::setw( 2 ) << std::setfill( '0' ) << std::hex << int(conf.bConfigurationValue) << std::dec << '\n';
		os << "  Name: '" << pullString(conf.iConfiguration) << "'\n";
		os << "  Config Characteristics: " << std::setw( 2 ) << std::setfill( '0' ) << std::hex << int(conf.bmAttributes) << std::dec << '\n';
		os << "  Max Power: " << int(conf.MaxPower)*2 << "mA\n";
		os << "  Extra Descriptors Len: " << conf.extra_length << '\n';
		os << "  Interfaces: " << int(conf.bNumInterfaces) << '\n';
		for ( size_t iface = 0; iface < conf.bNumInterfaces; ++iface )
		{
			const struct libusb_interface &curIface = conf.interface[iface];
			int nset = curIface.num_altsetting;
			os << "    Interface " << iface << ": " << nset << " settings\n";
			for ( int curset = 0; curset < nset; ++curset )
			{
				const struct libusb_interface_descriptor &iFaceDesc = curIface.altsetting[curset];
				os << "      Setting " << curset << ":\n";
				os << "        Interface Number: " << int(iFaceDesc.bInterfaceNumber) << '\n';
				os << "        Alternate Setting: " << int(iFaceDesc.bAlternateSetting) << '\n';
				dumpHex( os, "        Interface Class", iFaceDesc.bInterfaceClass );
				dumpHex( os, "        Interface SubClass", iFaceDesc.bInterfaceSubClass );
				dumpHex( os, "        Interface Protocol", iFaceDesc.bInterfaceProtocol );
				os << "        Interface Description: '" << pullString( iFaceDesc.iInterface ) << "'\n";
				os << "        Interface Extra Len: " << int(iFaceDesc.extra_length) << '\n';
				os << "        Num Endpoints: " << int(iFaceDesc.bNumEndpoints) << '\n';

				for ( int ep = 0, nep = int(iFaceDesc.bNumEndpoints); ep < nep; ++ep )
				{
					const struct libusb_endpoint_descriptor &epDesc = iFaceDesc.endpoint[ep];
					os << "          Endpoint " << ep << ":\n";
					os << "            Address: " << std::setw( 2 ) << std::setfill( '0' ) << std::hex << int(epDesc.bEndpointAddress) << std::dec;
					bool isOutput = ( epDesc.bEndpointAddress & 0x80 ) == LIBUSB_ENDPOINT_OUT;
					int endPointNum = int(epDesc.bEndpointAddress & 0xF);

					os << " " << (isOutput ? "OUTPUT" : "INPUT") << " number " << endPointNum << '\n';
					os << "            Attributes: " << std::setw( 2 ) << std::setfill( '0' ) << std::hex << int(epDesc.bmAttributes) << std::dec << '\n';
						
					os << "            Max Packet Size: " << int(epDesc.wMaxPacketSize) << '\n';
					os << "            Interval: " << int(epDesc.bInterval) << '\n';
					os << "            Extra Len: " << int(epDesc.extra_length) << '\n';
					auto x = mySSEndpointCompanion.find( epDesc.bEndpointAddress );
					if ( x != mySSEndpointCompanion.end() )
					{
						libusb_ss_endpoint_companion_descriptor &ssec = *(x->second);
						dumpHex( os, "            SS Len", ssec.bLength, true );
						dumpHex( os, "            SS MaxBurst", ssec.bMaxBurst, true );
						dumpHex( os, "            SS Attribs", ssec.bmAttributes, true );
						dumpHex( os, "            SS BytesPerInterval", ssec.wBytesPerInterval, true );
					}
				}

				os << "        --- EXTRA INTERFACE INFO ---\n";
				dumpExtraInterfaceInfo( os, iFaceDesc );
				os << '\n';
			}
		}
	}
	os << std::endl;
}


////////////////////////////////////////


std::shared_ptr<AsyncTransfer>
Device::createTransfer( const struct libusb_endpoint_descriptor &epDesc )
{
	std::shared_ptr<AsyncTransfer> xfer;

	bool isInput = ( epDesc.bEndpointAddress & 0x80 ) == LIBUSB_ENDPOINT_IN;
	if ( ! isInput )
		return xfer;

	libusb_transfer_type tt = static_cast<libusb_transfer_type>( epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK );
	switch ( tt )
	{
		case LIBUSB_TRANSFER_TYPE_CONTROL:
			error() << "Subclass must implement control transfers" << send;
			break;

		case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
			error() << "Subclass must implement isochronous transfers" << send;
			break;

		case LIBUSB_TRANSFER_TYPE_BULK:
			error() << "Subclass must implement bulk transfers" << send;
			break;

		case LIBUSB_TRANSFER_TYPE_INTERRUPT:
			xfer.reset( new InterruptTransfer( myContext, epDesc.wMaxPacketSize ) );
			break;

		default:
			error() << "Unknown transfer type " << int(tt) << send;
			throw std::logic_error( "Unknown transfer type encounterd" );
	}

	if ( xfer )
		xfer->init( myHandle, epDesc.bEndpointAddress );

	return xfer;
}


////////////////////////////////////////


void
Device::parseInterface( uint8_t inum, const struct libusb_interface_descriptor &ifacedesc )
{
}


////////////////////////////////////////


void
Device::dumpExtraInterfaceInfo( std::ostream &os, const struct libusb_interface_descriptor &iface )
{
}


////////////////////////////////////////


void
Device::dispatchEvent( libusb_transfer *xfer )
{
	int endpoint = xfer->endpoint & ~(LIBUSB_ENDPOINT_IN);

	uint8_t *buf = xfer->buffer;
	int buflen = xfer->actual_length;

	if ( handleEvent( endpoint, buf, buflen, xfer ) )
	{
		AsyncTransfer *t = reinterpret_cast<AsyncTransfer *>( xfer->user_data );
		if ( t )
			t->submit();
	}
}


////////////////////////////////////////


void
Device::openHandle( void )
{
	if ( myHandle )
		return;

	check_error( libusb_open( myDevice, &myHandle ) );
	myInputs.clear();

	libusb_set_auto_detach_kernel_driver( myHandle, 1 );
	myManufacturer = pullString( myDescriptor.iManufacturer );
	myProduct = pullString( myDescriptor.iProduct );
	mySerialNumber = pullString( myDescriptor.iSerialNumber );

	for ( uint8_t c = 0; c < myDescriptor.bNumConfigurations; ++c )
	{
		libusb_config_descriptor *config = nullptr;
		check_error( libusb_get_config_descriptor( myDevice, c, &config ) );
		myConfigs.push_back( config );
		if ( ! myContext )
			continue;

		for ( size_t iface = 0; iface < config->bNumInterfaces; ++iface )
		{
			const struct libusb_interface &curIface = config->interface[iface];
			int nset = curIface.num_altsetting;
			for ( int curset = 0; curset < nset; ++curset )
			{
				const struct libusb_interface_descriptor &iFaceDesc = curIface.altsetting[curset];
				for ( int ep = 0, nep = int(iFaceDesc.bNumEndpoints); ep < nep; ++ep )
				{
					const struct libusb_endpoint_descriptor &epDesc = iFaceDesc.endpoint[ep];
					libusb_ss_endpoint_companion_descriptor *curSSEP = nullptr;
					int err = libusb_get_ss_endpoint_companion_descriptor(
						myContext, &epDesc, &curSSEP );
					if ( err == LIBUSB_SUCCESS && curSSEP )
						mySSEndpointCompanion[epDesc.bEndpointAddress] = curSSEP;
					else if ( err != LIBUSB_ERROR_NOT_FOUND )
						check_error( err );
				}
			}
		}
	}

	if ( myDescriptor.bNumConfigurations > 0 )
	{
		int curConfig = 0;
		check_error( libusb_get_configuration( myHandle, &curConfig ) );
		if ( curConfig == 0 )
		{
			myCurConfig = 0;
			check_error( libusb_set_configuration( myHandle, int(myConfigs[myCurConfig]->bConfigurationValue) ) );
		}
	}

	libusb_get_bos_descriptor( myHandle, &myBOS );

	if ( myBOS && myContext )
	{
		for ( uint8_t i = 0; i < myBOS->bNumDeviceCaps; ++i )
		{
			libusb_bos_dev_capability_descriptor *curDesc = myBOS->dev_capability[i];
			switch ( curDesc->bDevCapabilityType )
			{
				case LIBUSB_BT_USB_2_0_EXTENSION:
					check_error( libusb_get_usb_2_0_extension_descriptor( myContext,
																		  curDesc,
																		  &myUSB2Ext ) );
					break;
				case LIBUSB_BT_SS_USB_DEVICE_CAPABILITY:
					check_error( libusb_get_ss_usb_device_capability_descriptor(
									 myContext, curDesc, &mySSDesc ) );
					break;
				default:
					
					break;
			}
		}
	}
}


////////////////////////////////////////


void
Device::closeHandle( void )
{
	if ( ! myHandle )
		return;

	for ( std::shared_ptr<AsyncTransfer> &i: myInputs )
	{
		i->cancel();
		i->wait();
	}
	myInputs.clear();

	for ( auto &i: mySSEndpointCompanion )
		libusb_free_ss_endpoint_companion_descriptor( i.second );
	mySSEndpointCompanion.clear();

	if ( myUSB2Ext )
	{
		libusb_free_usb_2_0_extension_descriptor( myUSB2Ext );
		myUSB2Ext = nullptr;
	}
	if ( mySSDesc )
	{
		libusb_free_ss_usb_device_capability_descriptor( mySSDesc );
		mySSDesc = nullptr;
	}
	if ( myBOS )
	{
		libusb_free_bos_descriptor( myBOS );
		myBOS = nullptr;
	}

	for ( size_t i = 0; i < myClaimedInterfaces.size(); ++i )
		libusb_release_interface( myHandle, myClaimedInterfaces[i] );
	myClaimedInterfaces.clear();

	for ( size_t i = 0; i < myConfigs.size(); ++i )
		libusb_free_config_descriptor( myConfigs[i] );
	myConfigs.clear();

	libusb_close( myHandle );
	myHandle = nullptr;
}


////////////////////////////////////////


std::string
Device::pullString( uint8_t desc_idx )
{
	if ( ! myHandle || desc_idx == 0 )
		return std::string();


	if ( myLangID == 0 )
		setLanguageID();

	unsigned char tmpbuf[256];
	int len = libusb_get_string_descriptor( myHandle, desc_idx, myLangID, tmpbuf, 256 );
	if ( len < 0 )
	{
		error() << "Retrieving string " << int(desc_idx) << ": " << libusb_strerror( (enum libusb_error)(len) ) << send;
		return std::string();
	}

	if ( tmpbuf[1] != LIBUSB_DT_STRING )
	{
		// hrm, no string???
		return std::string();
	}

	if ( tmpbuf[0] > len )
	{
		error() << "Retrieving string " << int(desc_idx) << ": returned size mismatch" << send;
		return std::string();
	}

	// blah, strings are in u16 unicode, convert them to someting
	// more useable
#ifdef USE_OLD_UCHAR
	char xxx[512];
	const char16_t *uCode = reinterpret_cast<const char16_t *>( tmpbuf );
	std::mbstate_t ps;
	char *curX = xxx;
	memset( &ps, 0, sizeof(ps) );
	size_t xSz = 0;
	len = tmpbuf[0];
	for ( int i = 1, n16 = len / 2; i < n16; ++i )
	{
		size_t n = c16rtomb( curX, uCode[i], &ps );
		if ( n == std::string::npos )
		{
			// hrm, bad character?
			n = 1;
			curX[0] = 'x';
		}
		xSz += n;
		curX += n;
	}
	std::string retval;
	for ( size_t i = 0; i < xSz; ++i )
	{
		if ( isprint( xxx[i] ) )
			retval.push_back( static_cast<char>( xxx[i] ) );
	}

	return retval;
#else
	std::wstring_convert< std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>, wchar_t> conv;
	std::wstring ws = conv.from_bytes(
								 reinterpret_cast<const char*> ( tmpbuf + 2 ),
								 reinterpret_cast<const char*> ( tmpbuf + len ) );
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv2;
	return std::string( conv2.to_bytes( ws ) );
#endif
}


////////////////////////////////////////


} // USB



