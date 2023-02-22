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
 #include <iostream>
 #include <udjat/agent/abstract.h>
 #include <udjat/tools/logger.h>
 #include <udjat/tools/application.h>
 #include <udjat/tools/threadpool.h>
 #include <udjat/factory.h>
 #include <host.h>
 #include <udjat/module.h>
 #include <udjat/moduleinfo.h>
 #include <udjat/tools/logger.h>
 #include <indicator.h>
 #include <udjat/tools/intl.h>

 using namespace std;
 using namespace Udjat;

//---[ Implement ]------------------------------------------------------------------------------------------

int main(int argc, char **argv) {

	static const Udjat::ModuleInfo moduleinfo{"Remote host monitor"};

	class Application : public Udjat::Application, private Udjat::Factory {
	protected:

		void root(std::shared_ptr<Abstract::Agent> agent) override {

			debug("---------------> Setting root agent to <------------------------------",agent->name());

			/// @brief Listen for root agent states.
			class Listener : public Activatable {
			public:
				constexpr Listener() : Activatable("notifier") {
				}

				bool activated() const noexcept override {
					debug("Listener was activated");
					auto agent = Abstract::Agent::root();
					if(agent) {
						auto state = agent->state();
						Watcher::Indicator::getInstance().show(state);
					}
					return false;
				}

				void activate(const std::function<bool(const char *key, std::string &value)> &) override {
				}

			};

			agent->push_back(
				(Abstract::Agent::Event) (Abstract::Agent::STARTED|Abstract::Agent::LEVEL_CHANGED),
				std::make_shared<Listener>()
			);

		}

		std::shared_ptr<Abstract::Agent> AgentFactory(const Abstract::Object UDJAT_UNUSED(&parent), const pugi::xml_node &node) const override {
			return make_shared<Watcher::Host>(node);
		}

#ifdef _WIN32
		int uninstall() override {
			try {

				Application::ShortCut shortcut;
				shortcut.remove();

			} catch(const std::exception &e) {
				cerr << "Shortcut failed: " << e.what() << endl;
				return -1;
			}

			return Udjat::Application::uninstall();

		}

		int install(const char *name) override {

			try {

				Application::ShortCut shortcut;
				shortcut.save();
				shortcut.autostart();

			} catch(const std::exception &e) {
				cerr << "Shortcut failed: " << e.what() << endl;
				return -1;
			}

			return Udjat::Application::install(name);
		}
#endif // _WIN32

	public:
		Application() : Udjat::Factory{"server",moduleinfo} {
		}

		int run(const char *definitions) override {

			Watcher::Indicator::getInstance().show();
			int rc = Udjat::Application::run(definitions);
			Watcher::Indicator::getInstance().hide();

			return rc;
		}

	};

	Logger::redirect();

#ifdef DEBUG
	Logger::verbosity(9);
	return Application{}.Udjat::Application::run(argc,argv,"./debug.xml");
#else
	return Application{}.Udjat::Application::run(argc,argv);
#endif // DEBUG


}

