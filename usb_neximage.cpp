// usb_neximage.cpp -*- C++ -*-

//
// Copyright (c) 2013 Kimball Thurston
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

#include "DeviceManager.h"
#include "Exception.h"
#include "UVCDevice.h"

#include <signal.h>


////////////////////////////////////////


static const uint16_t kCelestron = 0x199e;
static const uint16_t kNexImage5 = 0x8207;
static USB::DeviceManager *theMgr = nullptr;


////////////////////////////////////////


static void
sighandler( int signum )
{
	if ( theMgr )
		theMgr->quit();
}


////////////////////////////////////////


int
main( int argc, char *argv[] )
{
	struct sigaction sigact;
	int rval = 0;

	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);

	using namespace USB;

	DeviceManager mgr;
	theMgr = &mgr;
	try
	{
		mgr.registerDevice( kCelestron, kNexImage5,
							&UVCDevice::factory );

		mgr.start();

		std::shared_ptr<Device> neximage = mgr.findDevice( kCelestron, kNexImage5 );
		if ( neximage )
		{
			UVCDevice *dev = reinterpret_cast<UVCDevice *>( neximage.get() );
			std::cout << "Querying values:" << std::endl;
			dev->queryValues();
			std::cout << "Starting Video:" << std::endl;
			dev->startVideo();
		}
		mgr.processEventLoop();
	}
	catch ( usb_error &e )
	{
		int err = e.getUSBError();
		libusb_error ec = (libusb_error)( err );
		std::cerr << "ERROR: " << libusb_error_name( err ) << " - " <<
			libusb_strerror( ec ) << std::endl;
		rval = -1;
	}
	catch ( ... )
	{
		rval = -1;
	}

	mgr.shutdown();
	theMgr = nullptr;

	return rval;
}


