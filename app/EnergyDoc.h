#pragma once

#include <afxwin.h>

#include <vector>

#include "energy/Measurement.hpp"

// The Document owns the data. Views only display it.
class CEnergyDoc : public CDocument {
    DECLARE_DYNCREATE(CEnergyDoc)

public:
    const std::vector<energy::Measurement>& measurements() const {
        return measurements_;
    }

    // Both show an error message box and return FALSE when they fail.
    BOOL LoadSite(const CString& site);
    BOOL AddSampleData(const CString& site);

protected:
    afx_msg void OnDataLoad();
    afx_msg void OnDataSample();
    DECLARE_MESSAGE_MAP()

private:
    CString site_;
    std::vector<energy::Measurement> measurements_;
};
