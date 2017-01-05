//
// Copyright (c) 2017 Kimball Thurston
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
#include "HIDDevice.h"
#include "TangentWaveDevice.h"

#include "ColorState.h"

#include <signal.h>
#include <mutex>
#include <condition_variable>


////////////////////////////////////////


using namespace USB;

static std::vector<std::shared_ptr<Device>> theDevs;
ColorState *myState = nullptr;

////////////////////////////////////////


static void
addDevice( const std::shared_ptr<Device> &dev )
{
	if ( dev )
	{
		theDevs.push_back( dev );

		TangentWaveDevice *twPtr = dynamic_cast<TangentWaveDevice *>( dev.get() );
		if ( twPtr )
		{
			twPtr->clearText();
			twPtr->resetCallbacks();
			if ( myState )
				myState->addDevice( twPtr );
		}
	}
}


////////////////////////////////////////


static void
removeDevice( const std::shared_ptr<USB::Device> &dev )
{
	if ( dev )
	{
		if ( myState )
			myState->removeDevice();

		for ( auto i = theDevs.begin(); i != theDevs.end(); ++i )
		{
			if ( (*i) == dev )
			{
				std::cout << "Device " << dev->getProductName() << " removed" << std::endl;
				theDevs.erase( i );
				return;
			}
		}
	}
}


////////////////////////////////////////


static bool theQuit = false;
static std::mutex theQuitMutex;
static std::condition_variable theQuitCond;

static void
sighandler( int signum )
{
	std::unique_lock<std::mutex> lk( theQuitMutex );
	theQuit = true;
	theQuitCond.notify_all();
}


////////////////////////////////////////


static void
processLoop( void )
{
	while ( true )
	{
		std::unique_lock<std::mutex> lk( theQuitMutex );
		theQuitCond.wait( lk );
		if ( theQuit )
			break;
	}
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

	ColorState ms;
	myState = &ms;

	DeviceManager mgr;

	try
	{
		// hrm, this doesn't seem to work, the device reports a class
		// of 0xfe
//		mgr.registerClass( LIBUSB_CLASS_VIDEO, &UVCDevice::factory );
//		mgr.registerClass( LIBUSB_CLASS_IMAGE, &UVCDevice::factory );
//		mgr.registerVendor( kCelestron, &UVCDevice::factory );
//		mgr.registerDevice( kCelestron, kNexImage5, &UVCDevice::factory );
		mgr.registerDevice( 0x04d8, 0xfdcf, &TangentWaveDevice::factory );
//		mgr.registerDevice( 0x0403, 0xac60, &ORBOptronixDevice::factory );
		mgr.start( addDevice, removeDevice );

		processLoop();

		std::cout << "\nexiting..." << std::endl;
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

	theDevs.clear();
	mgr.shutdown();

	return rval;
}
