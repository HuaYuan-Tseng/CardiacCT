
// CCTDoc.h : interface of the CCTDoc class
//

#pragma once
#include "DcmDir.h"
#include <vector>
using namespace std;

class CCTDoc : public CDocument
{
protected: // create from serialization only
	CCTDoc() noexcept;
	DECLARE_DYNCREATE(CCTDoc)

//================//
//   Attributes   //
//================//
public:
	DcmDir*		m_dir;					// �����}�Ҫ�DICOMDIR��T
	short**		m_HUimg;				// ����HU���v���x�}
	BYTE**		m_img;					// ����pixel���v���x�} ( ��l. �������
	BYTE**		m_imgPro;				// ����pixel���v���x�} ( �i���.�B�z
	
unsigned int	displaySeries;			// ��ӵ{���n��ܪ��ɧ�


std::map<int, std::vector<std::pair<int, int>>> spine_vertex;		// ������ժ�B�B�z�᪺�T�ӳ��I
																	// 0 : �����W�����I
																	// 1 : ���U
																	// 2 : �k�U

std::map<int, std::vector<std::pair<float, float>>> spine_line;		// ������ըC�islice�����u��{�����Y��(�ײv.�I�Z)
																	// 0 : ��
																	// 1 : �k

std::map<int, std::vector<int>> spine_edge;							// �O����ըC�islice�N�i��B�z���d��(�������)
																	// 0 : x_min
																	// 1 : x_max
																	// 2 : y_min
																	// 3 : y_max

std::map<int, std::vector<std::pair<int, int>>> sternum_vertex;		// �����ݰ��C�islice���T���I
																	// 0 : �����W�����I
																	// 1 : ���U
																	// 2 : �k�U

std::map<int, std::vector<std::pair<float, float>>> sternum_line;	// �����ݰ��C�islice�����u��{���Y��(�ײv.�I�Z)
																	// 0 : ��
																	// 1 : �k

std::map<int, std::vector<int>> sternum_edge;						// �O���ݰ��C�islice�N�i��B�z���d��(�������)
																	// 0 : x_min
																	// 1 : x_max
																	// 2 : y_min
																	// 3 : y_max


//================//
//   Operations   //
//================//
public:
	void	BuildDataMatrix();						// �إ߼v���x�}
	void*	new2Dmatrix(int h, int w, int size);	// �ʺA�t�m�G���x�}

//================//
//   Overrides    //
//================//
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

//================//
// Implementation //
//================//
public:
	virtual ~CCTDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

//===================================//
//  Generated message map functions  //
//===================================//
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	afx_msg void OnToolButtonOpenDicomdir();
	afx_msg void OnToolButtonDirContents();
	afx_msg void OnToolButton3DProcess();
};
