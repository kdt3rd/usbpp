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


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


AsyncTransfer::AsyncTransfer( void )
		: myXfer( libusb_alloc_transfer( 0 ) )
{
}


////////////////////////////////////////


AsyncTransfer::~AsyncTransfer( void )
{
	cancel();
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
	if ( myXferSubmit )
		return;

	check_error( libusb_submit_transfer( myXfer ) );
	myXferSubmit = true;
}


////////////////////////////////////////


void
AsyncTransfer::cancel( void )
{
	if ( myXferSubmit )
	{
		libusb_cancel_transfer( myXfer );
		myXferSubmit = false;
	}
	// How do we drain any pending prior to destroying the transfer?
	// it seems to crash if we destroy too quickly...
}


////////////////////////////////////////


void
AsyncTransfer::dispatchCB( void )
{
	if ( myXferSubmit && myCallBack )
	{
		myXferSubmit = false;
		myCallBack( myXfer );
	}
}


////////////////////////////////////////


InterruptTransfer::InterruptTransfer( void )
		: AsyncTransfer()
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
InterruptTransfer::init( libusb_device_handle *handle, const struct libusb_endpoint_descriptor &epDesc )
{
	uint8_t endPointNum = (epDesc.bEndpointAddress & 0xF);
	uint16_t maxPacket = epDesc.wMaxPacketSize;

	myData = (uint8_t *)malloc( maxPacket );
	libusb_fill_interrupt_transfer( myXfer, handle,
									( endPointNum | LIBUSB_ENDPOINT_IN ),
									myData,
									int(maxPacket),
									&InterruptTransfer::recvInterrupt,
									this, 1000 );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


void
InterruptTransfer::recvInterrupt( libusb_transfer *xfer )
{
	InterruptTransfer *inp = reinterpret_cast<InterruptTransfer *>( xfer->user_data );
	switch ( xfer->status )
	{
		case LIBUSB_TRANSFER_COMPLETED:
			break;

		case LIBUSB_TRANSFER_ERROR:
			std::cout << "ERROR: TRANSFER ERROR" << std::endl;
			return;

		case LIBUSB_TRANSFER_TIMED_OUT:
			libusb_submit_transfer( xfer );
			return;

		case LIBUSB_TRANSFER_CANCELLED:
			return;

		case LIBUSB_TRANSFER_STALL:
			std::cout << "ERROR: TRANSFER STALL" << std::endl;
			return;

		case LIBUSB_TRANSFER_NO_DEVICE:
			std::cout << "ERROR: LOST DEVICE" << std::endl;
			return;

		case LIBUSB_TRANSFER_OVERFLOW:
			std::cout << "ERROR: TRANSFER OVERFLOW" << std::endl;
			return;

		default:
			std::cout << "ERROR: Unknown transfer status: " << int(xfer->status) << std::endl;
			return;
	}


	try
	{
		inp->dispatchCB();
	}
	catch ( ... )
	{
		std::cerr << "ERROR in dispatch transfer CB, what to do? In C routine..." << std::endl;
	}
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


BulkTransfer::BulkTransfer( size_t bufSize )
		: AsyncTransfer(), myBufSize( bufSize )
{
}


////////////////////////////////////////


BulkTransfer::~BulkTransfer( void )
{
}


////////////////////////////////////////


void
BulkTransfer::init( libusb_device_handle *handle, const struct libusb_endpoint_descriptor &epDesc )
{
	uint8_t endPointNum = (epDesc.bEndpointAddress & 0xF);
	myData = (uint8_t *)malloc( myBufSize );
	libusb_fill_bulk_transfer( myXfer, handle,
							   ( endPointNum | LIBUSB_ENDPOINT_IN ),
							   myData,
							   int(myBufSize),
							   &BulkTransfer::recvBulk,
							   this, 1000 );
	myXfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
}


////////////////////////////////////////


void
BulkTransfer::recvBulk( libusb_transfer *xfer )
{
	BulkTransfer *inp = reinterpret_cast<BulkTransfer *>( xfer->user_data );
	switch ( xfer->status )
	{
		case LIBUSB_TRANSFER_COMPLETED:
			break;

		case LIBUSB_TRANSFER_ERROR:
			std::cout << "ERROR: BULK TRANSFER ERROR" << std::endl;
			return;

		case LIBUSB_TRANSFER_TIMED_OUT:
			libusb_submit_transfer( xfer );
			return;

		case LIBUSB_TRANSFER_CANCELLED:
			return;

		case LIBUSB_TRANSFER_STALL:
			std::cout << "ERROR: BULK TRANSFER STALL" << std::endl;
			return;

		case LIBUSB_TRANSFER_NO_DEVICE:
			std::cout << "ERROR: LOST BULK DEVICE" << std::endl;
			return;

		case LIBUSB_TRANSFER_OVERFLOW:
			std::cout << "ERROR: BULK TRANSFER OVERFLOW" << std::endl;
			return;

		default:
			std::cout << "ERROR: Unknown transfer status: " << int(xfer->status) << std::endl;
			return;
	}


	try
	{
		inp->dispatchCB();
	}
	catch ( ... )
	{
		std::cerr << "ERROR in dispatch transfer CB, what to do? In C routine..." << std::endl;
	}
}

} // USB



