#include "stdafx.h"
#include "main.h"
#include "broadcaster.h"
#include "scanner.h"
#include "windowManager.h"
#include "config.h"

//---------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Dispatcher& dispatcher = Dispatcher::getInstance();

	dispatcher.push_back('q',onQuit);	
	dispatcher.push_back('h',onHelp);	
	dispatcher.push_back('s',onScan);
	dispatcher.push_back('b',onBroadcast);
	dispatcher.push_back('w',onWindows);
	dispatcher.push_back('l',onLayout);

	dispatcher.execute('b', "start");
	dispatcher.execute('s',"start");

	dispatcher.mainloop();
	ExitProcess(0);
}
//---------------------------------------------------------------------------
// ONLAYOUT
//---------------------------------------------------------------------------
int onLayout(Dispatcher::CMDLINE&)
{
	WindowManager::getInstance().layout();
	return 1;
}
//---------------------------------------------------------------------------
// ONBROADCAST
//---------------------------------------------------------------------------
int onBroadcast(Dispatcher::CMDLINE& cmdLine) 
{
	if (cmdLine.size()==1) {
		printf("broadcast is %s\n", Broadcaster::getInstance().isBroadcasting() ? "ON" : "OFF");
		return 1;
	}

	bool ok = cmdLine.size() == 2;
	bool start = ok && !_stricmp(cmdLine[1],"start");
	bool stop = ok && !start && !_stricmp(cmdLine[1],"stop");

	if (!ok || (!start && !stop)) {
		printf("command format is: \"b(roadcast) {start|stop}\"\n");
		return 1;
	}

	if (start) {
		Broadcaster::getInstance().start();
	} else {
		Broadcaster::getInstance().stop();
	} 

	return 1;
}
//---------------------------------------------------------------------------
// ONSCAN
//---------------------------------------------------------------------------
int onScan(Dispatcher::CMDLINE& cmdLine)
{
	if (cmdLine.size()==1) {
		printf("scan is %s\n", Scanner::getInstance().scanning() ? "ON" : "OFF");
		return 1;
	}

	bool ok = cmdLine.size() == 2;
	bool start = ok && !_stricmp(cmdLine[1],"start");
	bool stop = ok && !start && !_stricmp(cmdLine[1],"stop");

	if (!ok || (!start && !stop)) {
		printf("command format is: \"s(scan) {start|stop}\"\n");
		return 1;
	}

	Scanner& scanner = Scanner::getInstance();

	if (start) {
		if (!scanner.scanning()) {
			scanner.start();
		} else {
			printf("already scanning\n");
		}
	} else {
		scanner.stop();
	}

	return 1;
}
//---------------------------------------------------------------------------
// ONQUIT
//---------------------------------------------------------------------------
int onQuit(Dispatcher::CMDLINE& cmdLine)
{
	Scanner::getInstance().terminate();
	Broadcaster::getInstance().terminate();
	Sleep(Scanner::SCAN_PERIOD_MS);
	printf("Keyshare exit\n");
	return 0;
}
//---------------------------------------------------------------------------
// ONINFO
//---------------------------------------------------------------------------
int onWindows(Dispatcher::CMDLINE& cmdLine)
{
	Broadcaster& bc = Broadcaster::getInstance();
	Scanner& sc = Scanner::getInstance();
	printf("Broadcast %s, Scan %s\n", bc.isBroadcasting()?"ON":"OFF",sc.scanning()?"ON":"OFF");
	sc.printWindowList();
	return 1;
}
//---------------------------------------------------------------------------
// ONHELP
//---------------------------------------------------------------------------
int onHelp(Dispatcher::CMDLINE& cmdLine) 
{
	printf("List of Commands:\n");
	printf("  s(can) [start|stop]       starts/stops window scanning, without arg prints current state\n"
			 "  b(roadcast) [start|stop]  starts/stops key broadcasting, without arg prints current state\n"
			 "  l(ayout)                  lays out windows, master left hand 3/4, others down right hand side\n"
			 "  w(indows)                 lists windows\n"
			 "  h(elp)                    prints this info\n"
			 "  q(uit)                    exits application\n");
	printf("Hotkeys:\n");
	printf("  `                         zooms slave under cursor, press again to unzoom\n"
			 "  Ctrl+]                    toggle broadcasting\n");
	printf("Mouse arrow keys:\n");
	printf("  middle button             up arrow\n"
			 "  Ctrl+middle button        down arrow\n"
			 "  button4                   left arrow\n"
			 "  Ctrl+button4              right arrow\n");

	return 1;
}
