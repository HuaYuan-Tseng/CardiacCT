
// C3DProcess.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "C3DProcess.h"
#include "afxdialogex.h"

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))

#define Display_Series m_pDoc->displaySeries
#define Total_Slice m_pDoc->m_dir->SeriesList[0]->TotalSliceCount
#define ROW m_pDoc->m_dir->SeriesList[0]->ImageList[0]->Row
#define COL m_pDoc->m_dir->SeriesList[0]->ImageList[0]->Col
#define Window_Center_1 m_pDoc->m_dir->Window_1_Center
#define Window_Center_2 m_pDoc->m_dir->Window_2_Center
#define Window_Width_1 m_pDoc->m_dir->Window_1_Width
#define Window_Width_2 m_pDoc->m_dir->Window_2_Width
#define Rescale_Intercept atoi(m_pDoc->m_dir->Rescale_Intercept)
#define Rescale_Slope atoi(m_pDoc->m_dir->Rescale_Slope)

//==========================//
//   3D Processing Dialog   //
//==========================//

IMPLEMENT_DYNAMIC(C3DProcess, CDialogEx)

C3DProcess::C3DProcess(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPROCESS, pParent)
{
	m_pDoc = nullptr;
	m_2D_dib = nullptr;
	m_2D_frame = nullptr;

	DisplaySlice = 0;
}

C3DProcess::~C3DProcess()
{
	if (m_2D_dib != nullptr)
		delete  m_2D_dib;

	if (DisplaySlice != 0)
		DisplaySlice = 0;
}

void C3DProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_2D, m_ScrollBar);
}

BEGIN_MESSAGE_MAP(C3DProcess, CDialogEx)
	ON_WM_PAINT()
	ON_WM_MOUSEWHEEL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

//=================================//
//   C3DProcess message handlers   //
//=================================//

BOOL C3DProcess::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	// Dialog ��l�ƪ��ƥ�
	//

	// ��Dialog���D
	//
	SetWindowText("3D Processing");

	// �]�wø�Ϯy�жb�B�ϰ�P����
	//
	m_2D_frame = GetDlgItem(IDC_STATIC_2D);			// �X�� Dialog ��������
	m_2D_frame->GetWindowRect(&m_2D_rect);			// ��o����b�ù��W����m�]�H�ù����y�жb�^
	ScreenToClient(&m_2D_rect);						// �N����b�H"�ù�"���y�жb��t�U����m�A
													// �ഫ��"�Τ��"�y�жb��t�U����m
	m_2D_dib = new CDIB();							
	m_2D_dib->InitDIB(COL, ROW);					// ��l�Ƶe��

	// �]�wScrollBar���d��
	//
	m_ScrollBar.SetScrollRange(0, Total_Slice-1);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void C3DProcess::OnPaint()
{
	// ø�Ϩƥ�
	//
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDialogEx::OnPaint() for painting messages

	Draw2DImage(DisplaySlice);
}

BOOL C3DProcess::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ��u���ƥ�
	//
	int TotalSlice = Total_Slice;

	if (zDelta < 0)
		DisplaySlice += 1;
	else if (zDelta > 0 && DisplaySlice > 0)
		DisplaySlice -= 1;

	if (DisplaySlice >= TotalSlice)
		DisplaySlice = TotalSlice - 1;
	else if (DisplaySlice < 0)
		DisplaySlice = 0;

	Draw2DImage(DisplaySlice);
	m_ScrollBar.SetScrollPos(DisplaySlice);
	
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void C3DProcess::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// ScrollBar �������ʨƥ�
	//
	int n = m_ScrollBar.GetScrollPos();			// n ���P�� slice
	int TotalSlice = Total_Slice;

	switch (nSBCode)
	{
	case SB_LINEUP :
		n -= 1;
		break;
	case SB_LINEDOWN :
		n += 1;
		break;
	case SB_PAGEUP :
		n -= 5;
		break;
	case SB_PAGEDOWN :
		n += 5;
		break;
	case SB_TOP : 
		n = 0;
		break;
	case SB_BOTTOM : 
		n = TotalSlice - 1;
		break;
	case SB_THUMBPOSITION :
		n = nPos;
		break;
	case SB_THUMBTRACK :
		n = nPos;
		break;
	}

	if (n >= TotalSlice)
		n = TotalSlice - 1;
	else if (n < 0)
		n = 0;
	
	m_ScrollBar.SetScrollPos(n);
	DisplaySlice = n;
	Draw2DImage(DisplaySlice);

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

//==========================//
//   C3DProcess Functions   //
//==========================//

void C3DProcess::Draw2DImage(unsigned short &slice)
{
	//	ø�s 2D �v��
	//
	CClientDC dc(m_2D_frame);

	if (m_pDoc->m_img != nullptr)
	{
		m_2D_dib->ShowInverseDIB(&dc, m_pDoc->m_img[slice]);
	}

	// �g�r (slice)
	//
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void* C3DProcess::new2Dmatrix(int h, int w, int size)
{
	// �ʺA�t�m�G���x�}
	//
	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w*size];
	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}

