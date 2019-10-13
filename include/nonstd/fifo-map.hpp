#pragma once
// A hash map that guarantees iteration in insertion-order for C++14 or above.
// Or you can say, "a FIFO-ordered associative container" if you feel like it.
//
// (Yes, it's similar to nlohmann::fifo_map, but this one uses
// `std::unordered_map` instead of `std::map`.)
//
// It's basically an `std::forward_list` of key/value pairs,
// with a lookup index built using `std::unordered_map`.
//
// It's a drop-in replacement for `std::unordered_map`
// if the interface you used was implemented.
//
// Time complexity of all implemented operations are basically
// the same as `std::unordered_map`.
//
// Copyright (C) Giumo Clanjor (哆啦比猫/兰威举), 2019.
// Licensed under the MIT License.

#include <unordered_map>
#include <forward_list>
#include <cassert>

namespace nonstd
{
    template <
        class Key
        , class T
        , class Hash = std::hash<Key>
        , class Key_Equal = std::equal_to<Key>
    >
    struct fifo_map final
    {
        using key_type = Key;
        using mapped_type = T;
        using hasher = Hash;
        using key_equal = Key_Equal;

        using value_type = std::pair<key_type const, mapped_type>;
        using list_type = std::forward_list<value_type>;
        using list_iterator = typename list_type::iterator;
        using list_const_iterator = typename list_type::const_iterator;
        using iterator = list_iterator;
        using const_iterator = list_const_iterator;

        struct key_reference final
        {
            key_reference(key_type const& k): k{k} {}

            auto hash() const -> std::size_t
            {
                hasher h{};
                return h(k);
            }

            friend auto operator == (key_reference const& a, key_reference const& b) -> bool
            {
                key_equal eq{};
                return eq(a.k, b.k);
            }

        private:
            key_type const& k;
        };

        struct key_reference_hasher final
        {
            auto operator () (key_reference const& kr) const -> std::size_t
            {
                return kr.hash();
            }
        };

        using map_type = std::unordered_map<key_reference, list_iterator, key_reference_hasher>;
        using map_iterator = typename map_type::iterator;
        using size_type = typename map_type::size_type;

        // rule of five; only movable; force noexcept move constructible
        fifo_map() = default;
        fifo_map(fifo_map const&) = delete;
        auto operator = (fifo_map const&) -> fifo_map& = delete;

        fifo_map(fifo_map&& other) noexcept
            : map{std::move(other.map)}
            , list{std::move(other.list)}
            , list_back{(list.empty() ? list.before_begin() : other.list_back)}
        {
            if (!list.empty()) map.at(list.front().first) = list.before_begin();
        }

        auto operator = (fifo_map&& other) noexcept -> fifo_map&
        {
            map = std::move(other.map);
            list = std::move(other.list);
            list_back = (list.empty() ? list.before_begin() : other.list_back);
            if (!list.empty()) map.at(list.front().first) = list.before_begin();
            return *this;
        }

        // For interface compatibility with std::unordered_map.
        template <class... Args>
        auto emplace(Args&&... args) -> std::pair<iterator, bool>
        {
            return emplace_back(std::forward<Args>(args)...);
        }

        template <class... Args>
        auto emplace_back(Args&&... args) -> std::pair<iterator, bool>
        {
            value_type value{std::forward<Args>(args)...};

            auto map_it = map.find(value.first);
            if (map_it == map.end()) {
                auto list_before_item = list_back;
                list_back = list.insert_after(list_back, std::move(value));

                map.emplace(list_back->first, list_before_item);

                return { list_back, true };
            } else {
                auto list_it = map_it->second;
                ++list_it;
                return { list_it, false };
            }
        }

        auto erase(list_iterator list_it) -> void
        {
            auto map_it = map.find(list_it->first);
            assert(map_it != map.end());

            auto list_before_item = map_it->second;
            map.erase(map_it);

            list_it = list.erase_after(list_before_item);

            if (list_it == list.end()) {
                list_back = list_before_item;
            } else {
                map.at(list_it->first) = list_before_item;
            }
        }

        auto erase(key_type const& key) -> void
        {
            auto map_it = map.find(key);
            if (map_it == map.end()) return;

            auto list_before_item = map_it->second;
            map.erase(map_it);

            auto list_it = list.erase_after(list_before_item);

            if (list_it == list.end()) {
                list_back = list_before_item;
            } else {
                map.at(list_it->first) = list_before_item;
            }
        }

        auto clear() -> void
        {
            map.clear();
            list.clear();
            list_back = list.before_begin();
        }

        auto count(key_type const& key) const -> size_type
        {
            return map.count(key);
        }

        auto size() const -> size_type
        {
            return map.size();
        }

        auto empty() const -> bool
        {
            return map.empty();
        }

        auto find(key_type const& key) const -> const_iterator
        {
            auto map_it = map.find(key);
            if (map_it == map.end())
                return end();

            auto list_before_item = map_it->second;
            return ++list_before_item;
        }

        auto find(key_type const& key) -> iterator
        {
            auto map_it = map.find(key);
            if (map_it == map.end())
                return end();

            auto list_before_item = map_it->second;
            return ++list_before_item;
        }

        auto at(key_type const& key) const -> mapped_type const&
        {
            auto list_it = map.at(key);
            ++list_it;
            return list_it->second;
        }

        auto at(key_type const& key) -> mapped_type&
        {
            auto list_it = map.at(key);
            ++list_it;
            return list_it->second;
        }

        auto operator [] (key_type const& key) -> mapped_type&
        {
            auto list_it = find(key);
            if (list_it != end())
                return list_it->second;

            return emplace(key, mapped_type{}).first->second;
        }

        auto begin() -> iterator { return list.begin(); }
        auto   end() -> iterator { return list.  end(); }
        auto begin() const -> const_iterator { return list.begin(); }
        auto   end() const -> const_iterator { return list.  end(); }
        auto cbegin() const -> const_iterator { return list.cbegin(); }
        auto   cend() const -> const_iterator { return list.  cend(); }

    private:
        map_type map;
        list_type list;
        list_iterator list_back{list.before_begin()};
    };
}

