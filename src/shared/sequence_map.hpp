#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "id_hash.hpp"

namespace c4c {

template <typename key_t, typename = void>
struct IsSequenceMapKey : std::false_type {};

template <typename key_t>
struct IsSequenceMapKey<
    key_t,
    std::void_t<decltype(std::declval<const key_t&>().data()),
                decltype(std::declval<const key_t&>().size())>>
    : std::integral_constant<
          bool,
          std::is_convertible<
              decltype(std::declval<const key_t&>().data()),
              const uint32_t*>::value &&
              std::is_convertible<
                  decltype(std::declval<const key_t&>().size()),
                  size_t>::value &&
              std::is_same<
                  typename std::remove_cv<
                      typename std::remove_pointer<
                          decltype(std::declval<const key_t&>().data())>::type>::type,
                  uint32_t>::value> {};

template <typename value_t>
struct IsSequenceMapValue
    : std::integral_constant<bool,
                             std::is_move_constructible<value_t>::value &&
                                 std::is_move_assignable<value_t>::value> {};

template <typename key_t, typename value_t>
class SequenceMap {
 public:
  static_assert(IsSequenceMapKey<key_t>::value,
                "SequenceMap key must expose uint32_t data() and size().");
  static_assert(IsSequenceMapValue<value_t>::value,
                "SequenceMap value must be movable.");

  struct View {
    const key_t* key = nullptr;
    const value_t* value = nullptr;

    [[nodiscard]] bool found() const { return key && value; }
    explicit operator bool() const { return found(); }

    [[nodiscard]] const uint32_t* data() const { return key ? key->data() : nullptr; }
    [[nodiscard]] size_t size() const { return key ? key->size() : 0; }
  };

  struct MutView {
    key_t* key = nullptr;
    value_t* value = nullptr;

    [[nodiscard]] bool found() const { return key && value; }
    explicit operator bool() const { return found(); }

    [[nodiscard]] const uint32_t* data() const { return key ? key->data() : nullptr; }
    [[nodiscard]] size_t size() const { return key ? key->size() : 0; }
  };

  SequenceMap() { rehash(16); }

  [[nodiscard]] size_t size() const { return entries_.size(); }

  [[nodiscard]] bool empty() const { return entries_.empty(); }

  [[nodiscard]] View find(const key_t& key) const {
    return find_sequence(key.data(), key.size());
  }

  [[nodiscard]] View find_sequence(const uint32_t* atoms, size_t count) const {
    if (!atoms || count == 0 || buckets_.empty()) return {};
    const int32_t bucket = bucket_index(hash_u32_sequence(atoms, count), buckets_.size());
    for (int32_t entry_index = buckets_[bucket];
         entry_index != kNoEntry;
         entry_index = entries_[entry_index].next) {
      const Entry& entry = entries_[entry_index];
      if (same_sequence(entry.key, atoms, count)) {
        return View{&entry.key, &entry.value};
      }
    }
    return {};
  }

  template <typename ValueLike>
  std::pair<MutView, bool> insert(key_t key, ValueLike&& value) {
    if (MutView existing = find_mut(key); existing) return {existing, false};
    ensure_capacity_for_insert();

    const size_t hash = hash_u32_sequence(key.data(), key.size());
    const int32_t bucket = bucket_index(hash, buckets_.size());
    const int32_t new_index = static_cast<int32_t>(entries_.size());
    entries_.push_back(Entry{std::move(key), std::forward<ValueLike>(value), buckets_[bucket]});
    buckets_[bucket] = new_index;
    return {MutView{&entries_.back().key, &entries_.back().value}, true};
  }

  bool erase(const key_t& key) { return erase_sequence(key.data(), key.size()); }

  bool erase_sequence(const uint32_t* atoms, size_t count) {
    if (!atoms || count == 0 || buckets_.empty()) return false;
    const size_t hash = hash_u32_sequence(atoms, count);
    const int32_t bucket = bucket_index(hash, buckets_.size());
    int32_t* link = &buckets_[bucket];
    while (*link != kNoEntry) {
      const int32_t entry_index = *link;
      Entry& entry = entries_[entry_index];
      if (same_sequence(entry.key, atoms, count)) {
        *link = entry.next;
        const int32_t last_index = static_cast<int32_t>(entries_.size() - 1);
        if (entry_index != last_index) {
          Entry moved = std::move(entries_.back());
          entries_[entry_index] = std::move(moved);
          for (int32_t& bucket_head : buckets_) {
            if (bucket_head == last_index) {
              bucket_head = entry_index;
            }
          }
          for (Entry& scanned : entries_) {
            if (scanned.next == last_index) scanned.next = entry_index;
          }
        }
        entries_.pop_back();
        return true;
      }
      link = &entries_[entry_index].next;
    }
    return false;
  }

  [[nodiscard]] MutView find_mut(const key_t& key) {
    return find_mut_sequence(key.data(), key.size());
  }

  [[nodiscard]] MutView find_mut_sequence(const uint32_t* atoms, size_t count) {
    if (!atoms || count == 0 || buckets_.empty()) return {};
    const int32_t bucket = bucket_index(hash_u32_sequence(atoms, count), buckets_.size());
    for (int32_t entry_index = buckets_[bucket];
         entry_index != kNoEntry;
         entry_index = entries_[entry_index].next) {
      Entry& entry = entries_[entry_index];
      if (same_sequence(entry.key, atoms, count)) {
        return MutView{&entry.key, &entry.value};
      }
    }
    return {};
  }

 private:
  static constexpr int32_t kNoEntry = -1;

  struct Entry {
    key_t key;
    value_t value;
    int32_t next = kNoEntry;
  };

  static bool same_sequence(const key_t& key, const uint32_t* atoms, size_t count) {
    if (key.size() != count) return false;
    const uint32_t* key_atoms = key.data();
    for (size_t i = 0; i < count; ++i) {
      if (key_atoms[i] != atoms[i]) return false;
    }
    return true;
  }

  static int32_t bucket_index(size_t hash, size_t bucket_count) {
    return static_cast<int32_t>(hash % bucket_count);
  }

  void ensure_capacity_for_insert() {
    if (buckets_.empty()) {
      rehash(16);
      return;
    }
    if ((entries_.size() + 1) * 4 >= buckets_.size() * 3) {
      rehash(buckets_.size() * 2);
    }
  }

  void rehash(size_t new_bucket_count) {
    if (new_bucket_count < 16) new_bucket_count = 16;
    std::vector<int32_t> new_buckets(new_bucket_count, kNoEntry);
    for (int32_t i = 0; i < static_cast<int32_t>(entries_.size()); ++i) {
      Entry& entry = entries_[i];
      const int32_t bucket =
          bucket_index(hash_u32_sequence(entry.key.data(), entry.key.size()), new_bucket_count);
      entry.next = new_buckets[bucket];
      new_buckets[bucket] = i;
    }
    buckets_.swap(new_buckets);
  }

  std::vector<int32_t> buckets_;
  std::vector<Entry> entries_;
};

}  // namespace c4c
