#ifndef _H_DISPATCHER
#define _H_DISPATCHER

//---------------------------------------------------------------------------
class Dispatcher
{
	public:
		typedef char COMMAND;
		typedef std::vector<const char*> CMDLINE;
		typedef int (*COMMANDHANDLER)(CMDLINE& cmdLine);

		static Dispatcher& getInstance() {
			static Dispatcher instance;
			return instance;
		}

		void push_back(COMMAND cmd,COMMANDHANDLER handler) {
			m_dispatcher.insert(DISPATCHER::value_type(cmd,handler));
		}
		int execute(COMMAND cmd, const char* arg=0);
		void mainloop();

		const char* inputLine() {
			return m_inputLine;
		}

	private:
		void operator=(const Dispatcher&) = delete;
		Dispatcher(const Dispatcher&) = delete;
		Dispatcher() {}
		static const size_t INPUT_BUF_CHARS=32;
		typedef std::unordered_map<COMMAND,COMMANDHANDLER> DISPATCHER;
		DISPATCHER m_dispatcher;
		char m_input[INPUT_BUF_CHARS+1];
		char m_inputLine[_countof(m_input)];
		int dispatch(CMDLINE cmdLine);
		bool getInput();

};

#endif

