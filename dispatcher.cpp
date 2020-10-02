#include "stdafx.h"
#include "dispatcher.h"
#include "util.h"

//---------------------------------------------------------------------------
// DISPATCHER::DISPATCH
//---------------------------------------------------------------------------
int Dispatcher::dispatch(CMDLINE cmdLine)
{
	DISPATCHER::iterator iter = m_dispatcher.find(tolower(cmdLine[0][0]));
	if (iter != m_dispatcher.end()) {
		return (iter->second)(cmdLine);
	} else {
		printf("%s is not a recognised command\n",cmdLine[0]);
		return 1;
	}
}
//---------------------------------------------------------------------------
// DISPATCHER::GETINPUT
//---------------------------------------------------------------------------
bool Dispatcher::getInput() 
{
	util::zeroMem(m_input);
	int i;
	for (i=0; i<INPUT_BUF_CHARS; ++i) {
		m_input[i] = getchar();
		if (m_input[i] == '\n') {
			break;
		}
		if (m_input[i] == EOF) {
			return false;
		}
	}
	if (m_input[i] == '\n') {
		m_input[i] = 0;
	} else {
		char c;
		do {
			c = getchar();
		} while (c != '\n');
	}

	strcpy_s(m_inputLine,_countof(m_inputLine),m_input);
	return true;
}
//---------------------------------------------------------------------------
// DISPATCHER::MAINLOOP
//---------------------------------------------------------------------------
void Dispatcher::mainloop() 
{
	while ( printf(""), getInput()) {
		char* context = 0;
		char* token = strtok_s(m_input," ",&context);
		CMDLINE args;
		while (token) {
			args.push_back(token);
			token = strtok_s(NULL," ",&context);
		}
		if (args.size() && !dispatch(args)) {
			break;
		}
	}
}
//---------------------------------------------------------------------------
// DISPATCHER::EXECUTE
//---------------------------------------------------------------------------
int Dispatcher::execute(COMMAND cmd, const char* arg/*==0*/)
{
	CMDLINE cmdLine;
	char c[2] = { cmd, '\0' };
	cmdLine.push_back(c);
	if (arg) {
		cmdLine.push_back(arg);
	}
	return dispatch(cmdLine);
}

