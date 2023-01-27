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
 #include <iostream>
 #include <indicator.h>

 using namespace std;

 namespace Watcher {

	Indicator & Indicator::getInstance() {

		class LinuxIndicator : public Watcher::Indicator {
		public:
			LinuxIndicator() {
			}

			void notify(const char *title, Udjat::Level level, const char *summary, const char *body) override {
				cout << "NOTIFY: " << title << " - " << body << endl;
			}

		};

		static LinuxIndicator instance;
		return instance;

	}

 }
