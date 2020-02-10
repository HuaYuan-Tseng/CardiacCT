
// DcmDIr.h : DICOMDIR檔案基本資訊與開檔
// 開檔限制 : 只能開一名病患某一次檢查的單一或十時序
//

#pragma once
#include "DcmSeries.h"
#include <vector>
using namespace std;

enum class Action							// 列舉所有開檔動作
{											//
	Dir_SingleSeries,						// 目前開檔模式選擇
	Dir_AllSeries							// 這個功能有點多餘
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
	vector<DcmSeries*>	SeriesList;			// 記錄每一時序(series)資訊

	CString		DirPathName;				// 完整檔案路徑(含主檔名與副檔名)
	CString		DirFileName;				// 完整檔案名稱(含主檔名與副檔名)
	CString		DirFileTitle;				// 主檔名
	CString		DirFileExtension;			// 副檔名(不包括.)

	CString		PatientName;				// 病患 姓名
	CString		PatientID;					// 病患 ID
	CString		StudyID;					// 檢查 ID

	CString		Rescale_Intercept;			// 截距
	CString		Rescale_Slope;				// 斜率
	CString		Rescale_Type;				// Rescale_Value = Original_Value * Rescale_Slope + Rescale_Intercept
	
	CString		Window_Explanation;			// 說明Window_Width與Center的內容
	CString		Window_Center;				// 窗中心
	CString		Window_Width;				// 窗寬
	
	CString		Spacing_Between_Slices;		// 切片間距(mm)
	CString		Slice_Thickness;			// 切片厚度(mm)
	CString		Pixel_Spacing;				// 像素間距(mm)

	CString		Pixel_Representation;		// 像素數據的表現類型(0->unsigned；1->signed，如果是1，要減掉2^15)
	CString		Bits_Allocated;				// 每個像素分配的Bits
	CString		Bits_Stored;				// 每個像素儲存所佔用的Bits
	CString		Bits_HiBit;					// High Bit

	int			Window_1_Center;
	int			Window_2_Center;
	int			Window_1_Width;
	int			Window_2_Width;

	double		X_Spacing;
	double		Y_Spacing;

//================//
//   Operations   //
//================//
public:
	void	openDirFromSeries(CString &pathName);	// 開啟單一或十時序的DICOMDIR檔案

};

