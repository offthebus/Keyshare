#ifndef _H_WIN
#define _H_WIN

//---------------------------------------------------------------------------
class Win
{
	public:
		typedef int (*MSGHANDLER)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//		enum { MENU_ID_MAIN=100, ID_SELECT_FONT, ID_CLEAR_SCREEN, ID_SAVE_TO_FILE, ID_ENUMERATE };
		
		Win(HINSTANCE hInstance, const char* name, int x, int y, int w, int h, DWORD style,HMENU menuBar=0);
		~Win();

		HWND hwnd;
		const char *name;
		HINSTANCE hInstance;
		HFONT hFont;

		bool isValid() {
			return hwnd != 0;
		}
		
		void addMsgHandler(UINT msg, MSGHANDLER fn) {
			m_msgHandlers.insert(MSGHANDLERMAP::value_type(msg,fn));
		}

		void chooseFont(bool showDialog = true);

	private:
		typedef std::map<UINT,MSGHANDLER> MSGHANDLERMAP;

		static MSGHANDLERMAP m_msgHandlers;
		static LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;

		LOGFONT m_logFont;
		bool m_ownFont;
		void setFont(LOGFONT* lf=0);
};

#endif
