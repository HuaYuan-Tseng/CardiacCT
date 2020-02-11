
// C3DProcess.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "C3DProcess.h"
#include "CProgress.h"
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

	Mat_Offset = 0;
	ImageFrame = 1;
	DisplaySlice = 0;
	viewDistance = -4.0f;

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

	if (Mat_Offset != 0)
		Mat_Offset = 0;
	if (ImageFrame != 1)
		ImageFrame = 1;
	if (DisplaySlice != 0)
		DisplaySlice = 0;
	if (viewDistance != -4.0f)
		viewDistance = -4.0f;

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
	Draw3DImage(TRUE);

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

	// �إ�3D���z�x�s�Ŷ�
	//
	glGenTextures(ImageFrame, textureName);					// �i�D openGL �t�m�@���O����Ŷ��s�����
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);					// ���� Ū��(or�ǿ�) �����ƾڹ���覡 ��
															// �w�] 4 bytes�A�]�� 1 byte �קK padding

	// ���J���z
	//
	PrepareVolume(textureName);

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

void C3DProcess::PrepareVolume(unsigned int texName[10])
{
	// DO : �إ߯��z
	//
	int max = 255;
	float pixel = 0.0f;
	register int i, j, k;

	// �w�ƭn�Ψӫإ߯��z����Ưx�}
	//
	int Row = ROW;
	int Col = COL;
	int TotalSlice = Total_Slice;
	BYTE img1[256][256][256][4] = {0};

	Mat_Offset = (512 - Total_Slice) / 2;

	CProgress* m_progress = new CProgress();
	m_progress->Create(IDD_DIALOG_PROGRESSBAR);
	m_progress->ShowWindow(SW_NORMAL);
	m_progress->Set(Total_Slice/2, 0);
	m_progress->SetStatic("Construct 3D Image...");

	i = 0;	j = 0;	k = 0;
	if (m_pDoc->m_img != NULL)
	{
		while (k < TotalSlice)
		{
			for (j = 0; j < Row; j += 2)
			{
				for (i = 0; i < Col; i += 2)
				{
					if (i == 0 || i >= 510/2 || j == 0 || j >= 510/2)
						pixel = 0;
					else
						pixel = m_pDoc->m_img[k][j*Col + i];

					getRamp(img1[i/2][j/2][k/2], (float)pixel / (float)max / 2, 0);
				}
			}
			k += 2;
			m_progress->GetPro(k);
		}
	}
	m_progress->DestroyWindow();
	delete m_progress;

	//--------------------------------------------------------------------------//
	// �إ߯��z
	//
	if (gl_3DTexture)
	{
		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		// ���z�y�Шt�ΡGS����b�BT���a�b�BR�����J�ù����b
		// �]�m���z��¶�Ҧ�
		//
		glBindTexture(GL_TEXTURE_3D, texName[0]);								// �j�w���z�]���w����, ���z��HID�^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// ���z�L�o��ơ]����o��^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// ��j�ɪ��o��覡
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		// �Y�p�ɪ��o��覡
		glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, img1);											// �Ы�3D���z
	}

}

void C3DProcess::getRamp(GLubyte* color, float t, int n)
{
	// DO : �p��RGBA���ƭ�
	//
	t *= 2.0f;
	if (n == 0)				// Gray Scale
	{
		color[0] = 255 * t; // R
		color[1] = 255 * t; // G
		color[2] = 255 * t; // B
		color[3] = 60 * t;	// A
	}
	else if (n == 1)		// Red
	{
		color[0] = 255 * t;
		color[1] = 0;
		color[2] = 0;
		color[3] = 255 * t;
	}
	else if (n == 2)		// Green
	{
		color[0] = 0;
		color[1] = 255 * t;
		color[2] = 0;
		color[3] = 255 * t;
	}
	else if (n == 3)		// Blue
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = 255 * t;
		color[3] = 255 * t;
	}
	else if (n == 4)		// Orange
	{
		color[0] = 255 * t;
		color[1] = 100 * t;
		color[2] = 50 * t;
		color[3] = 255 * t;
	}
	else if (n == 5)		// Purple
	{
		color[0] = 255 * t;
		color[1] = 0;
		color[2] = 255 * t;
		color[3] = 255 * t;
	}
	else if (n == 6)		// Water Blue
	{
		color[0] = 0;
		color[1] = 255 * t;
		color[2] = 255 * t;
		color[3] = 255 * t;
	}
}

void C3DProcess::Draw3DImage(BOOL which)
{
	// DO : ø�s 3D �v��
	//
	CClientDC dc(m_3D_frame);

	// openGL �q�{ 4X4 �x�}�P�O���� 1D �}�C���Y
	// �z				 �{
	// ��	 m0	 m4  m8  m12 ��	
	// �� m1	 m5  m9  m13 ��
	// ��	 m2  m6  m10 m14 ��
	// �� m3  m7  m11 m15 ��
	// �|				 �}
	// (m12, m13, m14)�O�Χ@ Translation�A(m15)�O�����y�С]�Χ@ Projection�^�A
	// ���W 9 �Ӥ����Χ@ Rotate �M Scale�C

	// xform matrices(�Ȧs����᪺�ҫ��x�}?????)
	static float objectXform[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	static float planeXform[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// clip planes equation�]A, B, C, Z�^
	// Ax + By + Cz = 0�A�p�G�O(0, -1, 0, 0)�A
	// �N��O y<0 ���~����ܡA�̫�@�ӰѼƬ�"�qz=0�����}�l"
	double clip0[] = { -1.0,  0.0,  0.0, 1.0 };
	double clip1[] = { 1.0,  0.0,  0.0, 1.0 };
	double clip2[] = { 0.0, -1.0,  0.0, 1.0 };
	double clip3[] = { 0.0,  1.0,  0.0, 1.0 };
	double clip4[] = { 0.0,  0.0, -1.0, 1.0 };
	double clip5[] = { 0.0,  0.0,  1.0, 1.0 };

	// texgen planes
	float xPlane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float yPlane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	float zPlane[] = { 0.0f, 0.0f, 1.0f, 0.0f };

	float mat[16];
	float temp[3];
	float plane[12];
	int ii, gg, hh;
	int clip;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);								// �Ұʲ`�״���(�S���}�Ҫ��ܡA��Ӫ���|���I�z��)

	// �إ��[�ݪ���(��Ŧ)�� �z�������� �P ������V�B�Z��(viewDistance)
	//
	gluPerspective(90, 1, 1, 700);							// ����(���ѱ�)�o�ӡA3d seed���ĪG�N�|���n�ڡ�QQ
	glMatrixMode(GL_MODELVIEW);								// �ҫ����ϯx�}
		//	glGetFloatv(GL_MODELVIEW_MATRIX, testmat);
	glLoadIdentity();										// testmat[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
		//	glGetFloatv(GL_MODELVIEW_MATRIX, testmat);
	glTranslatef(0.0f, 0.0f, viewDistance);					// testmat[1,0,0,0, 0,1,0,0, 0,0,1,-4, 0,0,0,1] (�@�}�l�����A)
		//	glGetFloatv(GL_MODELVIEW_MATRIX, testmat);

	// �����(��Ŧ)����
	if (mode == MoveModes::MoveObject || mode == MoveModes::MoveView)
	{
		// Have OpenGL compute the new transformation (simple but slow)
		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(angle, axis[0], axis[1], axis[2]);
		glMultMatrixf((GLfloat *)objectXform);
		glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)objectXform);
		glPopMatrix();
	}
	glMultMatrixf((GLfloat *)objectXform);		// ������(��Ŧ)����A�R����N�S����k����F

	// �����(���ӥ���)����
	if (gbPlaneMove)
	{
		// handle the plane rotations
		temp[0] = objectXform[0] * pAxis[0] + objectXform[4] * pAxis[1] + objectXform[8] * pAxis[2];
		temp[1] = objectXform[1] * pAxis[0] + objectXform[5] * pAxis[1] + objectXform[9] * pAxis[2];
		temp[2] = objectXform[2] * pAxis[0] + objectXform[6] * pAxis[1] + objectXform[10] * pAxis[2];

		glPushMatrix();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(pAngle, temp[0], temp[1], temp[2]);
		glMultMatrixf(planeXform);
		glGetFloatv(GL_MODELVIEW_MATRIX, planeXform);
		glPopMatrix();
	}

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

