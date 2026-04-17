#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c {

using TextId = uint32_t;
using LinkNameId = uint32_t;

constexpr TextId kInvalidText = 0;
constexpr LinkNameId kInvalidLinkName = 0;

// Generic stable-id table keyed by a caller-provided value type.
template <typename Id, Id InvalidId, typename Key>
struct KeyIdTable {
  Id intern(const Key& key) {
    if (key == Key{}) return InvalidId;
    const auto it = id_by_key_.find(key);
    if (it != id_by_key_.end()) return it->second;
    key_by_id_.push_back(key);
    const Id id = static_cast<Id>(key_by_id_.size());
    id_by_key_.emplace(key_by_id_.back(), id);
    return id;
  }

  const Key* lookup(Id id) const {
    if (id == InvalidId || id > key_by_id_.size()) return nullptr;
    return &key_by_id_[id - 1];
  }

  size_t size() const { return key_by_id_.size(); }

  std::vector<Key> key_by_id_;
  std::unordered_map<Key, Id> id_by_key_;
};

// Immutable string storage keyed by a caller-provided id type. This owns the
// underlying strings so higher-level tables can reuse them without maintaining
// duplicate spelling storage.
template <typename Id, Id InvalidId>
struct StringIdTable : KeyIdTable<Id, InvalidId, std::string> {
  Id intern(std::string_view text) {
    if (text.empty()) return InvalidId;
    return KeyIdTable<Id, InvalidId, std::string>::intern(std::string(text));
  }

  std::string_view lookup(Id id) const {
    const std::string* stored =
        KeyIdTable<Id, InvalidId, std::string>::lookup(id);
    return stored ? std::string_view(*stored) : std::string_view{};
  }
};

// Immutable string storage for paths / file labels. Unlike StringIdTable, this
// preserves empty strings so callers can still attach a stable "unknown file"
// identity without inventing a placeholder spelling.
template <typename Id, Id InvalidId>
struct PathIdTable : KeyIdTable<Id, InvalidId, std::string> {
  Id intern(std::string_view text) {
    const auto it = this->id_by_key_.find(std::string(text));
    if (it != this->id_by_key_.end()) return it->second;
    this->key_by_id_.push_back(std::string(text));
    const Id id = static_cast<Id>(this->key_by_id_.size());
    this->id_by_key_.emplace(this->key_by_id_.back(), id);
    return id;
  }

  std::string_view lookup(Id id) const {
    const std::string* stored =
        KeyIdTable<Id, InvalidId, std::string>::lookup(id);
    return stored ? std::string_view(*stored) : std::string_view{};
  }
};

using TextTable = StringIdTable<TextId, kInvalidText>;

// Final logical symbol identity layer. Link names reuse TextTable storage
// rather than owning a second copy of the spelling bytes.
struct LinkNameTable {
  explicit LinkNameTable(TextTable* texts = nullptr) : texts_(texts) {}

  void attach_text_table(TextTable* texts) { texts_ = texts; }

  LinkNameId find(TextId text_id) const {
    if (!texts_ || text_id == kInvalidText) return kInvalidLinkName;
    const auto it = link_name_ids_.id_by_key_.find(text_id);
    return it == link_name_ids_.id_by_key_.end() ? kInvalidLinkName
                                                 : it->second;
  }

  LinkNameId find(std::string_view text) const {
    if (!texts_ || text.empty()) return kInvalidLinkName;
    const auto text_it = texts_->id_by_key_.find(std::string(text));
    if (text_it == texts_->id_by_key_.end()) return kInvalidLinkName;
    return find(text_it->second);
  }

  LinkNameId intern(TextId text_id) {
    if (!texts_ || text_id == kInvalidText) return kInvalidLinkName;
    return link_name_ids_.intern(text_id);
  }

  LinkNameId intern(std::string_view logical_name) {
    if (!texts_) return kInvalidLinkName;
    return intern(texts_->intern(logical_name));
  }

  TextId text_id(LinkNameId id) const {
    const TextId* stored = link_name_ids_.lookup(id);
    return stored ? *stored : kInvalidText;
  }

  std::string_view spelling(LinkNameId id) const {
    if (!texts_) return {};
    return texts_->lookup(text_id(id));
  }

  size_t size() const { return link_name_ids_.size(); }

  TextTable* texts_ = nullptr;
  KeyIdTable<LinkNameId, kInvalidLinkName, TextId> link_name_ids_;
};

}  // namespace c4c
