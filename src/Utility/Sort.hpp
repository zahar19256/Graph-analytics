#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "../IO/RawReader.hpp"

static const size_t kSortBatchSize = (1 << 20);
static const size_t kBatchLimit = 10;
static const size_t kMergeBufferSize = (1 << 14);
static const size_t kCopyBufferSize = (1 << 18);

template <typename T>
class SortBlockReader {
public:
    SortBlockReader(const std::string& path, size_t start_record, size_t count)
        : reader_(path), left_(count), buffer_(kMergeBufferSize) {
        reader_.Seek(start_record * sizeof(T));
        Fill();
    }

    bool HasValue() const {
        return position_ < size_;
    }

    const T& Value() const {
        return buffer_[position_];
    }

    void Next() {
        ++position_;
        if (position_ == size_) {
            Fill();
        }
    }

private:
    void Fill() {
        if (left_ == 0) {
            position_ = 0;
            size_ = 0;
            return;
        }

        size_t count = std::min(buffer_.size(), left_);
        reader_.ReadVector<T>(buffer_, count);
        position_ = 0;
        size_ = buffer_.size();
        left_ -= size_;
    }
    RawReader reader_;
    size_t left_ = 0;
    std::vector<T> buffer_;
    size_t position_ = 0;
    size_t size_ = 0;
};

template <typename T>
struct SortHeapNode {
    T value;
    size_t block;
};

template <typename T, typename Compare>
class SortHeapCompare {
public:
    explicit SortHeapCompare(Compare compare) : compare_(compare) {
    }

    bool operator()(const SortHeapNode<T>& left, const SortHeapNode<T>& right) const {
        if (compare_(right.value, left.value)) {
            return true;
        }
        if (compare_(left.value, right.value)) {
            return false;
        }
        return left.block > right.block;
    }

private:
    Compare compare_;
};

template <typename T, typename Compare>
static bool EqualRecords(const T& left, const T& right, Compare compare) {
    return !compare(left, right) && !compare(right, left);
}

static void ClearFile(const std::string& path) {
    std::ofstream output(path, std::ios::binary | std::ios::out | std::ios::trunc);
}

static void CopyFile(const std::string& from, const std::string& to) {
    std::ifstream input(from, std::ios::binary | std::ios::in);
    std::ofstream output(to, std::ios::binary | std::ios::out | std::ios::trunc);
    std::vector<char> buffer(kCopyBufferSize);
    while (input) {
        input.read(buffer.data(), buffer.size());
        size_t bytes = input.gcount();
        if (bytes > 0) {
            output.write(buffer.data(), bytes);
        }
    }
}

template <typename T, typename Compare>
static size_t MergeBlocks(const std::string& input_file, const std::vector<size_t>& offsets, const std::string& output_file, Compare compare, bool unique) {
    std::vector<std::unique_ptr<SortBlockReader<T>>> readers;
    readers.reserve(offsets.size());
    size_t start = 0;
    for (size_t i = 0; i < offsets.size(); ++i) {
        readers.push_back(std::unique_ptr<SortBlockReader<T>>(new SortBlockReader<T>(input_file, start, offsets[i])));
        start += offsets[i];
    }
    std::priority_queue<SortHeapNode<T>, std::vector<SortHeapNode<T>>, SortHeapCompare<T, Compare>> heap((SortHeapCompare<T, Compare>(compare)));
    for (size_t i = 0; i < readers.size(); ++i) {
        if (readers[i]->HasValue()) {
            heap.push({readers[i]->Value(), i});
        }
    }

    std::ofstream output(output_file, std::ios::binary | std::ios::out | std::ios::trunc);
    std::vector<T> output_buffer;
    output_buffer.reserve(kMergeBufferSize);
    bool has_last = false;
    T last{};
    size_t written = 0;
    while (!heap.empty()) {
        SortHeapNode<T> node = heap.top();
        heap.pop();

        if (!unique || !has_last || !EqualRecords(last, node.value, compare)) {
            output_buffer.push_back(node.value);
            last = node.value;
            has_last = true;
            ++written;
        }

        if (output_buffer.size() == kMergeBufferSize) {
            output.write(reinterpret_cast<const char*>(output_buffer.data()),
                         output_buffer.size() * sizeof(T));
            output_buffer.clear();
        }

        readers[node.block]->Next();
        if (readers[node.block]->HasValue()) {
            heap.push({readers[node.block]->Value(), node.block});
        }
    }

    if (!output_buffer.empty()) {
        output.write(reinterpret_cast<const char*>(output_buffer.data()),
                     output_buffer.size() * sizeof(T));
    }

    return written;
}

template <typename T, typename Compare>
class SortFile {
public:
    SortFile(const std::string& base_path, size_t index, Compare compare, bool unique)
        : base_path_(base_path),
          path_(base_path + ".sort_" + std::to_string(index)),
          index_(index),
          compare_(compare),
          unique_(unique) {
        ClearFile(path_);
    }

    void AddBatch(std::vector<T>& values) {
        if (values.empty()) {
            return;
        }
        std::ofstream output(path_, std::ios::binary | std::ios::out | std::ios::app);
        output.write(reinterpret_cast<const char*>(values.data()),
                     values.size() * sizeof(T));
        offsets_.push_back(values.size());
        ++counter_;
        if (counter_ == kBatchLimit) {
            Merge();
        }
    }

    void AddMergedBlock(const std::string& file, size_t count) {
        if (count == 0) {
            return;
        }
        std::ifstream input(file, std::ios::binary | std::ios::in);
        std::ofstream output(path_, std::ios::binary | std::ios::out | std::ios::app);
        output << input.rdbuf();
        offsets_.push_back(count);
        ++counter_;
        if (counter_ == kBatchLimit) {
            Merge();
        }
    }

    void Merge() {
        if (counter_ == 0) {
            return;
        }
        if (!next_) {
            next_.reset(new SortFile<T, Compare>(base_path_, index_ + 1, compare_, unique_));
        }
        std::string merged_file = base_path_ + ".sort_merge_" + std::to_string(index_);
        size_t merged_count = MergeBlocks<T, Compare>(path_, offsets_, merged_file, compare_, unique_);
        next_->AddMergedBlock(merged_file, merged_count);
        std::remove(merged_file.c_str());
        offsets_.clear();
        counter_ = 0;
        ClearFile(path_);
    }

    void Finish(const std::string& output_file) {
        if (next_) {
            if (counter_ > 0) {
                Merge();
            }
            next_->Finish(output_file);
            return;
        }

        if (counter_ == 0) {
            ClearFile(output_file);
        } else if (counter_ == 1) {
            CopyFile(path_, output_file);
        } else {
            MergeBlocks<T, Compare>(path_, offsets_, output_file, compare_, unique_);
        }
    }

private:
    std::vector<size_t> offsets_;
    std::unique_ptr<SortFile<T, Compare>> next_;
    std::string base_path_;
    std::string path_;
    size_t index_ = 0;
    Compare compare_;
    bool unique_ = false;
    size_t counter_ = 0;
};

class IntCompare {
public:
    bool operator()(int32_t left, int32_t right) const {
        return left < right;
    }
};

static void Sort(const std::string& file) {
    RawReader reader(file + ".raw");
    SortFile<int32_t, IntCompare> sort_file(file, 0, IntCompare(), true);
    std::vector<int32_t> storage;
    while(!reader.Empty()) {
        reader.ReadVector<int32_t>(storage, kSortBatchSize);
        std::sort(storage.begin(), storage.end());
        storage.erase(std::unique(storage.begin(), storage.end()), storage.end());
        sort_file.AddBatch(storage);
    }
    sort_file.Finish(file + ".sort");
}
