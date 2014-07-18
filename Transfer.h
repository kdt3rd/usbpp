// Transfer.h -*- C++ -*-

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

#include "libusb-1.0/libusb.h"
#include <functional>


////////////////////////////////////////


///
/// @file Transfer.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class Transfer provides async transfer...
///
class AsyncTransfer
{
public:
	AsyncTransfer( libusb_context *ctxt, int numPackets = 0 );
	virtual ~AsyncTransfer( void );

	inline bool matches( libusb_transfer *xfer ) const { return myXfer == xfer; }

	virtual void init( libusb_device_handle *handle, uint8_t endPoint ) = 0;

	void setCallback( const std::function<void (libusb_transfer *)> &func );

	// initiate async transfer
	void submit( void );
	// submit for async transfer to cancel
	void cancel( void );

	int submitAndWait( void ) { submit(); return wait(); }

	// wait for "completion" (cancel, error, etc.)
	// returns amount transmitted
	int wait( void );

	bool inFlight( void ) const { return myComplete == 0; }
	bool isComplete( void ) const { return myComplete == 1; }
	int *getCompleterReference( void ) { return &myComplete; }

protected:
	virtual const char *type( void ) const = 0;

	static void transfer_callback( libusb_transfer *xfer );

	// if a subclass overrides this, be careful to update complete
	// status, etc. or just decorate
	virtual void handleCallback( void );

	libusb_context *myContext = nullptr;
	int myComplete = 1;
	int myAmountTransferred = 0;
	libusb_transfer *myXfer = nullptr;
	uint8_t *myData = nullptr;
	std::function<void (libusb_transfer *)> myCallBack;
};

class ControlTransfer : public AsyncTransfer
{
public:
	ControlTransfer( libusb_context *ctxt );
	virtual ~ControlTransfer( void );

	void fill( libusb_device_handle *handle,
			   uint8_t bmRequestType, uint8_t bRequest,
			   uint16_t wValue, uint16_t wIndex, uint16_t wLength,
			   void *buf );

protected:
	virtual void init( libusb_device_handle *handle, uint8_t endPoint );

	virtual const char *type( void ) const { return "CONTROL"; }

	virtual void handleCallback( void );

private:
	uint8_t *myData = nullptr;
	size_t myDataLen = 0;
	size_t myDestLen = 0;
	void *myDestBuffer = nullptr;
};

class InterruptTransfer : public AsyncTransfer
{
public:
	InterruptTransfer( libusb_context *ctxt, uint16_t maxPacket );
	virtual ~InterruptTransfer( void );

	virtual void init( libusb_device_handle *handle, uint8_t endPoint );

protected:
	virtual const char *type( void ) const { return "INTERRUPT"; }

private:
	uint16_t myMaxPacketSize;
};

class BulkTransfer : public AsyncTransfer
{
public:
	BulkTransfer( libusb_context *ctxt, size_t bufSize );
	virtual ~BulkTransfer( void );

	virtual void init( libusb_device_handle *handle, uint8_t endPoint );

	void send( libusb_device_handle *handle,
			   unsigned char endpoint,
			   unsigned char *buffer, int length,
			   unsigned int timeout = 1000 );

protected:
	virtual const char *type( void ) const { return "BULK"; }

private:
	size_t myBufSize;
};

class ISOTransfer : public AsyncTransfer
{
public:
	ISOTransfer( libusb_context *ctxt, int nPackets, size_t bufSize, size_t maxPacketSize );
	virtual ~ISOTransfer( void );

	virtual void init( libusb_device_handle *handle, uint8_t endPoint );

protected:
	virtual const char *type( void ) const { return "ISO"; }

private:
	int myNumPackets;
	size_t myBufSize;
	size_t myMaxPacketSize;
};

} // namespace USB

