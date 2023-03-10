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
 #include <udjat/tools/configuration.h>
 #include <udjat/win32/charset.h>
 #include <udjat/win32/exception.h>
 #include <shlwapi.h>
 #include "resources.h"

 #include <commctrl.h>

 using namespace std;
 using namespace Udjat;

 #define WM_USER_START		WM_USER+1000
 #define WM_USER_SHELLICON	WM_USER+1001
 #define WM_USER_SHOW		WM_USER+1002
 #define WM_USER_HIDE		WM_USER+1003
 #define WM_USER_NOTIFY		WM_USER+1004
 #define WM_USER_UPDATED	WM_USER+1005

 namespace Watcher {

	namespace Win32 {

		class Indicator : public Watcher::Indicator {
		private:

			struct {
				HICON small;	///< @brief Corresponds to SM_CXSMICON, the recommended pixel width of a small icon.
				HICON large;	///< @brief Corresponds toSM_CXICON, the default pixel width of an icon.
			} icons[ID_STATE_COUNT];

			HWND 		hwnd;
			mutex		guard;

			/// @brief Contains information that the system needs to display notifications in the notification area.
			NOTIFYICONDATA nidApp;

			/// @brief Is the icon visible?
			bool visible = false;

			static LRESULT WINAPI hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

		memset(&nidApp,0,sizeof(nidApp));
		nidApp.cbSize = sizeof(NOTIFYICONDATA); 	// sizeof the struct in bytes
		nidApp.uID = 1;								// ID of the icon that will appear in the system tray
		nidApp.uVersion = NOTIFYICON_VERSION_4;
		nidApp.dwState = NIS_SHAREDICON;
		nidApp.dwStateMask = NIS_SHAREDICON;
		nidApp.dwInfoFlags = NIIF_USER|NIIF_LARGE_ICON;

		// https://github.com/OpenNMS-Archives/opennms-windows/blob/master/OpenNMS_Tray/OpenNMS_Tray.cpp
		{
			HMODULE hMod = LoadLibrary("shell32.dll");
			if(hMod) {
				DLLGETVERSIONPROC fnDllGetVersion = (DLLGETVERSIONPROC) (void *) GetProcAddress(hMod, "DllGetVersion");

				if(fnDllGetVersion) {
					DLLVERSIONINFO VerInfo;
					memset(&VerInfo,0,sizeof(VerInfo));
					VerInfo.cbSize = sizeof(VerInfo);
					fnDllGetVersion(&VerInfo);

					Logger::String{"Using shell 32 verion ",((int)VerInfo.dwMajorVersion)," build ",((int)VerInfo.dwBuildNumber)}.trace("indicator");

					if(VerInfo.dwMajorVersion >= 6) {
						if(VerInfo.dwBuildNumber > 6) {
							nidApp.cbSize = sizeof(NOTIFYICONDATA);
						} else {
							nidApp.cbSize = NOTIFYICONDATA_V3_SIZE;
						}

						nidApp.uVersion = NOTIFYICON_VERSION_4;

						Logger::String{"Using notify icon version ",((int) nidApp.uVersion) }.trace("indicator");

					} else if(VerInfo.dwMajorVersion == 5) {

						nidApp.cbSize = NOTIFYICONDATA_V2_SIZE;
						nidApp.uVersion = NOTIFYICON_VERSION;

						Logger::String{"Using notify icon version ",((int) nidApp.uVersion) }.trace("indicator");

					} else {

						nidApp.cbSize = NOTIFYICONDATA_V1_SIZE;
						Logger::String{"Using notify icon version 1"}.trace("indicator");

					}

				}
			}

		}

		// Load icons
		for(size_t ix = 0; ix < (sizeof(icons)/sizeof(icons[0])); ix++) {

			icons[ix].small =
				(HICON) LoadImage(
							GetModuleHandle(NULL),
							MAKEINTRESOURCE(IDI_STATE_BEGIN+ix),
							IMAGE_ICON,
							0,0,
							LR_DEFAULTSIZE
						);

			if(!icons[ix].small) {
				throw Udjat::Win32::Exception(string{"Cant load small icon "} + std::to_string(ix));
			}

			icons[ix].large =
				(HICON) LoadImage(
							GetModuleHandle(NULL),
							MAKEINTRESOURCE(IDI_STATE_BEGIN+ix),
							IMAGE_ICON,
							256,256,
							LR_DEFAULTSIZE
						);

			if(!icons[ix].large) {
				throw Udjat::Win32::Exception(string{"Cant load large icon "} + std::to_string(ix));
			}

			/*
			// https://learn.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-loadiconmetric
			LoadIconMetric(
				GetModuleHandle(NULL),
				(PCWSTR) MAKEINTRESOURCE(IDI_STATE_BEGIN+ix),
				LIM_SMALL,
				&(icons[ix].small)
			);

			LoadIconMetric(
				GetModuleHandle(NULL),
				(PCWSTR) MAKEINTRESOURCE(IDI_STATE_BEGIN+ix),
				LIM_LARGE,
				&(icons[ix].large)
			);
			*/

		}

		WNDCLASSEX wc;
		memset(&wc,0,sizeof(wc));

		wc.cbSize 			= sizeof(wc);
		wc.style  			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc  	= hwndProc;
		wc.hInstance  		= GetModuleHandle(NULL);
		wc.lpszClassName  	= PACKAGE_NAME;
		wc.hIcon			= icons[ID_STATE_UNDEFINED].small;
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

		PostMessage(hwnd,WM_USER_START,0,0);

	}

	Win32::Indicator::~Indicator() {

		if(visible) {
			debug("Removing indicator");
			if(Shell_NotifyIcon(NIM_DELETE, &nidApp)) {
				visible = false;
			}
		}

		for(size_t ix = 0; ix < (sizeof(icons)/sizeof(icons[0])); ix++) {
			DestroyIcon(icons[ix].large);
			DestroyIcon(icons[ix].small);
		}
		DestroyWindow(hwnd);
	}

	void Win32::Indicator::show() {
		PostMessage(hwnd,WM_USER_SHOW,0,0);
	}

	void Win32::Indicator::hide() {
		PostMessage(hwnd,WM_USER_HIDE,0,0);
	}

	void Win32::Indicator::notify(const char *title, Udjat::Level level, const char *summary, const char *body) {

		nidApp.hIcon = icons[level].small;
		nidApp.hBalloonIcon = icons[level].large;
		nidApp.uFlags |= (NIF_TIP|NIF_ICON|NIIF_LARGE_ICON);
		nidApp.dwInfoFlags = NIIF_USER;

		if(title && *title) {
			strncpy(
				nidApp.szInfoTitle,
				Udjat::Win32::Charset::to_windows(title).c_str(),
				sizeof(nidApp.szInfoTitle)
			);
		}

		if(summary && *summary) {
			nidApp.uFlags |= NIF_TIP;
			strncpy(
				nidApp.szTip,
				Udjat::Win32::Charset::to_windows(summary).c_str(),
				sizeof(nidApp.szTip)
			);
		}

		if(body && *body) {
			strncpy(
				nidApp.szInfo,
				Udjat::Win32::Charset::to_windows(body).c_str(),
				sizeof(nidApp.szInfo)
			);
		}

		if(level > Level::ready) {
			PostMessage(hwnd,WM_USER_NOTIFY,0,0);
		} else {
			PostMessage(hwnd,WM_USER_UPDATED,0,0);
		}

	}

	LRESULT Win32::Indicator::hwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

		Indicator & indicator = *((Indicator *) GetWindowLongPtr(hWnd,0));

		switch(uMsg) {
		case WM_USER_START:

			strncpy(
				indicator.nidApp.szTip,
				Udjat::Win32::Charset::to_windows(_("Initializing watcher")).c_str(),
				sizeof(indicator.nidApp.szTip)
			);

			indicator.visible = true;
			indicator.nidApp.hWnd = hWnd;	// handle of the window which will process this app. messages
			indicator.nidApp.uCallbackMessage = WM_USER_SHELLICON;

			indicator.nidApp.hIcon = indicator.icons[ID_STATE_UNDEFINED].large;
			indicator.nidApp.hBalloonIcon = indicator.icons[ID_STATE_UNDEFINED].large;
			indicator.nidApp.uFlags = NIF_TIP|NIF_MESSAGE;
			SendMessage(hWnd,WM_USER_SHOW,0,0);
			break;

		case WM_DESTROY:
			SendMessage(hWnd,WM_USER_HIDE,0,0);
			break;

		case WM_USER_SHOW:
			debug("WM_USER_SHOW");
			indicator.nidApp.uFlags |= NIF_TIP|NIF_MESSAGE|NIF_ICON;
			indicator.nidApp.dwInfoFlags = NIIF_USER|NIIF_LARGE_ICON;

			if(Shell_NotifyIcon(NIM_ADD, &indicator.nidApp)) {
				indicator.visible = true;
				indicator.nidApp.uFlags = 0;
			}
			indicator.nidApp.uVersion = NOTIFYICON_VERSION_4;
			Shell_NotifyIcon(NIM_SETVERSION, &indicator.nidApp);
			break;

		case WM_USER_UPDATED:
			debug("WM_USER_UPDATED");
			indicator.nidApp.uFlags |= NIF_TIP|NIF_MESSAGE|NIF_ICON;
			indicator.nidApp.dwInfoFlags = NIIF_USER|NIIF_LARGE_ICON;
			if(indicator.visible && Shell_NotifyIcon(NIM_MODIFY, &indicator.nidApp)) {
				indicator.nidApp.uFlags = 0;
			}
			break;

		case WM_USER_HIDE:
			debug("Removendo icone");
			if(Shell_NotifyIcon(NIM_DELETE, &indicator.nidApp)) {
				indicator.visible = false;
				indicator.nidApp.uFlags = 0;
			}
			break;

		case WM_USER_NOTIFY:
			indicator.nidApp.uFlags |= NIF_INFO|NIF_TIP|NIF_MESSAGE;
			indicator.nidApp.uTimeout = Config::Value<unsigned int>{"appindicator","timeout",5000}.get();
			if(indicator.visible) {
				SendMessage(hWnd,WM_USER_UPDATED,0,0);
			} else {
				SendMessage(hWnd,WM_USER_SHOW,0,0);
			}
			break;

		case WM_USER_SHELLICON:
			// systray msg callback
			debug("Systray message callback");
			if(LOWORD(lParam) == WM_LBUTTONDOWN) {
				PostMessage(hWnd,WM_USER_NOTIFY,0,0);
			}
			break;

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}

 }
