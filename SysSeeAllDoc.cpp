// SysSeeAllDoc.cpp : implementation of the CSysSeeAllDoc class
//

#include "stdafx.h"
#include "PestRid.h"

#include "SysSeeAllDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSysSeeAllDoc

IMPLEMENT_DYNCREATE(CSysSeeAllDoc, CDocument)

BEGIN_MESSAGE_MAP(CSysSeeAllDoc, CDocument)
END_MESSAGE_MAP()


// CSysSeeAllDoc construction/destruction

CSysSeeAllDoc::CSysSeeAllDoc()
{
	// TODO: add one-time construction code here

}

CSysSeeAllDoc::~CSysSeeAllDoc()
{
}

BOOL CSysSeeAllDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CSysSeeAllDoc serialization

void CSysSeeAllDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CSysSeeAllDoc diagnostics

#ifdef _DEBUG
void CSysSeeAllDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSysSeeAllDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CSysSeeAllDoc commands
