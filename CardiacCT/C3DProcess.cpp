
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
	m_3D_frame = nullptr;

	gl_3DTexture = FALSE;

	ImageFrame = 1;
	DisplaySlice = 0;

	glVertexPt = New2Dmatrix(64, 3, float);
}

C3DProcess::~C3DProcess()
{
	if (m_2D_dib != nullptr)
		delete  m_2D_dib;
	if (glVertexPt != nullptr)
		delete[] glVertexPt;

	if (gl_3DTexture != FALSE)
		gl_3DTexture = FALSE;

	if (ImageFrame != 1)
		ImageFrame = 1;
	if (DisplaySlice != 0)
		DisplaySlice = 0;

	glDeleteTextures(10, textureName);
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
	m_2D_frame = GetDlgItem(IDC_STATIC_2D);									// �X�� Dialog ��������C
	m_2D_frame->GetWindowRect(&m_2D_rect);									// ��o����b�ù��W����m�]�H�ù����y�жb�^�C
	ScreenToClient(&m_2D_rect);												// �N����b�H"�ù�"���y�жb��t�U����m�A�ഫ��"�Τ��"�y�жb��t�U����m�C
	m_2D_frame->MoveWindow(m_2D_rect.left, m_2D_rect.top, COL, ROW, true);	// �վ㪫���m�P�j�p�C

	m_3D_frame = GetDlgItem(IDC_STATIC_3D);
	m_3D_frame->GetWindowRect(&m_3D_rect);
	ScreenToClient(&m_3D_rect);
	m_3D_frame->MoveWindow(m_3D_rect.left, m_3D_rect.top, COL, ROW, false);

	m_2D_dib = new CDIB();							
	m_2D_dib->InitDIB(COL, ROW);							// ��l�Ƶe��

	// �]�wScrollBar���d��
	//
	m_ScrollBar.SetScrollRange(0, Total_Slice-1);

	//------------------------------------------------------------------------------------//
	// openGL�Ŷ��إ�
	//
	m_hDC = ::GetDC(m_3D_frame->m_hWnd);					// ��o�e������DC��HANDLE�]hDC�^
	SetupPixelFormat(m_hDC);
	if ((m_hRC = ::wglCreateContext(m_hDC)) == 0)			// ���� openGL �һݪ��e���]hRC�^
	{
		AfxMessageBox("Fail to create hRC context!");
	}
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)			// �إ� hDC �P hRC �������s��
	{
		AfxMessageBox("Fail to make current!");
	}
	PerspectiveBuild();
	gl_3DTexture = ExtensionSupported("GL_EXT_texture3D");	// �T�{�O�_�䴩3D���z
	if (gl_3DTexture)
	{
		// Return the address of an openGL extension function.
		glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");

		GLInitialization();
	}

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
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	Draw2DImage(DisplaySlice);


}

BOOL C3DProcess::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ��u���ƥ�
	//
	POINT mpt;						// ��o���з�U��m
	GetCursorPos(&mpt);
	ScreenToClient(&mpt);
	int TotalSlice = Total_Slice;

	// �b�G���v�������d��
	//
	if (mpt.x < m_2D_rect.right && mpt.x > m_2D_rect.left && mpt.y < m_2D_rect.bottom && mpt.y > m_2D_rect.top)
	{
		if (zDelta < 0)
			DisplaySlice += 1;
		else if (zDelta > 0 && DisplaySlice > 0)
			DisplaySlice -= 1;

		if (DisplaySlice >= TotalSlice)
			DisplaySlice = TotalSlice - 1;
		else if (DisplaySlice < 0)
			DisplaySlice = 0;
	}

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

BOOL C3DProcess::SetupPixelFormat(HDC hDC)
{
	// DO : �]�m(��l��)hDC���������c
	//
	PIXELFORMATDESCRIPTOR pfd = {									// pfd, Pixel Format Descriptor
		sizeof(PIXELFORMATDESCRIPTOR),								// size of this pfd
		1,															// version number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,	// support window, opengl, double buffer
		PFD_TYPE_RGBA,												// RGBA type
		24,															// 24-bit color depth
		0,0,0,0,0,0,												// color bits ignored
		0,															// no alpha buffer
		0,															// shift bit ignored
		0,															// no accumulation buffer
		0,0,0,0,													// accum bits ignored
		16,															// 16-bit z-buffer
		0,															// no stencil buffer
		0,															// no auxiliary buffer
		PFD_MAIN_PLANE,												// main layer
		0,															// reserved
		0,0,0														// layer masks ignored
	};

	// �]�m�����榡
	//
	int pixelformat;
	if ((pixelformat = ::ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		AfxMessageBox("Fail to choose pixel format!");
		return FALSE;
	}
	if (::SetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
	{
		AfxMessageBox("Fail to set pixel format!");
		return FALSE;
	}

	// ���չ����榡
	//
	int npixelformat;
	npixelformat = GetPixelFormat(hDC);
	::DescribePixelFormat(hDC, npixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if (!(pfd.dwFlags & PFD_NEED_PALETTE))
		return NULL;

	return TRUE;
}

BOOL C3DProcess::ExtensionSupported(const char* exten)
{
	// DO : �X�i�䴩�T�{
	//
	static const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *)strchr(exten, ' ');
	if (where || *exten == '\0')
		return false;

	if (!extensions)
		extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;)
	{
		where = (GLubyte *)strstr((const char *)start, exten);
		if (!where)
			break;
		terminator = where + strlen(exten);
		if (where == start || *(where - 1) == ' ')
		{
			if (*terminator == ' ' || *terminator == '\0')
			{
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

void C3DProcess::PerspectiveBuild()
{
	// DO : �إ�openGL�z���Ŷ�
	//
	GLint	gl_x, gl_y;
	gl_x = m_3D_rect.right - m_3D_rect.left;
	gl_y = m_3D_rect.bottom - m_3D_rect.top;

	glViewport(0, 0, gl_x, gl_y);									// �]�w�ù��������f��ܽd��
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();												// �����e�x�}�ର���x�}
	gluPerspective(40.0, (GLfloat)gl_x/(GLfloat)gl_y, 0.5, 10.0);	// �إ߹�٪��z����ı�Ŷ�
	glMatrixMode(GL_MODELVIEW);
}

void C3DProcess::GLInitialization()
{
	// DO : openGLø�Ϫ�l��
	//
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
		return;
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// �إ�3D���z
	//
	glGenTextures(ImageFrame, textureName);					// �i�D openGL �t�m�@���O����Ŷ��s�����
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);					// ���� Ū��(or�ǿ�) �����ƾڹ���覡 ��
															// �w�] 4 bytes�A�]�� 1 byte �קK padding

	// ���J���z
	//
	LoadVolume(textureName);

	// ���w���z�y��(�إ�openGLø�ϮɡAglVertex()�һݭn���I)
	//
	register int i, j;
	float temp = 0.0f;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			glVertexPt[i * 8 + j][0] = (float)(j / 7.0f) - 0.5f;
			glVertexPt[i * 8 + j][1] = (float)(i / 7.0f) - 0.5f;
			glVertexPt[i * 8 + j][2] = -0.5f;
			temp = glVertexPt[i * 8 + j][0] * glVertexPt[i * 8 + j][0] +
				glVertexPt[i * 8 + j][1] * glVertexPt[i * 8 + j][1] +
				glVertexPt[i * 8 + j][2] * glVertexPt[i * 8 + j][2];
			temp = 1.0f / (float)sqrt(temp);
			glVertexPt[i * 8 + j][0] *= temp;
			glVertexPt[i * 8 + j][1] *= temp;
			glVertexPt[i * 8 + j][2] *= temp;
			glVertexPt[i * 8 + j][0] *= 2.0f;
			glVertexPt[i * 8 + j][1] *= 2.0f;
			glVertexPt[i * 8 + j][2] += 1.0f;
		}
	}
}

void C3DProcess::LoadVolume(unsigned int texName[10])
{

}

void C3DProcess::Draw2DImage(unsigned short &slice)
{
	//	DO : ø�s 2D �v��
	//
	CClientDC dc(m_2D_frame);

	PerspectiveBuild();

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
	// DO : �ʺA�t�m�G���x�}
	//
	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w*size];
	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}

