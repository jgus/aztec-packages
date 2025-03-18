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

    T get(size_t index)
    {
        auto i = find_chunk(index);
        if (i == chunks_.end())
            return DefaultValue;
        return chunk_element(i, index);
    }

    auto allocate(size_t begin, size_t end)
    {
        if (size_ <= begin || size_ <= end)
            throw std::out_of_range("bad index");

        if (end <= begin)
            return chunks_.end();

        if (chunks_.empty()) {
            return chunks_.emplace(end, std::vector<T>(end - begin, DefaultValue)).first;
        }

        auto i = chunks_.lower_bound(begin);

        size_t prefix_size = (begin < chunk_begin(i)) ? 0 : begin - chunk_begin(i);

        std::vector<T> new_chunk;
        new_chunk.reserve(end - begin + prefix_size);

        while (begin < end) {
            if (begin < chunk_begin(i)) {
                if (end < chunk_begin(i)) {
                    new_chunk.insert(new_chunk.end(), end - begin, DefaultValue);
                    break;
                }
                new_chunk.insert(new_chunk.end(), chunk_begin(i) - begin, DefaultValue);
                begin = chunk_begin(i);
            }
            new_chunk.insert(new_chunk.end(), i->second.begin(), i->second.end());
            begin = i->first;
            end = std::max(begin, end);
            i = chunks_.erase(i);
        }

        return chunks_.emplace(end, std::move(new_chunk)).first;
    }

    template <typename U> void set(size_t index, U&& value)
    {
        auto i = allocate(index, index + 1);
        ASSERT(i != chunks_.end());
        chunk_element(i, index) = std::forward<U>(value);
    }

    Generator<std::pair<size_t, std::span<T>>> chunks()
    {
        for (auto i = chunks_.begin(); i != chunks_.end(); ++i)
            co_yield std::pair<size_t, std::span<T>>{ chunk_begin(i), i->second };
    }

    Generator<std::pair<size_t, T>> entries()
    {
        for (auto i = chunks_.begin(); i != chunks_.end(); ++i)
            for (size_t j = 0; j < i->second.size(); ++j)
                co_yield std::pair<size_t, T>{ chunk_begin(i) + j, i->second[j] };
    }

  private:
    size_t size_;
    using map_t = std::map<size_t, std::vector<T>>;
    map_t chunks_;

    static size_t chunk_begin(map_t::iterator const& i) { return i->first - i->second.size(); }

    static T& chunk_element(map_t::iterator const& i, size_t index) { return i->second[index - chunk_begin(i)]; }

    auto find_chunk(size_t index)
    {
        if (size_ <= index)
            throw std::out_of_range("bad index");

        if (chunks_.empty())
            return chunks_.end();

        auto i = chunks_.upper_bound(index);
        if (i == chunks_.end())
            return i;

        ASSERT(index < i->first);
        if (index < chunk_begin(i))
            return chunks_.end();
        return i;
    }
};
