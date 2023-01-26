/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2021 Perry Werneck <perry.werneck@gmail.com>
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
 #include <iostream>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/systemservice.h>
 #include <udjat/tools/threadpool.h>
 #include <udjat/factory.h>
 #include <host.h>
 #include <udjat/module.h>
 #include <udjat/moduleinfo.h>

 using namespace std;
 using namespace Udjat;

 #ifdef DEBUG
	#define SETTINGS_FILE "./devel.xml"
 #else
	#define SETTINGS_FILE Quark(Application::DataFile{"watcher.xml"}).c_str()
 #endif // DEBUG

//---[ Implement ]------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	static const Udjat::ModuleInfo moduleinfo{"Remote host monitor"};

	class Service : public Udjat::SystemService, private Udjat::Factory {
	protected:

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const override {
			return make_shared<Watcher::Host>(node);
		}

	public:
		Service() : Udjat::SystemService{SETTINGS_FILE}, Udjat::Factory("remote-host",moduleinfo) {
		}

		virtual ~Service() {
			Module::unload();
		}

		void init() override {
			SystemService::init();
			Application::info() << "Running build " << STRINGIZE_VALUE_OF(BUILD_DATE) << endl;
		}

		void deinit() override {
			Application::info() << "Deinitializing" << endl;
			ThreadPool::getInstance().wait(); // Just in case.
			SystemService::deinit();
			Module::unload();
		}


	} ;

	return Service().run(argc,argv);

}

