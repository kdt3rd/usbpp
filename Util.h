// Util.h -*- C++ -*-

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
#ifndef _usbpp_Util_h_
#define _usbpp_Util_h_ 1

#include <iostream>
#include <iomanip>


////////////////////////////////////////


///
/// @file Util.h
///
/// @author Kimball Thurston
///

namespace USB
{

template <typename T>
inline void
dumpHex( std::ostream &os, const char *prefix, T val, bool includeInt = false, bool eol = true )
{
	os << prefix << ": 0x" << std::setfill( '0' ) << std::setw(sizeof(T)*2) << std::hex << int(val) << std::dec;
	if ( includeInt )
		os << " (" << int(val) << ")";
	if ( eol )
		os << std::endl;
}

inline void
dumpHexBuf( std::ostream &os, const char *prefix, const unsigned char *buf, size_t n, bool asChars = true )
{
	os << prefix << ": {";
	for ( size_t i = 0; i < n; ++i )
	{
		if ( i > 0 )
			std::cout << ',';
		os << " 0x" << std::setfill( '0' ) << std::setw(2) << std::hex << int(buf[i]) << std::dec;
		if ( asChars && isascii( buf[i] ) && buf[i] > 6 )
		{
			if ( buf[i] == '\n' )
				os << " '\\n'";
			else if ( buf[i] == '\r' )
				os << " '\\r'";
			else if ( buf[i] == '\t' )
				os << " '\\t'";
			else if ( buf[i] == '\a' )
				os << " '\\a'";
			else if ( buf[i] == '\b' )
				os << " '\\b'";
			else if ( buf[i] == '\v' )
				os << " '\\v'";
			else if ( buf[i] == '\f' )
				os << " '\\f'";
			else if ( isprint( buf[i] ) )
				os << " '" << char(buf[i]) << "'";
			os << " (" << int(buf[i]) << ")";
		}
		else
			os << " (" << int(buf[i]) << ")";
	}
	os << " }" << std::endl;
}

inline void
dumpHexRaw( std::ostream &os, const char *prefix, const uint8_t *buf, size_t n, bool eol = true )
{
	os << prefix << ": {";
	for ( size_t i = 0; i < n; ++i )
	{
		os << ' ' << std::setfill( '0' ) << std::setw(2) << std::hex << int(buf[i]) << std::dec;
	}
	os << " }";
	if ( eol )
		os << std::endl;
}

inline void
dumpGUID( std::ostream &os, const char *prefix, const uint8_t buf[16], bool doFourCC = false )
{
	os << prefix << ": {";
	for ( size_t i = 0; i < 16; ++i )
	{
		if ( doFourCC && i < 4 )
		{
			if ( i > 0 )
				os << ",";
			if ( isprint( buf[i] ) )
				os << '\'' << char(buf[i]) << '\'';
			else
				os << '(' << int(buf[i]) << ')';
		}
		else
			os << std::setfill( '0' ) << std::setw(2) << std::hex << int(buf[i]) << std::dec;
		if ( i == 3 || i == 5 || i == 7 || i == 9 )
			os << '-';
	}
	os << "}" << std::endl;
}

inline bool
matchGUID( const uint8_t buf[16], const uint8_t guid[16] )
{
	for ( size_t i = 0; i < 16; ++i )
	{
		if ( buf[i] != guid[i] )
			return false;
	}
	return true;
}

inline void
outputEntity( std::ostream &os, const char *tag, uint16_t type, uint8_t id, unsigned int num_pads, unsigned int extra_size )
{
	os << tag << " type 0x" << std::setfill( '0' ) << std::setw( 4 ) << std::hex << type << " id 0x" << std::setfill( '0' ) << std::setw( 2 ) << int(id) << std::dec << " pad " << num_pads << " extra sz " << extra_size << std::endl;
}

struct __una_u16 { uint16_t x; } __attribute__((packed));
struct __una_u32 { uint32_t x; } __attribute__((packed));
static inline uint16_t get_unaligned_le16(const void *p)
{
	const struct __una_u16 *ptr = (const struct __una_u16 *)p;
	return ptr->x;
}

static inline uint32_t get_unaligned_le32(const void *p)
{
	const struct __una_u32 *ptr = (const struct __una_u32 *)p;
	return ptr->x;
}

static inline int
test_bit( const uint8_t *data, int bit )
{
	return (data[bit >> 3] >> (bit & 7)) & 1;
}

} // namespace USB

#endif // _usbpp_Util_h_


