
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
