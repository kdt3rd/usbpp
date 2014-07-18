EnableModule( "cc" )
UseCXX11();

USBX = FindExternalLibrary( "libusb-1.0" )
if USBX is not None:
    Info( "libusb version: %s" % USBX.version )
else:
    Error( "libusb not found" )

GLFW = FindExternalLibrary( "glfw3" )
if GLFW is not None:
    Info( "GLFW version: %s" % GLFW.version )
else:
    Error( "GLFW not found" )

TIFF = FindExternalLibrary( "tiff" )
if TIFF is not None:
    Info( "TIFF version: %s" % TIFF.version )

JPEG = FindExternalLibrary( "jpeg" )
if JPEG is not None:
    Info( "JPEG version: %s" % JPEG.version )

BuildConfig( "debug", "Debug" )
BuildConfig( "build", "Build", True )
BuildConfig( "release", "Release" )

