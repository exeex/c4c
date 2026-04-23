#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "id_hash.hpp"
#include "sequence_map.hpp"
#include "text_id_table.hpp"

namespace c4c {

using NamePathId = uint32_t;
constexpr NamePathId kInvalidNamePath = 0;

// Interned storage for qualifier segments such as `ns1::ns2`.
// This stays purely TextId-oriented so parser and HIR can share the same path
// identity without flattening names into persistent strings.
class NamePathTable {
 public:
  using Key = std::vector<TextId>;
  using Map = SequenceMap<Key, NamePathId>;
  struct View {
    const TextId* data = nullptr;
    size_t size = 0;

    [[nodiscard]] bool empty() const { return size == 0; }
    [[nodiscard]] const TextId& operator[](size_t index) const { return data[index]; }
  };

  [[nodiscard]] NamePathId find(const TextId* segments, size_t count) const {
    if (Map::View found = map_.find_sequence(segments, count); found) return *found.value;
    return kInvalidNamePath;
  }

  [[nodiscard]] NamePathId find(const std::vector<TextId>& segments) const {
    if (Map::View found = map_.find(segments); found) return *found.value;
    return kInvalidNamePath;
  }

  [[nodiscard]] NamePathId intern(const TextId* segments, size_t count) {
    if (!segments || count == 0) return kInvalidNamePath;
    if (Map::View found = map_.find_sequence(segments, count); found) return *found.value;
    const NamePathId id = static_cast<NamePathId>(keys_by_id_.size() + 1);
    Key key(segments, segments + count);
    auto inserted = map_.insert(key, id);
    if (inserted.second) keys_by_id_.push_back(std::move(key));
    return id;
  }

  [[nodiscard]] NamePathId intern(const std::vector<TextId>& segments) {
    return intern(segments.data(), segments.size());
  }

  [[nodiscard]] NamePathId append(NamePathId prefix_id, TextId segment) {
    if (segment == kInvalidText) return prefix_id;
    if (prefix_id == kInvalidNamePath) return intern(&segment, 1);

    const View prefix = lookup(prefix_id);
    std::vector<TextId> combined;
    combined.reserve(prefix.size + 1);
    for (size_t i = 0; i < prefix.size; ++i) combined.push_back(prefix.data[i]);
    combined.push_back(segment);
    return intern(combined);
  }

  [[nodiscard]] View lookup(NamePathId id) const {
    if (id == kInvalidNamePath || static_cast<size_t>(id) > keys_by_id_.size()) return {};
    const Key& key = keys_by_id_[id - 1];
    return View{key.data(), key.size()};
  }

  [[nodiscard]] std::string render(NamePathId id,
                                   const TextTable& texts) const {
    const View path = lookup(id);
    std::string rendered;
    for (size_t i = 0; i < path.size; ++i) {
      if (i != 0) rendered += "::";
      const std::string_view segment = texts.lookup(path.data[i]);
      if (!segment.empty()) {
        rendered.append(segment);
      } else {
        rendered += "<?>";
      }
    }
    return rendered;
  }

  [[nodiscard]] size_t size() const { return keys_by_id_.size(); }

 private:
  Map map_;
  std::vector<Key> keys_by_id_;
};

struct QualifiedNameKey {
  int context_id = -1;
  bool is_global_qualified = false;
  NamePathId qualifier_path_id = kInvalidNamePath;
  TextId base_text_id = kInvalidText;

  [[nodiscard]] bool operator==(const QualifiedNameKey& other) const {
    return context_id == other.context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_path_id == other.qualifier_path_id &&
           base_text_id == other.base_text_id;
  }
};

struct QualifiedNameKeyHash {
  [[nodiscard]] size_t operator()(const QualifiedNameKey& key) const {
    return static_cast<size_t>(hash_id_words(
        kIdHashSeed, static_cast<uint32_t>(key.context_id), key.qualifier_path_id,
        key.base_text_id, static_cast<uint32_t>(key.is_global_qualified)));
  }
};

[[nodiscard]] inline std::string render_qualified_name(
    const QualifiedNameKey& key,
    const NamePathTable& paths,
    const TextTable& texts) {
  std::string rendered;
  if (key.is_global_qualified) rendered = "::";

  const std::string qualifiers = paths.render(key.qualifier_path_id, texts);
  if (!qualifiers.empty()) {
    rendered += qualifiers;
    if (key.base_text_id != kInvalidText) rendered += "::";
  }

  const std::string_view base = texts.lookup(key.base_text_id);
  if (!base.empty()) {
    rendered.append(base);
  } else if (key.base_text_id != kInvalidText) {
    rendered += "<?>";
  } else if (rendered.empty()) {
    rendered = "<?>";
  }

  return rendered;
}

[[nodiscard]] inline std::string render_name_path(const TextId* segments,
                                                  size_t count,
                                                  const TextTable& texts) {
  std::string rendered;
  for (size_t i = 0; i < count; ++i) {
    if (i != 0) rendered += "::";
    const std::string_view segment =
        segments ? texts.lookup(segments[i]) : std::string_view{};
    rendered += segment.empty() ? "<?>" : std::string(segment);
  }
  return rendered;
}

[[nodiscard]] inline std::string render_qualified_name(
    TextId base_text_id,
    const TextId* qualifier_ids,
    size_t qualifier_count,
    bool is_global_qualified,
    const TextTable& texts) {
  std::string rendered;
  if (is_global_qualified) rendered = "::";

  const std::string qualifiers =
      render_name_path(qualifier_ids, qualifier_count, texts);
  if (!qualifiers.empty()) {
    rendered += qualifiers;
    if (base_text_id != kInvalidText) rendered += "::";
  }

  const std::string_view base = texts.lookup(base_text_id);
  if (!base.empty()) {
    rendered.append(base);
  } else if (base_text_id != kInvalidText) {
    rendered += "<?>";
  } else if (rendered.empty()) {
    rendered = "<?>";
  }

  return rendered;
}

[[nodiscard]] inline std::string render_qualified_name(
    const char* base_name,
    const char* const* qualifier_segments,
    size_t qualifier_count,
    bool is_global_qualified) {
  std::string rendered;
  if (is_global_qualified) rendered = "::";

  for (size_t i = 0; i < qualifier_count; ++i) {
    const char* segment = qualifier_segments ? qualifier_segments[i] : nullptr;
    if (!rendered.empty() && rendered != "::") rendered += "::";
    rendered += (segment && segment[0]) ? segment : "<?>";
  }

  if (base_name && base_name[0]) {
    if (!rendered.empty() && rendered != "::") rendered += "::";
    rendered += base_name;
  } else if (rendered.empty()) {
    rendered = "<?>";
  } else if (rendered == "::") {
    rendered += "<?>";
  }

  return rendered;
}

struct QualifiedNameScopeInfo {
  bool has_scope_separator = false;
  bool is_global_qualified = false;
  TextId owner_text_id = kInvalidText;
  TextId base_text_id = kInvalidText;
};

[[nodiscard]] inline bool has_qualified_name_scope(
    TextId qualified_name_id,
    const TextTable& texts) {
  const std::string_view name = texts.lookup(qualified_name_id);
  return name.rfind("::") != std::string_view::npos;
}

[[nodiscard]] inline QualifiedNameScopeInfo split_qualified_name_scope(
    TextId qualified_name_id, TextTable& texts) {
  QualifiedNameScopeInfo info;
  const std::string_view name = texts.lookup(qualified_name_id);
  if (name.empty()) return info;

  info.is_global_qualified = name.size() >= 2 && name[0] == ':' &&
                             name[1] == ':';

  const size_t sep = name.rfind("::");
  if (sep == std::string_view::npos) return info;

  info.has_scope_separator = true;
  if (sep > 0) {
    info.owner_text_id = texts.intern(name.substr(0, sep));
  }
  if (sep + 2 < name.size()) {
    info.base_text_id = texts.intern(name.substr(sep + 2));
  }
  return info;
}

[[nodiscard]] inline bool qualified_name_base_matches(
    TextId qualified_name_id, TextId expected_base_id, TextTable& texts) {
  if (qualified_name_id == kInvalidText || expected_base_id == kInvalidText) {
    return false;
  }
  if (qualified_name_id == expected_base_id) return true;
  return split_qualified_name_scope(qualified_name_id, texts).base_text_id ==
         expected_base_id;
}

[[nodiscard]] inline TextId strip_text_id_suffix(TextId text_id,
                                                 std::string_view suffix,
                                                 TextTable& texts) {
  if (text_id == kInvalidText || suffix.empty()) return kInvalidText;
  const std::string_view text = texts.lookup(text_id);
  if (text.size() <= suffix.size() ||
      text.substr(text.size() - suffix.size()) != suffix) {
    return kInvalidText;
  }
  return texts.intern(text.substr(0, text.size() - suffix.size()));
}

// Generic structured-name lookup storage shared by parser/HIR-facing clients.
// The table is intentionally payload-agnostic: callers decide whether the
// mapped value is a lightweight classification, a symbol id, or a richer node.
template <typename Payload>
class QualifiedNameLookupTable {
 public:
  [[nodiscard]] const Payload* find(const QualifiedNameKey& key) const {
    const auto it = values_.find(key);
    return it == values_.end() ? nullptr : &it->second;
  }

  [[nodiscard]] Payload* find(const QualifiedNameKey& key) {
    const auto it = values_.find(key);
    return it == values_.end() ? nullptr : &it->second;
  }

  [[nodiscard]] bool contains(const QualifiedNameKey& key) const {
    return values_.find(key) != values_.end();
  }

  void insert_or_assign(const QualifiedNameKey& key, Payload payload) {
    values_.insert_or_assign(key, std::move(payload));
  }

  template <typename... Args>
  Payload& get_or_emplace(const QualifiedNameKey& key, Args&&... args) {
    auto [it, inserted] =
        values_.try_emplace(key, std::forward<Args>(args)...);
    if (!inserted && sizeof...(Args) != 0) {
      it->second = Payload(std::forward<Args>(args)...);
    }
    return it->second;
  }

  [[nodiscard]] size_t size() const { return values_.size(); }

 private:
  std::unordered_map<QualifiedNameKey, Payload, QualifiedNameKeyHash> values_;
};

}  // namespace c4c
