
// DcmDir.cpp : DcmDir實作
//

#include "pch.h"
#include "math.h"
#include "DcmDir.h"
#include "dcmtk/dcmdata/dcdicdir.h"
#include "dcmtk/dcmdata/dcdeftag.h"

//=====================================//
//   DcmDir construction/destruction   //
//=====================================//

DcmDir::DcmDir()
{
	DirPathName = _T("");
	DirFileName = _T("");
	DirFileTitle = _T("");
	DirFileExtension = _T("");

	PatientName = _T("");
	PatientID = _T("");
	StudyID = _T("");

	Rescale_Intercept = _T("");
	Rescale_Slope = _T("");
	Rescale_Type = _T("");
	
	Window_Explanation = _T("");
	Window_Center = _T("");
	Window_Width = _T("");

	Spacing_Between_Slices = _T("");
	Slice_Thickness = _T("");
	Voxel_Spacing = _T("");
	Pixel_Spacing = _T("");

	Pixel_Representation = _T("");
	Bits_Allocated = _T("");
	Bits_Stored = _T("");
	Bits_HiBit = _T("");

	Row = 0;
	Col = 0;
	HU_max = 0;
	HU_min = 0;
	Window_1_Width = 0;
	Window_2_Width = 0;
	Window_1_Center = 0;
	Window_2_Center = 0;

	Voxel_Spacing_X = 0.0F;
	Voxel_Spacing_Y = 0.0F;
	Voxel_Spacing_Z = 0.0F;
}

DcmDir::DcmDir(Action act, CString& pathName) : DcmDir()
{
	// 此建構子直接開檔，引數為"開檔模式選擇"與"開檔路徑"
	// 最後發現開檔的寫法都通用(單一時序或十時序)，
	// 所以這邊怎麼選目前都沒差 = ="
	//
	DirPathName = pathName;

	switch (act)
	{
	case Action::Dir_SingleSeries :
		openDirFromSeries(pathName);
		break;
	case Action::Dir_AllSeries : 
		openDirFromSeries(pathName);
		break;
	}

}

DcmDir::~DcmDir()
{
	if (DirPathName.IsEmpty() != true)
		DirPathName.Empty();
	if (DirFileName.IsEmpty() != true)
		DirFileName.Empty();
	if (DirFileTitle.IsEmpty() != true)
		DirFileTitle.Empty();
	if (DirFileExtension.IsEmpty() != true)
		DirFileExtension.Empty();

	if (PatientName.IsEmpty() != true)
		PatientName.Empty();
	if (PatientID.IsEmpty() != true)
		PatientID.Empty();
	if (StudyID.IsEmpty() != true)
		StudyID.Empty();

	if (Rescale_Intercept.IsEmpty() != true)
		Rescale_Intercept.Empty();
	if (Rescale_Slope.IsEmpty() != true)
		Rescale_Slope.Empty();
	if (Rescale_Type.IsEmpty() != true)
		Rescale_Type.Empty();
	
	if (Window_Explanation.IsEmpty() != true)
		Window_Explanation.Empty();
	if (Window_Center.IsEmpty() != true)
		Window_Center.Empty();
	if (Window_Width.IsEmpty() != true)
		Window_Width.Empty();

	if (Spacing_Between_Slices.IsEmpty() != true)
		Spacing_Between_Slices.Empty();
	if (Slice_Thickness.IsEmpty() != true)
		Slice_Thickness.Empty();
	if (Voxel_Spacing.IsEmpty() != true)
		Voxel_Spacing.Empty();
	if (Pixel_Spacing.IsEmpty() != true)
		Pixel_Spacing.Empty();

	if (Pixel_Representation.IsEmpty() != true)
		Pixel_Representation.Empty();
	if (Bits_Allocated.IsEmpty() != true)
		Bits_Allocated.Empty();
	if (Bits_Stored.IsEmpty() != true)
		Bits_Stored.Empty();
	if (Bits_HiBit.IsEmpty() != true)
		Bits_HiBit.Empty();

	if (SeriesList.empty() != true)
	{
		SeriesList.clear();
		SeriesList.shrink_to_fit();
	}
}

//=====================//
//   DcmDir commands   //
//=====================//

void DcmDir::openDirFromSeries(CString& pathName)
{
	// DO : 開啟單一或十時序的DICOMDIR檔案
	//

	if (SeriesList.empty() != true)
	{
		SeriesList.clear();
		SeriesList.shrink_to_fit();
	}
	
	// 讀取DICOMDIR檔案
	//
	DcmDicomDir	dicomDir((LPCSTR)pathName);
	DcmDirectoryRecord* dicomdirRoot = &(dicomDir.getRootRecord());

	DcmDirectoryRecord* patientDir = NULL;
	DcmDirectoryRecord* studyDir = NULL;
	DcmDirectoryRecord* seriesDir = NULL;
	DcmDirectoryRecord* imageDir = NULL;

	OFString	str_temp;
	CString		tempDir;

	register int i = 0, j = 0, k = 0, l = 0;

	while ((patientDir = dicomdirRoot->getSub(i)) != NULL)
	{
		if (patientDir->findAndGetOFString(DCM_PatientID, str_temp).good())
		{
			PatientID = str_temp.c_str();
		}
		if (patientDir->findAndGetOFString(DCM_PatientName, str_temp).good())
		{
			PatientName = str_temp.c_str();
		}
		while ((studyDir = patientDir->getSub(j)) != NULL)
		{
			if (studyDir->findAndGetOFString(DCM_StudyID, str_temp).good())
			{
				StudyID = str_temp.c_str();
			}
			while ((seriesDir = studyDir->getSub(k)) != NULL)
			{
				DcmSeries* m_series = new DcmSeries();

				if (seriesDir->findAndGetOFString(DCM_SeriesNumber, str_temp).good())
				{
					m_series->SeriesNumber = str_temp.c_str();
				}
				while ((imageDir = seriesDir->getSub(l)) != NULL)
				{
					DcmImage* m_image = new DcmImage();

					if (imageDir->findAndGetOFString(DCM_Rows, str_temp).good())
					{
						Row = atoi(str_temp.c_str());
					}
					if (imageDir->findAndGetOFString(DCM_Columns, str_temp).good())
					{
						Col = atoi(str_temp.c_str());
					}
					if (imageDir->findAndGetOFString(DCM_InstanceNumber, str_temp).good())
					{
						m_image->SliceNumber = atoi(str_temp.c_str());
					}
					if (imageDir->findAndGetOFStringArray(DCM_ReferencedFileID, str_temp, true).good())
					{
						tempDir = pathName.Left(pathName.Find("DICOMDIR"));
						tempDir = tempDir + str_temp.c_str();

						m_image->AbsFilePath = tempDir;
					}
					m_series->ImageList.push_back(m_image);
					l++;
				}
				m_series->TotalSliceCount = l;
				SeriesList.push_back(m_series);
				l = 0;
				k++;
			}
			k = 0;
			j++;
		}
		j = 0;
		i++;
	}

	// 開啟一張DICOM影像，獲得以下參數
	// (體素的Z軸大小要開到第二張影像，並且用 Slice_Location 來計算)
	//
	double Slice_Location_0 = 0.0F;			// 用以計算體素的Z軸大小
	double Slice_Location_1 = 0.0F;

	DcmFileFormat dcm;
	OFCondition cond_0 = dcm.loadFile((LPCSTR)SeriesList[0]->ImageList[0]->AbsFilePath);

	if (cond_0.good())
	{
		if (dcm.getDataset()->findAndGetOFString(DCM_RescaleIntercept, str_temp).good())
		{
			Rescale_Intercept = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_RescaleSlope, str_temp).good())
		{
			Rescale_Slope = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_RescaleType, str_temp).good())
		{
			Rescale_Type = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_WindowCenterWidthExplanation, str_temp, true).good())
		{
			Window_Explanation = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_WindowCenter, str_temp, true).good())
		{
			Window_Center = str_temp.c_str();
			Window_1_Center = atoi(Window_Center.Left(Window_Center.Find('\\')));
			Window_2_Center = atoi(Window_Center.Right(Window_Center.GetLength() - Window_Center.Find('\\') - 1));
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_WindowWidth, str_temp, true).good())
		{
			Window_Width = str_temp.c_str();
			Window_1_Width = atoi(Window_Width.Left(Window_Width.Find('\\')));
			Window_2_Width = atoi(Window_Width.Right(Window_Width.GetLength() - Window_Width.Find('\\') - 1));
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_SpacingBetweenSlices, str_temp).good())
		{
			Spacing_Between_Slices = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_SliceThickness, str_temp, true).good())
		{
			Slice_Thickness = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_PixelSpacing, str_temp, true).good())
		{
			Pixel_Spacing = str_temp.c_str();
			Voxel_Spacing_X = atof(Pixel_Spacing.Left(Pixel_Spacing.Find('\\')));
			Voxel_Spacing_Y = atof(Pixel_Spacing.Right(Pixel_Spacing.GetLength() - Pixel_Spacing.Find('\\') - 1));
		}
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_SliceLocation, str_temp).good())
		{
			Slice_Location_0 = atof(str_temp.c_str());
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_PixelRepresentation, str_temp).good())
		{
			Pixel_Representation = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_BitsAllocated, str_temp).good())
		{
			Bits_Allocated = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_BitsStored, str_temp).good())
		{
			Bits_Stored = str_temp.c_str();
		}
		if (dcm.getDataset()->findAndGetOFString(DCM_HighBit, str_temp).good())
		{
			Bits_HiBit = str_temp.c_str();
		}
		if (Rescale_Intercept.IsEmpty() != true && Rescale_Slope.IsEmpty() != true && Bits_Stored.IsEmpty() != true)
		{
			HU_min = 0 * atoi(Rescale_Slope) + atoi(Rescale_Intercept);
			HU_max = (int)pow(2, atoi(Bits_Stored)) * atoi(Rescale_Slope) + atoi(Rescale_Intercept);
		}
	}

	OFCondition cond_1 = dcm.loadFile((LPCSTR)SeriesList[0]->ImageList[1]->AbsFilePath);

	if (cond_1.good())
	{
		if (dcm.getDataset()->findAndGetOFStringArray(DCM_SliceLocation, str_temp).good())
		{
			Slice_Location_1 = atof(str_temp.c_str());
		}
	}

	if (Slice_Location_0 != 0 && Slice_Location_1 != 0)
	{
		Voxel_Spacing_Z = Slice_Location_1 - Slice_Location_0;
		Voxel_Spacing_Z = (Voxel_Spacing_Z > 0) ? Voxel_Spacing_Z : -Voxel_Spacing_Z;
		Voxel_Spacing.Format(_T("%lf X %lf X %lf"), Voxel_Spacing_X, Voxel_Spacing_Y, Voxel_Spacing_Z);
	}

}