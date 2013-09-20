
USBPP = Library( 'usbpp',
                 Compile( 'Exception.cpp',
                          'Transfer.cpp',
                          'Device.cpp',
                          'DeviceManager.cpp',
                          'UVCDevice.cpp' ), USBX )

Executable( 'hid_info', Compile( 'hid_info.cpp' ) )
Executable( 'usb_neximage', Compile( 'usb_neximage.cpp' ), USBPP, USBX )
if JPEG:
    Executable( 'usb_spacepro', Compile( 'usb_spacepro.cpp', 'SpacePilotDevice.cpp' ), USBPP, USBX, JPEG )

