// The Application object: MFC creates `theApp` at startup and calls
// InitInstance(), which builds the window.

#include <afxwin.h>
#include <afxcmn.h>

#include "EnergyDoc.h"
#include "MainFrame.h"
#include "MeasurementView.h"
#include "resource.h"

// Use modern (visual style 6) controls instead of the Windows 95 look.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CEnergyApp : public CWinApp {
public:
    BOOL InitInstance() override {
        CWinApp::InitInstance();

        INITCOMMONCONTROLSEX controls{sizeof controls, ICC_LISTVIEW_CLASSES};
        InitCommonControlsEx(&controls);

        // A template links the 3 parts: Document + Frame + View.
        AddDocTemplate(new CSingleDocTemplate(
            IDR_MAINFRAME, RUNTIME_CLASS(CEnergyDoc),
            RUNTIME_CLASS(CMainFrame), RUNTIME_CLASS(CMeasurementView)));

        // Default shell command = "File > New": creates the empty document
        // together with its frame and view.
        CCommandLineInfo cmdInfo;
        ParseCommandLine(cmdInfo);
        if (!ProcessShellCommand(cmdInfo)) return FALSE;

        m_pMainWnd->ShowWindow(SW_SHOW);
        m_pMainWnd->UpdateWindow();
        return TRUE;
    }
};

CEnergyApp theApp;
