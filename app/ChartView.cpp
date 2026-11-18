#include "ChartView.h"

#include <GL/gl.h>

#include <algorithm>

#include "TimeFormat.h"

namespace {

// Layout, in pixels.
constexpr int kMarginLeft = 56;
constexpr int kMarginRight = 16;
constexpr int kMarginTop = 28;
constexpr int kMarginBottom = 30;
constexpr int kMinPlotSize = 20;
constexpr int kGridDivisions = 4;
constexpr int kLabelGap = 6;
constexpr int kFontPixels = 13;

// Font: we build OpenGL display lists for characters 32..255.
constexpr unsigned int kFirstGlyph = 32;
constexpr unsigned int kGlyphCount = 224;

constexpr float kLineWidth = 2.0f;

void setColor(float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
}

void setPixelProjection(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, 0, height, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

}  // namespace

IMPLEMENT_DYNCREATE(CChartView, CView)

BEGIN_MESSAGE_MAP(CChartView, CView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

CChartView::CChartView() = default;
CChartView::~CChartView() = default;

CEnergyDoc* CChartView::GetDocument() const {
    return static_cast<CEnergyDoc*>(m_pDocument);
}

BOOL CChartView::PreCreateWindow(CREATESTRUCT& cs) {
    // OpenGL needs to draw in a window that is not overpainted by siblings.
    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    return CView::PreCreateWindow(cs);
}

// ---- OpenGL setup / teardown ------------------------------------------

int CChartView::OnCreate(LPCREATESTRUCT cs) {
    if (CView::OnCreate(cs) == -1) return -1;
    glReady_ = initOpenGl();  // on failure we show a message instead
    return 0;
}

void CChartView::OnDestroy() {
    shutdownOpenGl();
    CView::OnDestroy();
}

bool CChartView::initOpenGl() {
    dc_ = std::make_unique<CClientDC>(this);

    // 1. Tell Windows what kind of pixels we want to draw.
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int format = ChoosePixelFormat(dc_->m_hDC, &pfd);
    if (format == 0 || !SetPixelFormat(dc_->m_hDC, format, &pfd)) return false;

    // 2. Create the OpenGL context and make it the current one.
    glContext_ = wglCreateContext(dc_->m_hDC);
    if (glContext_ == nullptr) return false;
    if (!wglMakeCurrent(dc_->m_hDC, glContext_)) return false;

    buildFontLists();
    return true;
}

void CChartView::buildFontLists() {
    font_.CreateFont(-kFontPixels, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                     ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

    CFont* previous = dc_->SelectObject(&font_);
    fontListBase_ = glGenLists(kGlyphCount);
    if (!wglUseFontBitmapsA(dc_->m_hDC, kFirstGlyph, kGlyphCount,
                            fontListBase_)) {
        glDeleteLists(fontListBase_, kGlyphCount);
        fontListBase_ = 0;  // labels will simply be skipped
    }
    dc_->SelectObject(previous);
}

void CChartView::shutdownOpenGl() {
    if (glContext_ != nullptr) {
        wglMakeCurrent(dc_->m_hDC, glContext_);
        if (fontListBase_ != 0) glDeleteLists(fontListBase_, kGlyphCount);
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glContext_);
    }
    glContext_ = nullptr;
    fontListBase_ = 0;
    glReady_ = false;
    dc_.reset();
}

// ---- MFC events -------------------------------------------------------

void CChartView::OnUpdate(CView*, LPARAM, CObject*) {
    // Document changed: recompute the axes and the list of curve points.
    const auto& rows = GetDocument()->measurements();
    bounds_ = energy::computeBounds(rows);

    std::vector<double> points;
    points.reserve(rows.size() * 2);
    for (const auto& m : rows) {
        points.push_back(static_cast<double>(m.timestamp - bounds_.tMin));
        points.push_back(m.powerKw);
    }
    vertices_ = std::move(points);
    Invalidate(FALSE);
}

void CChartView::OnSize(UINT type, int cx, int cy) {
    CView::OnSize(type, cx, cy);
    Invalidate(FALSE);  // views are not repainted on resize by default
}

BOOL CChartView::OnEraseBkgnd(CDC*) {
    return TRUE;  // OpenGL clears the whole window itself: avoids flicker
}

void CChartView::OnDraw(CDC* dc) {
    if (!glReady_) {
        drawFallbackMessage(dc, L"OpenGL indisponible sur cet ordinateur.");
        return;
    }
    CRect client;
    GetClientRect(&client);
    if (client.Width() <= 0 || client.Height() <= 0) return;

    wglMakeCurrent(dc_->m_hDC, glContext_);
    drawChart(client.Width(), client.Height());
    SwapBuffers(dc_->m_hDC);  // show the finished picture (double buffering)
}

void CChartView::drawFallbackMessage(CDC* dc, const wchar_t* text) {
    CRect client;
    GetClientRect(&client);
    dc->DrawText(text, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ---- Drawing ----------------------------------------------------------

void CChartView::drawChart(int width, int height) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    glViewport(0, 0, width, height);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const int plotWidth = width - kMarginLeft - kMarginRight;
    const int plotHeight = height - kMarginTop - kMarginBottom;
    if (plotWidth < kMinPlotSize || plotHeight < kMinPlotSize) return;

    drawPlotArea(kMarginLeft, kMarginBottom, plotWidth, plotHeight);
    drawLabels(width, height, kMarginLeft, kMarginBottom, plotWidth,
               plotHeight);
}

// Draws grid and curve. The viewport is limited to the plot rectangle and
// the projection maps data units (seconds, kW) straight to that rectangle,
// so we can send the raw data points to OpenGL without converting them.
void CChartView::drawPlotArea(int left, int bottom, int plotWidth,
                              int plotHeight) {
    const double span = static_cast<double>(bounds_.tMax - bounds_.tMin);

    glViewport(left, bottom, plotWidth, plotHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, span, bounds_.yMin, bounds_.yMax, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    setColor(0.14f, 0.15f, 0.18f);
    glRectd(0.0, bounds_.yMin, span, bounds_.yMax);

    setColor(0.28f, 0.30f, 0.35f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i <= kGridDivisions; ++i) {
        const double fraction = static_cast<double>(i) / kGridDivisions;
        const double y = bounds_.yMin + (bounds_.yMax - bounds_.yMin) * fraction;
        glVertex2d(0.0, y);
        glVertex2d(span, y);
        glVertex2d(span * fraction, bounds_.yMin);
        glVertex2d(span * fraction, bounds_.yMax);
    }
    glEnd();

    if (vertices_.empty()) return;
    setColor(1.0f, 0.62f, 0.15f);
    glLineWidth(kLineWidth);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_DOUBLE, 0, vertices_.data());
    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(vertices_.size() / 2));
    glDisableClientState(GL_VERTEX_ARRAY);
}

void CChartView::drawLabels(int width, int height, int left, int bottom,
                            int plotWidth, int plotHeight) {
    setPixelProjection(width, height);
    setColor(0.80f, 0.82f, 0.86f);

    CString title = GetDocument()->GetTitle();
    title += L"  -  Puissance (kW)";
    drawText(left, height - kMarginTop + kLabelGap + 2, title);

    if (vertices_.empty()) {
        const CString hint = L"Aucune donnée. Menu Données > Charger un site...";
        drawText(left + (plotWidth - textWidth(hint)) / 2,
                 bottom + plotHeight / 2, hint);
        return;
    }

    for (int i = 0; i <= kGridDivisions; ++i) {  // power axis
        const double fraction = static_cast<double>(i) / kGridDivisions;
        CString label;
        label.Format(L"%g", bounds_.yMin + (bounds_.yMax - bounds_.yMin) * fraction);
        drawText(left - kLabelGap - textWidth(label),
                 bottom + static_cast<int>(plotHeight * fraction) - kFontPixels / 3,
                 label);
    }
    for (int i = 0; i <= kGridDivisions; i += kGridDivisions / 2) {  // time axis
        const double fraction = static_cast<double>(i) / kGridDivisions;
        const auto t = bounds_.tMin + static_cast<std::int64_t>(
                           static_cast<double>(bounds_.tMax - bounds_.tMin) * fraction);
        const CString label = formatUnixTime(t, L"%d/%m %H:%M");
        drawText(left + static_cast<int>(plotWidth * fraction) - textWidth(label) / 2,
                 bottom - kFontPixels - kLabelGap, label);
    }
}

void CChartView::drawText(int x, int y, const CString& text) {
    if (fontListBase_ == 0) return;
    const CStringA ansi(CT2A(text, CP_ACP));  // our glyph lists are ANSI

    glRasterPos2i(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontListBase_ - kFirstGlyph);
    glCallLists(static_cast<GLsizei>(ansi.GetLength()), GL_UNSIGNED_BYTE,
                static_cast<const char*>(ansi));
    glPopAttrib();
}

int CChartView::textWidth(const CString& text) {
    CFont* previous = dc_->SelectObject(&font_);
    const int width = dc_->GetTextExtent(text).cx;
    dc_->SelectObject(previous);
    return width;
}
