#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>

#include "barretenberg/common/assert.hpp"
#include "generator.hpp"

template <typename T, T DefaultValue = T{}> class ChunkedSparseArray {
  public:
    ChunkedSparseArray(size_t size)
        : size_{ size }
    {}

    auto size() const { return size_; }

    T get(size_t index) const
    {
        auto i = find_span(index);
        if (i == spans_.end())
            return DefaultValue;
        return span_element(i, index);
    }

    auto allocate(size_t begin, size_t end)
    {
        if (size_ <= begin || size_ <= end)
            throw std::out_of_range("bad index");

        if (end <= begin)
            return spans_.end();

        if (spans_.empty()) {
            return spans_.emplace(end, std::vector<T>(end - begin, DefaultValue)).first;
        }

        auto i = spans_.lower_bound(begin);

        size_t prefix_size = (begin < span_begin(i)) ? 0 : begin - span_begin(i);

        std::vector<T> new_span;
        new_span.reserve(end - begin + prefix_size);

        while (begin < end) {
            if (begin < span_begin(i)) {
                if (end < span_begin(i)) {
                    new_span.insert(new_span.end(), end - begin, DefaultValue);
                    break;
                }
                new_span.insert(new_span.end(), span_begin(i) - begin, DefaultValue);
                begin = span_begin(i);
            }
            new_span.insert(new_span.end(), i->second.begin(), i->second.end());
            begin = i->first;
            end = std::max(begin, end);
            i = spans_.erase(i);
        }

        return spans_.emplace(end, std::move(new_span)).first;
    }

    template <typename U> void set(size_t index, U&& value)
    {
        auto i = allocate(index, index + 1);
        ASSERT(i != spans_.end());
        span_element(i, index) = std::forward<U>(value);
    }

    Generator<std::pair<size_t, std::span<T const>>> spans() const
    {
        for (auto i = spans_.begin(); i != spans_.end(); ++i)
            co_yield std::pair<size_t, std::span<T const>>{ span_begin(i), i->second };
    }

    Generator<std::pair<size_t, T>> entries() const
    {
        for (auto i = spans_.begin(); i != spans_.end(); ++i)
            for (size_t j = 0; j < i->second.size(); ++j)
                co_yield std::pair<size_t, T>{ span_begin(i) + j, i->second[j] };
    }

  private:
    size_t size_;
    std::map<size_t, std::vector<T>> spans_;

    static size_t span_begin(auto const& i) { return i->first - i->second.size(); }

    static auto& span_element(auto const& i, size_t index) { return i->second[index - span_begin(i)]; }

    auto find_span(size_t index) const
    {
        if (size_ <= index)
            throw std::out_of_range("bad index");

        if (spans_.empty())
            return spans_.end();

        auto i = spans_.upper_bound(index);
        if (i == spans_.end())
            return i;

        ASSERT(index < i->first);
        if (index < span_begin(i))
            return spans_.end();
        return i;
    }
};
