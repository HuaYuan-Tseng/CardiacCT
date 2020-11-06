
// CPhantom.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "CPhantom.h"
#include "CProgress.h"
#include "CWait.h"
#include "afxdialogex.h"

#include <vector>
#include <queue>

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))
#define M_PI 3.1415

//=======================//
//   3D Phantom Dialog   //
//=======================//

IMPLEMENT_DYNAMIC(CPhantom, CDialogEx)

CPhantom::CPhantom(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPHANTOM, pParent)
	, _PHANTOM_3D_SEED(FALSE)
	, _display_slice(0)
	, _total_slice(0)
	, _row(512)
	, _col(512)
	, _EDIT_1(_T(""))
	, _EDIT_2(_T(""))
	, _EDIT_3(_T(""))
	, _EDIT_4(_T(""))
	, _EDIT_5(_T(""))
{
	mode = ControlModes::ControlObject;

	_img = nullptr;
	_judge = nullptr;
	_2D_dib = nullptr;
	_2D_frame = nullptr;
	_3D_frame = nullptr;
	gl_3DTexture = FALSE;

	_get_2D_seed = false;
	_get_3D_seed = false;
	_act_rotate = false;
	_act_translate = false;
	_get_region_grow = false;
	_get_2D_image = false;
	_get_3D_image = false;
	_get_GL_build = true;
	
	_x_index = 0.5F;
	_y_index = 0.5F;
	_z_index = 0.7F;
	_scale_x = 0.5F;
	_scale_y = 0.5F;
	_scale_z = 0.5F;
	_density = 0.000F;
	_intensity = 0.8125F;
	_view_distance = -4.0F;
	
	_transY = 0.0F;
	_image_frame = 1;
	_gl_slices = 512;
	_obj_angle = 0.0F;
	_pln_angle = 0.0F;

	_seed_pt = { 0, 0, 0 };
	_seed_img = { 0, 0, 0 };
	_seed_gl = { 0.0L, 0.0L, 0.0L };

	_glVertexPt = New2Dmatrix(64, 3, float);
	_last_pos = new float[3]{ 0.0F, 0.0F, 0.0F };
	_obj_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	_pln_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	_user_plane = new double[4]{ 1.0L, 0.0L, 0.0L, 1.0L };

}

CPhantom::~CPhantom()
{
	if (_2D_dib != nullptr)
		delete _2D_dib;
	if (_img != nullptr)
		delete[] _img;
	if (_user_plane != nullptr)
		delete[] _user_plane;
	if (_glVertexPt != nullptr)
		delete[] _glVertexPt;
	if (_obj_axis != nullptr)
		delete[] _obj_axis;
	if (_pln_axis != nullptr)
		delete[] _pln_axis;
	if (_last_pos != nullptr)
		delete[] _last_pos;
	if (_judge != nullptr)
		delete[] _judge;
	
}

void CPhantom::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_PHANTOM, _scroll_bar);

	DDX_Check(pDX, IDC_CHECK_PHANTOM_3D_SEED, _PHANTOM_3D_SEED);

	DDX_Text(pDX, IDC_EDIT_PHANTOM_1, _EDIT_1);
	DDX_Text(pDX, IDC_EDIT_PHANTOM_2, _EDIT_2);
	DDX_Text(pDX, IDC_EDIT_PHANTOM_3, _EDIT_3);
	DDX_Text(pDX, IDC_EDIT_PHANTOM_4, _EDIT_4);
	DDX_Text(pDX, IDC_EDIT_PHANTOM_5, _EDIT_5);

}

BEGIN_MESSAGE_MAP(CPhantom, CDialogEx)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	
	ON_BN_CLICKED(IDC_BUTTON_PHANTOM_OPEN, &CPhantom::OnBnClickedButtonPhantomOpen)
	ON_BN_CLICKED(IDC_CHECK_PHANTOM_3D_SEED, &CPhantom::OnBnClickedCheckPhantom3dSeed)
	ON_BN_CLICKED(IDC_BUTTON_PHANTOM_SEED_CLEAR, &CPhantom::OnBnClickedButtonPhantomSeedClear)
	ON_BN_CLICKED(IDC_BUTTON_PHANTOM_REGION_GROWING, &CPhantom::OnBnClickedButtonPhantomRegionGrowing)
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

	_3D_frame = GetDlgItem(IDC_STATIC_PHANTOM_3D);
	_3D_frame->GetWindowRect(&_3D_rect);
	ScreenToClient(&_3D_rect);

	_2D_dib = new CDIB();
	_2D_dib->InitDIB(_col, _row);											// ��l�Ƶe��

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPhantom::OnPaint()
{
	// ø�Ϩƥ�
	//
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDialogEx::OnPaint() for painting messages
	if (!_get_GL_build)
	{
		/* openGL�Ŷ��إ� */
		_hDC = ::GetDC(_3D_frame->m_hWnd);										// ��o�e������DC��HANDLE�]hDC�^
		SetupPixelFormat(_hDC);
		if ((_hRC = ::wglCreateContext(_hDC)) == 0)								// ���� openGL �һݪ��e���]hRC�^
		{
			AfxMessageBox("Fail to create hRC context!");
			return;
		}
		if (::wglMakeCurrent(_hDC, _hRC) == FALSE)								// �إ� hDC �P hRC �������s��
		{
			AfxMessageBox("Fail to make current!");
			return;
		}
		PerspectiveBuild();														// �إ�openGL�z���Ŷ�
		gl_3DTexture = ExtensionSupported("GL_EXT_texture3D");					// �T�{�O�_�䴩3D���z
		if (gl_3DTexture == TRUE)
		{
			// Return the address of an openGL extension function.
			glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");

			GLInitialization();
		}
		else
		{
			AfxMessageBox("This program requires 3D Texture support!");
			return;
		}

		if (::wglMakeCurrent(_hDC, _hRC) == FALSE)
		{
			AfxMessageBox("Fail to make current!");
			return;
		}
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		_get_GL_build = true;
	}

	if (_get_2D_image)
		Draw2DImage(_display_slice);
	if (_get_3D_image)
		Draw3DImage(true);

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

	/* �b �T�� �v�������d�� */
	if (mpt.x < _3D_rect.right && mpt.x > _3D_rect.left && mpt.y < _3D_rect.bottom && mpt.y > _3D_rect.top)
	{	
		if (mode == ControlModes::ControlObject)
		{
			if (zDelta < 0)
				_view_distance += 0.05F;
			else if (zDelta > 0)
				_view_distance -= 0.05F;
		}
		else if (mode == ControlModes::ControlPlane)
		{
			if (zDelta < 0)
				_user_plane[3] += 0.01F;
			else if (zDelta > 0)
				_user_plane[3] -= 0.01F;
		}
		Draw3DImage(true);
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

void CPhantom::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ����ʨƥ�
	//
	/* �b �T�� �v������ */
	if (point.x < _3D_rect.right && point.x > _3D_rect.left && point.y < _3D_rect.bottom && point.y > _3D_rect.top)
	{	
		if (nFlags == MK_LBUTTON || nFlags == MK_RBUTTON)
		{
			ActTracking(point.x - _3D_rect.left, point.y - _3D_rect.top);

			Draw3DImage(true);
		}
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

void CPhantom::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ����� ���U �ƥ�
	//
	/* �b �T�� ���� */
	if (point.x < _3D_rect.right && point.x > _3D_rect.left && point.y < _3D_rect.bottom && point.y > _3D_rect.top)
	{
		ActStart(nFlags, point.x - _3D_rect.left, point.y - _3D_rect.top);

		if (_PHANTOM_3D_SEED == TRUE)
		{	// �I��3D�ؤl�I�\��
			if (_get_3D_seed == false)
			{
				GLfloat		win_x, win_y, win_z;
				GLdouble	obj_x, obj_y, obj_z;

				GLint* viewPort = new GLint[16];
				GLdouble* modelView_matrix = new GLdouble[16];
				GLdouble* projection_matrix = new GLdouble[16];

				glGetIntegerv(GL_VIEWPORT, viewPort);
				glGetDoublev(GL_MODELVIEW_MATRIX, modelView_matrix);
				glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);

				win_x = (GLfloat)(point.x - _3D_rect.left);
				win_y = (GLfloat)(viewPort[3] - (point.y - _3D_rect.top));
				glReadPixels((int)win_x, (int)win_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &win_z);
				gluUnProject(win_x, win_y, win_z, modelView_matrix, projection_matrix, viewPort, &obj_x, &obj_y, &obj_z);

				_seed_gl.x = obj_z;						// x_axis
				_seed_gl.y = obj_y;						// y_axis
				_seed_gl.z = obj_x;						// slice

				_seed_img = coordiConvert(_seed_gl);		// �y���I�P��Ưx�}��m���ഫ

				if (_seed_img.z >= 0 && _seed_img.z <= _total_slice - 1)
				{
					if (_seed_img.y >= 0 && _seed_img.y <= _row - 1)
					{
						if (_seed_img.x >= 0 && _seed_img.x <= _col - 1)
						{
							_get_3D_seed = true;
							_display_slice = _seed_img.z;
							_scroll_bar.SetScrollPos(_display_slice);

							//--------------------------------------------------------------------------//

							short pos_1 = _seed_img.x;
							short pos_2 = _seed_img.y;
							short pos_3 = _seed_img.z;
							short pos_4 = _img[_seed_img.z][_seed_img.y * _col + _seed_img.x];

							_EDIT_1.Format("%d", (int)pos_1);
							_EDIT_2.Format("%d", (int)pos_2);
							_EDIT_3.Format("%d", (int)pos_3);
							_EDIT_4.Format("%d", (int)pos_4);

						}
					}
				}
				delete[] viewPort;
				delete[] modelView_matrix;
				delete[] projection_matrix;

				UpdateData(FALSE);
				Draw3DImage(true);
				Draw2DImage(_display_slice);
			}
		}
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CPhantom::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ����� ��} �ƥ�
	//
	/* �b �T�� ���� */
	if (point.x < _3D_rect.right && point.x > _3D_rect.left && point.y < _3D_rect.bottom && point.y > _3D_rect.top)
	{
		ActStop(nFlags, point.x - _3D_rect.left, point.y - _3D_rect.top);

		Draw3DImage(true);
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CPhantom::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ��k�� ���U �ƥ�
	//
	if (point.x < _3D_rect.right && point.x > _3D_rect.left && point.y < _3D_rect.bottom && point.y > _3D_rect.top)
	{
		ActStart(nFlags, point.x - _3D_rect.left, point.y - _3D_rect.top);

		Draw3DImage(true);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}

void CPhantom::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// �ƹ��k�� ��} �ƥ�
	//
	if (point.x < _3D_rect.right && point.x > _3D_rect.left && point.y < _3D_rect.bottom && point.y > _3D_rect.top)
	{
		ActStop(nFlags, point.x - _3D_rect.left, point.y - _3D_rect.top);

		Draw3DImage(true);
	}
	CDialogEx::OnRButtonUp(nFlags, point);
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
	unsigned long n = 0;
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
				if (_img[slice][j * width + i] > 0)
					n += 1;
			}
		}

		img.Destroy();
	}
	img_path_list.clear();
	img_path_list.shrink_to_fit();
	TRACE1("Original volume = %d. \n", n);
	
	/* ��l�Ư��z�x�}�H�ΰϰ즨���P�w���x�}�j�p�M��l�� */
	register int i, j;
	int totalx = _row * _col;
	int totaly = _total_slice;

	if (_judge != nullptr)
		delete[] _judge;
	_judge = New2Dmatrix(_total_slice, _row * _col, short);

	for (j = 0; j < totaly; j++)
	{
		for (i = 0; i < totalx; i++)
		{
			_judge[j][i] = 0;
		}
	}
	for (j = 0; j < 256 * 256 * 256; j++)
	{
		for (i = 0; i < 4; i++)
		{
			_image0[j][i] = 0;
		}
	}

	_mat_offset = (512 - _total_slice) / 2;

	_x_index = (2.0F / 511.0F) * (_col / 2.0F) / 2;
	_x_index = _x_index / _scale_y;
	_y_index = (2.0F / 511.0F) * (_row / 2.0F) / 2;
	_y_index = _y_index / _scale_z;
	_z_index = (2.0F / 511.0F) * (_total_slice / 2.0F) / 2;
	_z_index = _z_index / _scale_x;

	/* �]�wScrollBar���d�� */
	_scroll_bar.SetScrollRange(0, _total_slice - 1);

	_get_GL_build = false;
	_get_2D_image = true;
	_get_3D_image = true;
	Invalidate();
	
}

void CPhantom::OnBnClickedButtonPhantomSeedClear()
{
	// TODO: Add your control notification handler code here
	// Button : 3D Seed Clear
	//
	if (_get_3D_seed)
	{
		_seed_gl.x = 0.0L;
		_seed_gl.y = 0.0L;
		_seed_gl.z = 0.0L;

		_seed_img.x = 0;
		_seed_img.y = 0;
		_seed_img.z = 0;

		_EDIT_1.Format("%3d", 0);
		_EDIT_2.Format("%3d", 0);
		_EDIT_3.Format("%3d", 0);
		_EDIT_4.Format("%3d", 0);

		_get_3D_seed = false;

		UpdateData(FALSE);
		Draw3DImage(true);
		Draw2DImage(_display_slice);
	}
}

void CPhantom::OnBnClickedCheckPhantom3dSeed()
{
	// TODO: Add your control notification handler code here
	//
	UpdateData(TRUE);
	Draw3DImage(true);
	Draw2DImage(_display_slice);
}

void CPhantom::OnBnClickedButtonPhantomRegionGrowing()
{
	// TODO: Add your control notification handler code here
	// Button :3D Region Growing
	//
	if (!_get_3D_seed) return;
	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("Region growing...");

	// �ŧi �����з� �Ѽ�
	//
	_RG_term.seed = _seed_img;
	_RG_term.s_kernel = 3;
	_RG_term.n_kernel = 3;
	_RG_term.pix_thresh = 50.0;
	_RG_term.sd_thresh = 20.0;
	_RG_term.sd_coeffi = 1.5;

	RG_3D_ConfidenceConnected(_judge, _RG_term);

	_get_region_grow = true;

	_volume = Calculate_volume(_judge);
	_EDIT_5.Format("%f", _volume);
	TRACE1("Growing volume = %f. \n", _volume);

	PrepareVolume();
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(_display_slice);
	m_wait->DestroyWindow();
	delete m_wait;

}

//========================//
//   CPhantom Functions   //
//========================//

void CPhantom::RG_3D_GlobalAvgConnected(short** src, RG_factor& factor)
{
	//	DO : 3D �ϰ즨�� 
	//	�Q�Ρu��e�w���������쥭���ȡv�Ӭɩw�����зǡA�åΡu�ثe�������j�סv�ӧP�_
	//
	int obj = 1;
	const int row = _row;
	const int col = _col;
	const int totalSlice = _total_slice;
	const int range = (factor.s_kernel - 1) / 2;	// �P�_�d��
	const double threshold = factor.pix_thresh;
	Seed_s seed = factor.seed;						// ��lseed

	double avg;
	unsigned long long cnt = 1;						// �p�Ʀ�����pixel�ƶq

	Seed_s temp;									// ��e �P�_���P��seed
	Seed_s current;									// ��e �P�_������seed
	std::queue<double> avg_que;						// �Ȧs�Y�I�����P�_���A��U�w�����ϰ쪺����avg
	std::queue<Seed_s> sed_que;						// �Ȧs�����P�_���ؤl�I��������m

	avg = _img[seed.z][(seed.y) * col + (seed.x)];
	src[seed.z][(seed.y) * col + (seed.x)] = obj;
	sed_que.push(seed);
	avg_que.push(avg);

	while (!sed_que.empty())
	{
		avg = avg_que.front();
		current = sed_que.front();

		// �P�_�P��ϰ�O�_�ŦX�����з�
		// �æP�ɭp��u�w�����ϰ�v���`����
		register int i, j, k;
		for (k = -range; k <= range; ++k)
		{
			for (j = -range; j <= range; ++j)
			{
				for (i = -range; i <= range; ++i)
				{
					if ((current.x + i) < (col) && (current.x + i) >= 0 &&
						(current.y + j) < (row) && (current.y + j) >= 0 &&
						(current.z + k) < (totalSlice) && (current.z + k) >= 0)
					{
						if (src[current.z + k][(current.y + j) * col + (current.x + i)] == 0)
						{
							short n_pixel =
								_img[current.z + k][(current.y + j) * col + (current.x + i)];

							if (abs(n_pixel - avg) <= threshold)
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								sed_que.push(temp);

								src[current.z + k][(current.y + j) * col + (current.x + i)] = obj;

								avg = (avg * cnt + n_pixel) / (cnt + 1);
								avg_que.push(avg);
								cnt += 1;
							}
							else
								src[current.z + k][(current.y + j) * col + (current.x + i)] = -obj;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

}

void CPhantom::RG_3D_ConfidenceConnected(short** src, RG_factor& factor)
{
	// DO : 3D �ϰ즨��
	// �Q�η�e�ϰ쪺�u�����ȡv�P�u�зǮt�v�ɩw�����зǡA�åH�u�����j�סv�ӧP�_
	//
	int obj = 1;
	const int row = _row;
	const int col = _col;
	const int totalSlice = _total_slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const double sd_coeffi = factor.sd_coeffi;
	const double sd_thresh = factor.sd_thresh;
	const double pix_thresh = factor.pix_thresh;
	Seed_s	seed = factor.seed;

	Seed_s	n_site;
	Seed_s	s_current;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	double	s_avg;
	unsigned long long	s_cnt = 0;
	unsigned long long  n_pixel = 0, s_pixel = 0;

	s_avg = _img[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = obj;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;

	auto outOfImg = [=](int px, int py, int pz)
	{	// �P�_���L�W�X�v�����
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else
			return true;
	};

	while (!sed_que.empty())
	{
		register int si, sj, sk;
		s_avg = avg_que.front();
		s_current = sed_que.front();

		// �p�� �`�X �P ����
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						sum +=
							_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						cnt += 1;
					}
				}
			}
		}
		n_avg = sum / cnt;

		// �p�� �зǮt
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						n_pixel =
							_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);

		// ��w�B�ץ������зǪ��W�U��
		double up_limit = n_avg + (sd_coeffi * n_sd);
		double down_limit = n_avg - (sd_coeffi * n_sd);

		// �P�_�O�_�ŦX�����з�
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] == 0)
						{
							n_pixel =
								_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];

							if ((n_sd <= sd_thresh && abs(n_pixel - s_avg) <= pix_thresh))
							{
								n_site.x = s_current.x + si;
								n_site.y = s_current.y + sj;
								n_site.z = s_current.z + sk;
								sed_que.push(n_site);

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = obj;
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
							else
								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = -obj;
						}
					}
				}
			}
		}
		sed_que.pop();
		avg_que.pop();
	}

}

double CPhantom::Calculate_volume(short** src)
{
	int obj = 1;
	register int i, j;
	const int totalXY = _col * _row;
	const int totalSlice = _total_slice;
	double n = 0;							// �p�Ʀ�����pixel�ƶq
	for (j = 0; j < totalSlice; ++j)
	{
		for (i = 0; i < totalXY; ++i)
		{
			if (src[j][i] == obj)
				n += 1;
		}
	}
	return n;
}

BOOL CPhantom::SetupPixelFormat(HDC hDC)
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

BOOL CPhantom::ExtensionSupported(const char* exten)
{
	// DO : �X�i�䴩�T�{
	//
	static const GLubyte* extensions = NULL;
	const GLubyte* start;
	GLubyte* where, * terminator;

	where = (GLubyte*)strchr(exten, ' ');
	if (where || *exten == '\0')
		return false;

	if (!extensions)
		extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;)
	{
		where = (GLubyte*)strstr((const char*)start, exten);
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

void CPhantom::PerspectiveBuild()
{
	// DO : �إ�openGL�z���Ŷ�
	//
	GLint	gl_x, gl_y;
	gl_x = _3D_rect.right - _3D_rect.left;
	gl_y = _3D_rect.bottom - _3D_rect.top;

	glViewport(0, 0, gl_x, gl_y);									// �]�w�ù��������f��ܽd��
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();												// �����e�x�}�ର���x�}
	gluPerspective(40.0, (GLfloat)gl_x / (GLfloat)gl_y, 0.5, 10.0);	// �إ߹�٪��z����ı�Ŷ�
	glMatrixMode(GL_MODELVIEW);
}

void CPhantom::GLInitialization()
{
	// DO : openGLø�Ϫ�l��
	//
	if (::wglMakeCurrent(_hDC, _hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
		return;
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// �إ�3D���z�x�s�Ŷ�
	//
	glGenTextures(_image_frame, _textureName);					// �i�D openGL �t�m�@���O����Ŷ��s�����
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);					// ���� Ū��(or�ǿ�) �����ƾڹ���覡 ��
															// �w�] 4 bytes�A�]�� 1 byte �קK padding
	// ���J���z
	//
	PrepareVolume();

	// �إ߳��I�y��(�إ�openGLø�ϮɡAglVertex()�һݭn���I)
	//
	register int i, j;
	float temp = 0.0f;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			_glVertexPt[i * 8 + j][0] = (float)(j / 7.0f) - 0.5f;
			_glVertexPt[i * 8 + j][1] = (float)(i / 7.0f) - 0.5f;
			_glVertexPt[i * 8 + j][2] = -0.5f;

			temp = _glVertexPt[i * 8 + j][0] * _glVertexPt[i * 8 + j][0] +
				_glVertexPt[i * 8 + j][1] * _glVertexPt[i * 8 + j][1] +
				_glVertexPt[i * 8 + j][2] * _glVertexPt[i * 8 + j][2];
			temp = 1.0f / (float)sqrt(temp);

			_glVertexPt[i * 8 + j][0] *= temp;
			_glVertexPt[i * 8 + j][1] *= temp;
			_glVertexPt[i * 8 + j][2] *= temp;

			_glVertexPt[i * 8 + j][0] *= 2.0f;
			_glVertexPt[i * 8 + j][1] *= 2.0f;
			_glVertexPt[i * 8 + j][2] += 1.0f;
		}
	}
}

void CPhantom::PrepareVolume()
{
	// DO : �إ߯��z����Ưx�}
	//
	if (_img == NULL)	return;

	// �w�ƭn�Ψӫإ߯��z����Ưx�}
	//
	const int row = _row;
	const int col = _col;
	const int totalSlice = _total_slice;
	const int sample_start = 0 + _mat_offset;
	const int sample_end = 0 + _mat_offset + totalSlice;

	float pixel = 0.0F;
	register int i, j, k;

	CProgress* m_progress = new CProgress();
	m_progress->Create(IDD_DIALOG_PROGRESSBAR);
	m_progress->ShowWindow(SW_NORMAL);
	m_progress->SetInitial(0, 1, totalSlice / 2);
	m_progress->SetStatic("Construct 3D Image...");

	i = 0;	j = 0;	k = 0;

	if (!_get_3D_image)
	{
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				for (j = 2; j < row - 2; j += 2)
				{
					for (i = 2; i < col - 2; i += 2)
					{
						pixel = _img[k - (_mat_offset + 1)][j * col + i];

						getRamp(&_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							pixel / 255.0F, 0);
					}
				}
			}
			k += 2;
			m_progress->Run();
		}
	}
	else
	{
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				for (j = 2; j < row - 2; j += 2)
				{
					for (i = 2; i < col - 2; i += 2)
					{
						pixel = _img[k - (_mat_offset + 1)][j * col + i];

						if (_judge[k - (_mat_offset + 1)][j * col + i] <= 0)
						{
							getRamp(&_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								pixel / 255.0F, 0);
						}
						else
						{
							getRamp(&_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								pixel / 255.0F, 1);
						}
					}
				}
			}
			k += 2;
			m_progress->Run();
		}
	}

	LoadVolume();
	m_progress->DestroyWindow();
	delete m_progress;
}

void CPhantom::LoadVolume()
{
	// DO : �إ�3D���z
	//
	if (gl_3DTexture)
	{
		float color[] = { 1.0F, 1.0F, 1.0F, 1.0F };

		// ���z�y�Шt�ΡGS����b�BT���a�b�BR�����J�ù����b
		// �]�m���z��¶�Ҧ�
		//
		glBindTexture(GL_TEXTURE_3D, _textureName[0]);							// �j�w���z�]���w����, ���z��HID�^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// ���z�L�o��ơ]����o��^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// ��j�ɪ��o��覡
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		// �Y�p�ɪ��o��覡
		glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, (GLvoid*)_image0);								// �Ы�3D���z
	}
}

void CPhantom::getRamp(GLubyte* color, float t, int n)
{
	// DO : �p��RGBA���ƭ�
	//
	if (n == 0)							// Gray Scale
	{
		color[0] = (GLubyte)(255 * t);	// R
		color[1] = (GLubyte)(255 * t);	// G
		color[2] = (GLubyte)(255 * t);	// B
		color[3] = (GLubyte)(60 * t);	// A
	}
	else if (n == 1)					// Red
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = 0;
		color[2] = 0;
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 2)					// Green
	{
		color[0] = 0;
		color[1] = (GLubyte)(255 * t);
		color[2] = 0;
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 3)					// Blue
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 4)					// Orange
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = (GLubyte)(100 * t);
		color[2] = (GLubyte)(50 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 5)					// Purple
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = 0;
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 6)					// Water Blue
	{
		color[0] = 0;
		color[1] = (GLubyte)(255 * t);
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
}

void CPhantom::Draw3DImage(bool which)
{
	// DO : ø�s 3D �v��
	//
	CClientDC dc(_3D_frame);

	// openGL �q�{ 4X4 �x�}�P�O���� 1D �}�C���Y
	// �z				 �{
	// ��	 m0	 m4  m8  m12 ��	
	// �� m1	 m5  m9  m13 ��
	// ��	 m2  m6  m10 m14 ��
	// �� m3  m7  m11 m15 ��
	// �|				 �}
	// (m12, m13, m14)�O�Χ@ Translation�A(m15)�O�����y�С]�Χ@ Projection�^�A
	// ���W 9 �Ӥ����Χ@ Rotate �M Scale�C

	// Xform matrices(����᪺�ҫ��x�})
	//
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
	// Ax + By + Cz + D = 0�A�p�G�O(0, -1, 0, 0)�A
	// �N��O y<0 ���~����ܡC
	//
	double clip0[] = { -1.0,  0.0,  0.0, 1.0 };
	double clip1[] = { 1.0,  0.0,  0.0, 1.0 };
	double clip2[] = { 0.0, -1.0,  0.0, 1.0 };
	double clip3[] = { 0.0,  1.0,  0.0, 1.0 };
	double clip4[] = { 0.0,  0.0, -1.0, 1.0 };
	double clip5[] = { 0.0,  0.0,  1.0, 1.0 };

	// Texgen planes
	//
	float xPlane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float yPlane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	float zPlane[] = { 0.0f, 0.0f, 1.0f, 0.0f };

	float mat[16];
	float temp[3];
	float plane[12];

	int clip;
	register int i, j, k;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);								// �Ұʲ`�״���(�S���}�Ҫ��ܡA��Ӫ���|���I�z��)

	// �إ��[�ݪ��� �z��������B������V�B�Z��(viewDistance)
	//
	gluPerspective(90, 1, 1, 700);							// ����(���ѱ�)�o�ӡA3D seed ���ĪG�N�|���n��~
	glMatrixMode(GL_MODELVIEW);								// �ҫ����ϯx�}
	glLoadIdentity();										// testmat[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
	glTranslatef(0.0f, 0.0f, _view_distance);					// testmat[1,0,0,0, 0,1,0,0, 0,0,1,-4, 0,0,0,1]

	// ���� ��Ŧ ����
	//
	if (mode == ControlModes::ControlObject)
	{
		glPushMatrix();
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(_obj_angle, _obj_axis[0], _obj_axis[1], _obj_axis[2]);
			glMultMatrixf((GLfloat*)objectXform);
			glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)objectXform);
		}
		glPopMatrix();
	}
	glMultMatrixf((GLfloat*)objectXform);					// ������(��Ŧ)����A�R����N�S����k����F

	// ���� ���ӥ��� ����
	//
	if ((mode == ControlModes::ControlPlane) && (_act_rotate == true))
	{
		// handle the plane rotations
		temp[0] = objectXform[0] * _pln_axis[0] + objectXform[4] * _pln_axis[1] + objectXform[8] * _pln_axis[2];
		temp[1] = objectXform[1] * _pln_axis[0] + objectXform[5] * _pln_axis[1] + objectXform[9] * _pln_axis[2];
		temp[2] = objectXform[2] * _pln_axis[0] + objectXform[6] * _pln_axis[1] + objectXform[10] * _pln_axis[2];

		glPushMatrix();
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(_pln_angle, temp[0], temp[1], temp[2]);
			glMultMatrixf((GLfloat*)planeXform);
			glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)planeXform);
		}
		glPopMatrix();
	}

	// �u�ݭn Rotation�A�ҥH�N Translation �]�� 0
	//
	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	InvertMat(mat);

	mat[12] = 0.0F;
	mat[13] = 0.0F;
	mat[14] = 0.0F;

	// ��o user_Plane ��������{���Y��
	// �� ���ӥ��� �b�ۨ����Шt�� x �b��V�첾�ɡA
	// �� ����(��Ŧ) �H ���ӥ��� �� x �b ���ʤ�V�ѭ�]�������^�C
	//
	_user_plane[0] = -planeXform[0];
	_user_plane[1] = -planeXform[1];
	_user_plane[2] = -planeXform[2];

	// �۰ʥͦ����z�y�Шt
	//
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

	// �إߵ��ť���(Clip plane)���|�Ө������I
	// �p�� clip plane �|�Ө������I plane[16]={1,-1,-1, 1,-1,1, 1,1,-1, 1,1,1}
	//
	for (i = 0; i < 4; i++)
	{
		plane[i * 3 + 0] = (float)(planeXform[0] * _user_plane[3]);
		plane[i * 3 + 1] = (float)(planeXform[1] * _user_plane[3]);
		plane[i * 3 + 2] = (float)(planeXform[2] * _user_plane[3]);
		plane[i * 3 + 0] += (float)(planeXform[4] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 1] += (float)(planeXform[5] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 2] += (float)(planeXform[6] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 0] += (float)(planeXform[8] * ((i & 0x1) ? 1.0f : -1.0f));
		plane[i * 3 + 1] += (float)(planeXform[9] * ((i & 0x1) ? 1.0f : -1.0f));
		plane[i * 3 + 2] += (float)(planeXform[10] * ((i & 0x1) ? 1.0f : -1.0f));
	}

	// �M��P�[���̬ۤϪ����ť���
	//
	if (fabs(objectXform[2]) > fabs(objectXform[6]))
	{
		if (fabs(objectXform[2]) > fabs(objectXform[10]))
		{
			if (objectXform[2] > 0.0f)		// X is largest
				clip = 1;					// positive
			else
				clip = 0;					// negative
		}
		else
		{
			if (objectXform[10] > 0.0f)		// Z is largest
				clip = 5;					// positive
			else
				clip = 4;					// negative
		}
	}
	else
	{
		if (fabs(objectXform[6]) > fabs(objectXform[10]))
		{
			if (objectXform[6] > 0.0f)		// Y is largest
				clip = 3;					// positive
			else
				clip = 2;					// negative
		}
		else
		{
			if (objectXform[10] > 0.0f)		// Z is largest
				clip = 4;					// positive
			else
				clip = 5;					// negative
		}
	}

	// �t�m���ť��� 0 ~ 5
	//
	glClipPlane(GL_CLIP_PLANE0, clip0);
	glClipPlane(GL_CLIP_PLANE1, clip1);
	glClipPlane(GL_CLIP_PLANE2, clip2);
	glClipPlane(GL_CLIP_PLANE3, clip3);
	glClipPlane(GL_CLIP_PLANE4, clip4);
	glClipPlane(GL_CLIP_PLANE5, clip5);

	// �N�P�[���̬ۤϪ����ť����]�w���[���̾ާ@�� ���ӥ���
	//
	glClipPlane(GL_CLIP_PLANE0 + clip, _user_plane);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
	glEnable(GL_CLIP_PLANE4);
	glEnable(GL_CLIP_PLANE5);

	glColor4f(1.0F, 1.0F, 1.0F, _intensity);				// �]�wopenGL��slcies�C��]R, G, B, Alpha�^

	// �Ұ� Alpha / �V�X ����
	//
	glEnable(GL_ALPHA_TEST);							// �ҥ� Alpha ����
	glAlphaFunc(GL_GREATER, _density * _intensity);			// �]�w Alpha ���ժ��Ѧҭ�

	glEnable(GL_BLEND);									// �ҥ� �C��V�X
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// �]�w �C��V�X(Source and Target)

	// �]�w���z�y�Шt��
	//
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glEnable(GL_TEXTURE_3D);

	glTranslatef(0.5F, 0.5F, 0.5F);						// �N����(��Ŧ)���쥿����
	glScalef(_scale_x, _scale_y, _scale_z);				// �N����(��Ŧ)�Y��

	glMultMatrixf(mat);
	glTranslatef(0.0F, 0.0F, -_view_distance);

	// set modelView to identity
	//
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	{
		glLoadIdentity();

		// �]�w���z�y�Шt�Ϊ��ͦ�
		//
		glTexGenfv(GL_S, GL_EYE_PLANE, xPlane);
		glTexGenfv(GL_T, GL_EYE_PLANE, yPlane);
		glTexGenfv(GL_R, GL_EYE_PLANE, zPlane);

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);

		glBindTexture(GL_TEXTURE_3D, _textureName[0]);

		glTranslatef(0.0f, 0.0f, _view_distance);

		// ø�sopenGL���C�@�hslices
		//
		for (k = 0; k < _gl_slices; k++)
		{
			glPushMatrix();
			{
				glTranslatef(0.0f, 0.0f, -1.0f + (float)k * (2.0f / (float)(_gl_slices - 1)));
				glBegin(GL_QUADS);
				{
					for (j = 0; j < (8 - 1); j++)
					{
						for (i = 0; i < (8 - 1); i++)
						{
							glVertex3fv(_glVertexPt[j * 8 + i]);
							glVertex3fv(_glVertexPt[j * 8 + (i + 1)]);
							glVertex3fv(_glVertexPt[(j + 1) * 8 + (i + 1)]);
							glVertex3fv(_glVertexPt[(j + 1) * 8 + i]);
						}
					}
				}
				glEnd();
			}
			glPopMatrix();
		}
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		glDisable(GL_CLIP_PLANE4);
		glDisable(GL_CLIP_PLANE5);
	}
	glPopMatrix();

	// draw the slice plane across to get a better image
	//
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	{
		glVertex3fv(&plane[0]);
		glVertex3fv(&plane[3]);
		glVertex3fv(&plane[6]);
		glVertex3fv(&plane[9]);
	}
	glEnd();
	glDepthMask(GL_TRUE);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);

	// ø�s�Ŷ��P�����
	//
	glLineWidth(2);
	glBegin(GL_LINES);
	{
		glColor4f(0.902f, 1.0f, 1.0f, 0.1f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);

		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
	}
	glEnd();

	// ø�s�[���̪�����
	//
	glLineWidth(1);
	glBegin(GL_LINES);
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3fv(&plane[0]);
		glVertex3fv(&plane[3]);

		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3fv(&plane[3]);
		glVertex3fv(&plane[9]);

		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3fv(&plane[9]);
		glVertex3fv(&plane[6]);

		glColor3f(1.0f, 1.0f, 0.0f);
		glVertex3fv(&plane[6]);
		glVertex3fv(&plane[0]);
	}
	glEnd();

	/* ��� 3D seed */
	if (_PHANTOM_3D_SEED == TRUE && _get_3D_seed == true)
	{
		glPointSize(5);
		glBegin(GL_POINTS);
		{
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3f((GLfloat)_seed_gl.z,
				(GLfloat)_seed_gl.y,
				(GLfloat)_seed_gl.x);
		}
		glEnd();
	}

	SwapBuffers(_hDC);
}

void CPhantom::Draw2DImage(const int& slice)
{
	//	DO : ø�s 2D �v��
	//
	CClientDC dc(_2D_frame);

	if (_img == nullptr) return;

	_2D_dib->ShowInverseDIB(&dc, _img[slice]);

	if (_PHANTOM_3D_SEED)
	{
		if (_get_3D_seed)
		{
			if (slice == _seed_img.z)
			{
				CPoint pt;
				register int i, j;
				for (i = -1; i <= 1; i++)
				{
					for (j = -1; j <= 1; j++)
					{
						pt.x = (LONG)_seed_img.x + j;
						pt.y = (LONG)_seed_img.y + i;

						dc.SetPixel(pt, RGB(255, 0, 0));
					}
				}
			}
		}
		if (_get_region_grow)
		{
			CPoint pt;
			register int i, j;
			for (j = 0; j < _row; ++j)
			{
				for (i = 0; i < _col; ++i)
				{
					if (_judge[slice][j * _col + i] == 1)
					{
						pt.x = i;
						pt.y = j;
						dc.SetPixel(pt, RGB(255, 120, 190));
					}
				}
			}
		}
	}

	/* �g�r (slice) */
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void CPhantom::pointToVector(int x, int y, int width, int height, float vec[3])
{
	// DO : project x, y onto a hemi-sphere centered within width, height
	//
	float d, a;

	vec[0] = (float)((2.0 * x - width) / width);
	vec[1] = (float)((height - 2.0 * y) / height);
	vec[1] = (float)((mode == ControlModes::ControlObject) ? vec[1] : -vec[1]);		//v1

	d = sqrt(vec[0] * vec[0] + vec[1] * vec[1]);

	vec[2] = (float)(cos((M_PI / 2.0) * ((d < 1.0) ? d : 1.0)));
	vec[2] = (float)((mode == ControlModes::ControlObject) ? vec[2] : -vec[2]);		//v2, just for moving plane normal

	a = (float)(1.0 / sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]));

	vec[0] *= a;
	vec[1] *= a;
	vec[2] *= a;
}

void CPhantom::ActStart(UINT nFlags, int x, int y)
{
	// DO : ���U�ƹ���}�l���ʧ@
	//
	if (nFlags == MK_LBUTTON)
	{
		_act_rotate = true;
		_LR_button = true;

		pointToVector(x, y, _3D_rect.right - _3D_rect.left, _3D_rect.bottom - _3D_rect.top, _last_pos);
	}
	else if (nFlags == MK_RBUTTON)
	{
		_act_translate = true;
		_LR_button = false;

		_transY = (float)y;
	}
}

void CPhantom::ActStop(UINT nFlags, int x, int y)
{
	// DO : �񱼷ƹ��᪺�ʧ@
	// �t�~�Τ@��bool���ܼƨӧP�_�ƹ�����O�]���񱼷ƹ��ɡAnFlags���O0�C
	//
	if (_LR_button == true)
	{
		_act_rotate = false;

		if (mode == ControlModes::ControlObject)
			_obj_angle = 0.0;
	}
	else if (_LR_button == false)
	{
		_act_translate = false;
	}
}

void CPhantom::ActTracking(int x, int y)
{
	// DO : ����ƹ��ɩҰ��檺�ʧ@
	//
	if (_act_rotate)
	{
		float curPos[3], dx, dy, dz;

		pointToVector(x, y, _3D_rect.right - _3D_rect.left, _3D_rect.bottom - _3D_rect.top, curPos);

		dx = curPos[0] - _last_pos[0];
		dy = curPos[1] - _last_pos[1];
		dz = curPos[2] - _last_pos[2];

		if (mode == ControlModes::ControlObject)
		{
			_obj_angle = (float)(90.0 * sqrt(dx * dx + dy * dy + dz * dz));

			_obj_axis[0] = _last_pos[1] * curPos[2] - _last_pos[2] * curPos[1];
			_obj_axis[1] = _last_pos[2] * curPos[0] - _last_pos[0] * curPos[2];
			_obj_axis[2] = _last_pos[0] * curPos[1] - _last_pos[1] * curPos[0];
		}
		else if (mode == ControlModes::ControlPlane)
		{
			_pln_angle = (float)(90.0 * sqrt(dx * dx + dy * dy + dz * dz));

			_pln_axis[0] = _last_pos[1] * curPos[2] - _last_pos[2] * curPos[1];
			_pln_axis[1] = _last_pos[2] * curPos[0] - _last_pos[0] * curPos[2];
			_pln_axis[2] = _last_pos[0] * curPos[1] - _last_pos[1] * curPos[0];
		}

		_last_pos[0] = curPos[0];
		_last_pos[1] = curPos[1];
		_last_pos[2] = curPos[2];
	}
	if (_act_translate)
	{
		if (mode == ControlModes::ControlObject)
		{
			_view_distance += 0.01F * (y - _transY);
			_transY = (float)y;
		}
		else if (mode == ControlModes::ControlPlane)
		{
			_user_plane[3] -= 0.01F * (y - _transY);
			_transY = (float)y;
		}
	}
	UpdateWindow();
}

void CPhantom::InvertMat(float(&m)[16])
{
	// DO : Invert Matrix
	//
	float temp;

	temp = m[1];
	m[1] = m[4];
	m[4] = temp;

	temp = m[2];
	m[2] = m[8];
	m[8] = temp;

	temp = m[6];
	m[6] = m[9];
	m[9] = temp;

	m[12] = -m[12];
	m[13] = -m[13];
	m[14] = -m[14];
}

CPhantom::Seed_s CPhantom::coordiConvert(Seed_d& pt)
{
	// openGL coordinate -> data array
	//
	// ���]�ߤ����.�e.�����O 512 (�H�x�}�ӻ�)�A���I��m���O�q -1 �� 1 (�HGL�Ŷ��ӻ�)�A
	// �h����ؤl�I���Y�b��m�ɡA���p��x�}�PGL�Ŷ������ �A�A�p��x�}(�Τ���)�@�b���ƶq�A
	// �̫��̬ۭ�����ᰣ�H 2 �A�N�O��l�v����ݬY�@�ݦb openGL �Ŷ����b��m�A
	// ���]���v���٦����Y��A�ҥH���H�Y���ҫ���o�����G�A�~�O���ݯu�����b��m�A
	// ���ۡA�]����ݦ�m�O������Y�A�ҥH�̫�Q�άY�@�ݺݦ�m�B�t�@�ݦ�m�B�x�}�ơB
	// �H�β{�b�ؤl�I���b��m�h���T����Ҵ���A��o�����G�Y���ؤl�I��������m�C

	Seed_s temp;
	double tmp_x, tmp_y, tmp_z;

	_x_index = (2.0F / 511.0F) * (_col / 2.0F) / 2;
	_x_index = _x_index / _scale_y;
	tmp_x = ((pt.x + 1) - (-_x_index + 1)) * (_col / ((_x_index + 1) - (-_x_index + 1)));

	if (tmp_x - (int)tmp_x == 0)
		temp.x = (short)tmp_x - 1;
	else
		temp.x = (short)tmp_x;

	//-------------------------------------------------------------------------------//

	_y_index = (2.0F / 511.0F) * (_row / 2.0F) / 2;
	_y_index = _y_index / _scale_z;
	tmp_y = ((pt.y + 1) - (-_y_index + 1)) * (_row / ((_y_index + 1) - (-_y_index + 1)));

	if (tmp_y - (int)tmp_y == 0)
		temp.y = (short)tmp_y - 1;
	else
		temp.y = (short)tmp_y;

	//-------------------------------------------------------------------------------//

	_z_index = (2.0F / 511.0F) * (_total_slice / 2.0F) / 2;
	_z_index = _z_index / _scale_x;

	tmp_z = ((pt.z + 1) - (-_z_index + 1)) * (_total_slice / ((_z_index + 1) - (-_z_index + 1)));

	if (tmp_z < 1)
		tmp_z = (double)1;
	else if (tmp_z > _total_slice)
		tmp_z = (double)_total_slice;

	tmp_z -= 1;
	temp.z = (short)tmp_z;

	return temp;
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


