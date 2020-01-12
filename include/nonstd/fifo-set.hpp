#pragma once
// A hash set that guarantees iteration in insertion-order for C++14 or above.
// Or you can say, "a FIFO-ordered duplication-free container" if you feel like it.
//
// It's basically an `std::forward_list` of values,
// with a lookup index built using `std::unordered_map`.
//
// It's a drop-in replacement for `std::unordered_set`
// if the interface you used was implemented.
//
// Time complexity of all implemented operations are basically
// the same as `std::unordered_map`, which should be the same as `std::unordered_set`.
//
// Copyright (C) Giumo Clanjor (哆啦比猫/兰威举), 2020.
// Licensed under the MIT License.

#include <unordered_map>
#include <forward_list>
#include <cassert>

namespace nonstd
{
    template <
        class T
        , class Hash = std::hash<T>
        , class Equal = std::equal_to<T>
    >
    struct fifo_set final
    {
        using value_type = T;
        using hasher = Hash;
        using equal = Equal;

        using list_type = std::forward_list<value_type>;
        using list_iterator = typename list_type::iterator;
        using list_const_iterator = typename list_type::const_iterator;
        using iterator = list_iterator;
        using const_iterator = list_const_iterator;

        struct value_reference final
        {
            value_reference(value_type const& x): x{x} {}

            auto hash() const -> std::size_t
            {
                hasher h{};
                return h(x);
            }

            friend auto operator == (value_reference const& a, value_reference const& b) -> bool
            {
                equal eq{};
                return eq(a.x, b.x);
            }

        private:
            value_type const& x;
        };

        struct value_reference_hasher final
        {
            auto operator () (value_reference const& xr) const -> std::size_t
            {
                return xr.hash();
            }
        };

        using map_type = std::unordered_map<value_reference, list_iterator, value_reference_hasher>;
        using map_iterator = typename map_type::iterator;
        using size_type = typename map_type::size_type;

        // rule of five; force noexcept move constructible
        fifo_set() = default;

        fifo_set(fifo_set const& x)
        {
            for (auto&& kv: x)
                emplace_back(kv);
        }

        auto operator = (fifo_set const& x) -> fifo_set&
        {
            clear();
            for (auto&& kv: x)
                emplace_back(kv);
            return *this;
        }

        fifo_set(fifo_set&& other) noexcept
            : map{std::move(other.map)}
            , list{std::move(other.list)}
            , list_back{(list.empty() ? list.before_begin() : other.list_back)}
        {
            if (!list.empty()) map.at(list.front()) = list.before_begin();
        }

        auto operator = (fifo_set&& other) noexcept -> fifo_set&
        {
            map = std::move(other.map);
            list = std::move(other.list);
            list_back = (list.empty() ? list.before_begin() : other.list_back);
            if (!list.empty()) map.at(list.front()) = list.before_begin();
            return *this;
        }

        // For interface compatibility with std::unordered_set.
        template <class... Args>
        auto emplace(Args&&... args) -> std::pair<iterator, bool>
        {
            return emplace_back(std::forward<Args>(args)...);
        }

        template <class... Args>
        auto emplace_back(Args&&... args) -> std::pair<iterator, bool>
        {
            value_type value{std::forward<Args>(args)...};

            auto map_it = map.find(value);
            if (map_it == map.end()) {
                auto list_before_item = list_back;
                list_back = list.insert_after(list_back, std::move(value));

                map.emplace(*list_back, list_before_item);

                return { list_back, true };
            } else {
                auto list_it = map_it->second;
                ++list_it;
                return { list_it, false };
            }
        }

        template <class... Args>
        auto emplace_front(Args&&... args) -> std::pair<iterator, bool>
        {
            value_type value{std::forward<Args>(args)...};

            auto map_it = map.find(value);
            if (map_it == map.end()) {
                auto& list_it = (empty() ? list_back : map.at(list.front()));
                auto list_before_item = list_it;
                list_it = list.insert_after(list_before_item, std::move(value));

                map.emplace(*list_it, list_before_item);

                return { list_it, true };
            } else {
                auto list_it = map_it->second;
                ++list_it;
                return { list_it, false };
            }
        }

        auto erase(list_iterator list_it) -> void
        {
            auto map_it = map.find(*list_it);
            assert(map_it != map.end());

            auto list_before_item = map_it->second;
            map.erase(map_it);

            list_it = list.erase_after(list_before_item);

            if (list_it == list.end()) {
                list_back = list_before_item;
            } else {
                map.at(*list_it) = list_before_item;
            }
        }

        auto erase(value_type const& x) -> void
        {
            auto map_it = map.find(x);
            if (map_it == map.end()) return;

            auto list_before_item = map_it->second;
            map.erase(map_it);

            auto list_it = list.erase_after(list_before_item);

            if (list_it == list.end()) {
                list_back = list_before_item;
            } else {
                map.at(*list_it) = list_before_item;
            }
        }

        auto clear() -> void
        {
            map.clear();
            list.clear();
            list_back = list.before_begin();
        }

        auto count(value_type const& x) const -> size_type
        {
            return map.count(x);
        }

        auto size() const -> size_type
        {
            return map.size();
        }

        auto empty() const -> bool
        {
            return map.empty();
        }

        auto find(value_type const& x) const -> const_iterator
        {
            auto map_it = map.find(x);
            if (map_it == map.end())
                return end();

            auto list_before_item = map_it->second;
            return ++list_before_item;
        }

        auto find(value_type const& x) -> iterator
        {
            auto map_it = map.find(x);
            if (map_it == map.end())
                return end();

            auto list_before_item = map_it->second;
            return ++list_before_item;
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

