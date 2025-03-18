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
        auto offset = index - i->first;
        return i->second[offset];
    }

    void allocate(size_t begin, size_t end)
    {
        if (size_ <= begin || size_ <= end)
            throw std::out_of_range("bad index");

        if (end <= begin)
            return;

        if (chunks_.empty()) {
            chunks_.emplace(begin, std::vector<T>(end - begin, DefaultValue));
            return;
        }

        auto i = chunks_.upper_bound(begin);

        if (i == chunks_.begin()) {
            i = chunks_.emplace(begin, std::vector<T>{}).first;
        } else {
            --i;
            auto chunk_end = i->first + i->second.size();
            if (chunk_end < begin) {
                i = chunks_.emplace(begin, std::vector<T>{}).first;
            } else {
                begin = chunk_end;
            }
        }

        auto chunk_begin = i->first;
        auto& chunk = i->second;

        chunk.reserve(chunk.size() + end - begin);

        ++i;

        while (begin < end) {
            if (i == chunks_.end() || end < i->first) {
                chunk.resize(chunk.size() + end - begin, DefaultValue);
                return;
            }
            chunk.resize(chunk.size() + i->first - begin, DefaultValue);
            chunk.insert(chunk.end(), i->second.begin(), i->second.end());
            i = chunks_.erase(i);
            begin = chunk_begin + chunk.size();
        }
    }

    template <typename U> void set(size_t index, U&& value)
    {
        allocate(index, index + 1);
        auto i = find_chunk(index);
        ASSERT(i != chunks_.end());
        auto offset = index - i->first;
        i->second[offset] = std::forward<U>(value);
    }

    Generator<std::pair<size_t, std::span<T>>> chunks()
    {
        for (auto i = chunks_.begin(); i != chunks_.end(); ++i)
            co_yield std::pair<size_t, std::span<T>>{ i->first, i->second };
    }

    Generator<std::pair<size_t, T>> entries()
    {
        for (auto i = chunks_.begin(); i != chunks_.end(); ++i)
            for (size_t j = 0; j < i->second.size(); ++j)
                co_yield std::pair<size_t, T>{ i->first + j, i->second[j] };
    }

  private:
    size_t size_;
    std::map<size_t, std::vector<T>> chunks_;

    auto find_chunk(size_t index)
    {
        if (size_ <= index)
            throw std::out_of_range("bad index");

        if (chunks_.empty())
            return chunks_.end();

        auto i = chunks_.upper_bound(index);
        if (i == chunks_.begin())
            return chunks_.end();
        --i;

        ASSERT(i->first <= index);
        auto offset = index - i->first;
        if (i->second.size() <= offset)
            return chunks_.end();
        return i;
    }
};
