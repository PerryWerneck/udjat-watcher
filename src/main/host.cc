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
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/logger.h>
 #include <host.h>
 #include <stdexcept>
 #include <json/reader.h>
 #include <json/value.h>

 using namespace Udjat;
 using namespace std;

 namespace Watcher {

	Host::Host(const pugi::xml_node &node) : Abstract::Agent{node}, url{getAttribute(node,"url","")} {

		if(url.empty()) {
			throw runtime_error("Required attribute 'url' is missing or empty");
		}

		if(!timer()) {
			timer(300);
		}

	}

	Host::~Host() {
	}

	bool Host::refresh() {

		debug("Checking '",url,"'");

		Json::Value response;

		{
			string errors;
			String text = Protocol::WorkerFactory(url.c_str())->get();

			Json::CharReaderBuilder builder;
			Json::CharReader * reader = builder.newCharReader();

			bool success = reader->parse(text.c_str(), text.c_str() + text.size(), &response, &errors);
			delete reader;

			if(!success) {
				error() << errors << endl << text << endl;
				throw runtime_error(_("Error parsing server state"));
			}

		}

		/*
		if(Logger::enabled(Logger::Trace)) {
			Logger::String{string{"Response:\n"} + response.toStyledString()}.write(Logger::Trace,name());
		} else {
			cout << "Trace is disabled?" << endl;
		}
		*/

		if(!response.isObject()) {

			Logger::String{string{"Response is not an object:\n"} + response.toStyledString()}.write(Logger::Error,name());

		} else if(response["global"].isObject()) {

			// It's a legacy server.
			auto value = response["global"];
			if(Logger::enabled(Logger::Trace)) {
				Logger::String{string{"Legacy response:\n"} + value.toStyledString()}.write(Logger::Trace,name());
			}


		} else {

			Logger::String{string{"Unable to identify response format:\n"} + response.toStyledString()}.write(Logger::Error,name());

		}



//		cout << json << endl;


		return false;
	}

 }
