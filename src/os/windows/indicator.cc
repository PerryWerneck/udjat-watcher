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
 #include <iostream>
 #include <indicator.h>
 #include <shellapi.h>
 #include <iconv.h>
 #include <mutex>
 #include <string>
 #include "resources.h"

 #ifndef WM_USER_SHELLICON
	#define WM_USER_SHELLICON WM_USER+1000
 #endif // WM_USER_SHELLICON

 using namespace std;
 using namespace Udjat;

 namespace Watcher {

	std::shared_ptr<Indicator> Indicator::getInstance() {

		class Indicator : public Watcher::Indicator {
		private:

			iconv_t		local;
			mutex		mtx;

			HICON icons[ID_STATE_COUNT];

			HWND hwnd;

			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

				if(uMsg == WM_USER_SHELLICON) {
					// systray msg callback
					debug("Systray message callback");
					return 0;
				}

				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}

			struct {

				/// @brief Contains information that the system needs to display notifications in the notification area.
				NOTIFYICONDATA nid;

				/// @brief Is the icon visible?
				bool visible = false;

			} shellicon;

			/// @brief Converte string UTF-8 para o formato windows
			///
			/// @param src	String UTF-8 a converter.
			/// @param dst	Buffer para a string convertida.
			/// @param sz	Tamanho do buffer de saída.
			///
			void convert(const char *src, CHAR *dst, size_t sz) {

				lock_guard<mutex> lock(mtx);

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
					clog << "Erro ao converter string \"" << src << "\"" << endl;
				}

			}

		public:
			Indicator() {

				local = iconv_open("CP1252","UTF-8");

				WNDCLASSEX wc;
				memset(&wc,0,sizeof(wc));

				wc.cbSize 			= sizeof(wc);
				wc.style  			= CS_HREDRAW | CS_VREDRAW;
				wc.lpfnWndProc  	= hwndProc;
				wc.hInstance  		= GetModuleHandle(NULL);
				wc.lpszClassName  	= PACKAGE_NAME;
				// wc.hIcon			= icons.def;
				wc.hCursor			= LoadCursor(NULL, IDC_ARROW);

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

				// Load icons
				for(size_t ix = 0; ix < (sizeof(icons)/sizeof(icons[0])); ix++) {
					icons[ix] = LoadIcon(GetModuleHandle(NULL),(LPCTSTR) MAKEINTRESOURCE((IDI_STATE_BEGIN+ix)));
				}

				// Create Shell Icon
				memset(&shellicon.nid,0,sizeof(shellicon.nid));
				shellicon.nid.cbSize = sizeof(NOTIFYICONDATA); 				// sizeof the struct in bytes

				shellicon.nid.hBalloonIcon = icons[ID_STATE_UNDEFINED];
				shellicon.nid.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;

				shellicon.nid.hIcon = icons[ID_STATE_UNDEFINED];
				shellicon.nid.uFlags |= NIF_ICON;

			}

			~Indicator() {
				hide();
				{
					lock_guard<mutex> lock(mtx);
					iconv_close(local);
				}
			}

			void sync() {
				debug("Syncing notifier");
				lock_guard<mutex> lock(mtx);
				if(shellicon.visible && shellicon.nid.uFlags) {
					if(Shell_NotifyIcon(NIM_MODIFY, &shellicon.nid)) {
						shellicon.nid.uFlags = 0;
					}
				}
			}

			void show() override {
				lock_guard<mutex> lock(mtx);
				debug("Enabling icon");
				if(!shellicon.visible) {
					shellicon.nid.uFlags = NIF_TIP|NIF_MESSAGE;
					shellicon.nid.uCallbackMessage = WM_USER_SHELLICON;

					if(shellicon.nid.hIcon)
						shellicon.nid.uFlags |= NIF_ICON;

					shellicon.nid.uID = 1;
					shellicon.nid.hWnd = hwnd;

					Shell_NotifyIcon(NIM_ADD, &shellicon.nid);
					shellicon.visible = true;
				}
			}

			void hide() override {
				lock_guard<mutex> lock(mtx);
				debug("Disabling icon");
				if(!shellicon.visible) {
					Shell_NotifyIcon(NIM_DELETE, &shellicon.nid);
					shellicon.visible = false;
				}
			}

			void notify(const char *title, Udjat::Level level, const char *summary, const char *body) override {

				cout << "NOTIFY: " << title << " - " << body << endl;

				// https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataa

				shellicon.nid.hIcon = icons[level];
				shellicon.nid.dwInfoFlags = NIF_ICON; // The hIcon member is valid.

				if(!shellicon.visible) {
					show();
				}
				sync();

				/*
				shellicon.nid.hBalloonIcon = icons[level];

				if(title && *title) {
					convert(title,shellicon.nid.szInfoTitle,sizeof(shellicon.nid.szInfoTitle));
				}

				if(body && *body) {
					convert(body,shellicon.nid.szInfo,sizeof(shellicon.nid.szInfo));
					convert(summary,shellicon.nid.szTip,sizeof(shellicon.nid.szTip));
				}
				*/

				//shellicon.nid.uTimeout = 10;


				// NIIF_USER | NIIF_LARGE_ICON | NIF_ICON | NIF_TIP | NIF_INFO;


			}

		};

		static std::shared_ptr<Indicator> instance;
		if(!instance) {
			instance = make_shared<Indicator>();
		}

		return instance;

	}

 }
