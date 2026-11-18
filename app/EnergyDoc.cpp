#include "EnergyDoc.h"

#include <afxdisp.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <exception>
#include <string>

#include "SiteDialog.h"
#include "energy/EnergyClient.hpp"
#include "resource.h"

namespace {

const char* const kServerHost = "127.0.0.1";
constexpr unsigned short kServerPort = 5555;

constexpr int kSampleHours = 24;
constexpr double kSamplePeakKw = 120.0;
constexpr double kPi = 3.14159265358979323846;

std::string toUtf8(const CString& text) {
    return std::string(CStringA(CT2A(text, CP_UTF8)));
}

CString fromUtf8(const char* text) { return CString(CA2W(text, CP_UTF8)); }

void showError(const std::exception& e) {
    CString msg;
    msg.Format(L"%s\n\nLe serveur est-il lancé (energy_server.exe, port %u) ?",
               static_cast<LPCWSTR>(fromUtf8(e.what())), kServerPort);
    AfxMessageBox(msg, MB_ICONERROR);
}

// Fake "solar farm" curve: zero at night, a bell shape peaking at noon (UTC).
std::vector<energy::Measurement> makeSampleData(const std::string& site) {
    const std::int64_t now = std::time(nullptr);
    const std::int64_t lastFullHour = now - now % 3600;

    std::vector<energy::Measurement> rows;
    rows.reserve(kSampleHours);
    for (int i = kSampleHours - 1; i >= 0; --i) {
        const std::int64_t ts = lastFullHour - i * 3600;
        const double hourOfDay = static_cast<double>((ts / 3600) % 24);
        const double sun = std::sin(kPi * (hourOfDay - 6.0) / 12.0);
        rows.push_back({site, ts, kSamplePeakKw * (std::max)(0.0, sun)});
    }
    return rows;
}

}  // namespace

IMPLEMENT_DYNCREATE(CEnergyDoc, CDocument)

// Connects menu commands to the member functions below.
BEGIN_MESSAGE_MAP(CEnergyDoc, CDocument)
    ON_COMMAND(ID_DATA_LOAD, &CEnergyDoc::OnDataLoad)
    ON_COMMAND(ID_DATA_SAMPLE, &CEnergyDoc::OnDataSample)
END_MESSAGE_MAP()

BOOL CEnergyDoc::LoadSite(const CString& site) {
    try {
        CWaitCursor wait;  // hourglass while we talk to the server
        energy::EnergyClient client(kServerHost, kServerPort);
        auto rows = client.get(toUtf8(site));

        measurements_ = std::move(rows);
        site_ = site;
        SetTitle(site_);
        UpdateAllViews(nullptr);  // tell every view to redraw
        return TRUE;
    } catch (const std::exception& e) {
        showError(e);
        return FALSE;
    }
}

BOOL CEnergyDoc::AddSampleData(const CString& site) {
    try {
        CWaitCursor wait;
        energy::EnergyClient client(kServerHost, kServerPort);
        for (const auto& m : makeSampleData(toUtf8(site))) {
            client.add(m);
        }
    } catch (const std::exception& e) {
        showError(e);
        return FALSE;
    }
    return LoadSite(site);
}

void CEnergyDoc::OnDataLoad() {
    CSiteDialog dlg(site_);
    if (dlg.DoModal() == IDOK) LoadSite(dlg.site());
}

void CEnergyDoc::OnDataSample() {
    CSiteDialog dlg(site_.IsEmpty() ? CString(L"solar-1") : site_);
    if (dlg.DoModal() == IDOK) AddSampleData(dlg.site());
}
