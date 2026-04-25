#pragma once

#include "../lowering.hpp"

// Private implementation index for pure memory layout and projection helpers.
//
// This is the shared declaration surface for memory helpers reused across
// `memory/` implementation files. `lowering.hpp` remains the complete private
// lowerer index and direct state owner; this header should not grow stateful
// policy, per-file helper families, or implementation control flow.

namespace c4c::backend {

struct ScalarLayoutLeafFacts {
  std::string type_text;
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t byte_offset = 0;
};

struct ScalarLayoutByteOffsetFacts {
  std::size_t object_size_bytes = 0;
  std::size_t target_byte_offset = 0;
  std::size_t remaining_object_bytes = 0;
  std::optional<ScalarLayoutLeafFacts> leaf;
};

struct AggregateByteOffsetProjection {
  enum class Kind {
    ArrayElement,
    StructField,
  };

  Kind kind = Kind::ArrayElement;
  BirFunctionLowerer::AggregateTypeLayout layout;
  BirFunctionLowerer::AggregateTypeLayout child_layout;
  std::string child_type_text;
  std::size_t child_index = 0;
  std::size_t byte_offset_within_child = 0;
  std::size_t target_byte_offset = 0;
  std::size_t child_start_byte_offset = 0;
  std::size_t child_stride_bytes = 0;
};

std::optional<ScalarLayoutByteOffsetFacts> resolve_scalar_layout_facts_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls);

std::optional<AggregateByteOffsetProjection> resolve_aggregate_byte_offset_projection(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls);

std::optional<AggregateByteOffsetProjection> resolve_aggregate_byte_offset_projection(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts);

std::optional<AggregateByteOffsetProjection> resolve_aggregate_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls);

std::optional<AggregateByteOffsetProjection> resolve_aggregate_child_index_projection(
    std::string_view type_text,
    std::size_t child_index,
    const BirFunctionLowerer::TypeDeclMap& type_decls,
    const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts);

bool can_reinterpret_byte_storage_as_type(
    std::string_view storage_type_text,
    std::size_t target_byte_offset,
    std::string_view target_type_text,
    const BirFunctionLowerer::TypeDeclMap& type_decls);

}  // namespace c4c::backend
