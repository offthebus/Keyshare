#ifndef _H_MAIN
#define _H_MAIN

#include "win.h"
#include "util.h"
#include "dispatcher.h"

int onHelp(Dispatcher::CMDLINE&);
int onQuit(Dispatcher::CMDLINE&);
int onInfo(Dispatcher::CMDLINE&);
int onRename(Dispatcher::CMDLINE&);
int onScan(Dispatcher::CMDLINE&);
int onLayout(Dispatcher::CMDLINE&);
int onMouse(Dispatcher::CMDLINE&);
int onKeyboard(Dispatcher::CMDLINE&);

void printInfo(HWND hwnd, const char* extra=0);
BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
DWORD WINAPI scanWindowsThread(LPVOID param);

////---------------------------------------------------------------------------
//class Config
//{
//	public:
//		Config() : valid(false) { parseScript(); }
//		bool valid;
//
//	private:
//		typedef std::vector<std::pair<const char*, HWND>> TARGET; // alias,handle per vkey
//		TARGET keyTable[256];
//
//		void parseScript();
//};

//---------------------------------------------------------------------------
struct Globals
{
	static const char* APPNAME;

	enum { H_QUIT, H_SCAN, H_NUM_EVENTS };
	HANDLE hEvents[H_NUM_EVENTS];

	HANDLE hThread;
	HWND toBeRenamed;
	bool scanning;

	static Globals& getInstance() {
		static Globals instance;
		return instance;
	}

	private:
		Globals(const Globals&) = delete;
		void operator=(const Globals&) = delete;

	Globals() {
		scanning = false;
		toBeRenamed = 0;
		util::zeroMem(hEvents);
		hThread = 0;
	};

	~Globals() {
	}


};

#endif
