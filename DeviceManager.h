// DeviceManager.h -*- C++ -*-

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

#include <cstdint>
#include <functional>
#include <memory>
#include <map>
#include <mutex>
#include <libusb-1.0/libusb.h>
#include "Device.h"


////////////////////////////////////////


///
/// @file DeviceManager.h
///
/// @author Kimball Thurston
///

namespace USB
{



////////////////////////////////////////


class DeviceManager
{
public:
	typedef std::function<std::shared_ptr<Device> (libusb_device *, const struct libusb_device_descriptor &)> FactoryFunction;

	DeviceManager( void );
	~DeviceManager( void );

	// Register any devices you care about prior to starting
	// the manager
	void registerDevice( uint16_t vendor, uint16_t prod, const FactoryFunction &factory );
	// Generic class handler
	void registerClass( uint8_t classID, const FactoryFunction &factory );
	void start( void );
	void shutdown( void );

	bool needDeviceProbe( void );
	void probeDevices( void );

	// NB: will only return the first instance of a particular device,
	// Given that this is the normal case (people don't have multiple of
	// the same device usually), we provide that as a convenience
	// NB #2: You will get un-defined behavior if you keep a pointer to a
	// device after you call shutdown
	std::shared_ptr<Device> findDevice( uint16_t vendor, uint16_t prod );

	// Gets all the devices with the specified vendor/prod
	// NB: You will get un-defined behavior if you keep a pointer to a
	// device after you call shutdown
	std::vector<std::shared_ptr<Device>> findAllDevices( uint16_t vendor, uint16_t prod );

	// async signal safe, can be called from signal handlers to
	// terminate event loop
	void quit( void );
	inline bool isQuit( void ) const { return myQuitFlag; }

	// blocks until quit is called, processing events from the USB devices
	void processEventLoop( void );

private:
	void dispatchEvent( libusb_transfer *xfer );

	void add( libusb_device *dev );
	void add( libusb_device *dev, const struct libusb_device_descriptor &desc );
	void remove( libusb_device *dev );

	static int hotplug_cb( struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event, void *user_data );

	libusb_context *myContext = nullptr;
	libusb_hotplug_callback_handle myPlugHandle = 0;
	std::mutex myMutex;
	std::map< libusb_device *, std::shared_ptr<Device> > myDevices;
	bool myQuitFlag = false;
	std::map<std::pair<uint16_t, uint16_t>, FactoryFunction> mySpecificFactories;
	std::map<uint8_t, FactoryFunction> myClassFactories;
};

} // namespace usbpp


