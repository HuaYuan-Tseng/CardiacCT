
// DcmSeries.h : �C�Ӯɧ�series����T
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
	vector<DcmImage*> ImageList;		// �O���C�@slice���v��

	CString		SeriesNumber;			// ��e�ɧǪ��ƥ�(�ĴX�ɧ�)
	int			TotalSliceCount;		// �`slice��

//================//
//   Operations   //
//================//
public:

};

