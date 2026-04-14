#pragma once

#include "../bir.hpp"
#include "../../codegen/lir/ir.hpp"

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend {

struct BirLoweringOptions {
  bool normalize_cfg = true;
  bool lower_phi = true;
  bool lower_memory = true;
  bool lower_calls = true;
  bool legalize_types = true;
  bool lower_aggregates = true;
  bool allow_bounded_pattern_folds = false;
};

struct BirLoweringNote {
  std::string phase;
  std::string message;
};

struct BirLoweringContext {
  const c4c::codegen::lir::LirModule& lir_module;
  BirLoweringOptions options;
  std::vector<BirLoweringNote> notes;

  void note(std::string phase, std::string message);
};

BirLoweringContext make_lowering_context(const c4c::codegen::lir::LirModule& module,
                                         const BirLoweringOptions& options);

struct BirFunctionPreScan {
  std::string function_name;
  std::size_t block_count = 0;
  std::size_t instruction_count = 0;
  bool has_calls = false;
  bool has_memory_ops = false;
  bool has_control_flow = false;
};

struct BirModuleAnalysis {
  std::size_t function_count = 0;
  std::size_t global_count = 0;
  std::size_t string_constant_count = 0;
  std::size_t extern_decl_count = 0;
  bool has_calls = false;
  bool has_memory_ops = false;
  bool has_control_flow = false;
  std::vector<BirFunctionPreScan> functions;
};

BirModuleAnalysis analyze_module(BirLoweringContext& context);

std::optional<bir::Module> lower_module(BirLoweringContext& context,
                                        const BirModuleAnalysis& analysis);

struct BirLoweringResult {
  std::optional<bir::Module> module;
  BirModuleAnalysis analysis;
  std::vector<BirLoweringNote> notes;
};

namespace lir_to_bir_detail {

using ValueMap = std::unordered_map<std::string, bir::Value>;

struct GlobalAddress {
  std::string global_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
};

struct GlobalInfo {
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t element_size_bytes = 0;
  std::size_t element_count = 0;
  std::size_t storage_size_bytes = 0;
  bool supports_direct_value = false;
  bool supports_linear_addressing = false;
  std::string type_text;
  std::optional<GlobalAddress> known_global_address;
  std::string initializer_symbol_name;
  bir::TypeKind initializer_offset_type = bir::TypeKind::Void;
  std::size_t initializer_byte_offset = 0;
  std::unordered_map<std::size_t, GlobalAddress> pointer_initializer_offsets;
};

using GlobalTypes = std::unordered_map<std::string, GlobalInfo>;
using TypeDeclMap = std::unordered_map<std::string, std::string>;
using FunctionSymbolSet = std::unordered_set<std::string>;
using LocalSlotTypes = std::unordered_map<std::string, bir::TypeKind>;
using LocalPointerSlots = std::unordered_map<std::string, std::string>;

struct ParsedTypedOperand {
  std::string type_text;
  c4c::codegen::lir::LirOperand operand;
};

struct AggregateField {
  std::size_t byte_offset = 0;
  std::string type_text;
};

struct AggregateTypeLayout {
  enum class Kind : unsigned char {
    Invalid,
    Scalar,
    Array,
    Struct,
  };

  Kind kind = Kind::Invalid;
  bir::TypeKind scalar_type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::size_t array_count = 0;
  std::string element_type_text;
  std::vector<AggregateField> fields;
};

struct GlobalPointerSlotKey {
  std::string global_name;
  std::size_t byte_offset = 0;

  bool operator==(const GlobalPointerSlotKey& other) const {
    return global_name == other.global_name && byte_offset == other.byte_offset;
  }
};

struct GlobalPointerSlotKeyHash {
  std::size_t operator()(const GlobalPointerSlotKey& key) const {
    return std::hash<std::string>{}(key.global_name) ^
           (std::hash<std::size_t>{}(key.byte_offset) << 1);
  }
};

using GlobalPointerMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalObjectPointerMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalAddressIntMap = std::unordered_map<std::string, GlobalAddress>;
using GlobalObjectAddressIntMap = std::unordered_map<std::string, GlobalAddress>;
using LocalAddressSlots = std::unordered_map<std::string, GlobalAddress>;
using GlobalAddressSlots = std::unordered_map<std::string, std::optional<GlobalAddress>>;
using AddressedGlobalPointerSlots =
    std::unordered_map<GlobalPointerSlotKey,
                       std::optional<GlobalAddress>,
                       GlobalPointerSlotKeyHash>;

struct LocalArraySlots {
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::vector<std::string> element_slots;
};

using LocalArraySlotMap = std::unordered_map<std::string, LocalArraySlots>;

struct DynamicLocalPointerArrayAccess {
  std::vector<std::string> element_slots;
  bir::Value index;
};

using DynamicLocalPointerArrayMap =
    std::unordered_map<std::string, DynamicLocalPointerArrayAccess>;

struct DynamicLocalAggregateArrayAccess {
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  std::unordered_map<std::size_t, std::string> leaf_slots;
  bir::Value index;
};

using DynamicLocalAggregateArrayMap =
    std::unordered_map<std::string, DynamicLocalAggregateArrayAccess>;

struct LocalPointerArrayBase {
  std::vector<std::string> element_slots;
  std::size_t base_index = 0;
};

using LocalPointerArrayBaseMap = std::unordered_map<std::string, LocalPointerArrayBase>;

struct DynamicGlobalPointerArrayAccess {
  std::string global_name;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

using DynamicGlobalPointerArrayMap =
    std::unordered_map<std::string, DynamicGlobalPointerArrayAccess>;

struct DynamicGlobalAggregateArrayAccess {
  std::string global_name;
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

using DynamicGlobalAggregateArrayMap =
    std::unordered_map<std::string, DynamicGlobalAggregateArrayAccess>;

struct LocalAggregateSlots {
  std::string storage_type_text;
  std::string type_text;
  std::size_t base_byte_offset = 0;
  std::unordered_map<std::size_t, std::string> leaf_slots;
};

using LocalAggregateSlotMap = std::unordered_map<std::string, LocalAggregateSlots>;
using LocalAggregateFieldSet = std::unordered_set<std::string>;
using LocalPointerValueAliasMap = std::unordered_map<std::string, bir::Value>;

struct CompareExpr {
  bir::BinaryOpcode opcode = bir::BinaryOpcode::Eq;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::Value lhs;
  bir::Value rhs;
};

using CompareMap = std::unordered_map<std::string, CompareExpr>;
using BlockLookup = std::unordered_map<std::string, const c4c::codegen::lir::LirBlock*>;
using AggregateValueAliasMap = std::unordered_map<std::string, std::string>;

struct BranchChain {
  std::vector<std::string> labels;
  std::string leaf_label;
  std::string join_label;
};

struct PhiLoweringPlan {
  std::string result_name;
  bir::TypeKind type = bir::TypeKind::Void;
  std::vector<std::pair<std::string, c4c::codegen::lir::LirOperand>> incomings;
};

using PhiBlockPlanMap = std::unordered_map<std::string, std::vector<PhiLoweringPlan>>;

struct AggregateParamInfo {
  std::string type_text;
  AggregateTypeLayout layout;
};

using AggregateParamMap = std::unordered_map<std::string, AggregateParamInfo>;

struct LoweredReturnInfo {
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool returned_via_sret = false;
};

TypeDeclMap build_type_decl_map(const std::vector<std::string>& type_decls);
std::optional<std::int64_t> parse_i64(std::string_view text);
std::optional<bir::TypeKind> lower_integer_type(std::string_view text);
std::size_t type_size_bytes(bir::TypeKind type);
std::vector<std::string_view> split_top_level_initializer_items(std::string_view text);
std::optional<ParsedTypedOperand> parse_typed_operand(std::string_view text);
std::optional<std::int64_t> resolve_index_operand(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases);
AggregateTypeLayout compute_aggregate_type_layout(std::string_view text,
                                                  const TypeDeclMap& type_decls);

std::optional<bir::Global> lower_minimal_global(const c4c::codegen::lir::LirGlobal& global,
                                                const TypeDeclMap& type_decls,
                                                GlobalInfo* info);
std::optional<bir::Global> lower_string_constant_global(
    const c4c::codegen::lir::LirStringConst& string_constant,
    GlobalInfo* info);
bool resolve_pointer_initializer_offsets(GlobalTypes& global_types,
                                         const FunctionSymbolSet& function_symbols);
std::optional<GlobalAddress> resolve_known_global_address(
    std::string_view global_name,
    GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    std::unordered_set<std::string>* active);
bool is_known_function_symbol(std::string_view symbol_name,
                              const FunctionSymbolSet& function_symbols);
std::optional<AggregateTypeLayout> lower_byval_aggregate_layout(
    std::string_view text,
    const TypeDeclMap& type_decls);
std::vector<std::pair<std::size_t, std::string>> collect_sorted_leaf_slots(
    const LocalAggregateSlots& aggregate_slots);
AggregateParamMap collect_aggregate_params(const c4c::codegen::lir::LirFunction& function,
                                           const TypeDeclMap& type_decls);
std::optional<bir::TypeKind> lower_scalar_or_function_pointer_type(std::string_view text);
std::optional<bir::CastOpcode> lower_cast_opcode(c4c::codegen::lir::LirCastKind kind);
std::optional<bir::BinaryOpcode> lower_scalar_binary_opcode(
    const c4c::codegen::lir::LirBinaryOpcodeRef& opcode);
std::optional<bir::Value> fold_i64_binary_immediates(bir::BinaryOpcode opcode,
                                                     std::int64_t lhs,
                                                     std::int64_t rhs);
std::optional<bir::BinaryOpcode> lower_cmp_predicate(
    const c4c::codegen::lir::LirCmpPredicateRef& predicate);
std::optional<bir::Value> lower_value(const c4c::codegen::lir::LirOperand& operand,
                                      bir::TypeKind expected_type,
                                      const ValueMap& value_aliases);
std::optional<bir::Value> fold_integer_cast(c4c::codegen::lir::LirCastKind kind,
                                            const bir::Value& operand,
                                            bir::TypeKind to_type);
bool lower_scalar_compare_inst(const c4c::codegen::lir::LirInst& inst,
                               ValueMap& value_aliases,
                               CompareMap& compare_exprs,
                               std::vector<bir::Inst>* lowered_insts);
bool resolve_select_chain_inst(const c4c::codegen::lir::LirInst& inst,
                               ValueMap& value_aliases,
                               CompareMap& compare_exprs,
                               std::vector<bir::Inst>* lowered_insts);
bool lower_canonical_select_entry_inst(const c4c::codegen::lir::LirInst& inst,
                                       ValueMap& value_aliases,
                                       CompareMap& compare_exprs,
                                       std::vector<bir::Inst>* lowered_insts);
std::optional<bir::TypeKind> lower_param_type(const c4c::TypeSpec& type);
std::optional<LoweredReturnInfo> lower_return_info_from_type(std::string_view type_text,
                                                             const TypeDeclMap& type_decls);
std::optional<LoweredReturnInfo> infer_function_return_info(
    const c4c::codegen::lir::LirFunction& function,
    const TypeDeclMap& type_decls);
std::optional<LoweredReturnInfo> lower_signature_return_info(std::string_view signature_text,
                                                             const TypeDeclMap& type_decls);
std::optional<bir::Function> lower_extern_decl(const c4c::codegen::lir::LirExternDecl& decl);
bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                           const std::optional<LoweredReturnInfo>& return_info,
                           const TypeDeclMap& type_decls,
                           bir::Function* lowered);
std::optional<bir::Function> lower_decl_function(const c4c::codegen::lir::LirFunction& function);
bool append_local_aggregate_scalar_slots(std::string_view type_text,
                                         std::string_view slot_prefix,
                                         std::size_t byte_offset,
                                         std::size_t align_bytes,
                                         const TypeDeclMap& type_decls,
                                         LocalSlotTypes& local_slot_types,
                                         LocalPointerSlots& local_pointer_slots,
                                         LocalAggregateFieldSet& local_aggregate_field_slots,
                                         bir::Function* lowered_function,
                                         LocalAggregateSlots* aggregate_slots);
bool declare_local_aggregate_slots(std::string_view type_text,
                                   std::string_view slot_name,
                                   std::size_t align_bytes,
                                   const TypeDeclMap& type_decls,
                                   LocalSlotTypes& local_slot_types,
                                   LocalPointerSlots& local_pointer_slots,
                                   LocalAggregateFieldSet& local_aggregate_field_slots,
                                   bir::Function* lowered_function,
                                   LocalAggregateSlotMap& local_aggregate_slots);
bool append_local_aggregate_copy_from_slots(
    const LocalAggregateSlots& source_slots,
    const LocalAggregateSlots& target_slots,
    const LocalSlotTypes& local_slot_types,
    std::string_view temp_prefix,
    std::vector<bir::Inst>* lowered_insts);
bool append_local_aggregate_copy_to_pointer(const LocalAggregateSlots& source_slots,
                                            const LocalSlotTypes& local_slot_types,
                                            const bir::Value& target_pointer,
                                            std::size_t target_align_bytes,
                                            std::string_view temp_prefix,
                                            std::vector<bir::Inst>* lowered_insts);
bool materialize_aggregate_param_aliases(const AggregateParamMap& aggregate_params,
                                         const TypeDeclMap& type_decls,
                                         LocalSlotTypes& local_slot_types,
                                         LocalPointerSlots& local_pointer_slots,
                                         LocalAggregateFieldSet& local_aggregate_field_slots,
                                         AggregateValueAliasMap& aggregate_value_aliases,
                                         bir::Function* lowered_function,
                                         LocalAggregateSlotMap& local_aggregate_slots,
                                         std::vector<bir::Inst>* lowered_insts);
bool lower_scalar_or_local_memory_inst(
    const c4c::codegen::lir::LirInst& inst,
    ValueMap& value_aliases,
    CompareMap& compare_exprs,
    AggregateValueAliasMap& aggregate_value_aliases,
    LocalSlotTypes& local_slot_types,
    LocalPointerSlots& local_pointer_slots,
    LocalArraySlotMap& local_array_slots,
    LocalPointerArrayBaseMap& local_pointer_array_bases,
    DynamicLocalPointerArrayMap& dynamic_local_pointer_arrays,
    DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
    LocalAggregateSlotMap& local_aggregate_slots,
    LocalAggregateFieldSet& local_aggregate_field_slots,
    LocalPointerValueAliasMap& local_pointer_value_aliases,
    LocalAddressSlots& local_address_slots,
    GlobalAddressSlots& global_address_slots,
    AddressedGlobalPointerSlots& addressed_global_pointer_slots,
    GlobalPointerMap& global_pointer_slots,
    DynamicGlobalPointerArrayMap& dynamic_global_pointer_arrays,
    DynamicGlobalAggregateArrayMap& dynamic_global_aggregate_arrays,
    GlobalObjectPointerMap& global_object_pointer_slots,
    GlobalAddressIntMap& global_address_ints,
    GlobalObjectAddressIntMap& global_object_address_ints,
    const AggregateParamMap& aggregate_params,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols,
    const TypeDeclMap& type_decls,
    bir::Function* lowered_function,
    std::vector<bir::Inst>* lowered_insts);
BlockLookup make_block_lookup(const c4c::codegen::lir::LirFunction& function);
std::optional<BranchChain> follow_empty_branch_chain(const BlockLookup& blocks,
                                                     const std::string& start_label);
std::optional<BranchChain> follow_canonical_select_chain(const BlockLookup& blocks,
                                                         const std::string& start_label);
std::optional<PhiBlockPlanMap> collect_phi_lowering_plans(
    const c4c::codegen::lir::LirFunction& function);

}  // namespace lir_to_bir_detail

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options);
std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
