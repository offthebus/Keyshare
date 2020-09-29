#ifndef _H_BROADCASTER
#define _H_BROADCASTER

#include "win.h"
#include "util.h"

//---------------------------------------------------------------------------
class Broadcaster
{
	public:
		enum { VK_0=0x30,VK_1,VK_2,VK_3,VK_4,VK_5,VK_6,VK_7,VK_8,VK_9,VK_A=0x41,VK_B, VK_C, VK_D, VK_E,VK_F,
				 VK_G,VK_H,VK_I,VK_J,VK_K,VK_L,VK_M,VK_N,VK_O,VK_P,VK_Q,VK_R,VK_S,VK_T,VK_U,VK_V,VK_W,VK_X,VK_Y,VK_Z }; 
			 
		static Broadcaster& getInstance() {
			static Broadcaster instance;
			return instance;
		}
	
		void start() {
			m_keyboardEnabled = true;
			printf("broadcast ON\n");
		}

		void stop() {
			printf("broadcast OFF\n");
			m_keyboardEnabled = false;
		}

		bool isKeyboardEnabled() { 
			return m_keyboardEnabled;
		}

		void terminate() {
			PostMessage(m_pWin->hwnd,WM_CLOSE,0,0);
			WaitForSingleObject(m_hThread,5000);
		}

		void windowsReadyToZoom(bool ok) {
			m_windowsReadyToZoom = ok;
		}
				
	private:
		void operator=(const Broadcaster&) = delete;
		Broadcaster(const Broadcaster&) = delete;

		Broadcaster();
		~Broadcaster();

		enum { MMB_DN, MMB_UP, MB4_DN, MB4_UP, NUM_MOUSESTATES };
		int m_ctrlDown[NUM_MOUSESTATES]; 

		typedef std::unordered_map<unsigned short int,int> BROADCAST_FILTER;
		BROADCAST_FILTER m_filter;

		bool pressed(unsigned int vk) {
			return vk<_countof(m_keyState) ? m_keyState[vk] == 1 : false;
		}
		bool released(unsigned int vk) {
			return vk<_countof(m_keyState) ? m_keyState[vk] == 0 : false;
		}
		bool repeated(unsigned int vk) {
			return vk<_countof(m_keyState) ? m_keyState[vk] == 0x81 : false;
		}
		bool down(unsigned int vk) {
			return vk<_countof(m_keyState) ? m_keyState[vk] != 0 : false;
		}
		bool rawKeyDown(unsigned short int flags) {
			return (flags&1) == 0;
		}
		bool rawKeyUp(unsigned short int flags) {
			return (flags&1) == 1;
		}
	
		unsigned char m_keyState[256];
		bool m_echo;
		bool m_windowsReadyToZoom;
		HANDLE m_hThread;
		bool m_keyboardEnabled;
		Win* m_pWin;

		void zoomSlave(bool zoom);
		void initFilter();
		static DWORD WINAPI inputThread(LPVOID param);
		static int broadcast(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
		
#endif

