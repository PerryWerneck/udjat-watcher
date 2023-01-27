/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2023 Perry Werneck <perry.werneck@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <config.h>
 #include <udjat/defs.h>
 #include <udjat/agent/state.h>
 #include <udjat/agent/level.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/intl.h>
 #include <iostream>
 #include <indicator.h>
 #include <shellapi.h>
 #include <iconv.h>
 #include <mutex>
 #include <string>
 #include "resources.h"

 using namespace std;
 using namespace Udjat;

 #define WM_USER_START		WM_USER+1000
 #define WM_USER_SHELLICON	WM_USER+1001
 #define WM_USER_SHOW		WM_USER+1002
 #define WM_USER_HIDE		WM_USER+1003

 namespace Watcher {

	namespace Win32 {

		class Indicator : public Watcher::Indicator {
		private:

			iconv_t		local;
			HICON		icons[ID_STATE_COUNT];
			HWND 		hwnd;

			/// @brief Contains information that the system needs to display notifications in the notification area.
			NOTIFYICONDATA nidApp;

			/// @brief Is the icon visible?
			bool visible = false;

			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

			void convert(const char *src, CHAR *dst, size_t sz);

		public:
			Indicator();
			~Indicator();
			void show() override;
			void hide() override;
			void notify(const char *title, Udjat::Level level, const char *summary, const char *body) override;

		};


	}

	Indicator & Indicator::getInstance() {
		static Win32::Indicator instance;
		return instance;
	}

	Win32::Indicator::Indicator() {

		local = iconv_open("CP1252","UTF-8");

		memset(&nidApp,0,sizeof(nidApp));
		nidApp.cbSize = sizeof(NOTIFYICONDATA); 	// sizeof the struct in bytes
		nidApp.uID = 1;								// ID of the icon that willl appear in the system tray

		WNDCLASSEX wc;
		memset(&wc,0,sizeof(wc));

		wc.cbSize 			= sizeof(wc);
		wc.style  			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc  	= hwndProc;
		wc.hInstance  		= GetModuleHandle(NULL);
		wc.lpszClassName  	= PACKAGE_NAME;
		// wc.hIcon			= icons.def;
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.cbWndExtra		= sizeof(LONG_PTR);

		static ATOM winClass = RegisterClassEx(&wc);

		if(!winClass) {
			throw runtime_error("Can't register main window class");
		}

		hwnd = CreateWindow(
					PACKAGE_NAME,
					"",
					WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT,
					0,
					CW_USEDEFAULT,
					0,
					NULL,
					NULL,
					GetModuleHandle(NULL),
					NULL
		);

		if(hwnd == NULL) {
			throw runtime_error("Can't create control window");
		}

		SetWindowLongPtr(hwnd, 0, (LONG_PTR) this);

		// Load icons
		for(size_t ix = 0; ix < (sizeof(icons)/sizeof(icons[0])); ix++) {
			icons[ix] = LoadIcon(GetModuleHandle(NULL),(LPCTSTR) MAKEINTRESOURCE((IDI_STATE_BEGIN+ix)));
		}

		PostMessage(hwnd,WM_USER_START,0,0);

	}

	Win32::Indicator::~Indicator() {
		DestroyWindow(hwnd);
		iconv_close(local);
	}

	void Win32::Indicator::convert(const char *src, CHAR *dst, size_t sz) {

		iconv(local,NULL,NULL,NULL,NULL);	// Reset state

		size_t	  	  szIn		= strlen(src);
		size_t	  	  szOut		= sz;
		char		* outBuff	= (char *) dst;

#if defined(WINICONV_CONST)
		WINICONV_CONST char 	* inBuf 	= (WINICONV_CONST char *) src;
#elif defined(ICONV_CONST)
		ICONV_CONST 			* inBuf 	= (ICONV_CONST *) src;
#else
		char 		 			* inBuf 	= (char *) src;
#endif // WINICONV_CONST

		// Limpa buffer de saída.
		memset(dst,0,sz);

		if(iconv(local,&inBuf,&szIn,&outBuff,&szOut) == ((size_t) -1)) {
			cerr << "Unable to convert charset of \"" << src << "\"" << endl;
		}
	}

	void Win32::Indicator::show() {
		PostMessage(hwnd,WM_USER_SHOW,0,0);
	}

	void Win32::Indicator::hide() {
		PostMessage(hwnd,WM_USER_HIDE,0,0);
	}

	void Win32::Indicator::notify(const char *title, Udjat::Level level, const char *summary, const char *body) {

		cout << "NOTIFY: " << title << " - " << body << endl;

	}

	LRESULT Win32::Indicator::hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

		Indicator & indicator = *((Indicator *) GetWindowLongPtr(hWnd,0));

		switch(uMsg) {
		case WM_USER_START:

			indicator.convert(_("Initializing watcher"),indicator.nidApp.szTip,sizeof(indicator.nidApp.szTip));

			indicator.visible = true;
			indicator.nidApp.hWnd = hWnd;	// handle of the window which will process this app. messages

			indicator.nidApp.hIcon = indicator.icons[ID_STATE_UNDEFINED];
			indicator.nidApp.hBalloonIcon = indicator.icons[ID_STATE_UNDEFINED];
			indicator.nidApp.uFlags = NIF_TIP|NIF_MESSAGE|NIF_ICON|NIIF_USER|NIIF_LARGE_ICON;

			Shell_NotifyIcon(NIM_ADD, &indicator.nidApp);

			break;

		case WM_DESTROY:
			SendMessage(hWnd,WM_USER_HIDE,0,0);
			break;

		case WM_USER_SHOW:
			break;

		case WM_USER_HIDE:
			break;

		case WM_USER_SHELLICON:
			// systray msg callback
			debug("Systray message callback");
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}

 }
