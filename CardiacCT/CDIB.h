
// CDIB.h : interface of the CDIB class
// Ã¸¹Ï¥Î
//

#pragma once

class CDIB
{
public:
	CDIB();
	virtual ~CDIB();

//================//
//   Attributes   //
//================//
public:
	LPBITMAPINFO	m_lpbmi;
	LPLOGPALETTE	m_lpPal;
	BYTE*			DICMRGB;
	
//================//
//   Operations   //
//================//
public:
	void InitDIB(int &width, int &height);
	void ShowInverseDIB(CDC* pDC, PBYTE bp);
};

