/*
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <gtk/gtk.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

namespace Help
{
	namespace String
	{
		void split(const std::string& str, std::list<std::string>& list, char delim);
		std::string toLowercase(std::string str);
		std::string numericOnly(std::string str);
		std::string getWord(std::string str, int index, char separator = ' ');
		std::string pathBasename(const std::string str, bool removeSuffix = false);
		std::string pathDirname(const std::string str);
		std::string trim(const std::string str);
	} // namespace String

	namespace Gtk
	{
		std::list<std::string> bufferToStdStringList(gchar** stringList);
		std::vector<char*> stdToBufferStringList(std::list<std::string>& stringList);

		int getChildPosition(GtkContainer* container, GtkWidget* child);

		void cssClassAdd(GtkWidget* widget, const char* className);
		void cssClassRemove(GtkWidget* widget, const char* className);

		class Timeout
		{
		public:
			Timeout();

			void setup(uint ms, std::function<bool()> function);

			void start();
			void stop();

			uint mDuration;
			std::function<bool()> mFunction;

			uint mTimeoutId;
		};

		class Idle
		{
		public:
			Idle();

			void setup(std::function<bool()> function);
			void start();
			void stop();

			std::function<bool()> mFunction;
			uint mIdleId;
		};
	} // namespace Gtk
} // namespace Help

#endif // HELPERS_HPP
