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
 #include <udjat/tools/object.h>
 #include <udjat/tools/protocol.h>
 #include <host.h>
 #include <stdexcept>

 using namespace Udjat;
 using namespace std;

 namespace Watcher {

	Host::Host(const pugi::xml_node &node) : Abstract::Agent{node}, url{getAttribute(node,"url","")} {

		if(url.empty()) {
			throw runtime_error("Required attribute 'url' is missing or empty");
		}

	}

	Host::~Host() {
	}

	bool Host::refresh() {

		String json = Protocol::WorkerFactory(url.c_str())->get();

		cout << json << endl;

	}

 }
