#include "stdafx.h"
#include "main.h"
#include "broadcaster.h"

const char* Globals::APPNAME = "Keyshare";

//---------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//TODO: targeting macro
	// zoom slaves
	
	Dispatcher dispatcher;
	dispatcher.push_back('q',onQuit);	
	dispatcher.push_back('h',onHelp);	
	dispatcher.push_back('r',onRename);
	dispatcher.push_back('s',onScan);
	dispatcher.push_back('k',onKeyboard);
	dispatcher.push_back('m',onMouse);
	dispatcher.push_back('i',onInfo);
	dispatcher.push_back('l',onLayout);

	Broadcaster& bc = Broadcaster::getInstance();
	bc.enableKeyboard(false);
	bc.enableMouse(false);

	Globals& globals = Globals::getInstance(); 
	globals.hEvents[Globals::H_SCAN] = CreateEvent(0,TRUE,FALSE,"ScanEvent");
	globals.hEvents[Globals::H_QUIT] = CreateEvent(0,TRUE,FALSE,"QuitEvent");
	globals.hThread = CreateThread( NULL, 0, scanWindowsThread, 0, 0, 0 );
	globals.scanning = false;

	dispatcher.execute('k');
	dispatcher.execute('s');
	dispatcher.mainloop();

	ExitProcess(0);
}
//---------------------------------------------------------------------------
// PRINTINFO
//---------------------------------------------------------------------------
void printInfo(HWND hwnd, const char* extra/*==0*/)
{
	char name[64];
	char classname[64];
	GetWindowText(hwnd,name,_countof(name));
	GetClassName(hwnd,classname,_countof(classname));
	printf("%shandle=0x%08X, name=%s, class=%s\n", extra?extra:"", (UINT)hwnd, name, classname);
}
//---------------------------------------------------------------------------
// CALLBACK ENUMWINDOWSPROC
//---------------------------------------------------------------------------
BOOL CALLBACK enumWindowsProc(HWND _hwnd, LPARAM lParam)
{
	Broadcaster& bc = Broadcaster::getInstance();
	char buf[256];
	GetWindowText(_hwnd,buf,_countof(buf));
	_strlwr_s(buf,_countof(buf));
	if (!strncmp(buf, "master", 6)) {
		bc.push_back(_hwnd,true);
	} else if (!strncmp(buf, "slave", 5)) {
		bc.push_back(_hwnd);
	}
	return TRUE;
}
//---------------------------------------------------------------------------
// WINAPI SCANWINDOWSTHREAD
//---------------------------------------------------------------------------
DWORD WINAPI scanWindowsThread(LPVOID param)
{
	Globals& globals = Globals::getInstance();

	while ( int iEvent = WaitForMultipleObjects(Globals::H_NUM_EVENTS, globals.hEvents, FALSE, INFINITE) - WAIT_OBJECT_0) {
		if ( iEvent == Globals::H_QUIT ) {
			break;
		} else if ( iEvent == Globals::H_SCAN ) {
			printf("scanning for master/slave windows ...\n");
			globals.scanning = true;		
			Broadcaster::getInstance().clear();
			while (globals.scanning) {
				EnumWindows(enumWindowsProc,0);
				Sleep(1000);
			}
		}
	}

	printf("Scan thread terminated\n");
	ExitThread(0);
}
//---------------------------------------------------------------------------
// ONLAYOUT
//---------------------------------------------------------------------------
int onLayout(Dispatcher::CMDLINE&)
{
	Globals& g = Globals::getInstance();
	Broadcaster& bc = Broadcaster::getInstance();
	bc.windowsReadyToZoom(false);

	if (bc.master()) {
		util::Screen screen;

		// master occupies left hand 2/3 of the screen
		int x = 0, y=0, w=screen.w*2/3, h=screen.h;
		
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
		SetWindowPos(bc.master(),NULL,x,y,w,h,SWP_NOZORDER|SWP_SHOWWINDOW);
		
		const Broadcaster::WINDOWLIST& slaves = bc.slaves();
		if (slaves.size()) {
			x += w; // slaves x from RH edge of master, y same as master
			h /= slaves.size(); // divide master height between slaves
			w = screen.w/3; // slaves occupy right hand third of screen

			// adjust for taskbar
			if (screen.getTaskBarPosition() == screen.TB_RIGHT) {
				w -= screen.tb.rc.right-screen.tb.rc.left;
			}

			// set slave(s)
			for (size_t i=0; i<slaves.size(); ++i) {
				SetWindowPos(slaves[i].hwnd,NULL,x,y,w,h,SWP_NOZORDER|SWP_SHOWWINDOW);
				GetWindowRect(slaves[i].hwnd,(LPRECT)&slaves[i].pos);
				y += h;
			}
		}
		bc.windowsReadyToZoom(true);
	}
	return 1;
}
//---------------------------------------------------------------------------
// ONKEYBOARD
//---------------------------------------------------------------------------
int onKeyboard(Dispatcher::CMDLINE& cmdLine) 
{
	Broadcaster& bc = Broadcaster::getInstance();
	printf("broadcast keyboard %s\n", bc.enableKeyboard(!bc.isKeyboardEnabled()) ? "stopped": "started" );
	return 1;
}
//---------------------------------------------------------------------------
// ONMOUSE
//---------------------------------------------------------------------------
int onMouse(Dispatcher::CMDLINE& cmdLine) 
{
	Broadcaster& bc = Broadcaster::getInstance();
	printf("broadcast mouse %s\n", bc.enableMouse(!bc.isMouseEnabled()) ? "stopped": "started" );
	return 1;
}
//---------------------------------------------------------------------------
// ONSCAN
//---------------------------------------------------------------------------
int onScan(Dispatcher::CMDLINE& cmdLine)
{
	Globals& globals = Globals::getInstance();

	if (!globals.scanning) {
		Broadcaster::getInstance().clear();
		SetEvent(globals.hEvents[Globals::H_SCAN]);
		globals.scanning = true;
	} else {
		ResetEvent(globals.hEvents[Globals::H_SCAN]);
		globals.scanning = false;
	}

	printf("scan %s\n", globals.scanning ? "started" : "stopped" );

	return 1;
}
//---------------------------------------------------------------------------
// ONRENAME
//---------------------------------------------------------------------------
int onRename(Dispatcher::CMDLINE& args)
{
	HWND toBeRenamed = Broadcaster::getInstance().toBeRenamed();
	char inp[64];
	
	if (!toBeRenamed) {
		return 1;
	}

	HWND console = GetConsoleWindow();
	SetWindowPos(console,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
	SetForegroundWindow(console);

	printInfo(toBeRenamed,"* Rename window: " );
	printf("* Enter name: ");
	gets_s(inp,sizeof(inp));
	if (*inp) {
		SetWindowText(toBeRenamed,inp);
		printf("* done\n");
	} else {
		printf("* no input - cancelled\n");
	}

	return 1;
}
//---------------------------------------------------------------------------
// ONQUIT
//---------------------------------------------------------------------------
int onQuit(Dispatcher::CMDLINE& cmdLine)
{
	Globals& globals = Globals::getInstance();
	SetEvent(globals.hEvents[Globals::H_QUIT]);
	globals.scanning = false;
	Broadcaster::getInstance().terminate();
	WaitForSingleObject(globals.hThread,5000);
	printf("Keyshare exit\n");
	Sleep(500);
	return 0;
}
//---------------------------------------------------------------------------
// ONINFO
//---------------------------------------------------------------------------
int onInfo(Dispatcher::CMDLINE& cmdLine)
{
	Broadcaster& bc = Broadcaster::getInstance();

	printf("Application state: broadcast keyboard %s, mouse %s, scan is %s\n", 
		bc.isKeyboardEnabled()?"ON":"OFF", bc.isMouseEnabled()?"ON":"OFF",Globals::getInstance().scanning?"ON":"OFF");
	if (bc.master()) {
		printInfo(bc.master(),"Master: ");
	} else {
		printf("No master window\n");
	}
	const Broadcaster::WINDOWLIST& slaves = bc.slaves();
	for (size_t i=0; i<slaves.size(); ++i) {
		printInfo(slaves[i].hwnd,"Slave: ");
	}

	return 1;
}
////---------------------------------------------------------------------------
//// ONRELOAD
////---------------------------------------------------------------------------
//int onReload(CMDLINE& cmdLine)
//{
//	Config* script = new Config;
//	if (!script->valid) {
//		printf("config.txt load error\n");
//		return 0;
//	} else {
//		delete globals.script;
//		globals.script = script;
//	}
//
//	return 1;
//}
//---------------------------------------------------------------------------
// ONHELP
//---------------------------------------------------------------------------
int onHelp(Dispatcher::CMDLINE& cmdLine) 
{
	printf("List of Commands:\n");
	printf("  s(can)        toggle scan\n"
			 "  k(eyboard)    toggle broadcasting keyboard\n"
			 "  m(ouse)       not implemented\n"
			 "  i(nfo)        list app state and known windows\n"
			 "  l(ayout)      layout windows\n"
			 "  h(elp)        print this info\n"
			 "  q(uit)        exit application\n");
	printf("List of Hotkeys:\n");
	printf("  Ctrl+]        rename active window\n"
			 "  Ctrl+=        toggle echo\n"
			 "  ` (backtick)  zoom slave under cursor\n");
	printf("List of mouse mappings:\n");
	printf("  middle button       up arrow\n"
			 "  Ctrl+middle button  down arrow\n"
			 "  button4             left arrow\n"
			 "  Ctrl+button4        right arrow\n");

	return 1;
}
////---------------------------------------------------------------------------
//// INITFILTER
////---------------------------------------------------------------------------
//void initFilter()
//{
//	Globals& g = Globals::getInstance();
//
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_TAB,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_SHIFT,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_CONTROL,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_MENU,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_SPACE,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_0,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_1,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_2,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_3,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_4,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_5,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_6,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_7,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_8,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_9,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_Q,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_E,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_R,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_T,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_Y,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_U,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_I,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_O,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_G,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_H,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_J,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_Z,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_X,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_V,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_N,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD0,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD1,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD2,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD3,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD4,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD5,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD6,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD7,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD8,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_NUMPAD9,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_MULTIPLY,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_ADD,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_SEPARATOR,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_SUBTRACT,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_DECIMAL,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_DIVIDE,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F1,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F2,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F3,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F4,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F5,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F6,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F7,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F8,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F9,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F10,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F11,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_F12,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_PLUS,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_COMMA,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_MINUS,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_PERIOD,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_1,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_2,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_3,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_4,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_5,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_6,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_7,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_8,1));
//	g.filter.insert(BROADCAST_FILTER::value_type(VK_OEM_102,1));
//}

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
