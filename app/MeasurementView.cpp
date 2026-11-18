#include "MeasurementView.h"

#include <afxdisp.h>

#include <cstdint>

namespace {

constexpr int kTimeColumnWidth = 180;
constexpr int kPowerColumnWidth = 120;
constexpr std::int64_t kMaxDisplayableTime = 32503680000;  // year 3000

CString formatTime(std::int64_t unixTime) {
    // CTime asserts on out-of-range values, so show raw numbers instead.
    if (unixTime < 0 || unixTime > kMaxDisplayableTime) {
        CString raw;
        raw.Format(L"%lld", unixTime);
        return raw;
    }
    return CTime(static_cast<__time64_t>(unixTime)).Format(L"%Y-%m-%d %H:%M");
}

}  // namespace

IMPLEMENT_DYNCREATE(CMeasurementView, CListView)

BEGIN_MESSAGE_MAP(CMeasurementView, CListView)
    // The table sends this notification each time it needs a cell's text.
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CMeasurementView::OnGetDispInfo)
END_MESSAGE_MAP()

CEnergyDoc* CMeasurementView::GetDocument() const {
    return static_cast<CEnergyDoc*>(m_pDocument);
}

BOOL CMeasurementView::PreCreateWindow(CREATESTRUCT& cs) {
    cs.style |= LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
    return CListView::PreCreateWindow(cs);
}

void CMeasurementView::OnInitialUpdate() {
    CListView::OnInitialUpdate();

    CListCtrl& list = GetListCtrl();
    list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    list.InsertColumn(0, L"Heure", LVCFMT_LEFT, kTimeColumnWidth);
    list.InsertColumn(1, L"Puissance (kW)", LVCFMT_RIGHT, kPowerColumnWidth);
}

void CMeasurementView::OnUpdate(CView*, LPARAM, CObject*) {
    // Called after UpdateAllViews(): tell the table how many rows exist.
    const auto count = static_cast<int>(GetDocument()->measurements().size());
    GetListCtrl().SetItemCountEx(count, LVSICF_NOSCROLL);
    GetListCtrl().Invalidate();
}

void CMeasurementView::OnGetDispInfo(NMHDR* header, LRESULT* result) {
    *result = 0;
    NMLVDISPINFO* info = reinterpret_cast<NMLVDISPINFO*>(header);
    if (!(info->item.mask & LVIF_TEXT)) return;

    const auto& rows = GetDocument()->measurements();
    const auto row = static_cast<std::size_t>(info->item.iItem);
    if (row >= rows.size()) return;

    CString text;
    if (info->item.iSubItem == 0) {
        text = formatTime(rows[row].timestamp);
    } else {
        text.Format(L"%.2f", rows[row].powerKw);
    }
    wcsncpy_s(info->item.pszText, info->item.cchTextMax, text, _TRUNCATE);
}
