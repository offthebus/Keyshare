#include "stdafx.h"
#include "win.h"
#include "util.h"

Win::MSGHANDLERMAP Win::m_msgHandlers;

//---------------------------------------------------------------------------
// ::WIN
//---------------------------------------------------------------------------
Win::Win(HINSTANCE instance, const char* name, int x, int y, int w, int h, DWORD style, HMENU menuBar/*==0*/)
: hInstance(instance), name(0), hwnd(0), hFont(0), m_ownFont(false)
{
	name = util::strAllocCopy(name);
	
	WNDCLASSEX wc;
	util::zeroMem(wc);

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = &wndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(instance, name);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));
	wc.lpszClassName = name;

//	HMENU hMenuBar = CreateMenu();
//	HMENU hMainMenu = CreatePopupMenu();

//	AppendMenu(hMainMenu,MF_STRING,ID_CLEAR_SCREEN,"&Clear");
//	AppendMenu(hMainMenu,MF_STRING,ID_SELECT_FONT,"&Font");
//	AppendMenu(hMainMenu,MF_STRING,ID_SAVE_TO_FILE,"&Save");
//	AppendMenu(hMainMenu,MF_STRING,ID_ENUMERATE,"&Enumerate");
//	AppendMenu(hMenuBar,MF_STRING|MF_POPUP,(UINT)hMainMenu,"&Main");
	
	RECT r = { x, y, x+w, y+h};
	AdjustWindowRect(&r,style,TRUE);

	hwnd = RegisterClassEx(&wc) ? CreateWindow(name, name, style, r.left, r.top, r.right-r.left, r.bottom-r.top, NULL,0, instance, NULL ) : 0;

//	util::zeroMem(m_logFont);
//	m_logFont.lfHeight = -15;
//	m_logFont.lfWeight = 400;
//	m_logFont.lfOutPrecision = 3;
//	m_logFont.lfClipPrecision = 2;
//	m_logFont.lfQuality = 1;
//	m_logFont.lfPitchAndFamily = 49;
//	strcpy_s(m_logFont.lfFaceName,_countof(m_logFont.lfFaceName), "Consolas");
//	chooseFont(false);
}

//---------------------------------------------------------------------------
// WIN::CHOOSEFONT
//---------------------------------------------------------------------------
void Win::chooseFont(bool showDialog /*==true*/)
{
	if (showDialog) {
		CHOOSEFONT cf;
		util::zeroMem(cf);
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = hwnd;
		cf.lpLogFont = &m_logFont;
		cf.Flags = CF_INITTOLOGFONTSTRUCT;
		if (!ChooseFont(&cf)) {
			return;
		}
	}

	hFont = CreateFontIndirect(&m_logFont);
	if (hFont) {
		m_ownFont = true;
	} else {
		hFont = (HFONT)GetStockObject(SYSTEM_FONT);
		m_ownFont = false;
	}

	InvalidateRect(hwnd,NULL,TRUE);
}
//---------------------------------------------------------------------------
// CALLBACK WNDPROC
//---------------------------------------------------------------------------
LRESULT CALLBACK Win::wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CLOSE: {
			DestroyWindow(hwnd);
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
		default: {
			MSGHANDLERMAP::iterator iter = m_msgHandlers.find(uMsg); 
			if ( iter != m_msgHandlers.end()) {
				return (iter->second)(hwnd,uMsg,wParam,lParam);
			} else {
				return DefWindowProc(hwnd, uMsg, wParam, lParam); 
			}
			break;
		}
	}
}
//---------------------------------------------------------------------------
// ::~WIN
//---------------------------------------------------------------------------
Win::~Win() {
	if (m_ownFont) {
		DeleteObject(hFont);
	}
	delete[] name;
}
