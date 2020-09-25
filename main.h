#ifndef _H_MAIN
#define _H_MAIN

#include "win.h"
#include "util.h"

//---------------------------------------------------------------------------
typedef std::vector<const char*> CMDLINE;
typedef int (*CMDHANDLER)(CMDLINE& cmdLine);
typedef std::unordered_map<char,CMDHANDLER> DISPATCHER;

//---------------------------------------------------------------------------
void injectSetMaster();
int onHelp(CMDLINE&);
int onQuit(CMDLINE&);
int onList(CMDLINE&);
int onRename(CMDLINE&);
int onScan(CMDLINE&);
void initDispatcher();
int dispatch(CMDLINE&);
DWORD WINAPI rawInputThread(LPVOID param);
BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
DWORD WINAPI scanWindowsThread(LPVOID param);
int broadcast(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

	Win* pWin;
	bool scanning;
	bool broadcast;
	bool quit;
	DISPATCHER dispatcher;
	HWND toBeRenamed, master, slave;
//	Config* script;

	static Globals& getInstance() {
		static Globals instance;
		return instance;
	}

	private:
		Globals(const Globals&) = delete;
		void operator=(const Globals&) = delete;

	Globals() {
		pWin = 0;
		quit = false;
		scanning = false;
		toBeRenamed = 0;
		master = 0; 
		slave = 0;
		broadcast = true;
//		script = 0;
	};

	~Globals() {
//		delete script;
	}


};

#endif
