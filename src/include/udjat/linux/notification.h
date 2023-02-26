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

 #pragma once
 #include <udjat/defs.h>
 #include <string>

 namespace Udjat {

 	using Notification = Linux::Notification;

	namespace Linux {

		class UDJAT_API Notification {
		public:

			enum Category : uint8_t {
				no_category,			///< @brief No category.
				device,					///< @brief A generic device-related notification that doesn't fit into any other category.
				device_added,			///< @brief A device, such as a USB device, was added to the system.
				device_error,			///< @brief A device had some kind of error.
				device_removed,			///< @brief A device, such as a USB device, was removed from the system.
				email,					///< @brief A generic e-mail-related notification that doesn't fit into any other category.
				email_arrived,			///< @brief A new e-mail notification.
				email_bounced,			///< @brief A notification stating that an e-mail has bounced.
				im,						///< @brief A generic instant message-related notification that doesn't fit into any other category.
				im_error,				///< @brief An instant message error notification.
				im_received,			///< @brief A received instant message notification.
				network,				///< @brief A generic network notification that doesn't fit into any other category.
				network_connected,		///< @brief A network connection notification, such as successful sign-on to a network service. This should not be confused with device.added for new network devices.
				network_disconnected,	///< @brief A network disconnected notification. This should not be confused with device.removed for disconnected network devices.
				network_error,			///< @brief A network-related or connection-related error.
				presence,				///< @brief A generic presence change notification that doesn't fit into any other category, such as going away or idle.
				presence_offline,		///< @brief An offline presence change notification.
				presence_online,		///< @brief An online presence change notification.
				transfer,				///< @brief A generic file transfer or download notification that doesn't fit into any other category.
				transfer_complete,		///< @brief A file transfer or download complete notification.
				transfer_error			///< @brief A file transfer or download error.
			};

			enum Urgency : uint8_t {
				Low = 0,
				Normal = 1,
				Critical = 2
			};

		protected:

			struct {
				Category category = no_category;
				Urgency urgency = Low.
				std::string summary;
				std::string body;
			} args;

		public:

			Notification();
			~Notification();

			inline Notification & category(Category c) noexcept {
				args.category = c;
				return *this;
			}

			inline Category category() const noexcept {
				return args.category;
			}

			inline Notification & urgency(Urgency u) noexcept {
				args.urgency = u;
				return *this;
			}

			inline Urgency urgency() const noexcept {
				return args.urgency;
			}

			inline Notification & summary(const char *s) noexcept {
				args.summary = s;
				return *this;
			}

			inline const char * summary() const noexcept {
				return args.summary.c_str();
			}

			inline Notification & body(const char *s) noexcept {
				args.body = s;
				return *this;
			}

			inline const char * body() const noexcept {
				return args.body.c_str();
			}


		};

	}

 }

 namespace std {

	inline Udjat::Linux::Notification & operator<<(Udjat::Linux::Notification &out, const Udjat::Linux::Category c) {
		return out.category(c);
	}

	inline Udjat::Linux::Notification & operator<<(Udjat::Linux::Notification &out, const Udjat::Linux::Urgency u) {
		return out.urgency(u);
	}

	inline Udjat::Linux::Notification & operator<<(Udjat::Linux::Notification &out, const char *s) {
		if(summary.empty()) {
			return out.summary(s);
		}
		return out.body();
	}


 }

