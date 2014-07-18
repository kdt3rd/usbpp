// Stream.cpp -*- C++ -*-

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

#include "Stream.h"
#include <algorithm>


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


ImageBuffer::ImageBuffer( void )
{
	myROI.x = 0;
	myROI.y = 0;
	myROI.w = 0;
	myROI.h = 0;
}


////////////////////////////////////////


ImageBuffer::~ImageBuffer( void )
{
}


////////////////////////////////////////


bool
ImageBuffer::addData( const uint8_t *buf, int &len )
{
	while ( len > 0 )
	{
		int leftInLine = ( myROI.w - myCurX ) * myBytesPerPixel;
		int nToCopy = std::min( len, leftInLine );

		if ( nToCopy <= 0 )
			return true;

		uint8_t *dest = myBuffer.data();
		dest += ( myROI.y + myCurY ) * myBytesPerLine;
		dest += ( myROI.x + myCurX ) * myBytesPerPixel;
		std::copy( buf, buf + nToCopy, dest );

		buf += nToCopy;
		len -= nToCopy;

		myCurX += nToCopy / myBytesPerPixel;
		if ( myCurX == myROI.w )
		{
			myCurX = 0;
			++myCurY;
			if ( myCurY == myROI.h )
				return true;
		}
	}

	return false;
}


////////////////////////////////////////


void
ImageBuffer::reset( Format fmt, int w, int h, int bpl, int bpp, const ROI &roi )
{
	if ( ( roi.x + roi.w ) > w || ( roi.y + roi.h ) > h )
		throw std::runtime_error( "Invalid ROI" );

	myFormat = fmt;
	myWidth = w;
	myHeight = h;
	myBytesPerLine = bpl;
	myBytesPerPixel = bpp;

	myROI = roi;

	myCurX = 0;
	myCurY = 0;

	myBuffer.resize( myBytesPerLine * myHeight );
}


////////////////////////////////////////


VideoStream::VideoStream( void )
{
}


////////////////////////////////////////


VideoStream::~VideoStream( void )
{
}


////////////////////////////////////////


std::shared_ptr<ImageBuffer>
VideoStream::get( void )
{
	std::unique_lock<std::mutex> lk( myMutex );

	std::shared_ptr<ImageBuffer> ret;

	if ( off_locked() )
		return ret;

	while ( ! ret )
	{
		if ( myMaxBuffers == 0 || myWidth <= 0 || myHeight <= 0 )
			break;

		if ( ! myBuffers.empty() )
		{
			ret = myBuffers.back();
			myBuffers.pop_back();
			break;
		}

		if ( myLiveBuffers < myMaxBuffers )
		{
			ret = std::make_shared<ImageBuffer>();
			++myLiveBuffers;
			break;
		}

		myHasBufferNotify.wait( lk );
	}

	if ( ret )
		ret->reset( myFormat, myWidth, myHeight, myBytesPerLine, myBytesPerPixel, myROI );

	return ret;
}


////////////////////////////////////////


void
VideoStream::put( std::shared_ptr<ImageBuffer> &buf )
{
	std::unique_lock<std::mutex> lk( myMutex );

	if ( ! buf || off_locked() )
		return;

	if ( myBuffers.size() < myMaxBuffers )
		myBuffers.push_back( buf );
	buf.reset();
	myHasBufferNotify.notify_one();
}


////////////////////////////////////////


void
VideoStream::setROI( const ROI &roi )
{
	std::unique_lock<std::mutex> lk( myMutex );

	myROI = roi;
}


////////////////////////////////////////


void
VideoStream::reset( int w, int h, int bpl, int bpp, ImageBuffer::Format fmt, size_t maxN )
{
	std::unique_lock<std::mutex> lk( myMutex );

	myWidth = w;
	myHeight = h;
	myBytesPerLine = bpl;
	myBytesPerPixel = bpp;
	myFormat = fmt;
	myMaxBuffers = maxN;
}


////////////////////////////////////////


void
VideoStream::clear( void )
{
	std::unique_lock<std::mutex> lk( myMutex );

	myROI.x = 0;
	myROI.y = 0;
	myROI.w = 0;
	myROI.h = 0;
	myWidth = 0;
	myHeight = 0;
	myBytesPerLine = 0;
	myBytesPerPixel = 0;
	myFormat = ImageBuffer::Format::MONO_8;
	myMaxBuffers = 0;
	myLiveBuffers = 0;
	myBuffers.clear();
}


////////////////////////////////////////


bool
VideoStream::off( void ) const
{
	std::unique_lock<std::mutex> lk( myMutex );
	return off_locked();
}


////////////////////////////////////////


} // usb



