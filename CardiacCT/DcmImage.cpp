
// DcmImage.cpp : DcmImage¹ê§@
//

#include "pch.h"
#include "DcmImage.h"

//=======================================//
//   DcmImage construction/destruction   //
//=======================================//

DcmImage::DcmImage()
{
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


