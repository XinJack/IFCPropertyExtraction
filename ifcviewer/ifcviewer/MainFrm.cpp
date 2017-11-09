#include "stdafx.h"
#include "ifcviewer.h"

#include "MainFrm.h"
#include "LeftPane.h"
#include "RightPane.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern	CWnd				* glLeftPane;
extern	STRUCT__IFC__OBJECT	* highLightedIfcObject;

bool	showFaces = true, showWireFrame = true, enableOnOver = true;


// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_VIEWFACES, &CMainFrame::OnViewFaces)
	ON_COMMAND(ID_VIEW_VIEWWIREFRAME, &CMainFrame::OnViewWireframe)
	ON_COMMAND(ID_VIEW_ONOVER, &CMainFrame::OnViewOnover)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	if (!m_wndSplitter.CreateStatic(this, 1, 2)) {
		return FALSE;
	}

	if ( !m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CLeftPane), CSize(200, 200), pContext)  ||
		 !m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CRightPane), CSize(10, 10), pContext) )
	{
		m_wndSplitter.DestroyWindow();
		return	FALSE;
	}

	return	true;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers


void CMainFrame::OnViewFaces()
{
	if	(showFaces) {
		this->GetMenu()->CheckMenuItem(ID_VIEW_VIEWFACES, MF_UNCHECKED);
		showFaces = false;
	} else {
		this->GetMenu()->CheckMenuItem(ID_VIEW_VIEWFACES, MF_CHECKED);
		showFaces = true;
	}

	glLeftPane->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
}


void CMainFrame::OnViewWireframe()
{
	if	(showWireFrame) {
		this->GetMenu()->CheckMenuItem(ID_VIEW_VIEWWIREFRAME, MF_UNCHECKED);
		showWireFrame = false;
	} else {
		this->GetMenu()->CheckMenuItem(ID_VIEW_VIEWWIREFRAME, MF_CHECKED);
		showWireFrame = true;
	}

	glLeftPane->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
}


void CMainFrame::OnViewOnover()
{
	if	(enableOnOver) {
		this->GetMenu()->CheckMenuItem(ID_VIEW_ONOVER, MF_UNCHECKED);
		enableOnOver = false;
		highLightedIfcObject = 0;
	} else {
		this->GetMenu()->CheckMenuItem(ID_VIEW_ONOVER, MF_CHECKED);
		enableOnOver = true;
	}

	glLeftPane->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
}
