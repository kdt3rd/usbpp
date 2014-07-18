// Control.h -*- C++ -*-

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
#ifndef _usb_Control_h_
#define _usb_Control_h_ 1

#include <libusb-1.0/libusb.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <iostream>
#include "Transfer.h"


////////////////////////////////////////


///
/// @file Control.h
///
/// @author Kimball Thurston
///

namespace USB
{

///
/// @brief Class Control provides a class for UVC controls...
///
class Control
{
public:
	Control( void );
	~Control( void );

	void init( std::string name, libusb_device_handle *handle, uint8_t endpointNum, uint8_t unit, uint8_t iface, uint16_t term, libusb_context *ctxt );

	const std::string &name( void ) const { return myName; }

	bool valid( void ) const { return myLength > 0; }
	bool read_only( void ) const { return myReadOnly; }

	size_t size( void ) const { return myLength; }

	void get( uint8_t *buf, uint8_t N ) const;
	// convenience utils for value-based settable controls (not read-only)
	uint32_t get( void ) const;
	uint32_t min( void ) const { return myMin; }
	uint32_t max( void ) const { return myMax; }
	uint32_t set( uint32_t val );
	uint32_t delta( int d );

	void coalesce( void );
	uint32_t update( void );

	void print( std::ostream &os ) const;

private:
	// make sure people use pointers to us and aren't
	// copying controls around so any transfers
	// are managed properly
	Control( const Control & ) = delete;
	Control &operator=( const Control & ) = delete;

	uint32_t doSetInternal( uint32_t val );
	template <typename T>
	int doGet( uint8_t type, T *buf, uint16_t N );

	std::string myName;
	libusb_context *myContext = nullptr;
	libusb_device_handle *myHandle = nullptr;
	// so we can properly cleanup....
	std::mutex myMutex;
	std::vector<std::shared_ptr<ControlTransfer>> myActiveTransfers;

	uint8_t myEndpoint = 0;
	uint8_t myUnit = 0;
	uint8_t myInterface = 0;
	uint16_t myTerminal = 0;
	uint8_t myLength = 0;
	bool myReadOnly = true;

	uint8_t myRawData[256];

	uint32_t myMin = 0;
	uint32_t myMax = 0;
};

std::ostream &operator<<( std::ostream &os, const Control &ctrl );

} // namespace usb

#endif // _usb_Control_h_


