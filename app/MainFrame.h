#pragma once

#include <afxext.h>
#include <afxwin.h>

// The Frame is the main window: title bar, menu, border.
// It also splits its inside into two panes: table (left), chart (right).
class CMainFrame : public CFrameWnd {
    DECLARE_DYNCREATE(CMainFrame)

protected:
    BOOL OnCreateClient(LPCREATESTRUCT cs, CCreateContext* context) override;

private:
    CSplitterWnd splitter_;
};
