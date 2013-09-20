/*
 * Hidraw Userspace Example
 *
 * Copyright (c) 2010 Alan Ott <alan@signal11.us>
 * Copyright (c) 2010 Signal 11 Software
 *
 * The code may be used by anyone for any purpose,
 * and can serve as a starting point for developing
 * applications using hidraw.
 */

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

const char *usage_page( uint16_t page )
{
	if ( page >= 0xFF00 )
		return "Vendor-defined";

	switch ( page )
	{
		case 0x00: return "Undefined";
		case 0x01: return "Generic Desktop Controls";
		case 0x02: return "Simulation Controls";
		case 0x03: return "VR Controls";
		case 0x04: return "Sport Controls";
		case 0x05: return "Game Controls";
		case 0x06: return "Generic Device Controls";
		case 0x07: return "Keyboard / Keypad";
		case 0x08: return "LEDs";
		case 0x09: return "Button";
		case 0x0A: return "Ordinal";
		case 0x0B: return "Telephony";
		case 0x0C: return "Consumer";
		case 0x0D: return "Digitizer";
		case 0x0F: return "PID Page";
		case 0x10: return "Unicode";
		case 0x14: return "Alphanumeric Display";
		case 0x40: return "Medical Instruments";
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83: return "Monitor Pages";
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87: return "Power Pages";
		case 0x8C: return "Bar Code Scanner Page";
		case 0x8D: return "Scale Page";
		case 0x8F: return "Reserved Point of Sale pages";
		case 0x90: return "Camera Control Page";
		case 0x91: return "Arcade Page";

		default:
			break;
	}
	return "Reserved";
}

static const char *
usage_generic_desktop( uint16_t usage )
{
	switch ( usage )
	{
		case 0x00: return "Undefined";
		case 0x01: return "Pointer [CP]";
		case 0x02: return "Mouse [CA]";
		case 0x04: return "Joystick [CA]";
		case 0x05: return "Game Pad [CA]";
		case 0x06: return "Keyboard [CA]";
		case 0x07: return "Keypad [CA]";
		case 0x08: return "Multi-axis Controller [CA]";
		case 0x30: return "X [DV]";
		case 0x31: return "Y [DV]";
		case 0x32: return "Z [DV]";
		case 0x33: return "Rx [DV]";
		case 0x34: return "Ry [DV]";
		case 0x35: return "Rz [DV]";
		case 0x36: return "Slider [DV]";
		case 0x37: return "Dial [DV]";
		case 0x38: return "Wheel [DV]";
		case 0x39: return "Hat switch [DV]";
		case 0x3A: return "Counted Buffer [CL]";
		case 0x3B: return "Byte Count [DV]";
		case 0x3C: return "Motion Wakeup [OSC]";
		case 0x3D: return "Start [OOC]";
		case 0x3E: return "Select [OOC]";
		case 0x40: return "Vx [DV]";
		case 0x41: return "Vy [DV]";
		case 0x42: return "Vz [DV]";
		case 0x43: return "Vbrx [DV]";
		case 0x44: return "Vbry [DV]";
		case 0x45: return "Vbrz [DV]";
		case 0x46: return "Vno [DV]";
		case 0x47: return "Feature Notification [DV,DF]";
		case 0x48: return "Resolution Multiplier [DV]";
		case 0x80: return "System Control [CA]";
		case 0x81: return "System Power Down [OSC]";
		case 0x82: return "System Sleep [OSC]";
		case 0x83: return "System Wake Up [OSC]";
		case 0x84: return "System Context Menu [OSC]";
		case 0x85: return "System Main Menu [OSC]";
		case 0x86: return "System App Menu [OSC]";
		case 0x87: return "System Menu Help [OSC]";
		case 0x88: return "System Menu Exit [OSC]";
		case 0x89: return "System Menu Select [OSC]";
		case 0x8A: return "System Menu Right [RTC]";
		case 0x8B: return "System Menu Left [RTC]";
		case 0x8C: return "System Menu Up [RTC]";
		case 0x8D: return "System Menu Down [RTC]";
		case 0x8E: return "System Cold Restart [OSC]";
		case 0x8F: return "System Warm Restart [OSC]";
		case 0x90: return "D-pad Up [OOC]";
		case 0x91: return "D-pad Down [OOC]";
		case 0x92: return "D-pad Right [OOC]";
		case 0x93: return "D-pad Left [OOC]";
		case 0xA0: return "System Dock [OSC]";
		case 0xA1: return "System Undock [OSC]";
		case 0xA2: return "System Setup [OSC]";
		case 0xA3: return "System Break [OSC]";
		case 0xA4: return "System Debugger Break [OSC]";
		case 0xA5: return "Application Break [OSC]";
		case 0xA6: return "Application Debugger Break [OSC]";
		case 0xA7: return "System Speaker Mute [OSC]";
		case 0xA8: return "System Hibernate [OSC]";
		case 0xB0: return "System Display Invert [OSC]";
		case 0xB1: return "System Display Internal [OSC]";
		case 0xB2: return "System Display External [OSC]";
		case 0xB3: return "System Display Both [OSC]";
		case 0xB4: return "System Display Dual [OSC]";
		case 0xB6: return "System Display Swap Primary/Secondary [OSC]";
		case 0xB7: return "System Display LCD Autoscale [OSC]";
			break;
		default:
			break;
	}
	return "Reserved";
}


static const char *
usage_leds( uint16_t usage )
{
	switch ( usage )
	{
		case 0x00: return "Undefined";
		case 0x01: return "Num Lock [OOC]";
		case 0x02: return "Caps Lock [OOC]";
		case 0x03: return "Scroll Lock [OOC]";
		case 0x04: return "Compose [OOC]";
		case 0x05: return "Kana [OOC]";
		case 0x06: return "Power [OOC]";
		case 0x07: return "Shift [OOC]";
		case 0x08: return "Do Not Disturb [OOC]";
		case 0x09: return "Mute [OOC]";
		case 0x0A: return "Tone Enable [OOC]";
		case 0x0B: return "High Cut Filter [OOC]";
		case 0x0C: return "Low Cut Filter [OOC]";
		case 0x0D: return "Equalizer Enable [OOC]";
		case 0x0E: return "Sound Field On [OOC]";
		case 0x0F: return "Sound Field Off [OOC]";
		case 0x10: return "Repeat [OOC]";
		case 0x11: return "Stereo [OOC]";
		case 0x12: return "Sampling Rate Detect [OOC]";
		case 0x13: return "Spinning [OOC]";
		case 0x14: return "CAV [OOC]";
		case 0x15: return "CLV [OOC]";
		case 0x16: return "Recording Formate Detect [OOC]";
		case 0x17: return "Off-Hook [OOC]";
		case 0x18: return "Ring [OOC]";
		case 0x1A: return "Data Mode [OOC]";
		case 0x1B: return "Battery Operation [OOC]";
		case 0x1C: return "Battery OK [OOC]";
		case 0x1D: return "Battery Low [OOC]";
		case 0x1E: return "Speaker [OOC]";
		case 0x1F: return "Head Set [OOC]";
		case 0x20: return "Hold [OOC]";
		case 0x21: return "Microphone [OOC]";
		case 0x22: return "Coverage [OOC]";
		case 0x23: return "Night Mode [OOC]";
		case 0x24: return "Send Calls [OOC]";
		case 0x25: return "Call Pickup [OOC]";
		case 0x26: return "Conference [OOC]";
		case 0x27: return "Stand-by [OOC]";
		case 0x28: return "Camera On [OOC]";
		case 0x29: return "Camera Off [OOC]";
		case 0x2A: return "On-Line [OOC]";
		case 0x2B: return "Off-Line [OOC]";
		case 0x2C: return "Busy [OOC]";
		case 0x2D: return "Ready [OOC]";
		case 0x2E: return "Paper Out [OOC]";
		case 0x2F: return "Paper Jam [OOC]";
		case 0x30: return "Remote [OOC]";
		case 0x31: return "Forward [OOC]";
		case 0x32: return "Reverse [OOC]";
		case 0x33: return "Stop [OOC]";
		case 0x34: return "Rewind [OOC]";
		case 0x35: return "Fast Forward [OOC]";
		case 0x36: return "Play [OOC]";
		case 0x37: return "Pause [OOC]";
		case 0x38: return "Record [OOC]";
		case 0x39: return "Error [OOC]";
		case 0x3A: return "Usage Selected Indicator [US]";
		case 0x3B: return "Usage In Use Indicator [US]";
		case 0x3C: return "Usage Multi Mode Indicator [UM]";
		case 0x3D: return "Indicator On [Sel]";
		case 0x3E: return "Indicator Flash [Sel]";
		case 0x3F: return "Indicator Slow Blink [Sel]";
		case 0x40: return "Indicator Fast Blink [Sel]";
		case 0x41: return "Indicator Off [Sel]";
		case 0x42: return "Flash On Time [DV]";
		case 0x43: return "Slow Blink On Time [DV]";
		case 0x44: return "Slow Blink Off Time [DV]";
		case 0x45: return "Fast Blink On Time [DV]";
		case 0x46: return "Fast Blink Off Time [DV]";
		case 0x47: return "Usage Indicator Color [UM]";
		case 0x48: return "Indicator Red [Sel]";
		case 0x49: return "Indicator Green [Sel]";
		case 0x4A: return "Indicator Amber [Sel]";
		case 0x4B: return "Generic Indicator [OOC]";
		case 0x4C: return "System Suspend [OOC]";
		case 0x4D: return "External Power Connected [OOC]";
		default:
			break;
	}
	return "Reserved";
}

const char *bus_str(int bus);

class data_item
{
public:
	data_item( void ) {}
	virtual ~data_item( void ) {}

	virtual size_t size( void ) const  = 0;
	virtual const uint8_t *data( void ) const = 0;

	virtual void print( void ) const = 0;
	virtual std::shared_ptr<data_item> clone( void ) const = 0;
};

class no_data_item : public data_item
{
public:
	no_data_item( void ) {}
	virtual ~no_data_item( void ) {}

	virtual size_t size( void ) const { return 0; }
	virtual const uint8_t *data( void ) const { return nullptr; }
	virtual void print( void ) const { std::cout << "<no data>"; }
	virtual std::shared_ptr<data_item> clone( void ) const { return std::shared_ptr<data_item>( new no_data_item ); }
};

class byte_data_item : public data_item
{
public:
	byte_data_item( uint8_t dv ) : myData( dv ) {}
	virtual ~byte_data_item( void ) {}

	virtual size_t size( void ) const { return 1; }
	virtual const uint8_t *data( void ) const { return &myData; }

	virtual void print( void ) const { std::cout << "BYTE: 0x" << std::setw(2) << std::setfill( '0' ) << std::hex << int(myData) << std::dec << " (" << int(int8_t(myData)) << ")"; }
	virtual std::shared_ptr<data_item> clone( void ) const { return std::shared_ptr<data_item>( new byte_data_item( myData ) ); }

private:
	uint8_t myData;
};

class short_data_item : public data_item
{
public:
	short_data_item( uint16_t dv ) : myData( dv ) {}
	virtual ~short_data_item( void ) {}

	virtual size_t size( void ) const { return 3; }
	virtual const uint8_t *data( void ) const { return reinterpret_cast<const uint8_t *>( &myData ); }

	virtual void print( void ) const { std::cout << "SHORT: 0x" << std::setw(4) << std::setfill( '0' ) << std::hex << int(myData) << std::dec << " (" << int(int16_t(myData)) << ")"; }
	virtual std::shared_ptr<data_item> clone( void ) const { return std::shared_ptr<data_item>( new short_data_item( myData ) ); }

private:
	uint16_t myData;
};

class word_data_item : public data_item
{
public:
	word_data_item( uint32_t dv ) : myData( dv ) {}
	virtual ~word_data_item( void ) {}

	virtual size_t size( void ) const { return 4; }
	virtual const uint8_t *data( void ) const { return reinterpret_cast<const uint8_t *>( &myData ); }

	virtual void print( void ) const { std::cout << "WORD: 0x" << std::setw(8) << std::setfill( '0' ) << std::hex << int(myData) << std::dec << " (" << int32_t(myData) << ")"; }
	virtual std::shared_ptr<data_item> clone( void ) const { return std::shared_ptr<data_item>( new word_data_item( myData ) ); }

private:
	uint32_t myData;
};

class long_data_item : public data_item
{
public:
	long_data_item( size_t l, const uint8_t *data )
			: mySize( l ), myData( new uint8_t[l] )
	{
		uint8_t *mData = myData.get();
		for ( size_t i = 0; i < mySize; ++i )
			mData[i] = data[i];
	}
	virtual ~long_data_item( void ) {}

	virtual size_t size( void ) const { return mySize; }
	virtual const uint8_t *data( void ) const { return myData.get(); }

	virtual void print( void ) const
	{
		std::cout << "LONGDATA ";
		const uint8_t *mData = myData.get();
		for ( size_t i = 0; i < mySize; ++i )
			std::cout << "0x" << std::setw(2) << std::setfill( '0' ) << std::hex << mData[i] << std::dec << " (" << int(mData[i]) << ") ";
	}
	virtual std::shared_ptr<data_item> clone( void ) const { return std::shared_ptr<data_item>( new long_data_item( mySize, myData.get() ) ); }

private:
	size_t mySize;
	std::unique_ptr<uint8_t []> myData;
};

class report_descriptor
{
public:
	report_descriptor( const std::string &type, uint8_t tag, uint8_t len, const uint8_t *data )
			: myTag( tag ), myType( type )
	{
		switch ( len )
		{
			case 0: myData.reset( new no_data_item ); break;
			case 1: myData.reset( new byte_data_item( data[0] ) ); break;
			case 2: myData.reset( new short_data_item( *( reinterpret_cast<const uint16_t *>( data ) ) ) ); break;
			case 3: myData.reset( new word_data_item( *( reinterpret_cast<const uint32_t *>( data ) ) ) ); break;
			default:
				myData.reset( new long_data_item( len, data ) );
				break;
		}
	}
	virtual ~report_descriptor( void ) {}

	virtual size_t size( void ) const { return myData->size(); }
	virtual void print( void ) const
	{
		std::cout << myType << ' ' << get_tag_name() << ' ';
		myData->print();
	}

	const data_item *data( void ) const { return myData.get(); }

	virtual std::string get_tag_name( void ) const = 0;

protected:
	uint8_t myTag;
	std::unique_ptr<data_item> myData;
	std::string myType;
};

class reserved_report_descriptor : public report_descriptor
{
public:
	reserved_report_descriptor( uint8_t data_type, uint8_t tag, uint8_t len, const uint8_t *data )
			: report_descriptor( "RESERVED", tag, len, data ), myDataType( data_type )
	{}

	virtual std::string get_tag_name( void ) const
	{
		std::stringstream retval;
		retval << "0x" << std::setw( 2 ) << std::setfill( '0' )
			   << std::hex << int(myDataType);
		return retval.str();
	}
private:
	uint8_t myDataType;
};

class main_report_descriptor : public report_descriptor
{
public:
	main_report_descriptor( uint8_t tag, uint8_t len, const uint8_t *data )
			: report_descriptor( "MAIN", tag, len, data )
	{
	}
	virtual ~main_report_descriptor( void ) {}

	virtual std::string get_tag_name( void ) const
	{
		switch ( myTag )
		{
			case 0x8: return "Input";
			case 0x9: return "Output";
			case 0xa: return "Collection";
			case 0xb: return "Feature";
			case 0xc: return "End Collection";
		}

		std::stringstream retval;
		retval << "<RESERVED 0x" << std::setw(2) << std::setfill( '0' ) << std::hex << myTag << std::dec << ">";
		return retval.str();
	}
};

class main_collection : public main_report_descriptor
{
public:
	main_collection( uint8_t tag, uint8_t len, const uint8_t *data )
			: main_report_descriptor( tag, len, data )
	{}
	virtual ~main_collection( void ) {}

	virtual void print( void ) const
	{
		std::cout << "MAIN Collection [";
		if ( myData->size() == 1 )
		{
			uint8_t v = myData->data()[0];
			switch ( v )
			{
				case 0x00: std::cout << "Physical (group of axes)"; break;
				case 0x01: std::cout << "Application (mouse, keyboard)"; break;
				case 0x02: std::cout << "Logical (interrelated data)"; break;
				case 0x03: std::cout << "Report"; break;
				case 0x04: std::cout << "Named Array"; break;
				case 0x05: std::cout << "Usage Switch"; break;
				case 0x06: std::cout << "Usage Modifier"; break;
				default:
					if ( v < 0x80 )
						std::cout << "Reserved 0x";
					else
						std::cout << "Vendor-defined ";
					std::cout << std::setw(2) << std::setfill( '0' ) << std::hex << v << std::dec;
					break;
			}
		}
		std::cout << "]";
	}
};

class main_input : public main_report_descriptor
{
public:
	main_input( uint8_t tag, uint8_t len, const uint8_t *data )
			: main_report_descriptor( tag, len, data )
	{}
	virtual ~main_input( void ) {}

	virtual void print( void ) const
	{
		std::cout << "MAIN Input [";
		switch ( myData->size() )
		{
			case 4:
			case 2:
			{
				uint8_t byte1 = myData->data()[1];
				if ( byte1 & 0x1 )
					std::cout << " Buffered Bytes |";
				else
					std::cout << " Bit Field |";
				// fallthrough
			}
			case 1:
			{
				uint8_t byte0 = myData->data()[0];
				const char *valTable[] = 
					{
						" Data", " Constant",
						" Array", " Variable",
						" Absolute", " Relative",
						" No Wrap", " Wrap",
						" Linear", " Non Linear",
						" Preferred State", " No Preferred",
						" No Null position", " Null state",
						" Bit Field", " Buffered Bytes"
					};
				const char **curVals = valTable;
				for ( int bit = 0; bit < 8; ++bit )
				{
					if ( bit == 7 )
					{
						byte0 >>= 1;
						continue;
					}
					if ( bit > 0 )
						std::cout << " |";
					std::cout << curVals[(byte0 & 0x1)];
					curVals += 2;
				}
				break;
			}
			default:
				std::cout << " ERROR <Invalid Data Size>";
		}
		std::cout << " ]";
	}
};


class main_output : public main_report_descriptor
{
public:
	main_output( const std::string &which, uint8_t tag, uint8_t len, const uint8_t *data )
			: main_report_descriptor( tag, len, data ), myOutType( which )
	{}
	virtual ~main_output( void ) {}

	virtual void print( void ) const
	{
		std::cout << "MAIN " << myOutType << " [";
		switch ( myData->size() )
		{
			case 4:
			case 2:
			{
				uint8_t byte1 = myData->data()[1];
				if ( byte1 & 0x1 )
					std::cout << " Buffered Bytes |";
				else
					std::cout << " Bit Field |";
				// fallthrough
			}
			case 1:
			{
				uint8_t byte0 = myData->data()[0];
				const char *valTable[] = 
					{
						" Data", " Constant",
						" Array", " Variable",
						" Absolute", " Relative",
						" No Wrap", " Wrap",
						" Linear", " Non Linear",
						" Preferred State", " No Preferred",
						" No Null position", " Null state",
						" Non Volatile", " Volatile",
						" Bit Field", " Buffered Bytes"
					};
				const char **curVals = valTable;
				for ( int bit = 0; bit < 8; ++bit )
				{
					if ( bit > 0 )
						std::cout << " |";
					std::cout << curVals[(byte0 & 0x1)];
					curVals += 2;
				}
				break;
			}
			default:
				std::cout << " ERROR <Invalid Data Size>";
		}
		std::cout << " ]";
	}
private:
	std::string myOutType;
};

class global_report_descriptor : public report_descriptor
{
public:
	global_report_descriptor( uint8_t tag, uint8_t len, const uint8_t *data )
			: report_descriptor( "GLOBAL", tag, len, data )
	{
	}
	virtual ~global_report_descriptor( void ) {}

	virtual std::string get_tag_name( void ) const
	{
		switch ( myTag )
		{
			case 0x0: return "Usage Page";
			case 0x1: return "Logical Minimum";
			case 0x2: return "Logical Maximum";
			case 0x3: return "Physical Minimum";
			case 0x4: return "Physical Maximum";
			case 0x5: return "Unit Exponent";
			case 0x6: return "Unit";
			case 0x7: return "Report Size";
			case 0x8: return "Report ID";
			case 0x9: return "Report Count";
			case 0xa: return "Push";
			case 0xb: return "Pop";
		}

		std::stringstream retval;
		retval << "<RESERVED 0x" << std::setw(2) << std::setfill( '0' ) << std::hex << myTag << std::dec << ">";
		return retval.str();
	}
};


class local_report_descriptor : public report_descriptor
{
public:
	local_report_descriptor( uint8_t tag, uint8_t len, const uint8_t *data )
			: report_descriptor( "LOCAL", tag, len, data )
	{
	}
	virtual ~local_report_descriptor( void ) {}

	virtual std::string get_tag_name( void ) const
	{
		switch ( myTag )
		{
			case 0x0: return "Usage";
			case 0x1: return "Usage Minimum";
			case 0x2: return "Usage Maximum";
			case 0x3: return "Designator Index";
			case 0x4: return "Designator Minimum";
			case 0x5: return "Designator Maximum";
			case 0x7: return "String Index";
			case 0x8: return "String Minimum";
			case 0x9: return "String Maximum";
			case 0xa: return "Report Count";
		}

		std::stringstream retval;
		retval << "<RESERVED 0x" << std::setw(2) << std::setfill( '0' ) << std::hex << myTag << std::dec << ">";
		return retval.str();
	}
};

class usage : public local_report_descriptor
{
public:
	usage( const std::shared_ptr<report_descriptor> &usagePage, uint8_t tag,
		   uint8_t len, const uint8_t *data )
			: local_report_descriptor( tag, len, data )
	{
		uint8_t lsbPage = 0;
		uint8_t msbPage = 0;

		uint8_t lsb = 0;
		uint8_t msb = 0;

		uint16_t page = 0;
		uint16_t usage = 0;
		switch ( len )
		{
			case 4:
				msbPage = myData->data()[3];
				// fall
			case 3:
				lsbPage = myData->data()[2];
				page = uint16_t(lsbPage) | (uint16_t(msbPage) << 8);
				myUsagePage.reset( new global_report_descriptor(
									   0x0, 2,
									   reinterpret_cast<const uint8_t *>( &page ) ) );
				// fall
			case 2:
				msb = myData->data()[1];
			case 1:
				lsb = myData->data()[0];
				myUsage = uint16_t(lsb) | (uint16_t(msb) << 8);
				if ( ! myUsagePage )
				{
					if ( ! usagePage )
						std::cerr << "ERROR: Invalid Missing (Global) Usage Page" << std::endl;
					myUsagePage = usagePage;
				}
				break;
			default:
				std::cerr << "ERROR: Invalid Usage Data Size" << std::endl;
				break;
		}
	}

	virtual void print( void ) const
	{
		uint16_t page = *( reinterpret_cast<const uint16_t *>( myUsagePage->data()->data() ) );
		const char *(*get_usage_func)( uint16_t ) = nullptr;
		switch ( page )
		{
			case 0x01: get_usage_func = usage_generic_desktop; break;
			case 0x08: get_usage_func = usage_leds; break;
			default:
				break;
		}
		if ( get_usage_func )
		{
			std::cout << "LOCAL Usage [" << usage_page( page ) << ":"
					  << get_usage_func( myUsage ) << " ]";
		}
		else
		{
			std::cout << "LOCAL Usage [" << usage_page( page ) << ": 0x"
					  << std::setw(4) << std::setfill( '0' ) << std::hex
					  << myUsage << std::dec << " ]";
		}
	}

private:
	uint16_t myUsage;
	std::shared_ptr<report_descriptor> myUsagePage;
};

class collection
{
};

int
main(int argc, char **argv)
{
	int fd;
	int i, res, desc_size = 0;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;
	fd_set fds;

	if ( argc < 2 )
	{
		printf( "usage: %s <devname>\n\n", argv[0] );
		return -1;
	}
	/* Open the Device with non-blocking reads. In real life,
	   don't use a hard coded path; use libudev instead. */
	fd = open(argv[1], O_RDWR|O_NONBLOCK);

	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	memset(&info, 0x0, sizeof(info));
	memset(buf, 0x0, sizeof(buf));

	/* Get Report Descriptor Size */
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		perror("HIDIOCGRDESCSIZE");
	else
		printf("Report Descriptor Size: %d\n", desc_size);

	std::map< int, std::shared_ptr<report_descriptor> > curGlobals;
	std::map< int, std::shared_ptr<report_descriptor> > curLocals;

	/* Get Report Descriptor */
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0) {
		perror("HIDIOCGRDESC");
	} else {
		int key_size, data_tag, data_type;
		unsigned int i = 0, data_len, j, cur_indent = 0;
		const uint8_t *curItem = rpt_desc.value;

		while ( i < rpt_desc.size )
		{
			int key = curItem[0];
			data_len = (key & 0x03);
			data_type = (key & 0x0c) >> 2;
			data_tag = (key & 0xf0) >> 4;
			key_size = 1;
			if ( data_tag == 0xf )
			{
				// long item
				if ( i+2 < rpt_desc.size )
				{
					printf( "Malformed report, long data does not have enough data\n" );
					exit( -1 );
				}
				data_len = rpt_desc.value[i+1];
				data_tag = rpt_desc.value[i+2];
				key_size = 3;
			}
			else
			{
				// len is a code really (per usb spec):
				// code 0: size 0
				// code 1: size 1
				// code 2: size 2
				// code 3: size 4
				if ( data_len == 3 )
					data_len = 4;
			}
			const uint8_t *dataPtr = curItem + key_size;

			for ( int ind = 0; ind < cur_indent; ++ind )
				std::cout << "  ";

			std::unique_ptr<report_descriptor> curDesc;
			switch ( data_type )
			{
				case 0: // MAIN
					switch ( data_tag )
					{
						case 0x8:
							curDesc.reset( new main_input( data_tag, data_len, dataPtr ) );
							break;
						case 0x9:
							// output
							curDesc.reset( new main_output( "Output", data_tag, data_len, dataPtr ) );
							break;
						case 0xa:
							curDesc.reset( new main_collection( data_tag, data_len, dataPtr ) );
							++cur_indent;
							break;
						case 0xb:
							// feature
							curDesc.reset( new main_output( "Feature", data_tag, data_len, dataPtr ) );
							break;
						case 0xc:
							--cur_indent;
							std::cout << "[END COLLECTION] --> Create Complete Description" << std::endl;
							break;
						default:
							
							break;
					}
					if ( curDesc )
					{
						curDesc->print();
						std::cout << std::endl;
					}
					break;
				case 1: // GLOBAL
					curGlobals[data_tag].reset( new global_report_descriptor( data_tag, data_len, dataPtr ) );
					curGlobals[data_tag]->print();
					std::cout << std::endl;
					switch ( data_tag )
					{
						case 0x0: // Usage Page
						case 0x1: // Logical Min
						case 0x2: // Logical Max
						case 0x3: // Physical Min
						case 0x4: // Physical Max
						case 0x5: // Unit Exponent
						case 0x6: // Unit
						case 0x7: // Report Size
						case 0x8: // Report ID
						case 0x9: // Report Count
						case 0xa: // Push
						case 0xb: // Pop
						default:
							// Reserved, ignore
							break;
					}
					break;
				case 2: // LOCAL
				{
					std::shared_ptr<report_descriptor> &curLoc = curLocals[data_tag];
					curLoc.reset();
					switch ( data_tag )
					{
						case 0x0: // Usage
							curLoc.reset( new usage( curGlobals[0x0], data_tag, data_len, dataPtr ) );
							break;
						case 0x1: // Usage Min
						case 0x2: // Usage Max
						case 0x3: // Designator Index
						case 0x4: // Designator Min
						case 0x5: // Designator Max
						case 0x7: // String Index
						case 0x8: // String Min
						case 0x9: // String Max
						case 0xa: // Delimiter
						default:
							// Reserved, ignore
							break;
					}
					if ( ! curLoc )
						curLoc.reset( new local_report_descriptor( data_tag, data_len, dataPtr ) );
					curLoc->print();
					std::cout << std::endl;
					break;
				}

				case 3: // RESERVED
					curDesc.reset( new reserved_report_descriptor(
									   data_type, data_tag, data_len, dataPtr ) );
					curDesc->print();
					std::cout << std::endl;
					break;
				default:
					
					break;
			}
			curItem += data_len + key_size;
			i += data_len + key_size;
		}
	}

	/* Get a report from the device */
	while ( 1 )
	{
		int ret, b;
		uint8_t curbyte;
		FD_ZERO( &fds );
		FD_SET( fd, &fds );
		ret = select( fd + 1, &fds, NULL, NULL, NULL );
		if ( ret > 0 && FD_ISSET( fd, &fds ) )
		{
			res = read(fd, buf, 16);
			if (res < 0)
			{
				if ( errno == EIO )
					break;
				perror("read");
			}

			if ( res > 0 )
			{
				
				printf("READ %d bytes: report %d", res, (int)(buf[0]));
				for (i = 1; i < res; i++)
				{
					if ( i > 0 )
						printf( " " );
					curbyte = buf[i];
					for ( b = 7; b >= 0; --b )
					{
						if ( curbyte & (1 << b) )
							printf( "1" );
						else
							printf( "0" );

					}
				}
				printf("\n");
			}
		}
	}
	close(fd);
	return 0;
}

const char *
bus_str(int bus)
{
	switch (bus) {
	case BUS_USB:
		return "USB";
		break;
	case BUS_HIL:
		return "HIL";
		break;
	case BUS_BLUETOOTH:
		return "Bluetooth";
		break;
	case BUS_VIRTUAL:
		return "Virtual";
		break;
	default:
		return "Other";
		break;
	}
}
