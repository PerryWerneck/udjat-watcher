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
 #include <windows.h>
 #include <udjat/defs.h>
 #include <udjat/agent/state.h>
 #include <udjat/agent/level.h>
 #include <iostream>
 #include <indicator.h>
 #include <shellapi.h>
 #include <iconv.h>
 #include <mutex>
 #include <string>
 #include "resources.h"

 using namespace std;

 namespace Watcher {

	std::shared_ptr<Indicator> Indicator::getInstance() {

		class Indicator : public Watcher::Indicator {
		private:

			iconv_t		local;
			mutex		mtx;

			std::string summary;
			std::string title;
			std::string body;

			HICON icons[ID_STATE_COUNT];

			HWND hwnd;

			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

				shellicon.nid.uID = 1;
				shellicon.nid.hWnd = hwnd;

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
				lock_guard<mutex> lock(mtx);
				if(this->visible && shellicon.nid.uFlags) {
					if(Shell_NotifyIcon(NIM_MODIFY, &shellicon.nid)) {
						shellicon.nid.uFlags = 0;
					}
				}
			}

			void show() override {
				lock_guard<mutex> lock(mtx);
				if(!shellicon.visible) {
					shellicon.nid.uFlags = NIF_TIP|NIF_MESSAGE;
					if(shellicon.nid.hIcon)
						shellicon.nid.uFlags |= NIF_ICON;
					Shell_NotifyIcon(NIM_ADD, &shellicon.nid);
					shellicon.visible = true;
				}
			}

			void hide() override {
				lock_guard<mutex> lock(mtx);
				if(!shellicon.visible) {
					Shell_NotifyIcon(NIM_DELETE, &shellicon.nid);
					shellicon.visible = false;
				}
			}

			void notify(const char *title, Udjat::Level level, const char *summary, const char *body) override {

				this->title = title;
				this->summary = summary;
				this->body = body;

				cout << "NOTIFY: " << title << " - " << body << endl;
			}

		};

		static std::shared_ptr<Indicator> instance;
		if(!instance) {
			instance = make_shared<Indicator>();
		}

		return instance;

	}

 }
