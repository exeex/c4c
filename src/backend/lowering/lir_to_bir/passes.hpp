#pragma once

#include "../lir_to_bir.hpp"
#include "../call_decode.hpp"

#include <initializer_list>
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
std::optional<bir::Module> try_lower_minimal_direct_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_void_direct_call_imm_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<std::vector<bir::Param>> lower_call_params_from_type_texts(
    const std::vector<std::string_view>& param_types);
std::optional<std::vector<bir::Param>> lower_function_params(
    const c4c::codegen::lir::LirFunction& lir_function);
bir::CallingConv default_calling_convention_for_target(std::string_view target_triple);
bool function_signature_is_variadic(std::string_view signature_text);
std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type);
std::optional<bir::TypeKind> lower_scalar_type_text(std::string_view text);
bool matches_minimal_i32_function_signature(
    const c4c::codegen::lir::LirFunction& function,
    std::initializer_list<std::string_view> signature_param_types);
bool lir_function_matches_minimal_no_param_integer_return(
    const c4c::codegen::lir::LirFunction& function,
    unsigned bit_width);
bool lir_function_returns_integer_width(
    const c4c::codegen::lir::LirFunction& function,
    unsigned bit_width);
std::optional<bir::Module> try_lower_minimal_declared_direct_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_direct_call_add_imm_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_direct_call_identity_arg_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_folded_two_arg_direct_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_repeated_zero_arg_call_compare_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_local_i32_inc_dec_compare_return_zero_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_local_i32_unary_not_minus_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_short_circuit_effect_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_dual_identity_direct_call_sub_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_call_crossing_direct_call_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_string_literal_strlen_sub_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_string_literal_char_sub_module(
    const c4c::codegen::lir::LirModule& module);
bir::CallInst make_direct_call_inst(std::string callee,
                                    bir::CallingConv calling_convention,
                                    bool is_variadic,
                                    bir::TypeKind return_type,
                                    std::string return_type_name,
                                    std::optional<bir::Value> result,
                                    std::vector<bir::Value> args);
std::optional<std::vector<bir::Value>> lower_direct_call_args(
    const std::vector<ParsedBackendExternCallArg>& args);
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
bool match_memory_global_base_gep_zero(const c4c::codegen::lir::LirGepOp& gep,
                                       std::string_view global_name,
                                       std::string_view global_llvm_type);
bool match_memory_string_base_gep_zero(const c4c::codegen::lir::LirGepOp& gep,
                                       std::string_view pool_name,
                                       std::string_view string_llvm_type);
std::optional<std::int64_t> match_memory_sext_i32_to_i64_immediate(
    const c4c::codegen::lir::LirCastOp& cast);
bool match_memory_indexed_gep_from_result(const c4c::codegen::lir::LirGepOp& gep,
                                          std::string_view expected_ptr,
                                          std::string_view expected_element_type,
                                          std::string_view expected_index_name);
std::optional<bir::Module> try_lower_minimal_global_char_pointer_diff_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_global_int_pointer_diff_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_scalar_global_load_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_extern_scalar_global_load_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_extern_global_array_load_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_scalar_global_store_reload_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_global_two_field_struct_store_sub_sub_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_local_i32_store_and_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_local_i32_store_xor_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_local_i32_store_or_sub_return_immediate_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_string_literal_compare_phi_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_local_i32_pointer_alias_compare_two_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_union_i32_alias_compare_three_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_nested_struct_i32_sum_compare_six_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_local_struct_shadow_store_compare_two_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_global_x_y_pointer_compare_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_local_i32_array_pointer_inc_dec_compare_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module>
try_lower_minimal_local_i32_array_pointer_add_deref_diff_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
std::optional<bir::Module> try_lower_minimal_sizeof_compare_chain_zero_return_module(
    const c4c::codegen::lir::LirModule& module);
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
bool legalize_lir_type_matches_integer_width(const c4c::codegen::lir::LirTypeRef& type,
                                             unsigned bit_width);
std::optional<bir::TypeKind> legalize_memory_value_type(
    const c4c::codegen::lir::LirTypeRef& type);
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
