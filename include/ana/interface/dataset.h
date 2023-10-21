#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "multithread.h"

namespace ana {

namespace dataset {

template <typename T> class input;

class player;

template <typename T> class column;

/**
 * @brief Range of a dataset to process by one thread slot.
 */
struct range {
  range(size_t slot, unsigned long long begin, unsigned long long end);
  ~range() = default;

  range operator+(const range &next);
  range &operator+=(const range &next);

  unsigned long long entries() const;

  /**
   * @brief Thread index that the processed range belongs to.
   */
  size_t slot;
  /**
   * @brief The first entry in the range
   */
  unsigned long long begin;
  /**
   * @brief The last entry+1 in the range
   */
  unsigned long long end;
};

/**
 * @brief Partition of a dataset.
 * @details A partition contains one or more ranges of the datset in sequential
 * order of their entries. They can dynamically truncated and merged according
 * to the maximum limit of entries of the dataset and multithreading
 * configuration as specified by the analyzer.
 */
struct partition {
  static std::vector<std::vector<range>>
  group_parts(const std::vector<range> &parts, size_t n);
  static range sum_parts(const std::vector<range> &parts);

  partition() : fixed(false) {}
  partition(unsigned long long nentries,
            unsigned long long max_entries_per_slot = 1);
  ~partition() = default;

  void emplace_back(size_t islot, unsigned long long begin,
                    unsigned long long end);

  range const &operator[](size_t irange) const;
  range total() const;

  size_t size() const;

  void truncate(long long max_entries = -1);

  void merge(size_t max_parts);

  bool fixed;
  std::vector<range> parts;
};

struct head {
  head(long long value) : value(value) {}
  long long value;
};

} // namespace dataset

template <typename T>
using open_player_t = typename decltype(std::declval<T const &>().open_player(
    std::declval<const ana::dataset::range &>()))::element_type;

template <typename T, typename Val>
using read_column_t =
    typename decltype(std::declval<T>().template read_column<Val>(
        std::declval<dataset::range const &>(),
        std::declval<std::string const &>()))::element_type;

template <typename T> struct is_unique_ptr : std::false_type {};
template <typename T>
struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};
template <typename T>
static constexpr bool is_unique_ptr_v = is_unique_ptr<T>::value;

} // namespace ana

#include "column.h"
#include "dataset_player.h"

inline ana::dataset::range::range(size_t slot, unsigned long long begin,
                                  unsigned long long end)
    : slot(slot), begin(begin), end(end) {}

inline unsigned long long ana::dataset::range::entries() const {
  assert(this->end > this->begin);
  return end - begin;
}

inline ana::dataset::range ana::dataset::range::operator+(const range &next) {
  assert(this->end == next.begin);
  return range(this->slot, this->begin, next.end);
}

inline ana::dataset::range &ana::dataset::range::operator+=(const range &next) {
  assert(this->end == next.begin);
  this->end = next.end;
  return *this;
}

inline std::vector<std::vector<ana::dataset::range>>
ana::dataset::partition::group_parts(const std::vector<range> &parts,
                                     size_t n) {
  std::vector<std::vector<range>> grouped_parts;
  size_t length = parts.size() / n;
  size_t remain = parts.size() % n;
  size_t begin = 0;
  size_t end = 0;
  for (size_t i = 0; i < std::min(n, parts.size()); ++i) {
    end += (remain > 0) ? (length + !!(remain--)) : length;
    grouped_parts.push_back(
        std::vector<range>(parts.begin() + begin, parts.begin() + end));
    begin = end;
  }
  return grouped_parts;
}

inline ana::dataset::range
ana::dataset::partition::sum_parts(const std::vector<range> &parts) {
  return std::accumulate(std::next(parts.begin()), parts.end(), parts.front());
}

inline ana::dataset::partition::partition(
    unsigned long long nentries, unsigned long long max_entries_per_slot)
    : fixed(false) {
  auto remaining = nentries;
  unsigned long long begin = 0;
  unsigned int islot = 0;
  while (remaining) {
    auto slot_entries = std::min(remaining, max_entries_per_slot);
    auto end = begin + slot_entries;
    this->emplace_back(islot++, begin, end);
    begin += slot_entries;
    remaining -= slot_entries;
  }
}

inline void ana::dataset::partition::emplace_back(size_t islot,
                                                  unsigned long long begin,
                                                  unsigned long long end) {
  this->parts.emplace_back(islot, begin, end);
}

inline ana::dataset::range const &
ana::dataset::partition::operator[](size_t islot) const {
  return this->parts[islot];
}

inline ana::dataset::range ana::dataset::partition::total() const {
  return sum_parts(this->parts);
}

inline size_t ana::dataset::partition::size() const {
  return this->parts.size();
}

inline void ana::dataset::partition::merge(size_t max_parts) {
  if (fixed)
    return;
  auto groups = group_parts(this->parts, max_parts);
  this->parts.clear();
  for (const auto &group : groups) {
    this->parts.push_back(sum_parts(group));
  }
}

inline void ana::dataset::partition::truncate(long long max_entries) {
  if (fixed)
    return;
  if (max_entries < 0)
    return;
  // remember the full parts
  auto full_parts = this->parts;
  // clear the parts to be added anew
  this->parts.clear();
  for (const auto &part : full_parts) {
    auto part_end = max_entries >= 0
                        ? std::min(part.begin + max_entries, part.end)
                        : part.end;
    this->parts.push_back(range(part.slot, part.begin, part_end));
    max_entries -= part_end;
    if (!max_entries)
      break;
  }
}
