
USBPP = Library( 'usbpp',
                 Compile( 'Control.cpp',
                          'Exception.cpp',
                          'Logger.cpp',
                          'Stream.cpp',
                          'Transfer.cpp',
                          'Device.cpp',
                          'DeviceManager.cpp',
                          'HIDDevice.cpp',
                          'ORBOptronixDevice.cpp',
                          'UVCDevice.cpp' ), USBX )

Executable( 'find_devices', Compile( 'find_devices.cpp' ), USBPP, USBX )
if sys.platform.startswith( 'linux' ):
    Executable( 'hid_info', Compile( 'hid_info.cpp' ) )
# Executable( 'usb_neximage', Compile( 'usb_neximage.cpp' ), USBPP, USBX )
# if JPEG:
#     Executable( 'usb_spacepro', Compile( 'usb_spacepro.cpp', 'SpacePilotDevice.cpp' ), USBPP, USBX, JPEG )
#
# if GLFW:
#     Executable( 'tellytubby', Compile( 'glfw.cpp' ), USBPP, USBX, GLFW, TIFF )
#     if sys.platform == 'linux':
#         for e in GetTarget( 'exe', 'tellytubby' ).dependencies:
#             e.add_to_variable( 'libs', [ "-lGL" ] )
# Executable( 'combinomatic', Compile( 'combinomatic.cpp' ), TIFF )
