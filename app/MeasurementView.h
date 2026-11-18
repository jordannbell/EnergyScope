#pragma once

#include <afxwin.h>
#include <afxcmn.h>
#include <afxcview.h>

#include "EnergyDoc.h"

// The View shows the Document's data. This one is a table (CListView).
// It is "virtual": the table asks us for each visible cell only, so it
// stays fast even with hundreds of thousands of rows.
class CMeasurementView : public CListView {
    DECLARE_DYNCREATE(CMeasurementView)

public:
    CEnergyDoc* GetDocument() const;

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnInitialUpdate() override;
    void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

    afx_msg void OnGetDispInfo(NMHDR* header, LRESULT* result);
    DECLARE_MESSAGE_MAP()
};
