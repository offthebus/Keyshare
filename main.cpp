#include "stdafx.h"
#include "main.h"

const char* Globals::APPNAME = "Keyshare";

//---------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------
int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	//TODO: 
	// make scan manual
	// turn broadcast off/on
	// permit multiple slaves
	// send Ctrl+click as Rbutton down/up for ctm

	char inp[256];
//	char* inp;

//	globals.script = new Config;
//	if (!globals.script->valid) {
//		printf("Failed to load script\n");
//		return 0;
//	}

	Globals& globals = Globals::getInstance(); 

	initDispatcher();

	HANDLE hThreads[2];
	hThreads[0] = CreateThread( NULL, 0, rawInputThread, (void*)hInstance, 0, 0 );
	hThreads[1] = CreateThread( NULL, 0, scanWindowsThread, 0, 0, 0 );

	printf("Keyshare\n");
	printf("Broadcast: ON\n");

	while ( printf("> "), gets_s(inp,sizeof(inp))) {
		char* context = 0;
		char* token = strtok_s(inp," ",&context);
		CMDLINE args;
		while (token) {
			args.push_back(token);
			token = strtok_s(NULL," ",&context);
		}
		if (dispatch(args) == 0) {
			break;
		}
	}

	// signal quit and wait for threads to exit
	globals.quit = true;
	if ( WAIT_TIMEOUT == WaitForMultipleObjects(2,hThreads,TRUE,5000)) {
		printf("Gave up waiting for threads to exit\n");
	}

	printf("Keyshare exit\n");
	Sleep(1000);
	ExitProcess(0);
}
//---------------------------------------------------------------------------
// INJECTSETMASTER
//---------------------------------------------------------------------------
void injectRename()
{
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
// BROADCAST
//---------------------------------------------------------------------------
int broadcast(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RAWINPUT raw;
	static UINT size = sizeof(raw);
	static USHORT& scanCode = raw.data.keyboard.MakeCode;
	static USHORT& vKey = raw.data.keyboard.VKey;
	static USHORT& flags = raw.data.keyboard.Flags;
	static UINT& msg = raw.data.keyboard.Message;
	static bool ctrl=false;
	static Globals& globals = Globals::getInstance();

	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, (LPVOID)&raw, &size, sizeof(RAWINPUTHEADER));
	HWND curActiveWindow = GetForegroundWindow();

	if (vKey==VK_CONTROL) {
		ctrl=flags==0;
	}

	// hotkey Ctrl+] rename active window
	if (ctrl && vKey==VK_OEM_6 && flags==0) {
		globals.toBeRenamed = curActiveWindow;
		SetForegroundWindow(GetConsoleWindow());
		injectRename();
		return 0;
	}

	if (curActiveWindow == globals.master) {
		if (vKey<VK_SHIFT || vKey>VK_F12 || (vKey>=VK_LWIN && vKey<=VK_SLEEP)) {
			return 0;
		}
		switch (vKey) {
			case VK_PAUSE: case VK_CAPITAL: case VK_ESCAPE: case VK_SELECT: case VK_PRINT: case VK_SNAPSHOT: 
				return 0;
		};

		UINT msg = flags==0?WM_KEYDOWN:flags==1?WM_KEYUP:0;
		if (msg) {
			LPARAM lparam = msg==WM_KEYDOWN ? 0x4000000|(scanCode<<16) : 0xC0000000|(scanCode<<16);
			PostMessage(globals.slave,msg,(WPARAM)vKey,lparam);
		}
	}

	return 0; 
}
//---------------------------------------------------------------------------
// WINAPI RAWINPUTTHREAD
//---------------------------------------------------------------------------
DWORD WINAPI rawInputThread(LPVOID param)
{
	Globals& globals = Globals::getInstance();
	globals.pWin = new Win((HINSTANCE)param,globals.APPNAME,0,0,10,10,0);
	globals.pWin->addMsgHandler(WM_INPUT,broadcast);

	RAWINPUTDEVICE rid = { HID_USAGE_PAGE_GENERIC, HID_USAGE_GENERIC_KEYBOARD, RIDEV_INPUTSINK|RIDEV_NOHOTKEYS, globals.pWin->hwnd };

	if (RegisterRawInputDevices(&rid,1,sizeof(RAWINPUTDEVICE))) {
		MSG msg;
		while (!globals.quit) {
			while ( PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE) {
				if (GetMessage(&msg, NULL, 0, 0)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				} 
			}
			Sleep(1);
		}
	}
	DestroyWindow(globals.pWin->hwnd);
	delete globals.pWin;
	globals.pWin = 0;
	printf("Input thread terminated\n");
	ExitThread(0);
}
//---------------------------------------------------------------------------
// CALLBACK ENUMWINDOWSPROC
//---------------------------------------------------------------------------
BOOL CALLBACK enumWindowsProc(HWND _hwnd, LPARAM lParam)
{
	Globals& globals = Globals::getInstance();
	char buf[256];
	GetWindowText(_hwnd,buf,_countof(buf));
	_strlwr_s(buf,_countof(buf));
	if (!strncmp(buf, "master", 6) && _hwnd!=globals.master) {
		globals.master = _hwnd;
		printf("found master\n> ");
	} else if (!strncmp(buf, "slave", 5) && _hwnd!=globals.slave) {
		globals.slave = _hwnd;
		printf("found slave\n> ");
	}
	return TRUE;
}
//---------------------------------------------------------------------------
// WINAPI SCANWINDOWSTHREAD
//---------------------------------------------------------------------------
DWORD WINAPI scanWindowsThread(LPVOID param)
{
	Globals& globals = Globals::getInstance();
	globals.scanning = true;		

	globals.master = globals.slave = 0;

	while (!globals.quit) {
		EnumWindows(enumWindowsProc,0);
		Sleep(1000);
	}

	globals.scanning = false;
	printf("Scan thread terminated\n");
	ExitThread(0);
}
//---------------------------------------------------------------------------
// INITDISPATCHER
//---------------------------------------------------------------------------
void initDispatcher()
{
	Globals& globals = Globals::getInstance();

	globals.dispatcher.insert(DISPATCHER::value_type('q',onQuit));	
	globals.dispatcher.insert(DISPATCHER::value_type('h',onHelp));	
	globals.dispatcher.insert(DISPATCHER::value_type('r',onRename));
	globals.dispatcher.insert(DISPATCHER::value_type('s',onScan));
}
//---------------------------------------------------------------------------
// DISPATCH
//---------------------------------------------------------------------------
int dispatch(CMDLINE& args)
{
	Globals& globals = Globals::getInstance();

	if (args.size()) { 
		DISPATCHER::iterator iter = globals.dispatcher.find(tolower(args[0][0]));
		if (iter != globals.dispatcher.end()) {
			return (iter->second)(args);
		} else {
			printf("%s is not a recognised command\n",args[0]);
		}
	}
	return 1;
}
//---------------------------------------------------------------------------
// ONSCAN
//---------------------------------------------------------------------------
int onScan(CMDLINE& cmdLine)
{
	static bool broadcastPrevState;
	Globals& globals = Globals::getInstance();
	bool ok = false;

	if (cmdLine.size() == 2 ) {
		if ( !_stricmp(cmdLine[1],"start")) {
			printf("> scanning ...\n");
			broadcastPrevState = globals.broadcast;
			globals.broadcast = false;
			if (broadcastPrevState == true) {
				printf("> Broadcast: OFF\n");
			}
			ok = true;
		} else if (!_stricmp(cmdLine[1],"stop")) {
			printf("> ending scan\n");
			globals.broadcast = broadcastPrevState;
			if (globals.broadcast) {
				printf("> Broadcast: ON\n");
			}
			ok = true;
		}
	}

	if (!ok) {
		printf("> Format of command is: s(can) {start|stop}\n");
	}

	return 1;
}
//---------------------------------------------------------------------------
// ONRENAME
//---------------------------------------------------------------------------
int onRename(CMDLINE& args)
{
	Globals& globals = Globals::getInstance();
	char inp[64];
	
	SetForegroundWindow(GetConsoleWindow());
	printf("> Enter name: ");
	gets_s(inp,sizeof(inp));
	if (*inp) {
		SetWindowText(globals.toBeRenamed,inp);
		printf("> done\n");
	} else {
		printf("> no input - cancelled\n");
	}

	return 1;
}
//---------------------------------------------------------------------------
// ONQUIT
//---------------------------------------------------------------------------
int onQuit(CMDLINE& cmdLine)
{
	return 0;
}
//---------------------------------------------------------------------------
// ONLIST
//---------------------------------------------------------------------------
int onList(CMDLINE& cmdLine)
{
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
int onHelp(CMDLINE& cmdLine) 
{
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
