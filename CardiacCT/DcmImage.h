
// DcmImage.h : �C�islice�v������T
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
	int		Row;					// �Ϥ��ؤo : �C(Y�b)
	int		Col;					// �Ϥ��ؤo : ��(X�b)
	int		SliceNumber;			// ��eslice���ƥ�(�ĴX�islice)

	CString AbsFilePath;			// Dicom�v����������|
	
//================//
//   Operations   //
//================//
public:

};

