#include "SiteDialog.h"

#include <afxdisp.h>

#include <string>

#include "energy/EnergyClient.hpp"
#include "resource.h"

CSiteDialog::CSiteDialog(const CString& initialSite, CWnd* parent)
    : CDialog(IDD_SITE, parent), site_(initialSite) {}

void CSiteDialog::DoDataExchange(CDataExchange* dx) {
    CDialog::DoDataExchange(dx);
    // Copies text between the edit box and the site_ variable.
    DDX_Text(dx, IDC_SITE, site_);
}

void CSiteDialog::OnOK() {
    if (!UpdateData(TRUE)) return;  // box -> site_

    const CStringA utf8(CT2A(site_, CP_UTF8));
    if (!energy::EnergyClient::isValidSite(std::string(utf8))) {
        AfxMessageBox(
            L"Nom invalide : 1 à 64 caractères, sans espaces.", MB_ICONWARNING);
        return;  // keep the dialog open
    }
    CDialog::OnOK();
}
