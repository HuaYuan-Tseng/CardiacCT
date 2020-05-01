
// DcmImage.h : 每張slice影像的資訊
//

#pragma once


class DcmImage
{
public:
	DcmImage();
	~DcmImage();

//================//
//   Attributes   //
//================//
public:
	int		SliceNumber;			// 當前slice的數目(第幾張slice)

	CString AbsFilePath;			// Dicom影像的絕對路徑
	
//================//
//   Operations   //
//================//
public:

};

