
// DcmImage.cpp : DcmImage¹ê§@
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
	if (Row != 0)
		Row = 0;
	if (Col != 0)
		Col = 0;
	if (SliceNumber != 0)
		SliceNumber = 0;
	if (AbsFilePath.IsEmpty() != true)
		AbsFilePath.Empty();
}

//=======================//
//   DcmImage commands   //
//=======================//


