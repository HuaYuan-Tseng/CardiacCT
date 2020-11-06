
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
	// Dialog ��l�ƪ��ƥ�
	//

	/* ��Dialog���D */
	SetWindowTextA("Phantom Verify");

	/* �]�wø�Ϯy�жb�B�ϰ�P���� */
	_2D_frame = GetDlgItem(IDC_STATIC_PHANTOM_2D);							// �X�� Dialog ��������C
	_2D_frame->GetWindowRect(&_2D_rect);									// ��o����b�ù��W����m�]�H�ù����y�жb�^�C
	ScreenToClient(&_2D_rect);												// �N����b�H"�ù�"���y�жb��t�U����m�A�ഫ��"�Τ��"�y�жb��t�U����m�C
	//_2D_frame->MoveWindow(_2D_rect.left, _2D_rect.top, COL, ROW, true);	// �վ㪫���m�P�j�p�C

	_2D_dib = new CDIB();
	_2D_dib->InitDIB(_col, _row);											// ��l�Ƶe��



	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPhantom::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ��u���ƥ�
	//
	POINT mpt;						// ��o���з�U��m
	GetCursorPos(&mpt);
	ScreenToClient(&mpt);
	const int TotalSlice = _total_slice;

	/* �b �G�� �v�������� */
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
	// ScrollBar �������ʨƥ�
	//
	int n = _scroll_bar.GetScrollPos();			// n ���P�� slice
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

	BOOL isOpen = TRUE;							// TRUE:Open�FFALSE:Save
	CString initName = "";						// ��l�}�ɤ��W
	CString initDir = "C:\\";					// ��l�}�ɸ��|
	CString filter = "All Files(*.*)|*.*||";	// ���L�o����

	CFolderPickerDialog open_folder_dlg(initDir, 0, NULL, 0);
	
	/* ��ܸ�Ƨ�����o��Ƨ����| */
	if (open_folder_dlg.DoModal() != IDOK) return;
	
	CString folder_root = open_folder_dlg.GetPathName();
	
	/* ���N�j�M��Ƨ����U��bmp */
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

	/* �w�����t�Ŷ��A�����Ӧsbmp��������T */
	if (_img != nullptr)
		delete[] _img;

	_total_slice = static_cast<int>(img_path_list.size());
	_img = New2Dmatrix(_total_slice, _row*_col, BYTE);

	/* �v�@�}��bmp�æs��������T */
	CImage img;
	for (const auto& path : img_path_list)
	{
		img.Load(path.first);
		int width = img.GetWidth();
		int height = img.GetHeight();
		int slice = atoi(path.second);

		int pitch_dst = img.GetPitch();				// �C�@�涡�Z
		int bits_per_pix = img.GetBPP() / 8;		// �C�@�������줸��
		BYTE* img_dst = (BYTE*)(img.GetBits());		// �����ƾڪ��}�l����

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

	/* �]�wScrollBar���d�� */
	_scroll_bar.SetScrollRange(0, _total_slice - 1);

	Draw2DImage(_display_slice);
	
}


//========================//
//   CPhantom Functions   //
//========================//

void CPhantom::Draw2DImage(const int& slice)
{
	//	DO : ø�s 2D �v��
	//
	CClientDC dc(_2D_frame);

	if (_img == nullptr) return;

	_2D_dib->ShowInverseDIB(&dc, _img[slice]);

	/* �g�r (slice) */
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void* CPhantom::new2Dmatrix(int h, int w, int size)
{
	// �ʺA�t�m�G���x�}
	//
	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w * size];

	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}

