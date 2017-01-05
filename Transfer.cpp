// Transfer.cpp -*- C++ -*-

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

#include "Transfer.h"
#include "Exception.h"
#include <iostream>
#include <iomanip>
#include "DeviceManager.h"
#include <algorithm>


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


AsyncTransfer::AsyncTransfer( libusb_context *ctxt, int numPackets )
		: myContext( ctxt ), myXfer( libusb_alloc_transfer( numPackets ) )
{
}


////////////////////////////////////////


AsyncTransfer::~AsyncTransfer( void )
{
	cancel();
	wait();

	libusb_free_transfer( myXfer );
}


////////////////////////////////////////


void
AsyncTransfer::setCallback( const std::function<void (libusb_transfer *)> &func )
{
	myCallBack = func;
}


////////////////////////////////////////


void
AsyncTransfer::submit( void )
{
	if ( inFlight() )
	{
//		std::cout << "transfer already submitted, skipping submit" << std::endl;
		return;
	}

	myComplete = 0;
	myAmountTransferred = 0;
	check_error( libusb_submit_transfer( myXfer ) );
}


////////////////////////////////////////


void
AsyncTransfer::cancel( void )
{
	if ( inFlight() )
		libusb_cancel_transfer( myXfer );
}


////////////////////////////////////////


int
AsyncTransfer::wait( void )
{
	if ( myXfer )
	{
		while ( inFlight() )
			libusb_handle_events_completed( myContext, getCompleterReference() );
	}

	return myAmountTransferred;
}


////////////////////////////////////////


void
AsyncTransfer::transfer_callback( libusb_transfer *xfer )
{
	if ( xfer->status == LIBUSB_TRANSFER_TIMED_OUT )
	{
		libusb_submit_transfer( xfer );
		return;
	}

	AsyncTransfer *t = reinterpret_cast<AsyncTransfer *>( xfer->user_data );
	t->handleCallback();
}


////////////////////////////////////////


void
AsyncTransfer::handleCallback( void )
{
	myComplete = 1;

	switch ( myXfer->status )
	{
		case LIBUSB_TRANSFER_COMPLETED:
			myAmountTransferred = myXfer->actual_length;
			break;

		case LIBUSB_TRANSFER_ERROR:
			std::cout << "ERROR: " << type() << " TRANSFER ERROR" << std::endl;
			break;

//		case LIBUSB_TRANSFER_TIMED_OUT:
//			return;

		case LIBUSB_TRANSFER_CANCELLED:
			return;

		case LIBUSB_TRANSFER_STALL:
			std::cout << "ERROR: " << type() << " TRANSFER STALL" << std::endl;
			break;

		case LIBUSB_TRANSFER_NO_DEVICE:
			std::cout << "ERROR: " << type() << " LOST DEVICE" << std::endl;
			break;

		case LIBUSB_TRANSFER_OVERFLOW:
			std::cout << "ERROR: " << type() << " TRANSFER OVERFLOW" << std::endl;
			break;

		default:
			std::cout << "ERROR: Unknown transfer status: " << int(myXfer->status) << std::endl;
			return;
	}


	try
	{
		if ( myCallBack )
			myCallBack( myXfer );
	}
	catch ( std::exception &e )
	{
		std::cerr << "ERROR in dispatch transfer CB: '" << e.what() << "', ignoring because in C routine" << std::endl;
	}
	catch ( ... )
	{
		std::cerr << "ERROR in dispatch transfer CB, what to do? In C routine, unable to propagate exception..." << std::endl;
	}
}


////////////////////////////////////////


ControlTransfer::ControlTransfer( libusb_context *ctxt )
		: AsyncTransfer( ctxt )
{
}


////////////////////////////////////////


ControlTransfer::~ControlTransfer( void )
{
	if ( myData )
		free( myData );
}


////////////////////////////////////////


void
ControlTransfer::fill( libusb_device_handle *handle,
					   uint8_t bmRequestType, uint8_t bRequest,
					   uint16_t wValue, uint16_t wIndex, uint16_t wLength,
					   void *buf )
{
	bool needReinit = false;
	size_t bytesNeeded = LIBUSB_CONTROL_SETUP_SIZE + wLength;
	if ( myDataLen < bytesNeeded )
	{
		if ( myData )
			free( myData );

		myData = reinterpret_cast<uint8_t *>( malloc( bytesNeeded ) );
		myDataLen = bytesNeeded;
		needReinit = true;
	}

	libusb_fill_control_setup( myData, bmRequestType, bRequest,
							   wValue, wIndex, wLength );

	if ( (bmRequestType & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT )
	{
		uint8_t *dataOut = myData + LIBUSB_CONTROL_SETUP_SIZE;
		uint8_t *src = reinterpret_cast<uint8_t *>( buf );
		std::copy( src, src + wLength, dataOut );
		myDestBuffer = nullptr;
		myDestLen = 0;
	}
	else
	{
		myDestLen = wLength;
		myDestBuffer = buf;
	}

	if ( needReinit )
		init( handle, 0 );
}


////////////////////////////////////////


void
ControlTransfer::init( libusb_device_handle *handle, uint8_t endPoint )
{
	if ( ! myData )
		throw std::runtime_error( "Control transfers must be filled prior to initialization" );

	libusb_fill_control_transfer( myXfer, handle, myData,
								  &AsyncTransfer::transfer_callback,
								  this, 1000 );
	// let's not auto-free this one
	myXfer->flags = 0;
}


////////////////////////////////////////


void
ControlTransfer::handleCallback( void )
{
	if ( myXfer->status == LIBUSB_TRANSFER_COMPLETED && myDestBuffer )
	{
		uint8_t *inBuf = myData + LIBUSB_CONTROL_SETUP_SIZE;
		uint8_t *out = reinterpret_cast<uint8_t *>( myDestBuffer );
		std::copy( inBuf, inBuf + myDestLen, out );
	}
	AsyncTransfer::handleCallback();
}


////////////////////////////////////////


InterruptTransfer::InterruptTransfer( libusb_context *ctxt, uint16_t maxPacket )
		: AsyncTransfer( ctxt ), myMaxPacketSize( maxPacket )
{
}


////////////////////////////////////////


InterruptTransfer::~InterruptTransfer( void )
{
	// we told the transfer to delete our data buffer
	// when it is freed...
}


////////////////////////////////////////


void
InterruptTransfer::init( libusb_device_handle *handle, uint8_t endPoint )
{
	uint8_t endPointNum = (endPoint & 0xF);

	myData = (uint8_t *)malloc( myMaxPacketSize );
	libusb_fill_interrupt_transfer( myXfer, handle,
									( endPointNum | LIBUSB_ENDPOINT_IN ),
									myData,
									int(myMaxPacketSize),
									&AsyncTransfer::transfer_callback,
									this, 0 );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


void
InterruptTransfer::fill( libusb_device_handle *handle, uint8_t endPoint,
						 unsigned char *buffer, int length,
						 unsigned int timeout )
{
	if ( ! myData )
		myData = (uint8_t *)malloc( myMaxPacketSize );
	std::copy( buffer, buffer + std::min( length, int(myMaxPacketSize) ), myData );
	if ( length < myMaxPacketSize )
		std::fill( buffer + length, buffer + myMaxPacketSize, uint8_t(0) );

	libusb_fill_interrupt_transfer( myXfer, handle,
									endPoint,
									myData,
									int(myMaxPacketSize),
									&AsyncTransfer::transfer_callback,
									this, timeout );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


void
InterruptTransfer::send( libusb_device_handle *handle, uint8_t endPoint,
						 unsigned char *buffer, int length,
						 unsigned int timeout )
{
	fill( handle, endPoint, buffer, length, timeout );
	submitAndWait();
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


BulkTransfer::BulkTransfer( libusb_context *ctxt, size_t bufSize )
		: AsyncTransfer( ctxt ), myBufSize( bufSize )
{
}


////////////////////////////////////////


BulkTransfer::~BulkTransfer( void )
{
}


////////////////////////////////////////


void
BulkTransfer::init( libusb_device_handle *handle, uint8_t endPoint )
{
	uint8_t endPointNum = (endPoint & 0xF);
	myData = (uint8_t *)malloc( myBufSize );
	libusb_fill_bulk_transfer( myXfer, handle,
							   ( endPointNum | LIBUSB_ENDPOINT_IN ),
							   myData,
							   int(myBufSize),
							   &AsyncTransfer::transfer_callback,
							   this, 0 );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


void
BulkTransfer::send( libusb_device_handle *handle,
					unsigned char endpoint,
					unsigned char *buffer, int length,
					unsigned int timeout )
{
	if ( length > static_cast<int>( myBufSize ) )
		throw std::runtime_error( "Send buffer too large for max packet size" );

	libusb_fill_bulk_transfer( myXfer, handle, endpoint,
							   buffer, length,
							   &AsyncTransfer::transfer_callback,
							   this, timeout );

	submitAndWait();
}


////////////////////////////////////////


ISOTransfer::ISOTransfer( libusb_context *ctxt, int numPackets, size_t bufSize, size_t maxPacket )
		: AsyncTransfer( ctxt, numPackets ),
		  myNumPackets( numPackets ), myBufSize( bufSize ), myMaxPacketSize( maxPacket )
{
}


////////////////////////////////////////


ISOTransfer::~ISOTransfer( void )
{
}


////////////////////////////////////////


void
ISOTransfer::init( libusb_device_handle *handle, uint8_t endPoint )
{
	uint8_t endPointNum = (endPoint & 0xF);
	myData = (uint8_t *)malloc( myBufSize );
	libusb_fill_iso_transfer( myXfer, handle,
							  ( endPointNum | LIBUSB_ENDPOINT_IN ),
							  myData,
							  int(myBufSize),
							  myNumPackets,
							  &AsyncTransfer::transfer_callback,
							  this, 0 );
	libusb_set_iso_packet_lengths( myXfer, myMaxPacketSize );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


} // USB



