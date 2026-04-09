#pragma once

#include "../lir_to_bir.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend {

std::size_t normalize_cfg_in_place(c4c::codegen::lir::LirModule& module,
                                   const BirLoweringOptions& options,
                                   std::vector<BirLoweringNote>* notes = nullptr);
void lower_phi_nodes_in_place(c4c::codegen::lir::LirModule& module,
                              std::vector<BirLoweringNote>* notes = nullptr);
void record_memory_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                           std::vector<BirLoweringNote>* notes = nullptr);
void record_call_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                         std::vector<BirLoweringNote>* notes = nullptr);
void record_aggregate_lowering_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                              std::vector<BirLoweringNote>* notes = nullptr);
bir::LocalSlot make_memory_local_slot(std::string name,
                                      bir::TypeKind type,
                                      std::size_t size_bytes,
                                      std::size_t align_bytes = 0,
                                      bool is_address_taken = false);
bir::MemoryAddress make_memory_local_address(std::string slot_name,
                                             std::size_t byte_offset = 0,
                                             std::size_t align_bytes = 0);
bir::MemoryAddress make_memory_global_address(std::string global_name,
                                              std::size_t byte_offset = 0,
                                              std::size_t align_bytes = 0);
bir::MemoryAddress make_memory_pointer_address(bir::Value base_value,
                                               std::int64_t byte_offset = 0,
                                               std::size_t align_bytes = 0);
bir::LoadLocalInst make_memory_load_local(bir::Value result,
                                          std::string slot_name,
                                          std::size_t byte_offset = 0,
                                          std::size_t align_bytes = 0);
bir::StoreLocalInst make_memory_store_local(std::string slot_name,
                                            bir::Value value,
                                            std::size_t byte_offset = 0,
                                            std::size_t align_bytes = 0);
bir::LoadGlobalInst make_memory_load_global(bir::Value result,
                                            std::string global_name,
                                            std::size_t byte_offset = 0,
                                            std::size_t align_bytes = 0);
bir::StoreGlobalInst make_memory_store_global(std::string global_name,
                                              bir::Value value,
                                              std::size_t byte_offset = 0,
                                              std::size_t align_bytes = 0);

namespace lir_to_bir {

std::optional<bir::TypeKind> legalize_function_decl_return_type(
    const c4c::codegen::lir::LirFunction& function);
std::optional<bir::TypeKind> legalize_extern_decl_return_type(
    const c4c::codegen::lir::LirExternDecl& decl);
std::optional<bir::TypeKind> legalize_function_return_type(
    const c4c::codegen::lir::LirFunction& function,
    const c4c::codegen::lir::LirRet& ret);
std::optional<bir::TypeKind> legalize_global_type(const c4c::codegen::lir::LirGlobal& global);
std::optional<bir::TypeKind> legalize_call_arg_type(std::string_view text);
bool legalize_lir_type_is_pointer_like(const c4c::codegen::lir::LirTypeRef& type);
bool legalize_lir_type_is_i32(const c4c::codegen::lir::LirTypeRef& type);
std::size_t legalize_type_size_bytes(bir::TypeKind type);
std::size_t legalize_type_align_bytes(bir::TypeKind type);
bool legalize_types_compatible(bir::TypeKind lhs, bir::TypeKind rhs);
bool legalize_immediate_fits_type(std::int64_t value, bir::TypeKind type);
std::optional<bir::Value> legalize_immediate_or_name(std::string_view value_text,
                                                     bir::TypeKind type);
void record_type_legalization_scaffold_notes(const c4c::codegen::lir::LirModule& module,
                                             std::vector<BirLoweringNote>* notes = nullptr);

}  // namespace lir_to_bir

}  // namespace c4c::backend
