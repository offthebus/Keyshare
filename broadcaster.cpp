#include "stdafx.h"
#include "broadcaster.h"
#include "util.h"

//---------------------------------------------------------------------------
// ::BROADCAST
//---------------------------------------------------------------------------
Broadcaster::Broadcaster() 
: m_keyboardEnabled(false), m_mouseEnabled(false), m_toBeRenamed(0), m_master(0), 
  m_masterWarn(false), m_windowsReadyToZoom(false), m_echo(false) 
{
	util::zeroMem(m_keyState);
	initFilter();
	m_hThread = CreateThread( NULL, 0, inputThread, (LPVOID)this, 0, 0 );
}
//---------------------------------------------------------------------------
// ::~BROADCAST
//---------------------------------------------------------------------------
Broadcaster::~Broadcaster() {
	delete m_pWin;
}
//---------------------------------------------------------------------------
// BROADCAST::PUSH_BACK
//---------------------------------------------------------------------------
void Broadcaster::push_back(HWND hwnd, bool master/*== false*/) 
{
	if (master) {
		if (m_master) {
			if (hwnd != m_master && !m_masterWarn) {
				m_masterWarn=true;
				printf("You have more than one master window\n");
			}
		} else {
			printf("found master\n");
			m_master = hwnd;
		}
	} else {
		bool found=false;
		for (size_t i=0; !found && i<m_slaves.size(); ++i) {
			found = m_slaves[i].hwnd == hwnd;
		}
		if (!found) {
			printf("found slave\n");
			m_critSec.lock();
			WindowDetails win = {hwnd,false,{0,0,0,0}};
			m_slaves.push_back(win);
			m_critSec.unlock();
		}
	}
}
//---------------------------------------------------------------------------
// BROADCAST
//---------------------------------------------------------------------------
int Broadcaster::broadcast(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RAWINPUT raw;
	static UINT size = sizeof(raw);
	static DWORD& inputType = raw.header.dwType;
	static USHORT& scanCode = raw.data.keyboard.MakeCode;
	static USHORT& vKey = raw.data.keyboard.VKey;
	static USHORT& flags = raw.data.keyboard.Flags;
	static UINT& msg = raw.data.keyboard.Message;

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)&raw, &size, sizeof(RAWINPUTHEADER));
	HWND curActiveWindow = GetForegroundWindow();
	Broadcaster& bc = Broadcaster::getInstance();

	if (bc.m_echo && inputType == RIM_TYPEKEYBOARD) {
		printf("VKEY: 0x%04X SCAN: 0x%04X FLAGS: 0x%04X\n", vKey,scanCode,flags);
	}
	
	// update state
	if (inputType == RIM_TYPEKEYBOARD) {	
		if (bc.rawKeyUp(flags)) {
			bc.m_keyState[vKey] = 0; // key up
		} else if (bc.rawKeyDown(flags)) {
			if (bc.m_keyState[vKey]) {
				bc.m_keyState[vKey] |= 0x80; // repeat
			} else {
				bc.m_keyState[vKey] = 1; // key down 
			}
		}
	}

	// Hotkeys 
	if ( bc.down(VK_CONTROL) && bc.pressed(VK_OEM_6)) {
		bc.m_toBeRenamed = curActiveWindow;
		SetForegroundWindow(GetConsoleWindow());
		bc.injectRename();
		return 0;
	}
	if ( bc.down(VK_CONTROL) && bc.pressed(VK_OEM_PLUS) ) {
		bc.m_echo = !bc.m_echo;
		printf("echo %s\n", bc.m_echo ? "ON" : "OFF");
	}
	
	// zoom key '`'
	if (bc.pressed(VK_OEM_8)) {
		bc.zoomSlave(true);
	} else if (bc.released(VK_OEM_8)) {
		bc.zoomSlave(false);
	}

	// special: send up arrow, left arrow on middle mouse
	// through Blizz keymapping sends slaves forward and left/right
	// with 3 toons, the slaves move up to stand alongide master
	if (inputType == RIM_TYPEMOUSE ) {
		unsigned short int& flags = raw.data.mouse.usButtonFlags;
		UINT msg = (flags&RI_MOUSE_MIDDLE_BUTTON_DOWN) ? WM_KEYDOWN : (flags&RI_MOUSE_MIDDLE_BUTTON_UP) ? WM_KEYUP : 0;
		if (msg) {
			for (size_t i=0; i<bc.m_slaves.size(); ++i) {
				PostMessage(bc.m_slaves[i].hwnd,msg,(WPARAM)VK_UP,0);
				PostMessage(bc.m_slaves[i].hwnd,msg,(WPARAM)VK_LEFT,0);
			}
		}
	}

	// ignore input not for master
	if (curActiveWindow != bc.m_master) {
		return 0;
	}

	// handle keyboard
	if (bc.m_keyboardEnabled && inputType == RIM_TYPEKEYBOARD ) {
		BROADCAST_FILTER::iterator iter = bc.m_filter.find(vKey);
		if (iter==bc.m_filter.end()) {
			return 0;
		}
		UINT msg = bc.rawKeyUp(flags) ? WM_KEYUP : bc.rawKeyDown(flags) ? WM_KEYDOWN : 0;
		if (msg) {
			LPARAM lparam = bc.pressed(vKey) ? 0 : bc.repeated(vKey) ? 0x40000000 : 0xC0000000;
			lparam |= (scanCode<<16);
			for (size_t i=0; i<bc.m_slaves.size(); ++i) {
				PostMessage(bc.m_slaves[i].hwnd,msg,(WPARAM)vKey,lparam);
			}
		}
	}

	return 0; 
}
//---------------------------------------------------------------------------
// BROADCAST::DEBUGPRINT
//---------------------------------------------------------------------------
void Broadcaster::debugPrint(HWND hwnd, const char* extra/*==0*/)
{
	char name[64];
	char classname[64];
	GetWindowText(hwnd,name,_countof(name));
	GetClassName(hwnd,classname,_countof(classname));
	printf("%shandle=0x%08X, name=%s, class=%s\n", extra?extra:"", (UINT)hwnd, name, classname);
}
//---------------------------------------------------------------------------
// WINAPI BROADCAST::RAWINPUTTHREAD
//---------------------------------------------------------------------------
DWORD WINAPI Broadcaster::inputThread(LPVOID param)
{
	Broadcaster& bc = Broadcaster::getInstance();
	bc.m_pWin = new Win(HINST_THISCOMPONENT,"BroadcasterThread",0,0,10,10,0);
	bc.m_pWin->addMsgHandler(WM_INPUT,broadcast);

	RAWINPUTDEVICE rid[2] = {
		{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK|RIDEV_NOHOTKEYS, Broadcaster::getInstance().m_pWin->hwnd },
		{ HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_MOUSE, RIDEV_INPUTSINK, Broadcaster::getInstance().m_pWin->hwnd }
	};

	if (!RegisterRawInputDevices(rid,2,sizeof(rid[0]))) {
		printf("Warning: Broadcaster::inputThread failed to register raw input devices\n");
	}

	MSG msg;
	util::zeroMem(msg);

	while (msg.message != WM_QUIT) { 
		while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE && msg.message != WM_QUIT) {
			if (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			} 
		}
	}

	printf("Input thread terminated\n");
	ExitThread(0);
}
//---------------------------------------------------------------------------
// BROADCAST::ZOOMSLAVE
//---------------------------------------------------------------------------
void Broadcaster::zoomSlave(bool zoom)
{
	if (!m_windowsReadyToZoom) {
		return;
	}

	m_critSec.lock();
	POINT p;
	GetCursorPos(&p);
	if (zoom) {
		// zoom window under cursor
		for (size_t i=0; i<m_slaves.size(); ++i) {
			RECT r;
			GetWindowRect(m_slaves[i].hwnd, &r);
			bool mouseOver = ( p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom);
			if ( mouseOver ) {
				if (!m_slaves[i].zoomed) {
					// zoom - this is just for the layout with slaves down RH side of screen
					util::Screen screen;
					RECT& pos = m_slaves[i].pos;
					int w = (pos.right-pos.left)*4/3;
					int h = (pos.bottom-pos.top)*4/3;
					int x = pos.right-w; // grow left
					int y = pos.top + h < screen.h ? pos.top : pos.bottom-h; // grow down if possible else up
					SetWindowPos(m_slaves[i].hwnd,HWND_TOPMOST,x,y,w,h,SWP_SHOWWINDOW);
					m_slaves[i].zoomed = true;
					break;
				}
			}
		}
	} else {
		// unzoom all
		for (size_t i=0; i<m_slaves.size(); ++i) {
			if ( m_slaves[i].zoomed) {
				RECT& pos = m_slaves[i].pos;
				SetWindowPos(m_slaves[i].hwnd,0,pos.left,pos.top,pos.right-pos.left,pos.bottom-pos.top,SWP_NOZORDER|SWP_SHOWWINDOW);
				m_slaves[i].zoomed = false;
			}
		}
	}
	
	m_critSec.unlock();
}
//---------------------------------------------------------------------------
// BROADCAST::INJECTRENAME
//---------------------------------------------------------------------------
void Broadcaster::injectRename()
{
	//TODO: should be in dispatcher
	static struct { WORD vKey, scancode; } cmd = { 0x52, 0x13 }, newline = {VK_RETURN, 0x1C};
	static INPUT_RECORD input[2] = {
		{ KEY_EVENT, { TRUE,		0, cmd.vKey, cmd.scancode, {'r'}, 0 } },
		{ KEY_EVENT, { TRUE,		0, newline.vKey, cmd.scancode, {newline.vKey}, 0 } },
	};

	DWORD count = 0;
	HANDLE hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
	WriteConsoleInput(hConsoleIn,input,2,&count);
}
//---------------------------------------------------------------------------
// BROADCAST::INITFILTER
//---------------------------------------------------------------------------
void Broadcaster::initFilter()
{
	m_filter.insert(BROADCAST_FILTER::value_type(VK_LEFT,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_UP,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_DOWN,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_RIGHT,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_TAB,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_SHIFT,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_CONTROL,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_MENU,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_SPACE,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_0,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_1,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_2,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_3,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_4,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_5,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_6,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_7,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_8,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_9,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_Q,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_E,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_R,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_T,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_Y,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_U,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_I,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_O,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_G,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_H,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_J,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_Z,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_X,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_V,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_N,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD0,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD1,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD2,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD3,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD4,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD5,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD6,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD7,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD8,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD9,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_MULTIPLY,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_ADD,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_SEPARATOR,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_SUBTRACT,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_DECIMAL,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_DIVIDE,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F1,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F2,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F3,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F4,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F5,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F6,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F7,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F8,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F9,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F10,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F11,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_F12,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_PLUS,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_COMMA,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_MINUS,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_PERIOD,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_1,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_2,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_3,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_4,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_5,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_6,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_7,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_8,1));
	m_filter.insert(BROADCAST_FILTER::value_type(VK_OEM_102,1));
}
