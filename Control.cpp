// Control.cpp -*- C++ -*-

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

#include "Control.h"
#include "Device.h"
#include <algorithm>
#include <stdexcept>
#include "uvc_constants.h"
#include "Util.h"
#include <iomanip>


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


Control::Control( void )
{
}


////////////////////////////////////////


Control::~Control( void )
{
	std::unique_lock<std::mutex> lk( myMutex );
	for ( size_t x = 0; x != myActiveTransfers.size(); ++x )
	{
		auto &xfer = myActiveTransfers[x];
		xfer->cancel();
		xfer->wait();
	}
	myActiveTransfers.clear();
}


////////////////////////////////////////


template <typename T>
int
Control::doGet( uint8_t request, T *buf, uint16_t N )
{
	uint16_t value = myUnit << 8;
	uint8_t reqType = Device::endpoint_in( myEndpoint ) | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
	uint16_t idx = (myTerminal << 8) | myInterface;

	return libusb_control_transfer( myHandle, reqType, request, value, idx,
									reinterpret_cast<unsigned char *>( buf ),
									N, 1000 );
}


////////////////////////////////////////


void
Control::init( std::string name, libusb_device_handle *handle, uint8_t endpointNum, uint8_t unit, uint8_t iface, uint16_t term, libusb_context *ctxt )
{
	std::swap( myName, name );
	myContext = ctxt;
	myHandle = handle;
	myEndpoint = endpointNum;
	myUnit = unit;
	myInterface = iface;
	myTerminal = term;
	myReadOnly = true;
	myMin = 0;
	myMax = 0;

	int e = doGet( UVC_GET_LEN, &myLength, sizeof(myLength) );
	if ( e == sizeof(myLength) )
	{
		e = doGet( UVC_GET_CUR, myRawData, myLength );
		if ( e != myLength )
		{
			std::cerr << "Unable to get current value per length, received " << e << " vs request for " << myLength << std::endl;
			if ( e >= 0 )
			{
				myLength = e;
				e = doGet( UVC_GET_CUR, myRawData, myLength );
				if ( e != myLength )
				{
					std::cerr << "Request for other length fails as well, setting to be invalid" << std::endl;
					myLength = 0;
				}
			}
		}
		switch ( myLength )
		{
			case 0:
				break;

			case 1:
			{
				uint8_t mnV, mxV;
				int mne = doGet( UVC_GET_MIN, &mnV, 1 );
				int mxe = doGet( UVC_GET_MAX, &mxV, 1 );
				e = doGet( UVC_GET_CUR, myRawData, 1 );
				if ( mne == mxe && mne == myLength )
				{
					myMin = mnV;
					myMax = mxV;
					myReadOnly = false;
				}
				break;
			}
			case 2:
			{
				uint16_t mnV, mxV;
				int mne = doGet( UVC_GET_MIN, &mnV, 2 );
				int mxe = doGet( UVC_GET_MAX, &mxV, 2 );
				e = doGet( UVC_GET_CUR, myRawData, 2 );
				if ( mne == mxe && mne == myLength )
				{
					myMin = mnV;
					myMax = mxV;
					myReadOnly = false;
				}
				break;
			}
			case 4:
			{
				uint32_t mnV, mxV;
				int mne = doGet( UVC_GET_MIN, &mnV, 4 );
				int mxe = doGet( UVC_GET_MAX, &mxV, 4 );
				if ( mne == mxe && mne == myLength )
				{
					myMin = mnV;
					myMax = mxV;
					myReadOnly = false;
				}
				break;
			}
			default:
				// no ranges that we know of, it's just read-only to us
				break;
		}
	}
	else
	{
		myLength = 0;
	}
}


////////////////////////////////////////


void
Control::get( uint8_t *buf, uint8_t N ) const
{
	if ( ! valid() )
		throw std::runtime_error( "Attempt to query value for invalid control " + myName );

	std::copy( myRawData, myRawData + N, buf );
}


////////////////////////////////////////


uint32_t
Control::get( void ) const
{
	if ( ! valid() )
		throw std::runtime_error( "Attempt to query value for invalid control " + myName );

	if ( myLength == 1 )
		return static_cast<uint32_t>( myRawData[0] );

	if ( myLength == 2 )
		return static_cast<uint32_t>( *(reinterpret_cast<const uint16_t *>(myRawData)) );

	if ( myLength == 4 )
		return static_cast<uint32_t>( *(reinterpret_cast<const uint32_t *>(myRawData)) );

	throw std::runtime_error( "Attempt to query value using convenience routine on wrong sized-control" );
}


////////////////////////////////////////


uint32_t
Control::set( uint32_t val )
{
	if ( read_only() )
		return get();

	val = std::max( std::min( val, max() ), min() );

	if ( val == get() )
		return val;

	switch ( myLength )
	{
		case 1: myRawData[0] = static_cast<uint8_t>( val ); break;
		case 2: *(reinterpret_cast<uint16_t *>( myRawData )) = static_cast<uint16_t>( val ); break;
		case 4: *(reinterpret_cast<uint32_t *>( myRawData )) = val; break;
		default:
			break;
	}

	std::shared_ptr<ControlTransfer> ctrl = std::make_shared<ControlTransfer>( myContext );
	ctrl->fill( myHandle,
				Device::endpoint_out( myEndpoint ) | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, // bmRequestType
				UVC_SET_CUR, // bRequest
				myUnit << 8, // wValue
				(myTerminal << 8) | myInterface, // wIndex
				myLength, // wLength
				myRawData );
	ctrl->submit();

	std::unique_lock<std::mutex> lk( myMutex );
	for ( size_t x = 0; x != myActiveTransfers.size(); ++x )
	{
		if ( myActiveTransfers[x]->isComplete() )
		{
			myActiveTransfers.erase( myActiveTransfers.begin() + x );
			--x;
		}
	}
	myActiveTransfers.push_back( ctrl );

	return get();
}


////////////////////////////////////////


uint32_t
Control::delta( int d )
{
	if ( read_only() )
		return get();

	uint32_t curV = get();
	int64_t nVal = static_cast<int64_t>( curV ) + d;
	nVal = std::min( nVal, static_cast<int64_t>( myMax ) );
	nVal = std::max( nVal, static_cast<int64_t>( myMin ) );
	return set( static_cast<uint32_t>( nVal ) );
}


////////////////////////////////////////


void
Control::coalesce( void )
{
	std::unique_lock<std::mutex> lk( myMutex );
	for ( size_t x = 0; x != myActiveTransfers.size(); ++x )
		myActiveTransfers[x]->wait();
	myActiveTransfers.clear();
}


////////////////////////////////////////


uint32_t
Control::update( void )
{
	if ( read_only() )
		return get();

	ControlTransfer ctrl( myContext );
	ctrl.fill( myHandle,
			   Device::endpoint_in( myEndpoint ) | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, // bmRequestType
			   UVC_GET_CUR, // bRequest
			   myUnit << 8, // wValue
			   (myTerminal << 8) | myInterface, // wIndex
			   myLength, // wLength
			   myRawData );
	ctrl.submitAndWait();
	return get();
}


////////////////////////////////////////


void
Control::print( std::ostream &os ) const
{
	os << "Control '" << name() << "': unit 0x" << std::setfill('0') << std::setw(2) << std::hex << int(myUnit) << std::dec;

	if ( ! valid() )
	{
		os << " -- INVALID CONTROL";
		return;
	}

	os << " size " << int(myLength) << " value ";
	switch ( myLength )
	{
		case 1: dumpHex( os, "", static_cast<uint8_t>( get() ), false ); break;
		case 2: dumpHex( os, "", static_cast<uint16_t>( get() ), false ); break;
		case 4: dumpHex( os, "", get(), false ); break;
		default:
			dumpHexRaw( os, "", myRawData, myLength, false ); break;
			break;
	}

	if ( read_only() )
		os << " READ ONLY";
	else
		os << " [range: " << myMin << " - " << myMax << "]";
}


////////////////////////////////////////


std::ostream &operator<<( std::ostream &os, const Control &ctrl )
{
	ctrl.print( os );
	return os;
}


////////////////////////////////////////


} // usb



