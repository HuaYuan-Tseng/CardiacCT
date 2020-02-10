
// DcmSeries.cpp : DcmSeries¹ê§@
//

#include "pch.h"
#include "DcmSeries.h"

//========================================//
//   DcmSeries construction/destruction   //
//========================================//

DcmSeries::DcmSeries()
{
	TotalSliceCount = 0;
	SeriesNumber = _T("");
}

DcmSeries::~DcmSeries()
{
	if (TotalSliceCount != 0)
		TotalSliceCount = 0;
	if (SeriesNumber.IsEmpty() != true)
		SeriesNumber.Empty();
	if (ImageList.empty() != true)
	{
		ImageList.clear();
		ImageList.shrink_to_fit();
	}
}

//========================//
//   DcmSeries commands   //
//========================//

