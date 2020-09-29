#ifndef _H_MAIN
#define _H_MAIN

#include "dispatcher.h"

int onHelp(Dispatcher::CMDLINE&);
int onQuit(Dispatcher::CMDLINE&);
int onWindows(Dispatcher::CMDLINE&);
int onScan(Dispatcher::CMDLINE&);
int onLayout(Dispatcher::CMDLINE&);
int onBroadcast(Dispatcher::CMDLINE&);
int onMaster(Dispatcher::CMDLINE&);

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

#endif
