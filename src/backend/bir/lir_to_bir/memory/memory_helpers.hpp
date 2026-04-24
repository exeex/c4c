#if !defined(C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS) && \
    !defined(C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_SCALAR_FACTS_DECL)
#define C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_SCALAR_FACTS_DECL

#include "../lowering.hpp"

namespace c4c::backend {

struct ScalarLayoutLeafFacts {
  std::string type_text;
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t byte_offset = 0;
};

struct ScalarLayoutByteOffsetFacts {
  std::size_t object_size_bytes = 0;
  std::optional<ScalarLayoutLeafFacts> leaf;
};

std::optional<ScalarLayoutByteOffsetFacts> resolve_scalar_layout_facts_at_byte_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const BirFunctionLowerer::TypeDeclMap& type_decls);

}  // namespace c4c::backend

#endif  // C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_SCALAR_FACTS_DECL

#ifndef C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_INDEX_HPP
#define C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_INDEX_HPP

#if !defined(C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS)
#include "../lowering.hpp"
#endif

// Private implementation index for pure memory layout and projection helpers.
//
// This is the shared declaration surface for memory helpers reused across
// `memory/` implementation files. `lowering.hpp` remains the complete private
// lowerer index and direct state owner; this header should not grow stateful
// policy, per-file helper families, or implementation control flow.

#endif  // C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_INDEX_HPP

#if defined(C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS) && \
    !defined(C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS_INCLUDED)
#define C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS_INCLUDED

// Shared pure layout and address-projection helpers.
static bool can_reinterpret_byte_storage_view(std::string_view storage_type_text,
                                              std::string_view target_type_text,
                                              const TypeDeclMap& type_decls);
static std::optional<AggregateArrayExtent> find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls);
static std::optional<AggregateArrayExtent> find_nested_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls);
static std::optional<LocalAggregateGepTarget> resolve_relative_gep_target(
    std::string_view type_text,
    std::int64_t base_byte_offset,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls);
static std::optional<std::size_t> find_pointer_array_length_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const TypeDeclMap& type_decls);
static std::optional<GlobalAddress> resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls);
static std::optional<GlobalAddress> resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls);
static std::optional<DynamicGlobalPointerArrayAccess> resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls);
static std::optional<DynamicGlobalAggregateArrayAccess>
resolve_global_dynamic_aggregate_array_access(const GlobalAddress& base_address,
                                              std::string_view base_type_text,
                                              const c4c::codegen::lir::LirGepOp& gep,
                                              const ValueMap& value_aliases,
                                              const GlobalTypes& global_types,
                                              const TypeDeclMap& type_decls);

#endif  // C4C_BACKEND_BIR_LIR_TO_BIR_MEMORY_HELPERS_MEMBERS_INCLUDED
