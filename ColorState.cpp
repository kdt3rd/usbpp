//
// Copyright (c) 2016 Kimball Thurston
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

#include "ColorState.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <float.h>
#include <math.h>

void ColorState::setTrimMode( TrimMode m )
{
	mTrimMode = m;
	resetText();
}

////////////////////////////////////////

void
ColorState::addDevice( USB::TangentWaveDevice *dev )
{
	mTangent = dev;
	resetText();
	if ( mTangent )
	{
		mTangent->resetCallbacks();
		mTangent->addButtonCallback( USB::TangentButton::F7, std::bind( &ColorState::setModeCDL, this, std::placeholders::_1 ) );
		mTangent->addButtonCallback( USB::TangentButton::F8, std::bind( &ColorState::setModeSixVec, this, std::placeholders::_1 ) );
		mTangent->addButtonCallback( USB::TangentButton::F9, std::bind( &ColorState::setModeSixVecHue, this, std::placeholders::_1 ) );
		mTangent->addButtonCallback( USB::TangentButton::F4, std::bind( &ColorState::setModeSat, this, std::placeholders::_1 ) );

		mTangent->addButtonCallback( USB::TangentButton::ENCODER_LEFT_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(0) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_LEFT_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(1) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_LEFT_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(2) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_CENTER_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(3) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_CENTER_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(4) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_CENTER_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(5) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_RIGHT_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(6) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_RIGHT_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(7) ) );
		mTangent->addButtonCallback( USB::TangentButton::ENCODER_RIGHT_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(8) ) );

		mTangent->addButtonCallback( USB::TangentButton::OPEN_CIRCLE_ENCODER_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(9) ) );
		mTangent->addButtonCallback( USB::TangentButton::OPEN_CIRCLE_ENCODER_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(10) ) );
		mTangent->addButtonCallback( USB::TangentButton::OPEN_CIRCLE_ENCODER_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(11) ) );

		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_LEFT_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(0) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_LEFT_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(1) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_LEFT_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(2) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_CENTER_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(3) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_CENTER_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(4) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_CENTER_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(5) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_RIGHT_1, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(6) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_RIGHT_2, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(7) ) );
		mTangent->addButtonCallback( USB::TangentButton::UNDER_DISPLAY_RIGHT_3, std::bind( &ColorState::resetValue, this, std::placeholders::_1, size_t(8) ) );

		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_LEFT_1, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(0) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_LEFT_2, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(1) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_LEFT_3, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(2) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_CENTER_1, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(3) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_CENTER_2, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(4) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_CENTER_3, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(5) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_RIGHT_1, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(6) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_RIGHT_2, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(7) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_TOP_RIGHT_3, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(8) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_LEFT, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(9) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_CENTER, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(10) ) );
		mTangent->addEncoderCallback( USB::TangentEncoder::ENCODER_RIGHT, std::bind( &ColorState::encoderValue, this, std::placeholders::_1, size_t(11) ) );

		mTangent->addBallCallback( USB::TangentBall::LEFT, std::bind( &ColorState::ballValue, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, size_t(0) ) );
		mTangent->addBallCallback( USB::TangentBall::CENTER, std::bind( &ColorState::ballValue, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, size_t(3) ) );
		mTangent->addBallCallback( USB::TangentBall::RIGHT, std::bind( &ColorState::ballValue, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, size_t(6) ) );
	}
}

////////////////////////////////////////

void
ColorState::removeDevice( void )
{
	mTangent = nullptr;
}

////////////////////////////////////////

void
ColorState::setModeCDL( bool b )
{
	if ( b )
		return;

	setTrimMode( TrimMode::CDL );
}

////////////////////////////////////////

void
ColorState::setModeSixVec( bool b )
{
	if ( b )
		return;

	setTrimMode( TrimMode::SIX_VECTOR );
}

////////////////////////////////////////

void
ColorState::setModeSixVecHue( bool b )
{
	if ( b )
		return;
	setTrimMode( TrimMode::SIX_VECTOR_HUE );
}

////////////////////////////////////////

void
ColorState::setModeSat( bool b )
{
	if ( b )
		return;
	setTrimMode( TrimMode::SATURATION );
}

////////////////////////////////////////

void
ColorState::resetValue( bool b, size_t idx )
{
	if ( b )
		return;

	switch ( mTrimMode )
	{
		case TrimMode::CDL:
			mCDL[idx] = 0.F;
			break;
		case TrimMode::SIX_VECTOR:
			if ( idx < 6 )
				mSixVec[idx] = 0.F;
			break;
		case TrimMode::SIX_VECTOR_HUE:
			if ( idx < 6 )
				mSixVec[6+idx] = 0.F;
			break;
		default:
			break;
	}
	updateText();
}

////////////////////////////////////////

const float kEncoderScale = 0.001F;
inline void clampValue( float &x )
{
	x = std::max( std::min( 1.F, x ), -1.F );
}

void
ColorState::encoderValue( int delta, size_t idx )
{
	if ( delta == 0 )
		return;

	switch ( mTrimMode )
	{
		case TrimMode::CDL:
			mCDL[idx] += kEncoderScale * static_cast<float>( delta );
			clampValue( mCDL[idx] );
			break;
		case TrimMode::SIX_VECTOR:
			if ( idx < 6 )
			{
				mSixVec[idx] += kEncoderScale * static_cast<float>( delta );
				clampValue( mSixVec[idx] );
			}
			break;
		case TrimMode::SIX_VECTOR_HUE:
			if ( idx < 6 )
			{
				mSixVec[6+idx] += kEncoderScale * static_cast<float>( delta );
				clampValue( mSixVec[6+idx] );
			}
			break;
		default:
			break;
	}
	updateText();
}

inline void colorize_hsv2rgb( float &r, float &g, float &b, float h, float s, float v )
{
	float c = v * s;
	float h2 = h * 6.F;
	float x = c * ( 1.F - fabsf( fmodf( h2, 2.F ) - 1.F ) );

	if ( 0.F <= h2 && h2 < 1.F )
	{ r = c; g = x; b = 0.F; }
	else if ( 1.F <= h2 && h2 < 2.F )
	{ r = x; g = c; b = 0.F; }
	else if ( 2.F <= h2 && h2 < 3.F )
	{ r = 0.F; g = c; b = x; }
	else if ( 3.F <= h2 && h2 < 4.F )
	{ r = 0.F; g = x; b = c; }
	else if ( 4.F <=h2 && h2 < 5.F)
	{ r = x; g = 0.F; b = c; }
	else if ( 5.F <=h2 && h2 <= 6.F )
	{ r = c; g = 0.F; b = x; }
	else if ( h2 > 6.F)
	{ r = 1.F; g = 0.F; b = 0.F; }
	else
	{ r = 0.F; g = 1.F; b = 0.F; }
}


////////////////////////////////////////


void
ColorState::ballValue( int deltaX, int deltaY, float angle, float magnitude, size_t idxRoot )
{
	if ( deltaX == 0 && deltaY == 0 )
		return;

	float amt = magnitude * kEncoderScale;
	float r, g, b;
	colorize_hsv2rgb( r, g, b, fabsf( angle ) / 180.F, 1.F, 1.F );
	amt = copysignf( amt, angle );
	std::cout << "red scale: " << r << " green scale: " << g << " blue scale: " << b << std::endl;
	float rOff = ceilf( amt * r * 1000.F ) / 1000.F;
	float gOff = ceilf( amt * g * 1000.F ) / 1000.F;
	float bOff = ceilf( amt * b * 1000.F ) / 1000.F;
	switch ( mTrimMode )
	{
		case TrimMode::CDL:
			mCDL[idxRoot] += rOff;
			mCDL[idxRoot+1] += gOff;
			mCDL[idxRoot+2] += bOff;
			mCDL[idxRoot] = std::max( std::min( 1.F, mCDL[idxRoot] ), -1.F );
			mCDL[idxRoot+1] = std::max( std::min( 1.F, mCDL[idxRoot+1] ), -1.F );
			mCDL[idxRoot+2] = std::max( std::min( 1.F, mCDL[idxRoot+2] ), -1.F );
			break;
		default:
			break;
	}
	updateText();
}

////////////////////////////////////////

void
ColorState::resetText( void )
{
	if ( mTangent )
	{
		mTangent->clearText();
		switch ( mTrimMode )
		{
			case TrimMode::CDL:
				mTangent->setText( 0, 0, 14, "CDL" );
				mTangent->setText( 1, 0, 14, "CDL" );
				mTangent->setText( 2, 0, 14, "CDL" );
				mTangent->setText( 0, 1, 13, "Slope" );
				mTangent->setText( 1, 1, 12, "Offset" );
				mTangent->setText( 2, 1, 13, "Power" );
				break;
			case TrimMode::SIX_VECTOR:
				mTangent->setText( 0, 0, 11, "Six Vector" );
				mTangent->setText( 1, 0, 11, "Six Vector" );
				break;
			case TrimMode::SIX_VECTOR_HUE:
				mTangent->setText( 0, 0, 8, "Six Vector Hue" );
				mTangent->setText( 1, 0, 8, "Six Vector Hue" );
				break;
			case TrimMode::SATURATION:
				mTangent->setText( 0, 0, 11, "Saturation" );
				mTangent->setText( 1, 0, 11, "Saturation" );
				break;
		}
	}
	updateText();
}

////////////////////////////////////////

inline std::string getDisplayNumber( float x )
{
	std::stringstream sbuf;
	sbuf << std::left << std::setw( 6 ) << std::setfill( ' ' ) << std::setprecision( 3 ) << x;
	return sbuf.str();
}

////////////////////////////////////////

void
ColorState::updateText( void )
{
	if ( mTangent )
	{
		// Red 0.000
		switch ( mTrimMode )
		{
			case TrimMode::CDL:
				mTangent->setText( 0, 3, 0, "Global: " + getDisplayNumber( mCDL[9] ) );
				mTangent->setText( 1, 3, 0, "Global: " + getDisplayNumber( mCDL[10] ) );
				mTangent->setText( 2, 3, 0, "Global: " + getDisplayNumber( mCDL[11] ) );
				mTangent->setText( 0, 4, 0, "R: " + getDisplayNumber( mCDL[0] ) );
				mTangent->setText( 0, 4, 10, "G: " + getDisplayNumber( mCDL[1] ) );
				mTangent->setText( 0, 4, 20, "B: " + getDisplayNumber( mCDL[2] ) );
				mTangent->setText( 1, 4, 0, "R: " + getDisplayNumber( mCDL[3] ) );
				mTangent->setText( 1, 4, 10, "G: " + getDisplayNumber( mCDL[4] ) );
				mTangent->setText( 1, 4, 20, "B: " + getDisplayNumber( mCDL[5] ) );
				mTangent->setText( 2, 4, 0, "R: " + getDisplayNumber( mCDL[6] ) );
				mTangent->setText( 2, 4, 10, "G: " + getDisplayNumber( mCDL[7] ) );
				mTangent->setText( 2, 4, 20, "B: " + getDisplayNumber( mCDL[8] ) );
				break;
			case TrimMode::SIX_VECTOR:
				mTangent->setText( 0, 4, 0, "R: " + getDisplayNumber( mSixVec[0] ) );
				mTangent->setText( 0, 4, 10, "Y: " + getDisplayNumber( mSixVec[1] ) );
				mTangent->setText( 0, 4, 20, "G: " + getDisplayNumber( mSixVec[2] ) );
				mTangent->setText( 1, 4, 0, "C: " + getDisplayNumber( mSixVec[3] ) );
				mTangent->setText( 1, 4, 10, "B: " + getDisplayNumber( mSixVec[4] ) );
				mTangent->setText( 1, 4, 20, "M: " + getDisplayNumber( mSixVec[5] ) );
				break;
			case TrimMode::SIX_VECTOR_HUE:
				mTangent->setText( 0, 4, 0, "R: " + getDisplayNumber( mSixVec[6+0] ) );
				mTangent->setText( 0, 4, 10, "Y: " + getDisplayNumber( mSixVec[6+1] ) );
				mTangent->setText( 0, 4, 20, "G: " + getDisplayNumber( mSixVec[6+2] ) );
				mTangent->setText( 1, 4, 0, "C: " + getDisplayNumber( mSixVec[6+3] ) );
				mTangent->setText( 1, 4, 10, "B: " + getDisplayNumber( mSixVec[6+4] ) );
				mTangent->setText( 1, 4, 20, "M: " + getDisplayNumber( mSixVec[6+5] ) );
				break;
			case TrimMode::SATURATION:
				break;
		}
	}
}

