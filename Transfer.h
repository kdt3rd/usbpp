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

#include <libusb-1.0/libusb.h>
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
	AsyncTransfer( void );
	virtual ~AsyncTransfer( void );

	inline bool matches( libusb_transfer *xfer ) const { return myXfer == xfer; }

	virtual void init( libusb_device_handle *handle, const struct libusb_endpoint_descriptor &epDesc ) = 0;

	void setCallback( const std::function<void (libusb_transfer *)> &func );
	
	void submit( void );
	void cancel( void );

protected:
	void dispatchCB( void );

	libusb_transfer *myXfer = nullptr;
	uint8_t *myData = nullptr;
	std::function<void (libusb_transfer *)> myCallBack;
	bool myXferSubmit = false;

};

class InterruptTransfer : public AsyncTransfer
{
public:
	InterruptTransfer( void );
	virtual ~InterruptTransfer( void );

	virtual void init( libusb_device_handle *handle, const struct libusb_endpoint_descriptor &epDesc );

private:
	static void recvInterrupt( libusb_transfer *xfer );
};

class BulkTransfer : public AsyncTransfer
{
public:
	BulkTransfer( size_t bufSize );
	virtual ~BulkTransfer( void );

	virtual void init( libusb_device_handle *handle, const struct libusb_endpoint_descriptor &epDesc );

private:
	static void recvBulk( libusb_transfer *xfer );

	size_t myBufSize;
};

} // namespace USB

