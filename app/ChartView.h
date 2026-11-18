#pragma once

#include <afxwin.h>

#include <memory>
#include <vector>

#include "EnergyDoc.h"
#include "energy/ChartScale.hpp"

// A second View on the same Document: draws the power curve with OpenGL.
//
// OpenGL draws through a "rendering context" (HGLRC) attached to the
// window's device context. We create it when the window is created and
// delete it when the window is destroyed.
class CChartView : public CView {
    DECLARE_DYNCREATE(CChartView)

public:
    CChartView();
    ~CChartView() override;

    CEnergyDoc* GetDocument() const;

protected:
    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    void OnDraw(CDC* dc) override;
    void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

    afx_msg int OnCreate(LPCREATESTRUCT cs);
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* dc);
    DECLARE_MESSAGE_MAP()

private:
    bool initOpenGl();
    void shutdownOpenGl();
    void buildFontLists();

    void drawChart(int width, int height);
    void drawPlotArea(int left, int bottom, int plotWidth, int plotHeight);
    void drawLabels(int width, int height, int left, int bottom, int plotWidth,
                    int plotHeight);
    void drawText(int x, int y, const CString& text);
    int textWidth(const CString& text);
    void drawFallbackMessage(CDC* dc, const wchar_t* text);

    std::unique_ptr<CClientDC> dc_;  // window device context used by OpenGL
    HGLRC glContext_ = nullptr;
    unsigned int fontListBase_ = 0;  // first OpenGL display list of the font
    CFont font_;
    bool glReady_ = false;

    energy::ChartBounds bounds_{0, 1, 0.0, 1.0};
    std::vector<double> vertices_;  // x = seconds since tMin, y = kW
};
