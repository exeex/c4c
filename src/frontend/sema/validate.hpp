#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "ast.hpp"

namespace c4c::sema {

struct Diagnostic {
  const char* file = nullptr;
  int line = 0;
  int column = 1;
  std::string message;
};

struct ValidateResult {
  bool ok = true;
  std::vector<Diagnostic> diagnostics;
};

struct SemaStructuredNameKey {
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_text_ids;
  TextId base_text_id = kInvalidText;

  bool valid() const { return base_text_id != kInvalidText; }
  bool operator==(const SemaStructuredNameKey& other) const;
  bool operator!=(const SemaStructuredNameKey& other) const { return !(*this == other); }
};

struct SemaStructuredNameKeyHash {
  std::size_t operator()(const SemaStructuredNameKey& key) const;
};

enum class SemaDualLookupMatch {
  BothMissing,
  Match,
  LegacyOnly,
  StructuredOnly,
  Mismatch,
};

std::optional<SemaStructuredNameKey> sema_local_name_key(const Node* node);
std::optional<SemaStructuredNameKey> sema_structured_name_key(const Node* node);
SemaDualLookupMatch compare_sema_lookup_presence(bool legacy_found, bool structured_found);

template <typename T>
SemaDualLookupMatch compare_sema_lookup_ptrs(const T* legacy, const T* structured) {
  if (!legacy && !structured) return SemaDualLookupMatch::BothMissing;
  if (legacy && !structured) return SemaDualLookupMatch::LegacyOnly;
  if (!legacy && structured) return SemaDualLookupMatch::StructuredOnly;
  return legacy == structured ? SemaDualLookupMatch::Match : SemaDualLookupMatch::Mismatch;
}

ValidateResult validate_program(const Node* root);
void print_diagnostics(const std::vector<Diagnostic>& diags, const std::string& file);

}  // namespace c4c::sema
