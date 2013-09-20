// DeviceManager.cpp -*- C++ -*-

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

#include "DeviceManager.h"
#include "Exception.h"
#include <mutex>


////////////////////////////////////////


namespace
{

} // empty namespace


////////////////////////////////////////


namespace USB
{


////////////////////////////////////////


DeviceManager::DeviceManager( void )
{
}


////////////////////////////////////////


DeviceManager::~DeviceManager( void )
{
}


////////////////////////////////////////


void
DeviceManager::registerDevice( uint16_t vendor, uint16_t prod,
							   const FactoryFunction &factory )
{
	std::lock_guard<std::mutex> lk( myMutex );
	mySpecificFactories[std::make_pair(vendor, prod)] = factory;
}


////////////////////////////////////////


void
DeviceManager::registerClass( uint8_t classID, const FactoryFunction &factory )
{
	myClassFactories[classID] = factory;
}


////////////////////////////////////////


void
DeviceManager::start( void )
{
	std::cout << "Starting device manager..." << std::endl;
	{
		std::lock_guard<std::mutex> lk( myMutex );
		
		if ( myContext )
			return;

		libusb_init( &myContext );
	}
	
//		libusb_set_debug( myContext, LIBUSB_LOG_LEVEL_DEBUG );
	if ( libusb_has_capability( LIBUSB_CAP_HAS_HOTPLUG ) )
	{
		libusb_hotplug_callback_handle plugHandle = 0;
		libusb_hotplug_event ev = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED;
		ev = static_cast<libusb_hotplug_event>( ev | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT );
		check_error(
			libusb_hotplug_register_callback( myContext,
											  ev,
											  LIBUSB_HOTPLUG_ENUMERATE,
											  LIBUSB_HOTPLUG_MATCH_ANY,
											  LIBUSB_HOTPLUG_MATCH_ANY,
											  LIBUSB_HOTPLUG_MATCH_ANY,
											  &DeviceManager::hotplug_cb,
											  this,
											  &plugHandle )
					);

		// can't have the lock when we register, as it runs a probe right away
		std::lock_guard<std::mutex> lk( myMutex );
		
		myPlugHandle = plugHandle;
	}
	else
	{
		std::cout << "WARNING: No built-in hotplug support, you must monitor and probe devices manually" << std::endl;
		probeDevices();
	}

}


////////////////////////////////////////


void
DeviceManager::shutdown( void )
{
	std::lock_guard<std::mutex> lk( myMutex );

	std::vector<libusb_device *> devs;
	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		devs.push_back( i->first );
	myDevices.clear();
	for ( libusb_device *d: devs )
		libusb_unref_device( d );

	if ( myPlugHandle != 0 )
		libusb_hotplug_deregister_callback( myContext, myPlugHandle );

	if ( myContext )
		libusb_exit( myContext );
	myPlugHandle = 0;
	myContext = nullptr;
}


////////////////////////////////////////


void
DeviceManager::probeDevices( void )
{
	std::lock_guard<std::mutex> lk( myMutex );

	std::map<libusb_device *, std::shared_ptr<Device>> oldDevs = myDevices;
	myDevices.clear();

	libusb_device **list = nullptr;
	libusb_device *found = nullptr;
	ssize_t cnt = libusb_get_device_list( myContext, &list );
	check_error( cnt );

	try
	{
		for ( ssize_t i = 0; i < cnt; ++i )
		{
			libusb_device *device = list[i];

			auto devExist = oldDevs.find( device );
			if ( devExist == oldDevs.end() )
				add( device );
			else
			{
				std::cout << "Device already exists, re-using..." << std::endl;
				myDevices[device] = devExist->second;
				oldDevs.erase( devExist );
			}
		}

		libusb_free_device_list( list, 1 );
	}
	catch ( ... )
	{
		libusb_free_device_list( list, 1 );
		throw;
	}

	if ( ! oldDevs.empty() )
	{
		std::vector<libusb_device *> devs;
		for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
			devs.push_back( i->first );
		oldDevs.clear();
		for ( libusb_device *d: devs )
			libusb_unref_device( d );
	}
}


////////////////////////////////////////


std::shared_ptr<Device>
DeviceManager::findDevice( uint16_t v, uint16_t p )
{
	std::lock_guard<std::mutex> lk( myMutex );

	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v, p ) )
			return i->second;

	return std::shared_ptr<Device>();
}


////////////////////////////////////////


std::vector<std::shared_ptr<Device>>
DeviceManager::findAllDevices( uint16_t v, uint16_t p )
{
	std::vector<std::shared_ptr<Device>> retval;
	std::lock_guard<std::mutex> lk( myMutex );

	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v, p ) )
			retval.push_back( i->second );

	return retval;
}


////////////////////////////////////////


void
DeviceManager::add( libusb_device *dev )
{
	struct libusb_device_descriptor dev_desc;
	check_error( libusb_get_device_descriptor( dev, &dev_desc ) );

	add( dev, dev_desc );
}


////////////////////////////////////////


void
DeviceManager::add( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	std::pair<uint16_t, uint16_t> devId = std::make_pair( desc.idVendor, desc.idProduct );

	std::lock_guard<std::mutex> lk( myMutex );

	std::shared_ptr<Device> newDev;

	auto f = mySpecificFactories.find( devId );
	if ( f != mySpecificFactories.end() )
		newDev = f->second( dev, desc );
	else
	{
		auto c = myClassFactories.find( desc.bDeviceClass );
		if ( c != myClassFactories.end() )
			newDev = c->second( dev, desc );
	}

	if ( newDev )
	{
		myDevices[dev] = newDev;
		libusb_ref_device( dev );

		newDev->dumpInfo( std::cout );
		newDev->claimInterfaces();
		newDev->startEventHandling();
	}
}


////////////////////////////////////////


void
DeviceManager::remove( libusb_device *dev )
{
	std::lock_guard<std::mutex> lk( myMutex );
	auto i = myDevices.find( dev );
	if ( i != myDevices.end() )
	{
		i->second->stopEventHandling();
		myDevices.erase( i );
		libusb_unref_device( dev );
	}
}


////////////////////////////////////////


void
DeviceManager::processEventLoop( void )
{
	{
		std::lock_guard<std::mutex> lk( myMutex );
		myQuitFlag = false;
	}
	
	while ( true )
	{
		{
			std::lock_guard<std::mutex> lk( myMutex );
			if ( myQuitFlag )
				break;
		}

		struct timeval tv = { 100, 0 };
		int ec = libusb_handle_events_timeout( myContext, &tv );
		if ( ec == LIBUSB_ERROR_INTERRUPTED || ec == LIBUSB_ERROR_TIMEOUT )
			continue;

		try
		{
			check_error( ec );
		}
		catch( ... )
		{
		}
	}

	std::cout << "Stopping USB device event loop..." << std::endl;
	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
	{
		i->second->stopEventHandling();
	}
	// Drain the cancelled events...

	do
	{
		struct timeval tv = { 0, 10 };
		int ec = libusb_handle_events_timeout( myContext, &tv );
	} while ( false );
}


////////////////////////////////////////


void
DeviceManager::quit( void )
{
	myQuitFlag = true;
}


////////////////////////////////////////


int
DeviceManager::hotplug_cb( struct libusb_context *ctx, struct libusb_device *dev,
						   libusb_hotplug_event event, void *user_data )
{
	DeviceManager *devMgr = reinterpret_cast<DeviceManager *>( user_data );

	if ( devMgr->isQuit() )
		return LIBUSB_SUCCESS;

	int rc = LIBUSB_SUCCESS;
	try
	{
		if ( LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event )
			devMgr->add( dev );
		else if ( LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event )
			devMgr->remove( dev );
		else
		{
			std::cerr << "ERROR: Unhandled hotplug event: " << int(event) << std::endl;
		}
	}
	catch ( usb_error &e )
	{
		rc = e.getUSBError();
		libusb_error ec = (libusb_error)( rc );
		std::cerr << "ERROR: " << libusb_error_name( rc ) << " - " <<
			libusb_strerror( ec ) << std::endl;
	}
	catch ( ... )
	{
		rc = LIBUSB_ERROR_OTHER;
	}

	return rc;
}


////////////////////////////////////////


} // USB



