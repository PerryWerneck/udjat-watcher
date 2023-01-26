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
 #include <udjat/agent/state.h>
 #include <udjat/tools/intl.h>
 #include <udjat/tools/logger.h>
 #include <host.h>
 #include <stdexcept>
 #include <json/reader.h>
 #include <json/value.h>
 #include <udjat/agent/level.h>

 using namespace Udjat;
 using namespace std;

 namespace Watcher {

	class State : public Udjat::Abstract::State {
	private:
		std::string name;	///< @brief State name.
		std::string label;	///< @brief State label.
		std::string body;

	public:
		State(const char *n, const char *level, const char *l, const char *b) : Abstract::State{"unnamed"}, name{n}, label{l}, body{b} {
			rename(name.c_str());

			Object::properties.summary = label.c_str();
			properties.level = Udjat::LevelFactory(level);
			properties.body = body.c_str();

			debug("Level(",level,") = ", properties.level, " ", std::to_string(properties.level));
			debug("body=",properties.body);

		}
	};

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

	std::shared_ptr<Abstract::State> Host::computeState() {
		if(this->state) {
			return this->state;
		}
		return Abstract::Agent::computeState();
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

			// {
			// 	"className" : "critical",
			// 	"href" : "",
			// 	"msg" : "Uso da parti\u00e7\u00e3o de sistema acima de 80%",
			// 	"name" : "sys80",
			// 	"state" : 4,
			// 	"summary" : "/ acima 80%"
			// }

			this->state = make_shared<Watcher::State>(
				value["name"].asCString(),
				value["className"].asCString(),
				value["summary"].asCString(),
				value["msg"].asCString()
			);

			return set(this->state);

		} else {

			Logger::String{string{"Unable to identify response format:\n"} + response.toStyledString()}.write(Logger::Error,name());

		}



//		cout << json << endl;


		return false;
	}

 }
