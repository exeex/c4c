#include "passes.hpp"

#include "../../../codegen/lir/ir.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

namespace {

using c4c::codegen::lir::LirCallOp;
using c4c::codegen::lir::LirExtractValueOp;
using c4c::codegen::lir::LirInsertValueOp;
using c4c::codegen::lir::LirTypeRef;

enum class AggregateKind : unsigned char {
  ScalarLike,
  ArrayLike,
  StructLike,
  UnionLike,
  VectorLike,
  ByValBlob,
  HiddenSRetBlob,
  Opaque,
};

struct AggregatePathStep {
  enum class Kind : unsigned char {
    Field,
    Index,
  };

  Kind kind = Kind::Field;
  std::string text;
  std::size_t index = 0;
};

struct AggregateInitAtom {
  std::vector<AggregatePathStep> path;
  std::string rhs_text;
  bool is_zero_fill = false;
  bool is_address = false;
};

struct AggregateMaterializationPlan {
  AggregateKind kind = AggregateKind::Opaque;
  std::string type_text;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool is_union = false;
  bool is_byval = false;
  bool uses_hidden_sret = false;
  std::string storage_name;
  std::vector<AggregateInitAtom> atoms;
};

[[maybe_unused]] static std::string trim(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }
  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return std::string(text.substr(begin, end - begin));
}

[[maybe_unused]] static bool starts_with(std::string_view text, std::string_view prefix) {
  return text.size() >= prefix.size() && text.substr(0, prefix.size()) == prefix;
}

[[maybe_unused]] static bool contains_top_level(std::string_view text, char needle) {
  int depth = 0;
  for (char ch : text) {
    if (ch == '[' || ch == '{' || ch == '<' || ch == '(') {
      ++depth;
    } else if (ch == ']' || ch == '}' || ch == '>' || ch == ')') {
      depth = std::max(0, depth - 1);
    } else if (ch == needle && depth == 0) {
      return true;
    }
  }
  return false;
}

[[maybe_unused]] static bool looks_like_struct_or_union_type(std::string_view text) {
  const auto trimmed = trim(text);
  return starts_with(trimmed, "%struct.") || starts_with(trimmed, "%union.") ||
         starts_with(trimmed, "{") || starts_with(trimmed, "<{");
}

[[maybe_unused]] static bool looks_like_array_type(std::string_view text) {
  const auto trimmed = trim(text);
  return starts_with(trimmed, "[");
}

[[maybe_unused]] static bool looks_like_vector_type(std::string_view text) {
  const auto trimmed = trim(text);
  return starts_with(trimmed, "<") && !starts_with(trimmed, "<{");
}

[[maybe_unused]] static bool looks_like_aggregate_type(std::string_view text) {
  return looks_like_struct_or_union_type(text) || looks_like_array_type(text) ||
         looks_like_vector_type(text);
}

[[maybe_unused]] static AggregateKind classify_aggregate_type(std::string_view text) {
  const auto trimmed = trim(text);
  if (starts_with(trimmed, "%union.")) {
    return AggregateKind::UnionLike;
  }
  if (starts_with(trimmed, "%struct.") || starts_with(trimmed, "{") ||
      starts_with(trimmed, "<{")) {
    return AggregateKind::StructLike;
  }
  if (starts_with(trimmed, "[")) {
    return AggregateKind::ArrayLike;
  }
  if (starts_with(trimmed, "<") && !starts_with(trimmed, "<{")) {
    return AggregateKind::VectorLike;
  }
  return AggregateKind::ScalarLike;
}

[[maybe_unused]] static bir::Param make_byval_param(std::string name,
                                                    std::size_t size_bytes,
                                                    std::size_t align_bytes) {
  return bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = std::move(name),
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .is_varargs = false,
      .is_sret = false,
      .is_byval = true,
  };
}

[[maybe_unused]] static bir::Param make_sret_param(std::string name,
                                                   std::size_t size_bytes,
                                                   std::size_t align_bytes) {
  return bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = std::move(name),
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .is_varargs = false,
      .is_sret = true,
      .is_byval = false,
  };
}

[[maybe_unused]] static bir::LocalSlot make_aggregate_slot(std::string name,
                                                          std::size_t size_bytes,
                                                          std::size_t align_bytes,
                                                          bool is_byval_copy) {
  return bir::LocalSlot{
      .name = std::move(name),
      .type = bir::TypeKind::Ptr,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .is_address_taken = true,
      .is_byval_copy = is_byval_copy,
  };
}

[[maybe_unused]] static bir::CallArgAbiInfo make_byval_arg_abi(std::size_t size_bytes,
                                                              std::size_t align_bytes) {
  return bir::CallArgAbiInfo{
      .type = bir::TypeKind::Ptr,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .primary_class = bir::AbiValueClass::Memory,
      .secondary_class = bir::AbiValueClass::None,
      .passed_in_register = false,
      .passed_on_stack = true,
      .byval_copy = true,
  };
}

[[maybe_unused]] static bir::CallResultAbiInfo make_memory_call_result_abi(
    std::size_t /*size_bytes*/) {
  return bir::CallResultAbiInfo{
      .type = bir::TypeKind::Ptr,
      .primary_class = bir::AbiValueClass::Memory,
      .secondary_class = bir::AbiValueClass::None,
      .returned_in_memory = true,
  };
}

[[maybe_unused]] static AggregateMaterializationPlan make_seed_plan(std::string_view type_text,
                                                                    std::size_t size_bytes,
                                                                    std::size_t align_bytes) {
  AggregateMaterializationPlan plan;
  plan.kind = classify_aggregate_type(type_text);
  plan.type_text = std::string(type_text);
  plan.size_bytes = size_bytes;
  plan.align_bytes = align_bytes;
  plan.is_union = plan.kind == AggregateKind::UnionLike;
  return plan;
}

[[maybe_unused]] static std::string render_path(const std::vector<AggregatePathStep>& path) {
  std::string rendered;
  for (const auto& step : path) {
    if (!rendered.empty()) {
      rendered.push_back('.');
    }
    if (step.kind == AggregatePathStep::Kind::Index) {
      rendered.push_back('[');
      rendered += std::to_string(step.index);
      rendered.push_back(']');
    } else {
      rendered += step.text;
    }
  }
  return rendered;
}

[[maybe_unused]] static void append_field(AggregateMaterializationPlan* plan,
                                          std::string field_name,
                                          std::string rhs_text,
                                          bool is_zero_fill = false) {
  AggregateInitAtom atom;
  atom.path.push_back(AggregatePathStep{
      .kind = AggregatePathStep::Kind::Field,
      .text = std::move(field_name),
  });
  atom.rhs_text = std::move(rhs_text);
  atom.is_zero_fill = is_zero_fill;
  plan->atoms.push_back(std::move(atom));
}

[[maybe_unused]] static void append_index(AggregateMaterializationPlan* plan,
                                          std::size_t index,
                                          std::string rhs_text,
                                          bool is_zero_fill = false) {
  AggregateInitAtom atom;
  atom.path.push_back(AggregatePathStep{
      .kind = AggregatePathStep::Kind::Index,
      .index = index,
  });
  atom.rhs_text = std::move(rhs_text);
  atom.is_zero_fill = is_zero_fill;
  plan->atoms.push_back(std::move(atom));
}

}  // namespace

// -----------------------------------------------------------------------------
// Aggregate ABI scaffolding
// -----------------------------------------------------------------------------
//
// The reference Rust lowering splits aggregate handling across:
// - struct_init.rs: positional/designated/nested struct initialization
// - structs.rs: layout discovery and anonymous-member drilling
// - global_init_compound_struct.rs: relocation-aware compound global init
//
// Backend BIR cannot keep those aggregate trees verbatim, so this file keeps
// the surviving representation intentionally coarse:
// - byval/sret are lowered to pointer-shaped ABI markers
// - stack temporaries become blob-like local slots
// - nested initializer paths are flattened into path atoms
// - global/compound forms are recorded as materialization plans
//
// The functions below are a scaffold, not a finished integration point.

[[maybe_unused]] static AggregateMaterializationPlan lower_struct_form(
    std::string_view type_text,
    std::size_t size_bytes,
    std::size_t align_bytes,
    bool is_union) {
  auto plan = make_seed_plan(type_text, size_bytes, align_bytes);
  plan.kind = is_union ? AggregateKind::UnionLike : AggregateKind::StructLike;
  plan.is_union = is_union;
  return plan;
}

[[maybe_unused]] static AggregateMaterializationPlan lower_byval_form(std::string_view type_text,
                                                                      std::size_t size_bytes,
                                                                      std::size_t align_bytes,
                                                                      std::string_view storage_name) {
  auto plan = make_seed_plan(type_text, size_bytes, align_bytes);
  plan.kind = AggregateKind::ByValBlob;
  plan.is_byval = true;
  plan.storage_name = std::string(storage_name);
  return plan;
}

[[maybe_unused]] static AggregateMaterializationPlan lower_hidden_sret_form(
    std::string_view type_text,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::string_view storage_name) {
  auto plan = make_seed_plan(type_text, size_bytes, align_bytes);
  plan.kind = AggregateKind::HiddenSRetBlob;
  plan.uses_hidden_sret = true;
  plan.storage_name = std::string(storage_name);
  return plan;
}

[[maybe_unused]] static AggregateMaterializationPlan lower_compound_global_form(
    std::string_view type_text,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::string_view storage_name) {
  auto plan = make_seed_plan(type_text, size_bytes, align_bytes);
  plan.storage_name = std::string(storage_name);
  return plan;
}

[[maybe_unused]] static std::vector<bir::Param> build_aggregate_call_params(
    std::string_view type_text,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::string_view value_name) {
  std::vector<bir::Param> params;
  if (looks_like_aggregate_type(type_text)) {
    params.push_back(make_byval_param(std::string(value_name), size_bytes, align_bytes));
  } else {
    params.push_back(bir::Param{
        .type = bir::TypeKind::Ptr,
        .name = std::string(value_name),
        .size_bytes = size_bytes,
        .align_bytes = align_bytes,
        .is_varargs = false,
        .is_sret = false,
        .is_byval = false,
    });
  }
  return params;
}

[[maybe_unused]] static bir::CallArgAbiInfo build_aggregate_call_arg_abi(std::string_view type_text,
                                                                        std::size_t size_bytes,
                                                                        std::size_t align_bytes) {
  if (looks_like_aggregate_type(type_text)) {
    return make_byval_arg_abi(size_bytes, align_bytes);
  }
  return bir::CallArgAbiInfo{
      .type = bir::TypeKind::Ptr,
      .size_bytes = size_bytes,
      .align_bytes = align_bytes,
      .primary_class = bir::AbiValueClass::None,
      .secondary_class = bir::AbiValueClass::None,
      .passed_in_register = true,
      .passed_on_stack = false,
      .byval_copy = false,
  };
}

[[maybe_unused]] static bir::CallResultAbiInfo build_aggregate_call_result_abi(
    std::string_view type_text,
    std::size_t size_bytes) {
  if (looks_like_aggregate_type(type_text)) {
    return make_memory_call_result_abi(size_bytes);
  }
  return bir::CallResultAbiInfo{
      .type = bir::TypeKind::Ptr,
      .primary_class = bir::AbiValueClass::Integer,
      .secondary_class = bir::AbiValueClass::None,
      .returned_in_memory = false,
  };
}

[[maybe_unused]] static bir::LocalSlot build_aggregate_local_slot(std::string name,
                                                                  std::size_t size_bytes,
                                                                  std::size_t align_bytes) {
  return make_aggregate_slot(std::move(name), size_bytes, align_bytes, true);
}

[[maybe_unused]] static std::string lower_designator_path(
    const std::vector<AggregatePathStep>& path) {
  return render_path(path);
}

[[maybe_unused]] static void append_nested_designator(AggregateMaterializationPlan* plan,
                                                      std::string field_name,
                                                      std::string nested_path,
                                                      std::string rhs_text) {
  AggregateInitAtom atom;
  atom.path.push_back(AggregatePathStep{
      .kind = AggregatePathStep::Kind::Field,
      .text = std::move(field_name),
  });
  atom.path.push_back(AggregatePathStep{
      .kind = AggregatePathStep::Kind::Field,
      .text = std::move(nested_path),
  });
  atom.rhs_text = std::move(rhs_text);
  plan->atoms.push_back(std::move(atom));
}

[[maybe_unused]] static void append_array_designator(AggregateMaterializationPlan* plan,
                                                     std::size_t index,
                                                     std::string rhs_text) {
  append_index(plan, index, std::move(rhs_text));
}

// -----------------------------------------------------------------------------
// LIR aggregate placeholders
// -----------------------------------------------------------------------------
//
// The backend currently has structured LIR forms for extractvalue/insertvalue
// and call ABI metadata, but the aggregate lowering split is still a staging
// area. Keep the translation logic here so the eventual wiring in
// lir_to_bir.cpp can import a single aggregate-focused unit.

[[maybe_unused]] static AggregateMaterializationPlan lower_extractvalue_scaffold(
    const LirExtractValueOp& op,
    std::size_t aggregate_size_bytes,
    std::size_t aggregate_align_bytes) {
  auto plan = make_seed_plan(op.agg_type.str(), aggregate_size_bytes, aggregate_align_bytes);
  plan.storage_name = op.result.str();
  append_index(&plan, static_cast<std::size_t>(std::max(op.index, 0)), op.agg.str());
  return plan;
}

[[maybe_unused]] static AggregateMaterializationPlan lower_insertvalue_scaffold(
    const LirInsertValueOp& op,
    std::size_t aggregate_size_bytes,
    std::size_t aggregate_align_bytes) {
  auto plan = make_seed_plan(op.agg_type.str(), aggregate_size_bytes, aggregate_align_bytes);
  plan.storage_name = op.result.str();
  append_index(&plan, static_cast<std::size_t>(std::max(op.index, 0)), op.elem.str());
  return plan;
}

[[maybe_unused]] static AggregateMaterializationPlan lower_call_aggregate_scaffold(
    const LirCallOp& op,
    std::size_t aggregate_size_bytes,
    std::size_t aggregate_align_bytes) {
  auto plan = make_seed_plan(op.return_type.str(), aggregate_size_bytes, aggregate_align_bytes);
  if (looks_like_aggregate_type(op.return_type.str())) {
    plan.kind = AggregateKind::HiddenSRetBlob;
    plan.uses_hidden_sret = true;
    plan.storage_name = op.result.str();
  }
  return plan;
}

void record_aggregate_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                              std::vector<BirLoweringNote>* notes) {
  (void)module;
  if (notes == nullptr) {
    return;
  }
  notes->push_back(BirLoweringNote{
      .phase = "aggregate-lowering",
      .message = "aggregate/byval/sret lowering scaffold lives in lir_to_bir/aggregates.cpp",
  });
}

}  // namespace c4c::backend
