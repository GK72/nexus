#pragma once

#include "type_traits.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <vector>
#include <utility>

namespace nxs {
namespace detail {

// Template meta-programming / meta-functions
namespace meta {

template <typename Container>
struct _iterator {
    using value = typename Container::iterator;
};

template <typename Container>
struct _const_iterator {
    using value = typename Container::const_iterator;
};

template <typename Container>
struct _reverse_iterator  {
    using value = typename Container::reverse_iterator;
};

template <typename Container>
struct _const_reverse_iterator {
    using value = typename Container::const_reverse_iterator;
};

} // namespace meta

template <typename ContainerType, typename Iterators>
class flat_map_iterator {
public:
    using self = flat_map_iterator<ContainerType, Iterators>;

    using value_type = typename ContainerType::value_type;
    using pointer = typename ContainerType::pointer;
    using reference = typename ContainerType::reference;
    using difference_type = std::ptrdiff_t;

    using iterator_category = std::random_access_iterator_tag;

    constexpr flat_map_iterator(Iterators iter)
        : _iter(iter)
    {}

    constexpr self& operator++() noexcept {
        ++_iter.key;
        ++_iter.value;
        return *this;
    }

    [[nodiscard]] constexpr self& operator++(int) noexcept {
        self iter = *this;
        ++(*this);
        return iter;
    }

    constexpr self& operator--() noexcept {
        --_iter.key;
        --_iter.value;
        return *this;
    }

    [[nodiscard]] constexpr self& operator--(int) noexcept {
        self iter = *this;
        --(*this);
        return iter;
    }

    [[nodiscard]] constexpr self operator+(difference_type n) const noexcept {
        return Iterators{ _iter.key + n, _iter.value + n };
    }

    [[nodiscard]] constexpr self operator+=(difference_type n) const noexcept {
        self iter = *this;
        iter = iter + n;
        return iter;
    }

    // Don't we need an abs()?
    [[nodiscard]] constexpr difference_type operator-(self other) const noexcept {
        return _iter.key - other._iter.key;
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return std::make_pair(std::ref(*_iter.key), std::ref(*_iter.value));
    }

    [[nodiscard]] constexpr bool operator==(self other) const noexcept {
        return _iter == other._iter;
    }

    [[nodiscard]] constexpr bool operator!=(self other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr auto key() const noexcept {
        return _iter.key;
    }

    [[nodiscard]] constexpr auto value() const noexcept {
        return _iter.value;
    }

private:
    Iterators _iter;
};

} // namespace detail

template <typename Key,
          typename T,
          typename Compare = std::less<Key>,
          typename KeyContainer = std::vector<Key>,
          typename MappedContainer = std::vector<T>
>
class flat_map {
    template <template <typename> typename MetaFunc>
    struct iterators {
        using key_iterator = typename MetaFunc<KeyContainer>::value;
        using value_iterator = typename MetaFunc<MappedContainer>::value;

        auto operator<=>(const iterators&) const = default;

        key_iterator key;
        value_iterator value;
    };

public:
    using self = flat_map<Key, T, Compare, KeyContainer, MappedContainer>;

    using key_type    = Key;
    using mapped_type = T;
    using key_compare = Compare;

    using value_type      = std::pair<const Key, T>;
    using pointer         = std::pair<const Key*, T*>;
    using reference       = std::pair<const Key&, T&>;
    using const_reference = std::pair<const Key&, const T&>;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator_pair               = iterators<detail::meta::_iterator>;
    using const_iterator_pair         = iterators<detail::meta::_const_iterator>;
    using reverse_iterator_pair       = iterators<detail::meta::_reverse_iterator>;
    using const_reverse_iterator_pair = iterators<detail::meta::_const_reverse_iterator>;

    using iterator               = detail::flat_map_iterator<self, iterator_pair>;
    using const_iterator         = detail::flat_map_iterator<self, const_iterator_pair>;
    using reverse_iterator       = detail::flat_map_iterator<self, reverse_iterator_pair>;
    using const_reverse_iterator = detail::flat_map_iterator<self, const_reverse_iterator_pair>;

    using key_container_type    = KeyContainer;
    using mapped_container_type = MappedContainer;

    constexpr flat_map() = default;

    constexpr flat_map(std::initializer_list<value_type> ilist) {
        range_initialize(std::begin(ilist), std::end(ilist));
    }

    [[nodiscard]] constexpr bool empty()                          const noexcept { return _keys.empty(); }
    [[nodiscard]] constexpr size_type size()                      const noexcept { return _keys.size(); }
    [[nodiscard]] constexpr const key_container_type& keys()      const noexcept { return _keys; }
    [[nodiscard]] constexpr const mapped_container_type& values() const noexcept { return _values; }

    [[nodiscard]] constexpr mapped_type& at(const key_type& key) const noexcept {
        const auto keyEqual = [&key](const auto& ref) { return key == ref.first; };
        const auto iter = std::find_if(begin(), end(), keyEqual);

        return *iter.value();
    }

    constexpr std::pair<iterator, bool> insert(const std::pair<Key, T>& value) {
        const auto keyEqual = [&value](const auto& ref) { return value.first == ref.first; };
        auto iter = std::find_if(begin(), end(), keyEqual);

        if (iter != end()) {
            return { iter, false };
        }

        auto key_iter = std::upper_bound(std::begin(_keys), std::end(_keys), value.first, Compare());
        key_iter = _keys.insert(key_iter, value.first);

        const auto value_iter = _values.insert(
            std::next(std::begin(_values), std::distance(std::begin(_keys), key_iter)),
            value.second
        );

        return { iterator_pair{ key_iter, value_iter }, true };
    }

    // [[nodiscard]] mapped_type& operator[](key_type&& key) {

    [[nodiscard]] constexpr mapped_type& operator[](const key_type& key) {
        const auto keyEqual = [&key](const auto& ref) { return key == ref.first; };
        const auto iter = std::find_if(begin(), end(), keyEqual);

        if (iter == end()) {
            const auto& [inserted, _] = insert({ key, T{} });
            return *inserted.value();
        }

        return *iter.value();
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator_pair{ std::begin(_keys), std::begin(_values) };
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator_pair{ std::end(_keys), std::end(_values) };
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator_pair{ std::begin(_keys), std::begin(_values) };
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator_pair{ std::end(_keys), std::end(_values) };
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return const_iterator_pair{ std::cbegin(_keys), std::cbegin(_values) };
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return const_iterator_pair{ std::cend(_keys), std::cend(_values) };
    }

    [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator_pair{ std::rbegin(_keys), std::rbegin(_values) };
    }

    [[nodiscard]] constexpr reverse_iterator rend() noexcept {
        return reverse_iterator_pair{ std::rend(_keys), std::rend(_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator_pair{ std::rbegin(_keys), std::rbegin(_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator_pair{ std::rend(_keys), std::rend(_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator_pair{ std::crbegin(_keys), std::crbegin(_values) };
    }

    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator_pair{ std::crend(_keys), std::crend(_values) };
    }

private:
    KeyContainer _keys;
    MappedContainer _values;

    template <class Iter>
    constexpr void range_initialize(Iter first, Iter last) {
        if constexpr (is_std_array_v<key_container_type>) {
            std::size_t index = 0;
            for (; first != last; ++first) {
                _keys[index] = first->first;
                _values[index] = first->second;
                ++index;
            }
        }
        else {
            for (; first != last; ++first) {
                _keys.push_back(first->first);
                _values.push_back(first->second);
            }
        }
    }
};

template <typename Key,
          typename T,
          std::size_t Capacity,
          typename Compare = std::less<Key>
>
using static_map = flat_map<Key, T, Compare, std::array<Key, Capacity>, std::array<T, Capacity>>;

} // namespace nxs
