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

#ifndef STORE_IPP
#define STORE_IPP

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <utility>

namespace Store
{
	template <typename K, typename V>
	class KeyStore
	{
	public:
		void push(K k, V v) { mList.push_front(std::make_pair(k, v)); }

		void pushSecond(K k, V v) { mList.insert(std::next(mList.begin()), std::make_pair(k, v)); }

		V pop(K k)
		{
			typename std::list<std::pair<const K, V>>::iterator it = std::find_if(
				mList.begin(), mList.end(), [&k](std::pair<const K, V> o) -> bool { return (o.first == k); });

			if (it != mList.end())
			{
				V v = it->second;
				mList.erase(it);
				return v;
			}

			return nullptr;
		}

		V get(K k)
		{
			typename std::list<std::pair<const K, V>>::iterator it = std::find_if(
				mList.begin(), mList.end(), [&k](std::pair<const K, V> o) -> bool { return (o.first == k); });

			if (it != mList.end())
				return it->second;

			return nullptr;
		}

		V moveToStart(K k)
		{
			V v = pop(k);
			mList.push_front(std::make_pair(k, v));

			return v;
		}

		V findIf(std::function<bool(std::pair<const K, V>)> pred)
		{
			typename std::list<std::pair<const K, V>>::iterator it = std::find_if(mList.begin(), mList.end(), pred);
			if (it != mList.end())
				return it->second;

			return nullptr;
		}

		void forEach(std::function<void(std::pair<const K, V>)> funct)
		{
			std::for_each(mList.begin(), mList.end(), funct);
		}

		void clear() { mList.clear(); }

		uint size() { return mList.size(); }

		V first() { return mList.front().second; }

		std::list<std::pair<const K, V>> mList;
	};

	template <typename K, typename V>
	class Map
	{
	public:
		void set(K k, V v) { mMap[k] = v; }

		V get(K k)
		{
			typename std::map<const K, V>::iterator it = mMap.find(k);
			if (it != mMap.end())
				return it->second;

			return nullptr;
		}

		void remove(K k)
		{
			typename std::map<const K, V>::iterator it = mMap.find(k);
			if (it != mMap.end())
				mMap.erase(it);
		}

		void clear() { mMap.clear(); }

	private:
		std::map<const K, V> mMap;
	};

	template <typename V>
	class List
	{
	public:
		void push(V v) { mList.push_back(v); }

		void pop(V v) { mList.remove(v); }

		V get(uint index) { return *std::next(mList.begin(), index); }

		uint getIndex(V v)
		{
			typename std::list<V>::iterator it = std::find(mList.begin(), mList.end(), v);
			return std::distance(mList.begin(), it);
		}

		V findIf(std::function<bool(V)> pred)
		{
			typename std::list<V>::iterator it = std::find_if(mList.begin(), mList.end(), pred);
			if (it != mList.end())
				return *it;

			return nullptr;
		}

		void forEach(std::function<void(V)> funct) { std::for_each(mList.begin(), mList.end(), funct); }

		uint size() { return mList.size(); }

	private:
		std::list<V> mList;
	};

	template <typename T>
	using AutoPtr = std::unique_ptr<T, std::function<void(void*)>>;
} // namespace Store

#endif // STORE_IPP
