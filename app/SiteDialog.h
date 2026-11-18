#pragma once

#include <afxwin.h>

// Small modal window asking for a site name.
class CSiteDialog : public CDialog {
public:
    explicit CSiteDialog(const CString& initialSite, CWnd* parent = nullptr);

    const CString& site() const { return site_; }

protected:
    void DoDataExchange(CDataExchange* dx) override;
    void OnOK() override;

private:
    CString site_;
};
