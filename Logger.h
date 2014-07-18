// Logger.h -*- C++ -*-

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

#include <functional>
#include <sstream>
#include <iostream>


////////////////////////////////////////


namespace USB
{

///
/// @brief Class Logger provides...
///

enum class NotificationType
{
	INFO,
	WARNING,
	ERROR,
	UNKNOWN
};

typedef std::function<void ( NotificationType, const std::string &msg )> NotificationFunction;
void set_log_callback( const NotificationFunction &cb );

class notification
{
public:
	notification( NotificationType nt );
	~notification( void );

	void send( void );

	template <typename T>
	notification &operator<<( const T &v )
	{
		init();
		(*myStream) << v;
		return *this;
	}

	notification &operator<<( notification &(*)(notification &) )
	{
		send();
		return *this;
	}

private:
	void init( void );

	NotificationType myType = NotificationType::UNKNOWN;
	std::stringstream *myStream = nullptr;
};

notification info( void );
notification warning( void );
notification error( void );

inline notification &
send( notification &n )
{
	return n;
}


} // namespace usb



