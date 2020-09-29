#include "stdafx.h"
#include "scanner.h"

//---------------------------------------------------------------------------
// ::SCANNER
//---------------------------------------------------------------------------
Scanner::Scanner()
: m_hThread(0), m_scanning(false), lock_count(0)
{
	m_hEvents[H_TERMINATE] = CreateEvent(0,TRUE,FALSE,"TerminateEvent");
	m_hEvents[H_SCAN] = CreateEvent(0,TRUE,FALSE,"ScanEvent");
	m_hThread = CreateThread( NULL, 0, scanWindowsThread, 0, 0, 0 );
}
//---------------------------------------------------------------------------
// ::~SCANNER
//---------------------------------------------------------------------------
Scanner::~Scanner()
{
	clear();
}
//---------------------------------------------------------------------------
// CALLBACK SCANNER::ENUMWINDOWSPROC
//---------------------------------------------------------------------------
BOOL CALLBACK Scanner::enumWindowsProc(HWND _hwnd, LPARAM lParam)
{
	char buf[256];
	GetWindowText(_hwnd,buf,_countof(buf));
	if (!strncmp(buf, "World of Warcraft",17)) {
		Scanner& self = Scanner::getInstance();
		if (!self.find(_hwnd)) {
			Window* win = new Window(_hwnd, self.m_windows.size()+1);
			if (strcmp(win->classname,"GxWindowClass")) {
				delete win;
			} else {
				win->master = !strncmp(buf+17," MASTER",7);
				self.lock();
				self.m_windows.push_back(win);
				self.unlock();
				printf("Window number %d: master flag: %s, handle 0x%08X, name %s, class %s\n",
					win->ordinal, win->master?"TRUE":"FALSE",(unsigned int)win->hwnd, win->name, win->classname);
			}
		}
	}
	return TRUE;
}
//---------------------------------------------------------------------------
// WINAPI SCANNER::SCANWINDOWSTHREAD
//---------------------------------------------------------------------------
DWORD WINAPI Scanner::scanWindowsThread(LPVOID param)
{
	Scanner& self = Scanner::getInstance();

	while ( int iEvent = WaitForMultipleObjects(H_NUM_EVENTS, self.m_hEvents, FALSE, INFINITE) - WAIT_OBJECT_0) {
		if ( iEvent == H_TERMINATE ) {
			break;
		} else if ( iEvent == H_SCAN ) {
			printf("scanning for World of Warcraft windows ...\n");
			while (self.m_scanning) {
				EnumWindows(enumWindowsProc,0);
				Sleep(SCAN_PERIOD_MS);
			}
		}
	}

	printf("Scan thread terminated\n");
	ExitThread(0);
}
//---------------------------------------------------------------------------
// SCANNER::MAKEMASTER
//---------------------------------------------------------------------------
void Scanner::makeMaster(int windowNumber /*one based*/) 
{
	static const int MAX_NAME = 32;
	char buf[MAX_NAME];
	int slave = 1;

	lock();

	if (windowNumber<1 || windowNumber>(int)m_windows.size()) {
		printf("window number %d not found\n",windowNumber);
	} else {
		int target = windowNumber-1;
		for (size_t i=0; i<m_windows.size(); ++i) {
			Window* win = m_windows[i];
			if ( i==target ) {
				sprintf_s(buf,_countof(buf),"%s MASTER", "World of Warcraft");
				win->master = true;
			} else {
				sprintf_s(buf,_countof(buf),"%s SLAVE %02d", "World of Warcraft", slave++);
				win->master = false;
			}
			SetWindowText(win->hwnd,buf);
			delete[] win->name;
			win->name = util::strAllocCopy(buf);
		}
	}

	unlock();
	printWindowList();

}
 
