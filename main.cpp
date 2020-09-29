#include "stdafx.h"
#include "main.h"
#include "broadcaster.h"
#include "scanner.h"

//---------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Dispatcher dispatcher;

	dispatcher.push_back('q',onQuit);	
	dispatcher.push_back('h',onHelp);	
	dispatcher.push_back('m',onMaster);
	dispatcher.push_back('s',onScan);
	dispatcher.push_back('b',onBroadcast);
	dispatcher.push_back('w',onWindows);
	dispatcher.push_back('l',onLayout);
	
	dispatcher.execute('s',"start");
	dispatcher.mainloop();

	ExitProcess(0);
}
//---------------------------------------------------------------------------
// ONMASTER
//---------------------------------------------------------------------------
int onMaster(Dispatcher::CMDLINE& cmdLine)
{
	if (cmdLine.size() == 2) {
		int windowNum = atoi(cmdLine[1]);
		Scanner& scanner = Scanner::getInstance();
		scanner.makeMaster(windowNum);
	} else {
		printf("format of command is: \"m(aster) {N}\"\n");
	}
	
	return 1;
}
//---------------------------------------------------------------------------
// ONLAYOUT
//---------------------------------------------------------------------------
int onLayout(Dispatcher::CMDLINE&)
{
	Broadcaster& bc = Broadcaster::getInstance();
	bc.windowsReadyToZoom(false);

	Scanner& scanner = Scanner::getInstance();
	scanner.lock();

	if (scanner.master()) {
		util::Screen screen;

		// master occupies left hand 3/4 of the screen
		int x = 0, y=0, w=screen.w*3/4, h=screen.h;
		
		// adjust for taskbar
		switch (screen.getTaskBarPosition()) {
			case util::Screen::TB_LEFT: {
				x = screen.tb.rc.right;
				w -= screen.tb.rc.right;
				break;
			}
			case util::Screen::TB_TOP: {
				y = screen.tb.rc.bottom;
				h -= screen.tb.rc.bottom;
				break;
			}
			case util::Screen::TB_BOTTOM: {
				h -= screen.tb.rc.bottom-screen.tb.rc.top;
				break;
			}
		}

		// set master
		SetWindowPos(scanner.master()->hwnd,NULL,x,y,w,h,SWP_NOZORDER|SWP_SHOWWINDOW);
		
		Scanner::WINDOWLIST slaves;
		scanner.getSlaves(slaves);
	
		if (slaves.size()) {
			x += w; // slaves x from RH edge of master, y same as master
			h /= slaves.size(); // divide master height between slaves
			w = screen.w/4; // slaves occupy right hand quarter of screen

			// adjust for taskbar
			if (screen.getTaskBarPosition() == screen.TB_RIGHT) {
				w -= screen.tb.rc.right-screen.tb.rc.left;
			}

			// set slave(s)
			for (size_t i=0; i<slaves.size(); ++i) {
				SetWindowPos(slaves[i]->hwnd,NULL,x,y,w,h,SWP_NOZORDER|SWP_SHOWWINDOW);
				GetWindowRect(slaves[i]->hwnd,(LPRECT)&slaves[i]->pos);
				y += h;
			}
		}
		bc.windowsReadyToZoom(true);
	}

	scanner.unlock();
	return 1;
}
//---------------------------------------------------------------------------
// ONBROADCAST
//---------------------------------------------------------------------------
int onBroadcast(Dispatcher::CMDLINE& cmdLine) 
{
	bool ok = cmdLine.size()==2;
	bool start = ok && !_stricmp(cmdLine[1],"start");
	bool stop = ok && !start && !_stricmp(cmdLine[1],"stop");

	if (!ok || (!start && !stop)) {
		printf("command format is: \"b(roadcast) {start|stop}\"\n");
		return 1;
	}

	if (start) {
		Scanner::getInstance().stop();
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
	bool ok = cmdLine.size()==2;
	bool start = ok && !_stricmp(cmdLine[1],"start");
	bool stop = ok && !start && !_stricmp(cmdLine[1],"stop");

	if (!ok || (!start && !stop)) {
		printf("command format is: \"s(scan) {start|stop}\"\n");
		return 1;
	}

	Scanner& scanner = Scanner::getInstance();

	if (start) {
		if (!scanner.scanning()) {
			Broadcaster::getInstance().stop();
			scanner.start();
		}
	} else {
		scanner.stop();
	}

	return 1;
}
////---------------------------------------------------------------------------
//// ONRENAME
////---------------------------------------------------------------------------
//int onRename(Dispatcher::CMDLINE& args)
//{
//	printf("command suspended\n");
//	return 1;
//
//	HWND toBeRenamed = Broadcaster::getInstance().toBeRenamed();
//	char inp[64];
//	
//	if (!toBeRenamed) {
//		return 1;
//	}
//
//	HWND console = GetConsoleWindow();
//	SetWindowPos(console,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
//	SetForegroundWindow(console);
//
////	printInfo(toBeRenamed,"* Rename window: " );
//	printf("* Enter name: ");
//	gets_s(inp,sizeof(inp));
//	if (*inp) {
//		SetWindowText(toBeRenamed,inp);
//		printf("* done\n");
//	} else {
//		printf("* no input - cancelled\n");
//	}
//
//	return 1;
//}
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
	printf("Broadcast %s, Scan %s\n", bc.isKeyboardEnabled()?"ON":"OFF",sc.scanning()?"ON":"OFF");
	sc.printWindowList();
	return 1;
}
//---------------------------------------------------------------------------
// ONHELP
//---------------------------------------------------------------------------
int onHelp(Dispatcher::CMDLINE& cmdLine) 
{
	printf("List of Commands:\n");
	printf("  s(can) {start|stop}       scan for World of Warcraft windows, turns off broadcast\n"
			 "  b(roadcast) {start|stop}  broadcast keyboard, turns off scan\n"
			 "  w(indows)                 list windows\n"
			 "  m(aster) {N}              designate window number N as the master\n"
			 "  l(ayout)                  layout windows\n"
			 "  `                         zoom slave under cursor, press again to unzoom\n"
			 "  h(elp)                    print this info\n"
			 "  q(uit)                    exit application\n");
	printf("Mouse to arrow key mappings:\n");
	printf("  middle button       up arrow\n"
			 "  Ctrl+middle button  down arrow\n"
			 "  button4             left arrow\n"
			 "  Ctrl+button4        right arrow\n");

	return 1;
}

////---------------------------------------------------------------------------
//// CONFIG::PARSESCRIPT
////---------------------------------------------------------------------------
//void Config::parseScript()
//{
//	// format KEY ID <keyname> TARGET <alias> [alias [...]]
//	
//	enum { ERR_INCOMPLETE=1, ERR_FIRST, ERR_ID, ERR_FMT };
//
//	// load the script
//	util::Log::getInstance().enable(true);
//	util::StringFile script("config.txt");
//	if (!script.isValid()) {
//		printf("Script file config.txt not found or unreadable, see log.txt\n");
//		return;
//	}
//	
//	// tokenise the script
//	char* context = 0;
//	char* token = strtok_s(script.data()," ",&context);
//	std::vector<char*> tokens;
//	while (token) {
//		tokens.push_back(token);
//		token = strtok_s(NULL," \t\r\n",&context);
//	}
//
//	// parse the script
//	size_t cur = 0;
//	int error = 0;
//	if (strcmp(tokens[0],"KEY")) {error=ERR_FIRST;} // first token must be a KEY command
//	while (!error) {
//		// must be minimum 4 tokens left for KEY command
//		if (cur>tokens.size()-4) { error=ERR_INCOMPLETE; continue; }
//
//		// check format: KEY id TARGET alias [alias...]
//		if ( strcmp(tokens[cur+2],"TARGET")) { error=ERR_FMT; continue; }
//
//		// translate key id to virtual key
//		int vkey = -1;
//		char* id = tokens[cur+1];
//		int c = *id;
//		int len = strlen(id);
//		if (len==1) { // 0-9, A-Z, a-z
//			if (c>='0' && c<='9') {
//				vkey = 0x30 + (c-'0');
//			} else if (c>='A' && c<='Z') {
//				vkey = 0x41 + (c-'A');
//			}
//		} else if ( c == 'F') { // F1 - F12
//			unsigned int fkey = atol(id+1);
//			if (fkey>0 && fkey<13) {
//				vkey = VK_DIVIDE+fkey;
//			}
//		}
//		if ( vkey == -1 ) { error=ERR_ID; continue; }
//		assert(vkey>=0 && vkey<_countof(keyTable));
//
//		// parse out aliases
//		cur += 3; // starting on first alias KEY id TARGET alias ...
//		while (cur<tokens.size() && strcmp(tokens[cur],"KEY")) {
//			keyTable[vkey].push_back(TARGET::value_type(tokens[cur++],0));// hwnd is fixed up by alias command
//		}
//
//		if (cur==tokens.size()) { // finished
//			break;
//		}
//
//		// repeat, cur is next KEY command
//	}
//
//	if (error) {
//		printf("Cannot parse script file: %s\n",
//			error==ERR_INCOMPLETE?"Unexpected EOF":
//			error==ERR_FIRST?"Unexpected text at start of file":
//			error==ERR_ID?"found an unrecognised key ID":
//			error==ERR_FMT?"found an invalid KEY command":" ");
//	} else {
//		for (int i=0; i<_countof(keyTable); ++i) {
//			if (keyTable[i].size()) {
//				printf("KEY %s VKEY %d TARGET ",util::VirtualKeyDictionary::getInstance().lookup(i),i);
//				for (int j=0; j<(int)keyTable[i].size(); ++j) {
//					printf("%s ", keyTable[i][j].first);
//				}
//				printf("\n");
//			}
//		}
//		valid = true;
//	}
//}
