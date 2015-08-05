////////////////////////////////////////////////////////////////////////////////
//
// CSplitterWndEx

#pragma once

class CSplitterWndEx : public CSplitterWnd
{

public:
   CSplitterWndEx(BOOL);

   void ShowColumn();
   void HideColumn(int colHide);

// ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CSplitterWndEx)
   //}}AFX_VIRTUAL

// Generated message map functions
protected:
   //{{AFX_MSG(CSplitterWndEx)
      // NOTE - the ClassWizard will add and remove member
      //        functions here.
      //}}AFX_MSG

	int m_nHidedCol; // hide column number, -1 if all columns are shown
	DECLARE_DYNAMIC(CSplitterWndEx)//need default contructor for DYNCREATE
    DECLARE_MESSAGE_MAP()
};
