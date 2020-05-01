
// DcmDIr.h : DICOMDIR�ɮװ򥻸�T�P�}��
// �}�ɭ��� : �u��}�@�W�f�w�Y�@���ˬd����@�ΤQ�ɧ�
//

#pragma once
#include "DcmSeries.h"
#include <vector>
using namespace std;

enum class Action							// �C�|�Ҧ��}�ɰʧ@
{											//
	Dir_SingleSeries,						// �ثe�}�ɼҦ����
	Dir_AllSeries							// �o�ӥ\�঳�I�h�l
};

class DcmDir
{
public:
	DcmDir();
	DcmDir(Action act, CString &pathName);	// Delegate DcmDir()

	~DcmDir();

//================//
//   Attributes   //
//================//
public:
	vector<DcmSeries*>	SeriesList;			// �O���C�@�ɧ�(series)��T

	CString		DirPathName;				// �����ɮ׸��|(�t�D�ɦW�P���ɦW)
	CString		DirFileName;				// �����ɮצW��(�t�D�ɦW�P���ɦW)
	CString		DirFileTitle;				// �D�ɦW
	CString		DirFileExtension;			// ���ɦW(���]�A.)

	CString		PatientName;				// �f�w �m�W
	CString		PatientID;					// �f�w ID
	CString		StudyID;					// �ˬd ID

	CString		Rescale_Intercept;			// �I�Z
	CString		Rescale_Slope;				// �ײv
	CString		Rescale_Type;				// Rescale_Value = Original_Value * Rescale_Slope + Rescale_Intercept
	
	CString		Window_Explanation;			// ����Window_Width�PCenter�����e
	CString		Window_Center;				// ������
	CString		Window_Width;				// ���e
	
	CString		Spacing_Between_Slices;		// �������Z(mm)
	CString		Slice_Thickness;			// �����p��(mm)
	CString		Voxel_Spacing;				// ����j�p(mm)
	CString		Pixel_Spacing;				// �������Z(mm)

	CString		Pixel_Representation;		// �����ƾڪ���{����(0->unsigned�F1->signed�A�p�G�O1�A�n�2^15)
	CString		Bits_Allocated;				// �C�ӹ������t��Bits
	CString		Bits_Stored;				// �C�ӹ����x�s�Ҧ��Ϊ�Bits
	CString		Bits_HiBit;					// High Bit

	int			Row;						// �Ϥ��ؤo : �C(Y�b)
	int			Col;						// �Ϥ��ؤo : ��(X�b)
	int			HU_max;						// �v������HU�̤j��
	int			HU_min;						// �v������HU�̤p��
	int			Window_1_Width;
	int			Window_2_Width;
	int			Window_1_Center;
	int			Window_2_Center;

	double		Voxel_Spacing_X;			// ���X�Ŷ��j�p(mm)
	double		Voxel_Spacing_Y;			// ���Y�Ŷ��j�p(mm)
	double		Voxel_Spacing_Z;			// ���Z�Ŷ��j�p(mm)

//================//
//   Operations   //
//================//
public:
	void	openDirFromSeries(CString& pathName);	// �}�ҳ�@�ΤQ�ɧǪ�DICOMDIR�ɮ�

};

