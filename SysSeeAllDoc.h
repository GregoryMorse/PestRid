// SysSeeAllDoc.h : interface of the CSysSeeAllDoc class
//


#pragma once


class CSysSeeAllDoc : public CDocument
{
protected: // create from serialization only
	CSysSeeAllDoc();
	DECLARE_DYNCREATE(CSysSeeAllDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CSysSeeAllDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


