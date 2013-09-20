EnableModule( "cc" )
UseCXX11();

USBX = FindExternalLibrary( "libusb-1.0" )
if USBX is not None:
    Info( "libusb version: %s" % USBX.version )
else:
    Error( "libusb not found" )

JPEG = FindExternalLibrary( "jpeg" )
if JPEG is not None:
    Info( "JPEG version: %s" % JPEG.version )

BuildConfig( "debug", "Debug" )
BuildConfig( "build", "Build", True )
BuildConfig( "release", "Release" )

