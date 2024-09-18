/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DRAWVERT_H__
#define __DRAWVERT_H__

#include "idlib/math/Vector.h"

/*
===============================================================================

	Draw Vertex.

===============================================================================
*/
typedef unsigned short halfFloat_t;

// GPU half-float bit patterns
#define HF_MANTISSA(x)	(x&1023)
#define HF_EXP(x)		((x&32767)>>10)
#define HF_SIGN(x)		((x&32768)?-1:1)
/*
========================
F16toF32
========================
*/
ID_INLINE float F16toF32( halfFloat_t x )
{
    int e = HF_EXP( x );
    int m = HF_MANTISSA( x );
    int s = HF_SIGN( x );

    if( 0 < e && e < 31 )
    {
        return s * powf( 2.0f, ( e - 15.0f ) ) * ( 1 + m / 1024.0f );
    }
    else if( m == 0 )
    {
        return s * 0.0f;
    }
    return s * powf( 2.0f, -14.0f ) * ( m / 1024.0f );
}

/*
========================
F32toF16
========================
*/
ID_INLINE halfFloat_t F32toF16( float a )
{
    unsigned int f = *( unsigned* )( &a );
    unsigned int signbit  = ( f & 0x80000000 ) >> 16;
    int exponent = ( ( f & 0x7F800000 ) >> 23 ) - 112;
    unsigned int mantissa = ( f & 0x007FFFFF );

    if( exponent <= 0 )
    {
        return 0;
    }
    if( exponent > 30 )
    {
        return ( halfFloat_t )( signbit | 0x7BFF );
    }

    return ( halfFloat_t )( signbit | ( exponent << 10 ) | ( mantissa >> 13 ) );
}

class idDrawVert {
public:
	idVec3			xyz;
	idVec2			st;
	idVec3			normal;
	idVec3			tangents[2];
	byte			color[4];
#if 0 // was MACOS_X see comments concerning DRAWVERT_PADDED in Simd_Altivec.h
	float			padding;
#endif
	float			operator[]( const int index ) const;
	float &			operator[]( const int index );

	void			Clear( void );

	void			Lerp( const idDrawVert &a, const idDrawVert &b, const float f );
	void			LerpAll( const idDrawVert &a, const idDrawVert &b, const float f );

	void			Normalize( void );

	void			SetColor( dword color );
	dword			GetColor( void ) const;

	void				SetTexCoord( const idVec2& st );
	void				SetTexCoord( float s, float t );
	void				SetTexCoordS( float s );
	void				SetTexCoordT( float t );
	const idVec2		GetTexCoord() const;
	const float			GetTexCoordS() const;
	const float			GetTexCoordT() const;
};

/*
========================
idDrawVert::SetTexCoord
========================
*/
ID_INLINE void idDrawVert::SetTexCoord( const idVec2& st )
{
    SetTexCoordS( st.x );
    SetTexCoordT( st.y );
}

/*
========================
idDrawVert::SetTexCoord
========================
*/
ID_INLINE void idDrawVert::SetTexCoord( float s, float t )
{
    SetTexCoordS( s );
    SetTexCoordT( t );
}

/*
========================
idDrawVert::SetTexCoordS
========================
*/
ID_INLINE void idDrawVert::SetTexCoordS( float s )
{
    st[0] = F32toF16( s );
}

/*
========================
idDrawVert::SetTexCoordT
========================
*/
ID_INLINE void idDrawVert::SetTexCoordT( float t )
{
    st[1] = F32toF16( t );
}

/*
========================
idDrawVert::GetTexCoord
========================
*/
ID_INLINE const idVec2	idDrawVert::GetTexCoord() const
{
    return idVec2( F16toF32( st[0] ), F16toF32( st[1] ) );
}

/*
========================
idDrawVert::GetTexCoordT
========================
*/
ID_INLINE const float idDrawVert::GetTexCoordS() const
{
    return F16toF32( st[0] );
}

/*
========================
idDrawVert::GetTexCoordS
========================
*/
ID_INLINE const float idDrawVert::GetTexCoordT() const
{
    return F16toF32( st[1] );
}

ID_INLINE float idDrawVert::operator[]( const int index ) const {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}
ID_INLINE float	&idDrawVert::operator[]( const int index ) {
	assert( index >= 0 && index < 5 );
	return ((float *)(&xyz))[index];
}

ID_INLINE void idDrawVert::Clear( void ) {
	xyz.Zero();
	st.Zero();
	normal.Zero();
	tangents[0].Zero();
	tangents[1].Zero();
	color[0] = color[1] = color[2] = color[3] = 0;
}

ID_INLINE void idDrawVert::Lerp( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
}

ID_INLINE void idDrawVert::LerpAll( const idDrawVert &a, const idDrawVert &b, const float f ) {
	xyz = a.xyz + f * ( b.xyz - a.xyz );
	st = a.st + f * ( b.st - a.st );
	normal = a.normal + f * ( b.normal - a.normal );
	tangents[0] = a.tangents[0] + f * ( b.tangents[0] - a.tangents[0] );
	tangents[1] = a.tangents[1] + f * ( b.tangents[1] - a.tangents[1] );
	color[0] = (byte)( a.color[0] + f * ( b.color[0] - a.color[0] ) );
	color[1] = (byte)( a.color[1] + f * ( b.color[1] - a.color[1] ) );
	color[2] = (byte)( a.color[2] + f * ( b.color[2] - a.color[2] ) );
	color[3] = (byte)( a.color[3] + f * ( b.color[3] - a.color[3] ) );
}

ID_INLINE void idDrawVert::SetColor( dword color ) {
	*reinterpret_cast<dword *>(this->color) = color;
}

ID_INLINE dword idDrawVert::GetColor( void ) const {
	return *reinterpret_cast<const dword *>(this->color);
}

#endif /* !__DRAWVERT_H__ */
