#include "stdafx.h"
#include "windowManager.h"
#include "scanner.h"
#include "config.h"

//---------------------------------------------------------------------------
// WINDOWMANAGER::LAYOUT
//---------------------------------------------------------------------------
void WindowManager::layout()
{
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
		SetWindowPos(scanner.master()->hwnd,HWND_TOP,x,y,w,h,SWP_SHOWWINDOW);
		
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
	}

	scanner.unlock();
}
//---------------------------------------------------------------------------
// WINDOWMANAGER::ZOOMSLAVE
//---------------------------------------------------------------------------
void WindowManager::zoomSlave(bool zoom)
{
	POINT p;
	GetCursorPos(&p);

	Scanner& scanner = scanner.getInstance();
	Scanner::WINDOWLIST slaves;

	scanner.lock();
	scanner.getSlaves(slaves);
	HWND master = scanner.master()->hwnd;

	if (zoom) {
		// zoom window under cursor
		for (size_t i=0; i<slaves.size(); ++i) {
			RECT r;
			GetWindowRect(slaves[i]->hwnd, &r);
			bool mouseOver = ( p.x >= r.left && p.x < r.right && p.y >= r.top && p.y < r.bottom);
			if ( mouseOver ) {
				if (!slaves[i]->zoomed) {
					// zoom - this is just for the layout with slaves down RH side of screen
					util::Screen screen;
					RECT& pos = slaves[i]->pos;
					int w = screen.w*4/5;
					int h = screen.h*4/5;
					int x = screen.centre.x - w/2;
					int y = screen.centre.y - h/2;
					SetWindowPos(slaves[i]->hwnd,HWND_TOPMOST,x,y,w,h,NULL);
					slaves[i]->zoomed = true;
					break;
				}
			}
		}
	} else {
		// unzoom 
		for (size_t i=0; i<slaves.size(); ++i) {
			if ( slaves[i]->zoomed) {
				RECT& pos = slaves[i]->pos;
				SetWindowPos(slaves[i]->hwnd,NULL,pos.left,pos.top,pos.right-pos.left,pos.bottom-pos.top,SWP_NOZORDER);
				slaves[i]->zoomed = false;
				break;
			}
		}
	}
	scanner.unlock();
}

