
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
#include "C3DProcess.h"
#include "CProgress.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))

#define Total_Slice m_dir->SeriesList[0]->TotalSliceCount
#define ROW m_dir->SeriesList[0]->ImageList[0]->Row
#define COL m_dir->SeriesList[0]->ImageList[0]->Col
#define Window_Center_1 m_dir->Window_1_Center
#define Window_Center_2 m_dir->Window_2_Center
#define Window_Width_1 m_dir->Window_1_Width
#define Window_Width_2 m_dir->Window_2_Width
#define Rescale_Intercept atoi(m_dir->Rescale_Intercept)
#define Rescale_Slope atoi(m_dir->Rescale_Slope)

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

	if (displaySeries != 0)
		displaySeries = 0;
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

	BOOL isOpen = TRUE;							// TRUE:Open；FALSE:Save
	CString initName = "";						// 初始開檔文件名
	CString initDir = "C:\\";					// 初始開檔路徑
	CString filter = "All Files(*.*)|*.*||";	// 文件過濾類型

	CFileDialog openDirDlg(isOpen, initDir, initName, OFN_HIDEREADONLY, filter, NULL);

	if (openDirDlg.DoModal() == IDOK)
	{
		if (m_dir != nullptr)
		{
			delete	m_dir;
			m_dir = nullptr;
			displaySeries = 0;
		}
		
		m_dir = new DcmDir(Action::Dir_SingleSeries, openDirDlg.GetPathName());

		m_dir->DirFileName = openDirDlg.GetFileName();
		m_dir->DirFileTitle = openDirDlg.GetFileTitle();
		m_dir->DirFileExtension = openDirDlg.GetFileExt();

		SetTitle("Cardiac CT - " + m_dir->PatientName);

		if (m_dir != nullptr)
		{
			CDirForm m_dirFormDlg;
			m_dirFormDlg.m_pDoc = this;
			m_dirFormDlg.m_clickItem = displaySeries;
			m_dirFormDlg.m_checkItem = displaySeries;

			if (m_dirFormDlg.DoModal() == IDOK)
			{
				BuildDataMatrix();
			}
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
		CDirForm m_dirFormDlg;
		m_dirFormDlg.m_pDoc = this;
		m_dirFormDlg.m_clickItem = displaySeries;
		m_dirFormDlg.m_checkItem = displaySeries;

		if (m_dirFormDlg.DoModal() == IDOK)
		{
			BuildDataMatrix();
		}
	}

}

void CCTDoc::OnToolButton3DProcess()
{
	// TODO: Add your command handler code here
	// ID : ID_TOOLBTN_3DPROCESS
	// DO : 開啟3D處理頁面

	if (m_dir != nullptr)
	{
		C3DProcess m_3DProcessDlg;
		m_3DProcessDlg.m_pDoc = this;

		if (m_3DProcessDlg.DoModal() == IDOK)
		{
		}
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
	const int TotalSlice = Total_Slice;
	const int RescaleSlope = Rescale_Slope;
	const int RescaleIntercept = Rescale_Intercept;
	const double Window_low = Window_Center_1 - 0.5 - (Window_Width_1 - 1) / 2;
	const double Window_high = Window_Center_1 - 0.5 + (Window_Width_1 - 1) / 2;

	int slice = 0;
	short value = 0;
	CString dir_temp;
	register int i = 0;

	if (m_HUimg != nullptr)		delete[] m_HUimg;
	if (m_img != nullptr)		delete[] m_img;
	m_HUimg = New2Dmatrix(TotalSlice, Row*Col, short);
	m_img = New2Dmatrix(TotalSlice, Row*Col, BYTE);

	CProgress* m_progress = new CProgress();
	m_progress->Create(IDD_DIALOG_PROGRESSBAR);
	m_progress->ShowWindow(SW_NORMAL);
	m_progress->Set(TotalSlice, 0);
	m_progress->SetStatic("Open DICOM Image...");

	while (slice < TotalSlice)
	{
		dir_temp = m_dir->SeriesList[displaySeries]->ImageList[slice]->AbsFilePath;
		DicomImage* image = new DicomImage(dir_temp);

		if (image != NULL)
		{
			Uint16* data = (Uint16*)(image->getOutputData(12));		// 獲得原始影像數據
			if (data != NULL)
			{
				i = 0;
				while (i < Row*Col)
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
						m_img[slice][i] = (BYTE)(255 * ((value - (Window_Center_1 - 0.5)) / (Window_Width_1 + 1) + 0.5));
					}
					i += 1;
				}
			}
		}
		delete image;
		slice += 1;
		m_progress->GetPro(slice);
	}
	m_progress->DestroyWindow();
	delete m_progress;
}

void* CCTDoc::new2Dmatrix(int h, int w, int size)
{
	// 動態配置二維矩陣
	//

	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w*size];
	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}