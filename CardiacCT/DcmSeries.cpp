
// DcmSeries.cpp : DcmSeries��@
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

