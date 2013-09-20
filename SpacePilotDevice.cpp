// SpacePilotDevice.cpp -*- C++ -*-

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

#include "SpacePilotDevice.h"

#include <functional>
#include <locale>
#include <fstream>
#include <setjmp.h>
#include <string.h>

extern "C" {
#include <jpeglib.h>
}


////////////////////////////////////////


namespace {

// Static USB header needed for the Logitech LCD used in the SpacePilot Pro
static const uint8_t kImageHeader[512] = {
	0x10, 0x0f, 0x00, 0x58, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3f, 0x01, 0xef, 0x00, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

inline uint16_t
rgb_to_uint16( uint8_t r, uint8_t g, uint8_t b )
{
	return ((uint16_t(r) >> 3) << 11)|((uint16_t(g) >> 2) << 5)|(uint16_t(b) >>3);
}


////////////////////////////////////////


static void
read_ppm( const char *filename, std::vector<uint8_t> &rgb, int &w, int &h )
{
	w = h = 0;
	rgb.clear();

	std::ifstream ppmfile( filename );
	std::string magic;
	int maxv;
	ppmfile >> magic >> w >> h >> maxv;
	char ws;
	ppmfile.read( &ws, 1 );
	if ( magic != "P6" )
	{
		w = h = 0;
		std::cout << "Reading image '" << filename << "': INVALID MAGIC: '" << magic << "'" << std::endl;
		return;
	}
	std::cout << "Reading image '" << filename << "': " << w << " x " << h << std::endl;
	if ( maxv > 255 )
	{
		std::cout << "Unsupported PPM bit depth: " << maxv << std::endl;
		return;
	}
	rgb.resize(w * h * 3);
	ppmfile.read( reinterpret_cast<char *>(rgb.data()), w * h * 3 );
}


////////////////////////////////////////


static void
read_jpeg( const char *filename, std::vector<uint8_t> &rgb, int &w, int &h )
{
	w = h = 0;
	rgb.clear();

	FILE *f = fopen( filename, "rb" );
	if ( f )
	{
		struct jpeg_decompress_struct jpegReader;
		struct jpeg_error_mgr error;
		jpegReader.err = jpeg_std_error( &error );
		jpeg_create_decompress( &jpegReader );
		sigjmp_buf jmpbuffer;

		jpeg_stdio_src( &jpegReader, f );

		if ( setjmp( jmpbuffer ) )
		{
			std::cout << "ERROR Reading JPEG '" << filename << "'" << std::endl;
			goto done;
		}

		if ( JPEG_HEADER_OK == jpeg_read_header( &jpegReader, true ) )
		{
			w = jpegReader.image_width;
			h = jpegReader.image_height;
			if ( jpegReader.num_components != 3 )
			{
				std::cout << "ERROR Reading JPEG '" << filename << "': unsuppored number of components: " << jpegReader.num_components << std::endl;
				goto done;
			}

			jpegReader.do_fancy_upsampling = FALSE;
			jpegReader.do_block_smoothing = FALSE;

			jpegReader.out_color_space = JCS_RGB;
			jpegReader.dct_method = JDCT_FLOAT;

			jpeg_start_decompress( &jpegReader );
			rgb.resize( w * h * 3 );
			uint8_t *outBuffer = rgb.data();
			for ( int y = 0; y < h; ++y, outBuffer += w * 3 )
			{
				JSAMPLE *curLine = static_cast<JSAMPLE *>( outBuffer );
				if ( 1 != jpeg_read_scanlines( &jpegReader, &curLine, 1 ) )
				{
					jpeg_abort_decompress( &jpegReader );
					std::cout << "ERROR Reading JPEG '" << filename << "'" << std::endl;
					rgb.clear();
					w = h = 0;
					goto done;
				}
			}
			jpeg_finish_decompress( &jpegReader );
		}
	  done:
		jpeg_destroy_decompress( &jpegReader );
	}
	fclose( f );
}

}


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


std::shared_ptr<Device>
SpacePilotDevice::factory( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	return std::shared_ptr<Device>( new SpacePilotDevice( dev, desc ) );
}


////////////////////////////////////////


SpacePilotDevice::SpacePilotDevice( void )
{
}


////////////////////////////////////////


SpacePilotDevice::SpacePilotDevice( libusb_device *dev )
		: Device( dev )
{
}


////////////////////////////////////////


SpacePilotDevice::SpacePilotDevice( libusb_device *dev, const struct libusb_device_descriptor &desc )
		: Device( dev, desc )
{
}


////////////////////////////////////////


SpacePilotDevice::~SpacePilotDevice( void )
{
}


////////////////////////////////////////


void
SpacePilotDevice::setLCD( uint8_t r, uint8_t g, uint8_t b )
{
	size_t idx = fillLCDData();
	uint16_t outVal = rgb_to_uint16( r, g, b );

	for ( size_t x = 0; x < 320; ++x )
		for ( size_t y = 0; y < 240; ++y, ++idx )
			myLCDData[idx] = outVal;

	loadLCD();
}


////////////////////////////////////////


void
SpacePilotDevice::setLCD( const uint8_t *rgbdata, int w, int h )
{
	// column oriented, NOT row oriented
	// Need to swap to column ordered
	size_t idx = fillLCDData();
	for ( size_t x = 0; x < 320; ++x )
	{
		for ( size_t y = 0; y < 240; ++y, ++idx )
		{
			uint8_t r = 0, g = 0, b = 0;
			if ( x < w && y < h )
			{
				r = rgbdata[y*w*3 + x*3];
				g = rgbdata[y*w*3 + x*3 + 1];
				b = rgbdata[y*w*3 + x*3 + 2];
			}
			myLCDData[idx] = rgb_to_uint16( r, g, b );
		}
	}

	loadLCD();
}


////////////////////////////////////////


void
SpacePilotDevice::setLCD( const std::string &filename )
{
	std::string::size_type ePos = filename.find_last_of( '.' );
	int w = 0, h = 0;
	std::vector<uint8_t> rgb;

	if ( ePos != std::string::npos )
	{
		std::string ext = filename.substr( ePos + 1 );
		std::use_facet<std::ctype<char>>( std::locale("") ).tolower(
			&ext[0], &ext[0] + ext.size() );

		if ( ext == "ppm" )
			read_ppm( filename.c_str(), rgb, w, h );
		else if ( ext == "jpg" || ext == "jpeg" )
			read_jpeg( filename.c_str(), rgb, w, h );
		else
		{
			std::cerr << "Unhandled extension type '" << ext << "' for image '" << filename << "'" << std::endl;
		}
	}
	else
	{
		std::cerr << "ERROR: Unable to determine image type for '" << filename << "'" << std::endl;
	}

	if ( ! rgb.empty() && w > 0 && h > 0 )
		setLCD( rgb.data(), w, h );
}


////////////////////////////////////////


void
SpacePilotDevice::handleLCDButton( uint8_t *buf )
{
	// LCD_UP_ARROW byte 0, bit 7
	// LCD_DOWN_ARROW byte 0, bit 6
	// LCD_RIGHT_ARROW byte 0, bit 4
	// LCD_LEFT_ARROW byte 0, bit 5
	// LCD_OK_BUTTON byte 0, bit 3
	// LCD_SQUARE_BUTTON byte 0, bit 2
	// LCD_BACK_BUTTON byte 0, bit 1
	// LCD_TOGGLE_SQUARE_BUTTON byte 0, bit 0
	// LCD_TOGGLE_DISPLAY byte 1, bit 1
	std::cout << "LCD button:";
	for ( int i = 0; i < 2; ++i )
	{
		std::cout << ' ';
		uint8_t curbyte = buf[i];
		for ( int b = 7; b >= 0; --b )
		{
			if ( curbyte & (1 << b) )
				std::cout << '1';
			else
				std::cout << '0';
		}
	}
	std::cout << std::endl;
}


////////////////////////////////////////


void
SpacePilotDevice::handleSpaceNav( int report, uint8_t *buf, int buflen )
{
	if ( report == 3 )
	{
		// SPACENAV_FIT byte 0, bit 0
		// SPACENAV_MENU byte 0, bit 1
		// SPACENAV_ESC byte 2, bit 6
		// SPACENAV_SHIFT byte 3, bit 0
		// SPACENAV_CTRL byte 3, bit 1
		// SPACENAV_ALT byte 2, bit 7
		// SPACENAV_1 byte 1, bit 4
		// SPACENAV_2 byte 1, bit 5
		// SPACENAV_3 byte 1, bit 6
		// SPACENAV_4 byte 1, bit 7
		// SPACENAV_5 byte 2, bit 0
		// SPACENAV_PLUS byte 3, bit 5
		// SPACENAV_MINUS byte 3, bit 6
		// SPACENAV_SNAP_SCALE byte 3, bit 4
		// SPACENAV_SNAP_TRANSLATE byte 3, bit 3
		// SPACENAV_SNAP_ROTATION byte 3, bit 2
		// SPACENAV_ROTATE_OBJ byte 1, bit 0
		// SPACENAV_TRANSLATE_OBJ byte 0, bit 2
		// SPACENAV_F_OBJ byte 0, bit 5
		// SPACENAV_B_OBJ byte 0, bit 4
		// SPACENAV_ISO1 byte 1, bit 2
		std::cout << "SpaceNav button:";
		for ( int i = 0; i < buflen; ++i )
		{
			std::cout << ' ';
			uint8_t curbyte = buf[i];
			for ( int b = 7; b >= 0; --b )
			{
				if ( curbyte & (1 << b) )
					std::cout << '1';
				else
					std::cout << '0';
			}
		}
		std::cout << std::endl;
	}
	else if ( report == 1 )
	{
		// per hidraw report, this is X, Y, Z, 16 bit values
		int16_t *xyzBuf = reinterpret_cast<int16_t *>( buf );
		std::cout << "SpaceNav: X "
				  << xyzBuf[0]
				  << " Y " << xyzBuf[1]
				  << " Z " << xyzBuf[2];
	}
	else if ( report == 2 )
	{
		// per hidraw report, this is rX, rY, rZ, 16 bit values
		int16_t *xyzBuf = reinterpret_cast<int16_t *>( buf );
		std::cout << " rX " << xyzBuf[0] << " rY " << xyzBuf[1] << " rZ " << xyzBuf[2] << std::endl;
	}
}


////////////////////////////////////////


bool
SpacePilotDevice::handleEvent( int endpoint, uint8_t *buf, int buflen, libusb_transfer *xfer )
{
	bool retval = false;
	if ( endpoint == 1 )
	{
		handleLCDButton( buf );
		retval = true;
	}
	else if ( endpoint == 3 )
	{
		int report = buf[0];
		handleSpaceNav( report, buf + 1, buflen - 1 );
		retval = true;
	}

	return retval;
}


////////////////////////////////////////


bool
SpacePilotDevice::wantInterface( const struct libusb_interface_descriptor &iface )
{
	for ( int ep = 0, nep = int(iface.bNumEndpoints); ep < nep; ++ep )
	{
		const struct libusb_endpoint_descriptor &epDesc = iface.endpoint[ep];
		if ( ( epDesc.bEndpointAddress & 0x80 ) == LIBUSB_ENDPOINT_OUT &&
			 ( epDesc.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK ) == LIBUSB_TRANSFER_TYPE_BULK )
			myLCDEndpoint = epDesc.bEndpointAddress & 0xF;
	}
	return true;
}


////////////////////////////////////////


size_t
SpacePilotDevice::fillLCDData( void )
{
	if ( myLCDData.empty() )
	{
		myLCDData.resize( sizeof(kImageHeader) / 2 + 320*240 );
		memcpy( myLCDData.data(), kImageHeader, sizeof(kImageHeader) );
	}

	// column oriented, NOT row oriented
	// Need to swap to column ordered
	return sizeof(kImageHeader)/2;
}


////////////////////////////////////////


void
SpacePilotDevice::loadLCD( void )
{
	if ( myLCDEndpoint == 0 )
		return;

	int xferSize = sizeof(uint16_t) * myLCDData.size();
	int actLen = 0;
	std::cout << "make this async at some point..." << std::endl;
	check_error( libusb_bulk_transfer(
					 myHandle, myLCDEndpoint,
					 reinterpret_cast<uint8_t *>(myLCDData.data()),
					 xferSize, &actLen, 1000 ) );
}


////////////////////////////////////////


} // usbpp



