
// DcmSeries.h : 每個時序series的資訊
//

#pragma once
#include "DcmImage.h"
#include <vector>
using namespace std;

class DcmSeries
{
public:
	DcmSeries();
	~DcmSeries();
	
//================//
//   Attributes   //
//================//
public:
	vector<DcmImage*> ImageList;		// 記錄每一slice的影像

	CString		SeriesNumber;			// 當前時序的數目(第幾時序)
	int			TotalSliceCount;		// 總slice數

//================//
//   Operations   //
//================//
public:

};

