
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
	DcmDir*		m_dir;					// 紀錄開啟的DICOMDIR資訊
	short**		m_HUimg;				// 紀錄HU的影像矩陣
	BYTE**		m_img;					// 紀錄pixel的影像矩陣 ( 原始. 絕不更改
	BYTE**		m_imgPro;				// 紀錄pixel的影像矩陣 ( 可更改.處理
	
unsigned int	displaySeries;			// 整個程式要顯示的時序

//================//
//   Operations   //
//================//
public:
	void	BuildDataMatrix();						// 建立影像矩陣
	void*	new2Dmatrix(int h, int w, int size);	// 動態配置二維矩陣

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
