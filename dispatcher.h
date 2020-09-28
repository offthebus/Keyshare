#ifndef _H_DISPATCHER
#define _H_DISPATCHER

//---------------------------------------------------------------------------
class Dispatcher
{
	public:
		typedef char COMMAND;
		typedef std::vector<const char*> CMDLINE;
		typedef int (*COMMANDHANDLER)(CMDLINE& cmdLine);

		void push_back(COMMAND cmd,COMMANDHANDLER handler) {
			dispatcher.insert(DISPATCHER::value_type(cmd,handler));
		}
			
		int execute(COMMAND cmd, const char* arg=0);
		void mainloop();

	private:
		static const size_t INPUT_BUF_CHARS=32;
		typedef std::unordered_map<COMMAND,COMMANDHANDLER> DISPATCHER;
		DISPATCHER dispatcher;
		char m_input[INPUT_BUF_CHARS+1];
		int dispatch(CMDLINE cmdLine);
		bool getInput();
};

#endif

