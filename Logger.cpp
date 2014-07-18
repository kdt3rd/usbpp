// Logger.cpp -*- C++ -*-

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

#include "Logger.h"


////////////////////////////////////////


namespace {

USB::NotificationFunction theCB;

}


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


void
set_log_callback( const NotificationFunction &cb )
{
	theCB = cb;
}


////////////////////////////////////////


notification::notification( NotificationType nt )
		: myType( nt )
{
	init();
}


////////////////////////////////////////


notification::~notification( void )
{
	send();
	delete myStream;
}


////////////////////////////////////////


void
notification::send( void )
{
	try
	{
		if ( myStream == nullptr )
			return;

		std::string msg = (*myStream).str();
		if ( msg.empty() )
			return;

		if ( theCB )
			theCB( myType, msg );
		else
		{
			switch ( myType )
			{
				case NotificationType::INFO:
					std::cout << msg << std::endl;
					break;
				case NotificationType::WARNING:
					std::cout << "WARNING: " << msg << std::endl;
					break;
				case NotificationType::ERROR:
					std::cerr << "ERROR: " << msg << std::endl;
					break;
				case NotificationType::UNKNOWN:
					std::cerr << "ERROR: Unknown Notification Type " << msg << std::endl;
					break;
			}
		}

		delete myStream;
		myStream = nullptr;
	}
	catch ( ... )
	{
		std::cerr << "ERROR with notification" << std::endl;
	}
}


////////////////////////////////////////


void
notification::init( void )
{
	if ( ! myStream )
		myStream = new std::stringstream;
}


////////////////////////////////////////


notification info( void )
{
	return notification( NotificationType::INFO );
}


////////////////////////////////////////


notification warning( void )
{
	return notification( NotificationType::WARNING );
}


////////////////////////////////////////


notification error( void )
{
	return notification( NotificationType::ERROR );
}


////////////////////////////////////////


} // usb



