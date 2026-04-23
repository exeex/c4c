#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "sequence_map.hpp"
#include "text_id_table.hpp"

namespace c4c {

using LocalNameAtom = uint32_t;
using LocalNameKey = std::vector<TextId>;

struct LocalNameFragmentView {
  const TextId* data = nullptr;
  size_t size = 0;

  [[nodiscard]] bool empty() const { return size == 0; }
  [[nodiscard]] const TextId& operator[](size_t index) const { return data[index]; }
};

[[nodiscard]] inline LocalNameFragmentView make_local_name_fragment_view(
    const TextId* text_ids, size_t count) {
  return LocalNameFragmentView{text_ids, count};
}

[[nodiscard]] inline LocalNameKey append_local_name_fragment(
    LocalNameFragmentView prefix, TextId segment) {
  LocalNameKey combined;
  combined.reserve(prefix.size + (segment != kInvalidText ? 1u : 0u));
  for (size_t i = 0; i < prefix.size; ++i) combined.push_back(prefix.data[i]);
  if (segment != kInvalidText) combined.push_back(segment);
  return combined;
}

[[nodiscard]] inline LocalNameKey rebase_local_name_fragment(
    LocalNameFragmentView prefix, LocalNameFragmentView suffix) {
  LocalNameKey combined;
  combined.reserve(prefix.size + suffix.size);
  for (size_t i = 0; i < prefix.size; ++i) combined.push_back(prefix.data[i]);
  for (size_t i = 0; i < suffix.size; ++i) combined.push_back(suffix.data[i]);
  return combined;
}

[[nodiscard]] inline LocalNameKey rebase_local_name_fragment(
    const TextId* prefix_text_ids,
    size_t prefix_count,
    const TextId* suffix_text_ids,
    size_t suffix_count) {
  return rebase_local_name_fragment(
      make_local_name_fragment_view(prefix_text_ids, prefix_count),
      make_local_name_fragment_view(suffix_text_ids, suffix_count));
}

template <KeyConcept key_t, ValueConcept value_t>
class LocalNameTable {
 public:
  using Key = key_t;
  using Map = SequenceMap<key_t, value_t>;
  using View = typename Map::View;
  using MutView = typename Map::MutView;

  // Shared lexical substrate: phase-owned payloads hang off a scope stack
  // keyed by TextId-native fragments, while lookup semantics stay explicit.
  void push_scope() { scopes_.emplace_back(); }

  bool pop_scope() {
    if (scopes_.empty()) return false;
    scopes_.pop_back();
    return true;
  }

  [[nodiscard]] bool empty() const { return scopes_.empty(); }

  [[nodiscard]] size_t scope_depth() const { return scopes_.size(); }

  template <typename ValueLike>
  std::pair<MutView, bool> bind_in_current_scope(key_t key, ValueLike&& value) {
    return current_scope().insert(std::move(key), std::forward<ValueLike>(value));
  }

  template <typename ValueLike>
  std::pair<MutView, bool> bind(key_t key, ValueLike&& value) {
    return bind_in_current_scope(std::move(key), std::forward<ValueLike>(value));
  }

  [[nodiscard]] bool erase_in_current_scope(const key_t& key) {
    return !scopes_.empty() && scopes_.back().erase(key);
  }

  [[nodiscard]] bool erase_in_current_scope_text_ids(const TextId* text_ids,
                                                     size_t count) {
    return !scopes_.empty() && scopes_.back().erase_sequence(text_ids, count);
  }

  [[nodiscard]] View probe_current_scope(const key_t& key) const {
    return scopes_.empty() ? View{} : scopes_.back().find(key);
  }

  [[nodiscard]] View probe_current_scope_text_ids(const TextId* text_ids,
                                                  size_t count) const {
    return scopes_.empty() ? View{} : scopes_.back().find_sequence(text_ids, count);
  }

  [[nodiscard]] View probe_current_scope_sequence(const TextId* text_ids,
                                                  size_t count) const {
    return probe_current_scope_text_ids(text_ids, count);
  }

  [[nodiscard]] View find_in_current_scope(const key_t& key) const {
    return probe_current_scope(key);
  }

  [[nodiscard]] View find_in_current_scope_text_ids(const TextId* text_ids,
                                                    size_t count) const {
    return probe_current_scope_text_ids(text_ids, count);
  }

  [[nodiscard]] View find_in_current_scope_sequence(const TextId* text_ids,
                                                    size_t count) const {
    return find_in_current_scope_text_ids(text_ids, count);
  }

  [[nodiscard]] bool contains_in_current_scope(const key_t& key) const {
    return probe_current_scope(key).found();
  }

  [[nodiscard]] bool contains_in_current_scope_text_ids(const TextId* text_ids,
                                                         size_t count) const {
    return probe_current_scope_text_ids(text_ids, count).found();
  }

  [[nodiscard]] View find_nearest_visible(const key_t& key) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (View found = it->find(key); found) return found;
    }
    return {};
  }

  [[nodiscard]] View find(const key_t& key) const { return find_nearest_visible(key); }

  [[nodiscard]] View find_nearest_visible_text_ids(const TextId* text_ids,
                                                   size_t count) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (View found = it->find_sequence(text_ids, count); found) return found;
    }
    return {};
  }

  [[nodiscard]] size_t find_nearest_visible_scope_index_text_ids(
      const TextId* text_ids,
      size_t count) const {
    for (size_t i = scopes_.size(); i > 0; --i) {
      if (scopes_[i - 1].find_sequence(text_ids, count)) return i - 1;
    }
    return scopes_.size();
  }

  [[nodiscard]] View find_nearest_visible_sequence(const TextId* text_ids,
                                                   size_t count) const {
    return find_nearest_visible_text_ids(text_ids, count);
  }

  [[nodiscard]] View find_sequence(const TextId* text_ids, size_t count) const {
    return find_nearest_visible_text_ids(text_ids, count);
  }

  [[nodiscard]] bool contains_visible(const key_t& key) const {
    return find_nearest_visible(key).found();
  }

  [[nodiscard]] bool contains_visible_text_ids(const TextId* text_ids,
                                               size_t count) const {
    return find_nearest_visible_text_ids(text_ids, count).found();
  }

  [[nodiscard]] MutView find_nearest_visible_mut(const key_t& key) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (MutView found = it->find_mut(key); found) return found;
    }
    return {};
  }

  [[nodiscard]] MutView find_mut(const key_t& key) {
    return find_nearest_visible_mut(key);
  }

  [[nodiscard]] MutView find_nearest_visible_mut_text_ids(const TextId* text_ids,
                                                          size_t count) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
      if (MutView found = it->find_mut_sequence(text_ids, count); found) return found;
    }
    return {};
  }

  [[nodiscard]] MutView find_nearest_visible_mut_sequence(const TextId* text_ids,
                                                          size_t count) {
    return find_nearest_visible_mut_text_ids(text_ids, count);
  }

  [[nodiscard]] MutView find_mut_sequence(const TextId* text_ids, size_t count) {
    return find_nearest_visible_mut_text_ids(text_ids, count);
  }

  [[nodiscard]] MutView find_mut_text_ids(const TextId* text_ids, size_t count) {
    return find_nearest_visible_mut_text_ids(text_ids, count);
  }

 private:
  Map& current_scope() {
    if (scopes_.empty()) push_scope();
    return scopes_.back();
  }

  std::vector<Map> scopes_;
};

}  // namespace c4c
