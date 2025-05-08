/*
 * Docklike Taskbar - A modern, minimalist taskbar for Xfce
 * Copyright (c) 2019-2020 Nicolas Szabo <nszabo@vivaldi.net>
 * Copyright (c) 2020-2021 David Keogh <davidtkeogh@gmail.com>
 * gnu.org/licenses/gpl-3.0
 */

#include "Helpers.hpp"

namespace Help
{
	namespace String
	{
		void split(const std::string& str, std::list<std::string>& list,
			char delim = ' ')
		{
			std::stringstream ss(str);
			std::string token;
			while (std::getline(ss, token, delim))
				list.push_back(token);
		}

		std::string toLowercase(std::string str)
		{
			std::for_each(str.begin(), str.end(), [](char& c) {
				c = std::tolower(static_cast<unsigned char>(c));
			});
			return str;
		}

		std::string numericOnly(std::string str)
		{
			str.erase(
				std::remove_if(str.begin(), str.end(), [](char chr) { return chr < 48 || chr > 57; }),
				str.end());

			return str;
		}

		std::string getWord(std::string str, int index, char separator)
		{
			if (index == (int)std::string::npos)
			{
				std::string::iterator it = --str.end();
				while (it != str.begin() && *it == separator)
					--it;

				std::string::iterator end = it + 1;

				while (it != str.begin() && *it != separator)
					--it;

				if (*it == separator)
					++it;

				return std::string(it, end);
			}

			std::string::iterator it = str.begin();
			while (it != str.end() && *it == separator)
				++it;

			while (index > 0)
			{
				--index;
				while (it != str.end() && *it != separator)
					++it;
				while (it != str.end() && *it == separator)
					++it;
			}
			if (it == str.end())
				return "";

			std::string::iterator start = it;

			while (it != str.end() && *it != separator)
				++it;

			return std::string(start, it);
		}

		std::string pathBasename(const std::string str, bool removeSuffix)
		{
			gchar* basename = g_path_get_basename(str.c_str());
			if (removeSuffix)
			{
				gchar* dot = g_strrstr_len(basename, -1, ".");
				if (dot != nullptr)
				{
					gchar* temp = g_strndup(basename, dot - basename);
					g_free(basename);
					basename = temp;
				}
			}
			std::string str_out = basename;
			g_free(basename);
			return str_out;
		}

		std::string trim(const std::string str)
		{
			std::string::const_iterator s = str.begin();
			std::string::const_iterator e = str.end();

			while (s != e && (*s == ' ' || *s == '\t' || *s == '"'))
				++s;
			if (e != s)
				--e;
			while (e != s && (*e == ' ' || *e == '\t' || *e == '"'))
				--e;

			return std::string(s, e + 1);
		}
	} // namespace String

	namespace Gtk
	{
		std::list<std::string> bufferToStdStringList(gchar** stringList)
		{
			std::list<std::string> ret;

			if (stringList != nullptr)
				for (int i = 0; stringList[i] != nullptr; ++i)
					ret.push_back(stringList[i]);

			return ret;
		}

		std::vector<char*> stdToBufferStringList(std::list<std::string>& stringList)
		{
			std::vector<char*> buf;

			for (std::string& s : stringList)
				buf.push_back(&s[0]);

			return buf;
		}

		int getChildPosition(GtkContainer* container, GtkWidget* child)
		{
			int ret;
			GValue gv = G_VALUE_INIT;
			g_value_init(&gv, G_TYPE_INT);

			gtk_container_child_get_property(container, child, "position", &gv);
			ret = g_value_get_int(&gv);
			g_value_unset(&gv);

			return ret;
		}

		void cssClassAdd(GtkWidget* widget, const char* className)
		{
			gtk_style_context_add_class(gtk_widget_get_style_context(widget), className);
		}

		void cssClassRemove(GtkWidget* widget, const char* className)
		{
			gtk_style_context_remove_class(gtk_widget_get_style_context(widget), className);
		}

		Timeout::Timeout()
		{
			mDuration = mTimeoutId = 0;
		}

		void Timeout::setup(uint ms, std::function<bool()> function)
		{
			mDuration = ms;
			mFunction = function;
		}

		void Timeout::start()
		{
			stop();
			mTimeoutId = g_timeout_add(mDuration, G_SOURCE_FUNC(+[](Timeout* me) {
				bool cont = me->mFunction();

				if (!cont)
					me->mTimeoutId = 0;
				return cont;
			}),
				this);
		}

		void Timeout::stop()
		{
			if (mTimeoutId)
			{
				g_source_remove(mTimeoutId);
				mTimeoutId = 0;
			}
		}

		Idle::Idle()
		{
			mIdleId = 0;
		}

		void Idle::setup(std::function<bool()> function)
		{
			mFunction = function;
		}

		void Idle::start()
		{
			stop();
			mIdleId = g_idle_add(G_SOURCE_FUNC(+[](Idle* me) {
				bool cont = me->mFunction();
				if (!cont)
					me->mIdleId = 0;
				return cont;
			}),
				this);
		}

		void Idle::stop()
		{
			if (mIdleId != 0)
			{
				g_source_remove(mIdleId);
				mIdleId = 0;
			}
		}
	} // namespace Gtk

} // namespace Help
