#ifndef _CIMAGE_PIXEL_ACCESS_OPTIMIZER_H_
#define _CIMAGE_PIXEL_ACCESS_OPTIMIZER_H_

#include <afx.h>
#include <atlimage.h>

/////
// DISCLAIMER : The inspiration for this class was taken from an article on codeproject,
//				and was modified to better reflect the needs of this application
//
// SOURCE USED : http://www.codeproject.com/Articles/19549/CImage-pixel-access-performance-optimization
// Author : Rafal Struzyk
////

////
//
//	The purpose of this class is to provide a faster pixel access mechanism for the CImage class
//
////
class CImagePixelAccessOptimizer
{
public:
	CImagePixelAccessOptimizer( CImage* _image );
	~CImagePixelAccessOptimizer();

	COLORREF GetPixel( int _x, int _y ) const;
	void SetPixel( int _x, int _y, const COLORREF _color );

private:
	void Init();

	CImage* m_image;
	const bool m_const;
	
	int m_width;
	int m_height;
	int m_bitCnt;
	int m_rowBytes;

	RGBQUAD* m_colors;
	BYTE* m_bits;
};

inline CImagePixelAccessOptimizer::CImagePixelAccessOptimizer( CImage* _image )
: m_image( _image )
, m_const( false )
{
	Init();
}

inline void CImagePixelAccessOptimizer::Init()
{
	m_colors = NULL;
	m_bits = NULL;

	m_width = m_image->GetWidth();
	m_height = m_image->GetHeight();

	m_bitCnt = m_image->GetBPP();
	m_rowBytes = ( ( m_width * m_bitCnt + 31) & (~31) ) / 8;
	if( m_image->GetPitch() < 0 )
	{
		m_rowBytes = -m_rowBytes;
	}

	m_bits = reinterpret_cast< BYTE* >( m_image->GetBits() );

	m_image->GetDC();
}

inline CImagePixelAccessOptimizer::~CImagePixelAccessOptimizer()
{
	// Clean up upon destruction of object
	delete m_colors;
	m_image->ReleaseDC();
}

inline COLORREF CImagePixelAccessOptimizer::GetPixel( int _x, int _y ) const
{
	// Access the request pixel from the BYTE pointer,
	// casting it into an appropriate object from which the 
	// individual colour values can be extracted from
	const RGBQUAD* rgbResult;

	rgbResult = (LPRGBQUAD)(m_bits + m_rowBytes*_y + _x * 3);
	
	// Return an RGB element using the appropriate colour information
	return RGB( rgbResult->rgbRed, rgbResult->rgbGreen, rgbResult->rgbBlue );
}

inline void CImagePixelAccessOptimizer::SetPixel( int _x, int _y, const COLORREF _color )
{
	// Access the correct pixel from the BYTE pointer and
	// adjust the colour information appropriately
	RGBQUAD* rgbValue = reinterpret_cast< RGBQUAD* >(m_bits + m_rowBytes*_y + _x*3);
	rgbValue->rgbRed = GetRValue(_color);
	rgbValue->rgbGreen = GetGValue(_color);
	rgbValue->rgbBlue = GetBValue(_color);
}
#endif