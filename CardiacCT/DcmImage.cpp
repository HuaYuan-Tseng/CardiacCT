
// DcmImage.cpp : DcmImage��@
//

#include "pch.h"
#include "DcmImage.h"

//=======================================//
//   DcmImage construction/destruction   //
//=======================================//

DcmImage::DcmImage()
{
	Row = 0;
	Col = 0;
	SliceNumber = 0;
	AbsFilePath = _T("");
}

DcmImage::~DcmImage()
{
	if (AbsFilePath.IsEmpty() != true)
		AbsFilePath.Empty();
}

//=======================//
//   DcmImage commands   //
//=======================//


