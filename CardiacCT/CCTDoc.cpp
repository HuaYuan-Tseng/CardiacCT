
// CCTDoc.cpp : implementation of the CCTDoc class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "CardiacCT.h"
#endif

#include "CCTDoc.h"
#include "CDirForm.h"
#include "CProgress.h"
#include "C3DProcess.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include <propkey.h>
#include "CWait.h"
#include <ctime>
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))

#define ROW m_dir->Row
#define COL m_dir->Col
#define Window_Width_1 m_dir->Window_1_Width
#define Window_Width_2 m_dir->Window_2_Width
#define Window_Center_1 m_dir->Window_1_Center
#define Window_Center_2 m_dir->Window_2_Center
#define Bits_Stored atoi(m_dir->Bits_Stored)
#define Rescale_Slope atoi(m_dir->Rescale_Slope)
#define Rescale_Intercept atoi(m_dir->Rescale_Intercept)
#define Total_Slice m_dir->SeriesList[0]->TotalSliceCount

//============//
//   CCTDoc   //
//============//

IMPLEMENT_DYNCREATE(CCTDoc, CDocument)

BEGIN_MESSAGE_MAP(CCTDoc, CDocument)
	ON_COMMAND(ID_TOOLBTN_OPENDICOMDIR, &CCTDoc::OnToolButtonOpenDicomdir)
	ON_COMMAND(ID_TOOLBTN_DIRCONTENTS, &CCTDoc::OnToolButtonDirContents)
	ON_COMMAND(ID_TOOLBTN_3DPROCESS, &CCTDoc::OnToolButton3DProcess)
END_MESSAGE_MAP()


//=====================================//
//   CCTDoc construction/destruction   //
//=====================================//

CCTDoc::CCTDoc() noexcept
{
	// TODO: add one-time construction code here
	//
	m_dir = nullptr;
	m_img = nullptr;
	m_HUimg = nullptr;
	m_imgPro = nullptr;
	displaySeries = 0;
}

CCTDoc::~CCTDoc()
{
	if (m_dir != nullptr)
		delete m_dir;
	if (m_HUimg != nullptr)
		delete[] m_HUimg;
	if (m_img != nullptr)
		delete[] m_img;
	if (m_imgPro != nullptr)
		delete[] m_imgPro;
}

BOOL CCTDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


//==========================//
//   CCTDoc serialization   //
//==========================//

void CCTDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CCTDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CCTDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CCTDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS


//========================//
//   CCTDoc diagnostics   //
//========================//

#ifdef _DEBUG
void CCTDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCTDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


//=====================//
//   CCTDoc commands   //
//=====================//

void CCTDoc::OnToolButtonOpenDicomdir()
{
	// TODO: Add your command handler code here
	// ID : ID_TOOLBTN_OPENDICOMDIR
	// DO : 開啟DICOMDIR檔案
	
	FILE* file;
	errno_t err;
	char prefix[4];								// 確認檔案格式的4個字元
	
	BOOL isOpen = TRUE;							// TRUE:Open；FALSE:Save
	CString initName = "";						// 初始開檔文件名
	CString initDir = "C:\\";					// 初始開檔路徑
	CString filter = "All Files(*.*)|*.*||";	// 文件過濾類型

	CFileDialog openDirDlg(isOpen, initDir, initName, OFN_HIDEREADONLY, filter, NULL);
	
	if (openDirDlg.DoModal() == IDOK)
	{
		// 確認檔案名稱是否為DICOMDIR
		if (openDirDlg.GetFileTitle() != "DICOMDIR")
		{
			AfxMessageBox("This file is not DICOMDIR!!");
			return;
		}

		// 確認檔案格式是否為DICOM
		if ((err = fopen_s(&file, openDirDlg.GetPathName(), "rb")) == 0)
		{
			fseek(file, 128, SEEK_SET);
			fread_s(prefix, 4, sizeof(char), 4, file);
			if (prefix[0] != 0x44 || prefix[1] != 0x49 || prefix[2] != 0x43 || prefix[3] != 0x4D)
			{
				AfxMessageBox("The format of this file is not DICOM!!");
				return;
			}
		}
		else
		{
			AfxMessageBox("This file can not be opened!!");
			return;
		}

		// 開啟DICOMDIR檔案前進行初始化
		if (m_dir != nullptr)
		{
			delete	m_dir;
			m_dir = nullptr;
			displaySeries = 0;
		}
		
		// 讀取並存取DICOMDIR內容(影像路徑)
		m_dir = new DcmDir(Action::Dir_SingleSeries, openDirDlg.GetPathName());
		m_dir->DirFileName = openDirDlg.GetFileName();
		m_dir->DirFileTitle = openDirDlg.GetFileTitle();
		m_dir->DirFileExtension = openDirDlg.GetFileExt();
		SetTitle("Cardiac CT - " + m_dir->PatientName);

		// 選擇顯示的時序並建立影像陣列
		if (m_dir != nullptr)
		{
			CDirForm* m_dirFormDlg = new CDirForm();
			m_dirFormDlg->m_pDoc = std::move(this);
			m_dirFormDlg->m_clickItem = displaySeries;
			m_dirFormDlg->m_checkItem = displaySeries;

			if (m_dirFormDlg->DoModal() == IDOK)
			{
				BuildDataMatrix();
				OnToolButton3DProcess();	//
			}
			delete m_dirFormDlg;
		}
	}

}

void CCTDoc::OnToolButtonDirContents()
{
	// TODO: Add your command handler code here
	// ID : ID_TOOLBTN_DIRCONTENTS
	// DO : 檢視DICOMDIR檔案內容並選擇要顯示的時序

	if (m_dir != nullptr)
	{
		CDirForm* m_dirFormDlg = new CDirForm();
		m_dirFormDlg->m_pDoc = std::move(this);
		m_dirFormDlg->m_clickItem = displaySeries;
		m_dirFormDlg->m_checkItem = displaySeries;

		if (m_dirFormDlg->DoModal() == IDOK)
		{
			BuildDataMatrix();
		}
		delete m_dirFormDlg;
	}
}

void CCTDoc::OnToolButton3DProcess()
{
	// TODO: Add your command handler code here
	// ID : ID_TOOLBTN_3DPROCESS
	// DO : 開啟3D處理頁面

	if (m_dir != nullptr)
	{
		if (m_img == nullptr)
		{
			AfxMessageBox("Please select a series data！");
			return;
		}

		C3DProcess* m_3DProcessDlg = new C3DProcess();
		m_3DProcessDlg->m_pDoc = std::move(this);

		if (m_3DProcessDlg->DoModal() == IDOK)
		{
		}
		delete m_3DProcessDlg;
	}
}

//======================//
//   CCTDoc Functions   //
//======================//

void CCTDoc::BuildDataMatrix()
{
	// 建立某一時序下，所有slice的影像資料(HU and pixel)矩陣
	//
	const int Row = ROW;
	const int Col = COL;
	const int BitStored = Bits_Stored;
	const int TotalSlice = Total_Slice;
	const int RescaleSlope = Rescale_Slope;
	const int RescaleIntercept = Rescale_Intercept;
	const int WindowCenter_1 = Window_Center_1;
	const int WindowWidth_1 = Window_Width_1;
	const double Window_low = Window_Center_1 - 0.5 - (Window_Width_1 - 1) / 2;
	const double Window_high = Window_Center_1 - 0.5 + (Window_Width_1 - 1) / 2;

	if (m_HUimg != nullptr)		delete[] m_HUimg;
	if (m_img != nullptr)		delete[] m_img;
	if (m_imgPro != nullptr)	delete[] m_imgPro;
	m_HUimg = New2Dmatrix(TotalSlice, Row*Col, short);
	m_img = New2Dmatrix(TotalSlice, Row*Col, BYTE);
	m_imgPro = New2Dmatrix(TotalSlice, Row * Col, BYTE);

	auto loadImage = [&](int start)
	{
		short value = 0;
		CString dir_temp;
		register int i = 0;
		int slice = start;

		while (slice < TotalSlice)
		{
			dir_temp = m_dir->SeriesList[displaySeries]->ImageList[slice]->AbsFilePath;
			DicomImage* image = new DicomImage(dir_temp);

			if (image != NULL)
			{
				Uint16* data = (Uint16*)(image->getOutputData(BitStored));			// 獲得原始影像數據
				if (data != NULL)
				{
					i = 0;
					while (i < Row * Col)
					{
						value = *(data + i) * RescaleSlope + RescaleIntercept;
						m_HUimg[slice][i] = value;

						if (value <= Window_low)
						{
							m_img[slice][i] = 0;
						}
						else if (value > Window_high)
						{
							if (value > 60000)
								m_img[slice][i] = 0;
							else
								m_img[slice][i] = 255;
						}
						else
						{
							m_img[slice][i] = (BYTE)(255 * ((value - (WindowCenter_1 - 0.5)) / (WindowWidth_1 + 1) + 0.5));
						}
						m_imgPro[slice][i] = std::move(m_img[slice][i]);
						i += 1;
					}
				}
			}
			delete image;
			slice += 2;
		} 
	};

	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("Open DICOM Image...");
	
	thread th0(loadImage, 0);
	thread th1(loadImage, 1);
	th0.join();
	th1.join();

	m_wait->DestroyWindow();
	delete m_wait;
}

void* CCTDoc::new2Dmatrix(int h, int w, int size)
{
	// 動態配置二維矩陣
	//
	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w * size];
	
	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}