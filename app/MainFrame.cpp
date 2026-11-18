#include "MainFrame.h"

#include "ChartView.h"
#include "MeasurementView.h"

namespace {
constexpr int kTableInitialWidth = 330;
constexpr int kPaneMinWidth = 200;
}  // namespace

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// MFC calls this while building the window: we create the two views here
// instead of letting MFC create a single one.
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT, CCreateContext* context) {
    if (!splitter_.CreateStatic(this, 1, 2)) return FALSE;

    if (!splitter_.CreateView(0, 0, RUNTIME_CLASS(CMeasurementView),
                              CSize(kTableInitialWidth, 0), context) ||
        !splitter_.CreateView(0, 1, RUNTIME_CLASS(CChartView), CSize(0, 0),
                              context)) {
        return FALSE;
    }
    splitter_.SetColumnInfo(0, kTableInitialWidth, kPaneMinWidth);
    SetActiveView(static_cast<CView*>(splitter_.GetPane(0, 0)));
    return TRUE;
}
