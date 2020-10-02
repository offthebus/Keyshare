#ifndef _H_SCANNER
#define _H_SCANNER

#include "util.h"

class Scanner
{
	public:
		static const int SCAN_PERIOD_MS = 1000;

		static Scanner& getInstance() {
			static Scanner instance;
			return instance;
		}


		void lock() {
			m_critSec.lock();
		}

		void unlock() {
			m_critSec.unlock();
		}

		void start() {
			if (!m_scanning) {
				clear();
				SetEvent(m_hEvents[H_SCAN]);
				m_scanning = true;
				printf("scanning ON\n");
			} else {
				printf("already scanning\n");
			}
		}

		void stop() {
			if (m_scanning) {
				ResetEvent(m_hEvents[H_SCAN]);
				m_scanning = false;
				printf("scanning OFF\n");
			}
		}

		void terminate() {
			m_scanning = false;
			SetEvent(m_hEvents[H_TERMINATE]);
		}

		bool scanning() {
			return m_scanning;
		}

		struct Window {
			HWND hwnd;
			const char* name;
			const char* classname;
			int ordinal;
			bool master;
			RECT pos;
			bool zoomed;

			Window(HWND _hwnd, int _ordinal) {
				master = zoomed = false;
				hwnd = _hwnd;
				ordinal = _ordinal;
				char buf[128];
				GetWindowText(hwnd,buf,_countof(buf));
				name = util::strAllocCopy(buf);
				GetClassName(hwnd,buf,_countof(buf));
				classname = util::strAllocCopy(buf);
			}

			~Window() {
				delete[] name;
				delete[] classname;
			}
		};

		void makeMaster(int windowNumber /*one based*/);

		void printWindowList() {
			for (size_t i = 0; i<m_windows.size(); ++i) {
				printf("Window number %d: master flag: %s, handle 0x%08X, name %s, class %s\n",
					m_windows[i]->ordinal, m_windows[i]->master?"TRUE":"FALSE",(unsigned int)m_windows[i]->hwnd, m_windows[i]->name, m_windows[i]->classname);
			}
		}

		typedef std::vector<Window*> WINDOWLIST;
		const WINDOWLIST& windows() {
			return m_windows;
		}

		const Window* find(HWND _hwnd) {
			for (WINDOWLIST::iterator iter = m_windows.begin(); iter != m_windows.end(); ++iter) {
				if ( (*iter)->hwnd == _hwnd) {
					unlock();
					return *iter;
				}
			}
			return NULL;
		}

		const Window* master() {
			for (size_t i=0; i<m_windows.size(); ++i) {
				if (m_windows[i]->master) {
					unlock();
					return m_windows[i];
				}
			}
			return NULL;
		}
		
		int getSlaves(WINDOWLIST& dst) {
			for (size_t i=0; i<m_windows.size(); ++i) {
				if (!m_windows[i]->master) {
					dst.push_back(m_windows[i]);
				}
			}
			return dst.size();
		}

	private:
		void operator=(const Scanner&) = delete;
		Scanner(const Scanner&) = delete;
		Scanner();
		~Scanner();

		int lock_count;
		WINDOWLIST m_windows;
		HANDLE m_hThread;
		bool m_scanning;
		util::CriticalSection m_critSec;

		enum {H_TERMINATE, H_SCAN, H_NUM_EVENTS};
		HANDLE m_hEvents[H_NUM_EVENTS];
		
		void clear() {
			for (size_t i=0; i<m_windows.size(); ++i) {
				delete m_windows[i];
			}
			m_windows.clear();
		}

		static BOOL CALLBACK enumWindowsProc(HWND _hwnd, LPARAM lParam);
		static DWORD WINAPI scanWindowsThread(LPVOID param);
};


#endif
