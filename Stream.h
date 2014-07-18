// Stream.h -*- C++ -*-

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

#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>


////////////////////////////////////////


///
/// @file Stream.h
///
/// @author Kimball Thurston
///

namespace USB
{

struct ROI
{
	int x, y;
	int w, h;
};

class ImageBuffer
{
public:
	enum class Format
	{
		BAYER_GRBG,
		BAYER_GBRG,
		BAYER_RGGB,
		BAYER_BGGR,
		MONO_8,
		MONO_16,
		YUY2,
		UYVY,
		UNKNOWN
	};

	ImageBuffer( void );
	~ImageBuffer( void );

	// returns true when full
	bool addData( const uint8_t *buf, int &len );

	void reset( Format fmt, int w, int h, int bpl, int bpp, const ROI &roi );

	bool empty( void ) const { return myCurX == myROI.x && myCurY == myROI.y; }
	bool partial( void ) const { return myCurY < myROI.h; }

	inline const ROI &roi( void ) const { return myROI; }

	inline Format format( void ) const { return myFormat; }
	inline int width( void ) const { return myWidth; }
	inline int height( void ) const { return myHeight; }
	inline int bytesPerLine( void ) const { return myBytesPerLine; }
	inline int bytesPerPixel( void ) const { return myBytesPerPixel; }

	inline const uint8_t *data( void ) const { return myBuffer.data(); }
private:
	ROI myROI;

	Format myFormat = Format::MONO_8;
	int myWidth = 0;
	int myHeight = 0;
	int myBytesPerLine = 0;
	int myBytesPerPixel = 0;

	int myCurX = 0;
	int myCurY = 0;

	std::vector<uint8_t> myBuffer;
};

///
/// @brief Class Stream provides...
///
class VideoStream
{
public:
	
	VideoStream( void );
	~VideoStream( void );

	std::shared_ptr<ImageBuffer> get( void );
	void put( std::shared_ptr<ImageBuffer> &buf );

	int width( void ) const { return myWidth; }
	int height( void ) const { return myHeight; }
	const ROI &roi( void ) const { return myROI; }

	void setROI( const ROI &roi );
	void reset( int w, int h, int bpl, int bpp, ImageBuffer::Format fmt, size_t maxN );
	void clear( void );
	bool off( void ) const;

private:
	inline bool off_locked( void ) const { return myMaxBuffers == 0; }

	mutable std::mutex myMutex;
	std::condition_variable myHasBufferNotify;

	ROI myROI;
	int myWidth = 0;
	int myHeight = 0;
	int myBytesPerLine = 0;
	int myBytesPerPixel = 1;
	ImageBuffer::Format myFormat = ImageBuffer::Format::MONO_8;

	size_t myMaxBuffers = 0;
	size_t myLiveBuffers = 0;
	std::vector<std::shared_ptr<ImageBuffer>> myBuffers;
};

} // namespace usb

