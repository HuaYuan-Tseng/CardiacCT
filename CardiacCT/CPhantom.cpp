
// CPhantom.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "CPhantom.h"
#include "afxdialogex.h"

#include <vector>

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))

//=======================//
//   3D Phantom Dialog   //
//=======================//

IMPLEMENT_DYNAMIC(CPhantom, CDialogEx)

CPhantom::CPhantom(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPHANTOM, pParent),
	_display_slice(0),
	_total_slice(0),
	_row(512),
	_col(512)
{
	_img = nullptr;
	_2D_dib = nullptr;
	_2D_frame = nullptr;
}

CPhantom::~CPhantom()
{
	if (_2D_dib != nullptr)
		delete _2D_dib;
	if (_img != nullptr)
		delete[] _img;
}

void CPhantom::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_PHANTOM, _scroll_bar);
}

BEGIN_MESSAGE_MAP(CPhantom, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_PHANTOM_OPEN, &CPhantom::OnBnClickedButtonPhantomOpen)
	ON_WM_MOUSEWHEEL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

//===============================//
//   CPhantom message handlers   //
//===============================//

BOOL CPhantom::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	// Dialog 初始化的事件
	//

	/* 更換Dialog標題 */
	SetWindowTextA("Phantom Verify");

	/* 設定繪圖座標軸、區域與物件 */
	_2D_frame = GetDlgItem(IDC_STATIC_PHANTOM_2D);							// 訪問 Dialog 中的物件。
	_2D_frame->GetWindowRect(&_2D_rect);									// 獲得物件在螢幕上的位置（以螢幕為座標軸）。
	ScreenToClient(&_2D_rect);												// 將物件在以"螢幕"為座標軸體系下的位置，轉換為"用戶區"座標軸體系下的位置。
	//_2D_frame->MoveWindow(_2D_rect.left, _2D_rect.top, COL, ROW, true);	// 調整物件位置與大小。

	_2D_dib = new CDIB();
	_2D_dib->InitDIB(_col, _row);											// 初始化畫框



	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPhantom::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠滾輪事件
	//
	POINT mpt;						// 獲得鼠標當下位置
	GetCursorPos(&mpt);
	ScreenToClient(&mpt);
	const int TotalSlice = _total_slice;

	/* 在 二維 影像視窗內 */
	if (mpt.x < _2D_rect.right && mpt.x > _2D_rect.left && mpt.y < _2D_rect.bottom && mpt.y > _2D_rect.top)
	{	
		if (zDelta < 0)
			_display_slice += 1;
		else if (zDelta > 0 && _display_slice > 0)
			_display_slice -= 1;

		if (_display_slice >= TotalSlice)
			_display_slice = TotalSlice - 1;
		else if (_display_slice < 0)
			_display_slice = 0;

		Draw2DImage(_display_slice);
		_scroll_bar.SetScrollPos(_display_slice);
	}


	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CPhantom::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// ScrollBar 垂直捲動事件
	//
	int n = _scroll_bar.GetScrollPos();			// n 等同於 slice
	int TotalSlice = _total_slice;

	switch (nSBCode)
	{
	case SB_LINEUP:
		n -= 1;
		break;
	case SB_LINEDOWN:
		n += 1;
		break;
	case SB_PAGEUP:
		n -= 5;
		break;
	case SB_PAGEDOWN:
		n += 5;
		break;
	case SB_TOP:
		n = 0;
		break;
	case SB_BOTTOM:
		n = TotalSlice - 1;
		break;
	case SB_THUMBPOSITION:
		n = nPos;
		break;
	case SB_THUMBTRACK:
		n = nPos;
		break;
	}

	if (n >= TotalSlice)
		n = TotalSlice - 1;
	else if (n < 0)
		n = 0;

	_scroll_bar.SetScrollPos(n);
	_display_slice = n;

	Draw2DImage(_display_slice);

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CPhantom::OnBnClickedButtonPhantomOpen()
{
	// TODO: Add your control notification handler code here
	// Button : Open (Open Phantom Folder)
	//

	BOOL isOpen = TRUE;							// TRUE:Open；FALSE:Save
	CString initName = "";						// 初始開檔文件名
	CString initDir = "C:\\";					// 初始開檔路徑
	CString filter = "All Files(*.*)|*.*||";	// 文件過濾類型

	CFolderPickerDialog open_folder_dlg(initDir, 0, NULL, 0);
	
	/* 選擇資料夾並獲得資料夾路徑 */
	if (open_folder_dlg.DoModal() != IDOK) return;
	
	CString folder_root = open_folder_dlg.GetPathName();
	
	/* 迭代搜尋資料夾底下的bmp */
	BOOL exist;
	CFileFind finder;	
	std::vector<std::pair<CString, CString>> img_path_list;

	exist = finder.FindFile(folder_root + "\\*.*");
	while (exist)
	{
		exist = finder.FindNextFileA();
		if (finder.IsDots()) continue;
		std::pair<CString, CString> info;
		info.first = finder.GetFilePath();
		info.second = finder.GetFileTitle();
		img_path_list.push_back(info);
	}

	/* 預先分配空間，等等來存bmp的像素資訊 */
	if (_img != nullptr)
		delete[] _img;

	_total_slice = static_cast<int>(img_path_list.size());
	_img = New2Dmatrix(_total_slice, _row*_col, BYTE);

	/* 逐一開啟bmp並存取像素資訊 */
	CImage img;
	for (const auto& path : img_path_list)
	{
		img.Load(path.first);
		int width = img.GetWidth();
		int height = img.GetHeight();
		int slice = atoi(path.second);

		int pitch_dst = img.GetPitch();				// 每一行間距
		int bits_per_pix = img.GetBPP() / 8;		// 每一像素的位元數
		BYTE* img_dst = (BYTE*)(img.GetBits());		// 像素數據的開始指標

		for (int j = 0; j < height; ++j)
		{
			for (int i = 0; i < width; ++i)
			{
				_img[slice][j * width + i] =
					*(img_dst + pitch_dst * j + i * bits_per_pix);
			}
		}

		img.Destroy();
	}

	/* 設定ScrollBar的範圍 */
	_scroll_bar.SetScrollRange(0, _total_slice - 1);

	Draw2DImage(_display_slice);
	
}


//========================//
//   CPhantom Functions   //
//========================//

void CPhantom::Draw2DImage(const int& slice)
{
	//	DO : 繪製 2D 影像
	//
	CClientDC dc(_2D_frame);

	if (_img == nullptr) return;

	_2D_dib->ShowInverseDIB(&dc, _img[slice]);

	/* 寫字 (slice) */
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void* CPhantom::new2Dmatrix(int h, int w, int size)
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

