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

#ifndef STATE_IPP
#define STATE_IPP

#include <functional>

template <typename V>
class State
{
public:
	void setup(V value, std::function<void(V)> feedback)
	{
		v = value;
		f = feedback;
	}

	void set(V value)
	{
		bool change = (v != value);
		v = value;

		if (change)
			f(v);
	}

	V get() { return v; }

	operator V() const { return v; }

private:
	V v;
	std::function<void(V)> f;
};

template <typename V>
class LogicalState
{
public:
	void setup(V value, std::function<V()> eval,
		std::function<void(V)> feedback)
	{
		v = value;
		e = eval;
		f = feedback;
	}

	void updateState()
	{
		V value = e();
		if (v != value)
		{
			v = value;
			f(v);
		}
	}

	operator V() const { return v; }
	operator V*() const { return v; }

private:
	V v;
	std::function<V()> e;
	std::function<void(V)> f;
};

#endif // STATE_IPP
