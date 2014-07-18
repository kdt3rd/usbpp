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
#include "Logger.h"
#include "Transfer.h"


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
	std::unique_lock<std::mutex> lk( myMutex );
	mySpecificFactories[std::make_pair(vendor, prod)] = factory;
}


////////////////////////////////////////


void
DeviceManager::registerClass( uint8_t classID, const FactoryFunction &factory )
{
	std::unique_lock<std::mutex> lk( myMutex );
	myClassFactories[classID] = factory;
}


////////////////////////////////////////


void
DeviceManager::registerVendor( uint16_t vendID, const FactoryFunction &factory )
{
	std::unique_lock<std::mutex> lk( myMutex );
	myVendorFactories[vendID] = factory;
}


////////////////////////////////////////


void
DeviceManager::start( const NewDeviceFunction &newFunc, const DeadDeviceFunction &deadFunc )
{
	std::unique_lock<std::mutex> lk( myMutex );

	myQuitFlag = false;
	myNewDeviceFunc = newFunc;
	myDeadDeviceFunc = deadFunc;

	if ( myContext )
		return;

	libusb_init( &myContext );
	
//		libusb_set_debug( myContext, LIBUSB_LOG_LEVEL_DEBUG );
	if ( libusb_has_capability( LIBUSB_CAP_HAS_HOTPLUG ) )
	{
		libusb_hotplug_callback_handle plugHandle = 0;
		libusb_hotplug_event ev = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED;
		ev = static_cast<libusb_hotplug_event>( ev | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT );

		// The callback is called prior to return from here, so temporarily
		// unlock....
		lk.unlock();
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

		lk.lock();
		myPlugHandle = plugHandle;
	}
	else
	{
		warning() << " No built-in hotplug support, we will be probing periodically" << send;
		myProbeThread = std::thread( &DeviceManager::probeLoop, this );
	}

	myEventThread = std::thread( &DeviceManager::eventLoop, this );
}


////////////////////////////////////////


void
DeviceManager::shutdown( void )
{
	std::unique_lock<std::mutex> lk( myMutex );
	myQuitFlag = true;
	
	if ( myPlugHandle != 0 )
	{
		libusb_hotplug_deregister_callback( myContext, myPlugHandle );
		myPlugHandle = 0;
	}

	if ( myProbeThread.joinable() )
	{
		myProbeNotify.notify_all();
		lk.unlock();
		myProbeThread.join();
		lk.lock();
		myProbeThread = std::thread();
	}

	if ( myEventThread.joinable() )
	{
		myEventNotify.notify_all();
		lk.unlock();
		myEventThread.join();
		lk.lock();
		myEventThread = std::thread();
	}

	std::vector<libusb_device *> devs;
	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
	{
		if ( myDeadDeviceFunc )
			myDeadDeviceFunc( i->second );
		devs.push_back( i->first );
	}
	myDevices.clear();

	for ( libusb_device *d: devs )
		libusb_unref_device( d );

	if ( myContext )
		libusb_exit( myContext );
	myContext = nullptr;
}


////////////////////////////////////////


void
DeviceManager::probeLoop( void )
{
	while ( true )
	{
		std::unique_lock<std::mutex> lk( myMutex );
		if ( myQuitFlag )
			break;

		std::map<libusb_device *, std::shared_ptr<Device>> oldDevs = myDevices;
		myDevices.clear();

		libusb_device **list = nullptr;
		ssize_t cnt = libusb_get_device_list( myContext, &list );
		check_error( static_cast<int>( cnt ) );

		if ( list )
		{
			try
			{
				for ( ssize_t i = 0; i < cnt; ++i )
				{
					libusb_device *device = list[i];

					auto devExist = oldDevs.find( device );
					if ( devExist == oldDevs.end() )
					{
						add( device );
					}
					else
					{
						info() << "Device already exists, re-using..." << send;
						myDevices[device] = devExist->second;
						oldDevs.erase( devExist );
					}
				}
			}
			catch ( ... )
			{
				warning() << "Error adding device..." << send;
			}

			libusb_free_device_list( list, 1 );
		}

		if ( ! oldDevs.empty() )
		{
			std::vector<libusb_device *> devs;
			// can't use remove since we already removed the entry from the map
			for ( auto i = oldDevs.begin(); i != oldDevs.end(); ++i )
			{
				if ( myDeadDeviceFunc )
					myDeadDeviceFunc( i->second );

				devs.push_back( i->first );
			}

			oldDevs.clear();
			for ( libusb_device *d: devs )
				libusb_unref_device( d );
		}

		int msec = 10000;
		if ( myDevices.empty() )
			msec = 250;

		myProbeNotify.wait_for( lk, std::chrono::milliseconds( msec ) );
	}
}


////////////////////////////////////////


void
DeviceManager::eventLoop( void )
{
	std::unique_lock<std::mutex> lk( myMutex );

	while ( ! myQuitFlag )
	{
		lk.unlock();

		struct timeval tv = { 0, 10000 };
		int ec = libusb_handle_events_timeout( myContext, &tv );

		switch ( ec )
		{
			case LIBUSB_SUCCESS:
			case LIBUSB_ERROR_INTERRUPTED:
			case LIBUSB_ERROR_TIMEOUT:
				break;

			default:
			{
				libusb_error err = static_cast<libusb_error>( ec );
				warning() << "Error while handling events: " << ec << " "
						  << libusb_error_name( err ) << " - "
						  << libusb_strerror( err ) << send;
				break;
			}
		}

		lk.lock();
	}

	// make sure all event handling is stopped
	for ( auto &dev: myDevices )
		dev.second->stopEventHandling();
}


////////////////////////////////////////


std::shared_ptr<Device>
DeviceManager::findDevice( uint16_t v, uint16_t p )
{
	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v, p ) )
			return i->second;

	return std::shared_ptr<Device>();
}


////////////////////////////////////////


std::shared_ptr<Device>
DeviceManager::findDevice( uint16_t v )
{
	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v ) )
			return i->second;

	return std::shared_ptr<Device>();
}


////////////////////////////////////////


std::vector<std::shared_ptr<Device>>
DeviceManager::getAllDevices( void )
{
	std::vector<std::shared_ptr<Device>> retval;

	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		retval.push_back( i->second );

	return retval;
}


////////////////////////////////////////


std::vector<std::shared_ptr<Device>>
DeviceManager::findAllDevices( uint16_t v, uint16_t p )
{
	std::vector<std::shared_ptr<Device>> retval;

	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v, p ) )
			retval.push_back( i->second );

	return retval;
}


////////////////////////////////////////


std::vector<std::shared_ptr<Device>>
DeviceManager::findAllDevices( uint16_t v )
{
	std::vector<std::shared_ptr<Device>> retval;

	for ( auto i = myDevices.begin(); i != myDevices.end(); ++i )
		if ( i->second->matches( v ) )
			retval.push_back( i->second );

	return retval;
}


////////////////////////////////////////


void
DeviceManager::add( libusb_device *dev )
{
	// caller has the lock
	struct libusb_device_descriptor dev_desc;
	check_error( libusb_get_device_descriptor( dev, &dev_desc ) );

	add( dev, dev_desc );
}


////////////////////////////////////////


void
DeviceManager::add( libusb_device *dev, const struct libusb_device_descriptor &desc )
{
	// caller has the lock
	std::pair<uint16_t, uint16_t> devId = std::make_pair( desc.idVendor, desc.idProduct );

	std::shared_ptr<Device> newDev;

	try
	{
		auto f = mySpecificFactories.find( devId );
		if ( f != mySpecificFactories.end() )
			newDev = f->second( dev, desc );

		if ( ! newDev )
		{
			auto v = myVendorFactories.find( desc.idVendor );
			if ( v != myVendorFactories.end() )
				newDev = v->second( dev, desc );
		}

		if ( ! newDev )
		{
			auto c = myClassFactories.find( desc.idVendor );
			if ( c != myClassFactories.end() )
				newDev = c->second( dev, desc );
		}
	}
	catch ( usb_error &e )
	{
		int err = e.getUSBError();
		libusb_error ec = (libusb_error)( err );
		error() << "Creating specific product device: "
				  << libusb_error_name( err ) << " - "
				  << libusb_strerror( ec ) << send;
	}
	catch ( std::exception &e )
	{
		error() << "Creating device for specific product: " << e.what() << send;
	}

	if ( newDev )
	{
		libusb_ref_device( dev );

		try
		{
			myDevices[dev] = newDev;

			newDev->setContext( myContext );
//			newDev->dumpInfo( std::cout );
			newDev->claimInterfaces();
			newDev->startEventHandling();

			if ( myNewDeviceFunc )
				myNewDeviceFunc( newDev );
		}
		catch ( usb_error &e )
		{
			int err = e.getUSBError();
			libusb_error ec = (libusb_error)( err );
			error() << "Initializing USB device: "
					  << libusb_error_name( err ) << " - "
					  << libusb_strerror( ec ) << send;

			libusb_unref_device( dev );
		}
		catch ( std::exception &e )
		{
			error() << "Initializing device: " << e.what() << send;
			libusb_unref_device( dev );
		}
	}
}


////////////////////////////////////////


void
DeviceManager::remove( libusb_device *dev )
{
	auto i = myDevices.find( dev );
	if ( i != myDevices.end() )
	{
		std::shared_ptr<Device> devPtr = i->second;
		devPtr->stopEventHandling();
		devPtr->shutdown();
		myDevices.erase( i );

		if ( myDeadDeviceFunc )
			myDeadDeviceFunc( devPtr );

		libusb_unref_device( dev );
	}
}


////////////////////////////////////////


int
DeviceManager::hotplug_cb( struct libusb_context *ctx, struct libusb_device *dev,
						   libusb_hotplug_event event, void *user_data )
{
	DeviceManager *devMgr = reinterpret_cast<DeviceManager *>( user_data );

	if ( devMgr->myQuitFlag )
		return LIBUSB_SUCCESS;

	int rc = LIBUSB_SUCCESS;
	try
	{
		std::unique_lock<std::mutex> lk( devMgr->myMutex );

		if ( LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event )
			devMgr->add( dev );
		else if ( LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event )
			devMgr->remove( dev );
		else
		{
			error() << "Unhandled hotplug event: " << int(event) << send;
		}
	}
	catch ( usb_error &e )
	{
		rc = e.getUSBError();
		libusb_error ec = (libusb_error)( rc );
		error() << libusb_error_name( rc ) << " - " << libusb_strerror( ec ) << send;
	}
	catch ( ... )
	{
		rc = LIBUSB_ERROR_OTHER;
	}

	return rc;
}


////////////////////////////////////////


} // USB



