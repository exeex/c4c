#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c {

using TextId = uint32_t;
using LinkNameId = uint32_t;
using MemberSymbolId = uint32_t;

constexpr TextId kInvalidText = 0;
constexpr LinkNameId kInvalidLinkName = 0;
constexpr MemberSymbolId kInvalidMemberSymbol = 0;

// Generic stable-id table keyed by a caller-provided value type.
template <typename Id, Id InvalidId, typename Key>
struct KeyIdTable {
  Id find(const Key& key) const {
    if (key == Key{}) return InvalidId;
    const auto it = id_by_key_.find(key);
    return it == id_by_key_.end() ? InvalidId : it->second;
  }

  Id intern(const Key& key) {
    const Id existing = find(key);
    if (existing != InvalidId) return existing;
    if (key == Key{}) return InvalidId;
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
struct StringIdTable {
  struct Slice {
    size_t offset = 0;
    size_t len = 0;
  };

  Id find(std::string_view text) const {
    if (text.empty()) return InvalidId;
    const auto hash = std::hash<std::string_view>{}(text);
    const auto bucket_it = ids_by_hash_.find(hash);
    if (bucket_it == ids_by_hash_.end()) return InvalidId;
    for (const Id id : bucket_it->second) {
      if (lookup(id) == text) return id;
    }
    return InvalidId;
  }

  Id intern(std::string_view text) {
    if (text.empty()) return InvalidId;
    const Id existing = find(text);
    if (existing != InvalidId) return existing;

    const size_t offset = bytes_.size();
    bytes_.insert(bytes_.end(), text.begin(), text.end());
    slices_by_id_.push_back(Slice{offset, text.size()});

    const Id id = static_cast<Id>(slices_by_id_.size());
    ids_by_hash_[std::hash<std::string_view>{}(text)].push_back(id);
    return id;
  }

  std::string_view lookup(Id id) const {
    if (id == InvalidId || id > slices_by_id_.size()) return {};
    const Slice& slice = slices_by_id_[id - 1];
    return std::string_view(bytes_.data() + slice.offset, slice.len);
  }

  size_t size() const { return slices_by_id_.size(); }

  std::vector<char> bytes_;
  std::vector<Slice> slices_by_id_;
  std::unordered_map<size_t, std::vector<Id>> ids_by_hash_;
};

// Immutable string storage for paths / file labels. Unlike StringIdTable, this
// preserves empty strings so callers can still attach a stable "unknown file"
// identity without inventing a placeholder spelling.
template <typename Id, Id InvalidId>
struct PathIdTable : KeyIdTable<Id, InvalidId, std::string> {
  Id intern(std::string_view text) {
    const Id existing = this->find(std::string(text));
    if (existing != InvalidId) return existing;
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
    return link_name_ids_.find(text_id);
  }

  LinkNameId find(std::string_view text) const {
    if (!texts_ || text.empty()) return kInvalidLinkName;
    return find(texts_->find(text));
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

// Struct/union member semantic identity layer. Member symbols are not
// link-visible, but still need a stable semantic id once a concrete owner tag
// is known. Like LinkNameTable, this reuses TextTable storage.
struct MemberSymbolTable {
  explicit MemberSymbolTable(TextTable* texts = nullptr) : texts_(texts) {}

  void attach_text_table(TextTable* texts) { texts_ = texts; }

  MemberSymbolId find(TextId text_id) const {
    if (!texts_ || text_id == kInvalidText) return kInvalidMemberSymbol;
    return member_symbol_ids_.find(text_id);
  }

  MemberSymbolId find(std::string_view text) const {
    if (!texts_ || text.empty()) return kInvalidMemberSymbol;
    return find(texts_->find(text));
  }

  MemberSymbolId intern(TextId text_id) {
    if (!texts_ || text_id == kInvalidText) return kInvalidMemberSymbol;
    return member_symbol_ids_.intern(text_id);
  }

  MemberSymbolId intern(std::string_view semantic_name) {
    if (!texts_) return kInvalidMemberSymbol;
    return intern(texts_->intern(semantic_name));
  }

  TextId text_id(MemberSymbolId id) const {
    const TextId* stored = member_symbol_ids_.lookup(id);
    return stored ? *stored : kInvalidText;
  }

  std::string_view spelling(MemberSymbolId id) const {
    if (!texts_) return {};
    return texts_->lookup(text_id(id));
  }

  size_t size() const { return member_symbol_ids_.size(); }

  TextTable* texts_ = nullptr;
  KeyIdTable<MemberSymbolId, kInvalidMemberSymbol, TextId> member_symbol_ids_;
};

}  // namespace c4c
