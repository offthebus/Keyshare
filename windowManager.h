#ifndef _H_WINDOWMANAGER
#define _H_WINDOWMANAGER

class WindowManager
{
	public:
		static WindowManager& getInstance() {
			static WindowManager instance;
			return instance;
		}

		void layout();
		void zoomSlave(bool zoom);

	private:
		void operator=(const WindowManager&) = delete;
		WindowManager(const WindowManager&) = delete;
		 
		WindowManager() {}
};

#endif
