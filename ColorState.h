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

#include "TangentWaveDevice.h"

enum class TrimMode
{
	CDL,
	SIX_VECTOR,
	SIX_VECTOR_HUE,
	SATURATION
};

class ColorState
{
public:
	TrimMode getTrimMode( void ) const { return mTrimMode; }
	void setTrimMode( TrimMode m );

	void addDevice( USB::TangentWaveDevice *dev );
	void removeDevice( void );

private:
	void setModeCDL( bool );
	void setModeSixVec( bool );
	void setModeSixVecHue( bool );
	void setModeSat( bool );

	void resetValue( bool b, size_t idx );
	void encoderValue( int delta, size_t idx );
	void ballValue( int deltaX, int deltaY, float angle, float magnitude, size_t idxRoot );

	void resetText( void );
	void updateText( void );

	TrimMode mTrimMode = TrimMode::CDL;
	USB::TangentWaveDevice *mTangent = nullptr;

	float mCDL[12] = {0.F};
	float mSixVec[12] = {0.F};
	float mSat[4] = {0.F};
};




