#pragma once

#include "bir.hpp"
#include "../../target_profile.hpp"
#include "../../codegen/lir/call_args_ops.hpp"
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
  bool preserve_dynamic_alloca = false;
  bool allow_bounded_pattern_folds = false;
};

struct BirLoweringNote {
  std::string phase;
  std::string message;
};

struct BirLoweringContext {
  const c4c::codegen::lir::LirModule& lir_module;
  c4c::TargetProfile target_profile;
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
using LocalIndirectPointerSlotSet = std::unordered_set<std::string>;

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

TypeDeclMap build_type_decl_map(const std::vector<std::string>& type_decls);
std::optional<std::int64_t> parse_i64(std::string_view text);
std::optional<bir::TypeKind> lower_integer_type(std::string_view text);
std::size_t type_size_bytes(bir::TypeKind type);
std::optional<bir::CallArgAbiInfo> compute_call_arg_abi(const c4c::TargetProfile& target_profile,
                                                        bir::TypeKind type);
std::optional<bir::CallResultAbiInfo> compute_function_return_abi(
    const c4c::TargetProfile& target_profile,
    bir::TypeKind type,
    bool returned_via_sret);
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

}  // namespace lir_to_bir_detail

class BirFunctionLowerer {
 public:
  struct ParsedFunctionSignatureParam {
    std::string type;
    std::string operand;
    bool is_varargs = false;
  };

  struct ParsedTypedCall {
    std::vector<std::string> owned_param_types;
    std::vector<std::string_view> param_types;
    std::vector<c4c::codegen::lir::LirTypedCallArgView> args;
    bool is_variadic = false;
  };

  struct ParsedDirectGlobalTypedCall {
    std::string_view symbol_name;
    ParsedTypedCall typed_call;
    bool is_variadic = false;
  };

  BirFunctionLowerer(BirLoweringContext& context,
                     const c4c::codegen::lir::LirFunction& function,
                     const lir_to_bir_detail::GlobalTypes& global_types,
                     const lir_to_bir_detail::FunctionSymbolSet& function_symbols,
                     const lir_to_bir_detail::TypeDeclMap& type_decls);

  std::optional<bir::Function> lower();

  static std::optional<ParsedTypedCall> parse_typed_call(
      const c4c::codegen::lir::LirCallOp& call);
  static std::optional<ParsedDirectGlobalTypedCall> parse_direct_global_typed_call(
      const c4c::codegen::lir::LirCallOp& call);
  static std::optional<std::vector<ParsedFunctionSignatureParam>> parse_function_signature_params(
      std::string_view signature_text);
  static std::optional<bir::Function> lower_extern_decl(
      const c4c::codegen::lir::LirExternDecl& decl,
      const c4c::TargetProfile& target_profile);
  static std::optional<bir::Function> lower_decl_function(
      const c4c::codegen::lir::LirFunction& function,
      const c4c::TargetProfile& target_profile);

  // Shared type buckets used by the split lowering translation units.
  using ValueMap = lir_to_bir_detail::ValueMap;
  using AggregateTypeLayout = lir_to_bir_detail::AggregateTypeLayout;
  using FunctionSymbolSet = lir_to_bir_detail::FunctionSymbolSet;
  using GlobalAddress = lir_to_bir_detail::GlobalAddress;
  using GlobalTypes = lir_to_bir_detail::GlobalTypes;
  using LocalIndirectPointerSlotSet = lir_to_bir_detail::LocalIndirectPointerSlotSet;
  using LocalPointerSlots = lir_to_bir_detail::LocalPointerSlots;
  using LocalSlotTypes = lir_to_bir_detail::LocalSlotTypes;
  using ParsedTypedOperand = lir_to_bir_detail::ParsedTypedOperand;
  using TypeDeclMap = lir_to_bir_detail::TypeDeclMap;

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
  struct LocalSlotAddress {
    std::string slot_name;
    bir::TypeKind value_type = bir::TypeKind::Void;
    std::int64_t byte_offset = 0;
    std::string storage_type_text;
    std::string type_text;
    std::vector<std::string> array_element_slots;
    std::size_t array_base_index = 0;
  };
  using LocalSlotAddressSlots = std::unordered_map<std::string, LocalSlotAddress>;
  using LocalSlotPointerValues = std::unordered_map<std::string, LocalSlotAddress>;
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

  struct DynamicPointerValueArrayAccess {
    bir::Value base_value;
    bir::TypeKind element_type = bir::TypeKind::Void;
    std::size_t byte_offset = 0;
    std::size_t element_count = 0;
    std::size_t element_stride_bytes = 0;
    bir::Value index;
  };

  using DynamicPointerValueArrayMap =
      std::unordered_map<std::string, DynamicPointerValueArrayAccess>;

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

  struct DynamicGlobalScalarArrayAccess {
    std::string global_name;
    bir::TypeKind element_type = bir::TypeKind::Void;
    std::size_t byte_offset = 0;
    std::size_t outer_element_count = 0;
    std::size_t outer_element_stride_bytes = 0;
    bir::Value outer_index;
    std::size_t element_count = 0;
    std::size_t element_stride_bytes = 0;
    bir::Value index;
  };

  using DynamicGlobalScalarArrayMap =
      std::unordered_map<std::string, DynamicGlobalScalarArrayAccess>;

  struct LocalAggregateSlots {
    std::string storage_type_text;
    std::string type_text;
    std::size_t base_byte_offset = 0;
    std::unordered_map<std::size_t, std::string> leaf_slots;
  };

  using LocalAggregateSlotMap = std::unordered_map<std::string, LocalAggregateSlots>;
  using LocalAggregateFieldSet = std::unordered_set<std::string>;
  using LocalPointerValueAliasMap = std::unordered_map<std::string, bir::Value>;
  struct PointerAddress {
    bir::Value base_value;
    bir::TypeKind value_type = bir::TypeKind::Void;
    std::size_t byte_offset = 0;
    std::string storage_type_text;
    std::string type_text;
  };

  using PointerAddressMap = std::unordered_map<std::string, PointerAddress>;

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
    std::optional<bir::CallResultAbiInfo> abi;
    bool returned_via_sret = false;
  };

  struct AggregateArrayExtent {
    std::size_t element_count = 0;
    std::size_t element_stride_bytes = 0;
  };

  struct LocalAggregateGepTarget {
    std::string type_text;
    std::int64_t byte_offset = 0;
  };

 private:
  // Scalar lowering helpers.
  static std::optional<AggregateTypeLayout> lower_byval_aggregate_layout(
      std::string_view text,
      const TypeDeclMap& type_decls);
  static std::string aggregate_param_slot_base(std::string_view param_name);
  static std::optional<bir::Value> lower_value(const c4c::codegen::lir::LirOperand& operand,
                                               bir::TypeKind expected_type,
                                               const ValueMap& value_aliases);
  static std::optional<bir::TypeKind> lower_scalar_or_function_pointer_type(
      std::string_view text);
  static std::optional<bir::TypeKind> lower_minimal_scalar_type(const c4c::TypeSpec& type);
  static std::optional<unsigned> integer_type_bit_width(bir::TypeKind type);
  static std::uint64_t integer_bit_mask(unsigned bits);
  static std::int64_t sign_extend_bits(std::uint64_t value, unsigned bits);
  static std::optional<bir::Value> make_integer_immediate(bir::TypeKind type,
                                                          std::int64_t value);
  static bool is_canonical_select_chain_binop(bir::BinaryOpcode opcode);
  static std::optional<bir::CastOpcode> lower_cast_opcode(
      c4c::codegen::lir::LirCastKind kind);
  static std::optional<bir::BinaryOpcode> lower_scalar_binary_opcode(
      const c4c::codegen::lir::LirBinaryOpcodeRef& opcode);
  static std::optional<bir::Value> fold_i64_binary_immediates(bir::BinaryOpcode opcode,
                                                              std::int64_t lhs,
                                                              std::int64_t rhs);
  static std::optional<bir::BinaryOpcode> lower_cmp_predicate(
      const c4c::codegen::lir::LirCmpPredicateRef& predicate);
  std::optional<bir::Value> lower_value(const c4c::codegen::lir::LirOperand& operand,
                                        bir::TypeKind expected_type) const;
  static std::optional<bir::Value> fold_integer_cast(c4c::codegen::lir::LirCastKind kind,
                                                     const bir::Value& operand,
                                                     bir::TypeKind to_type);
  bool lower_scalar_compare_inst(const c4c::codegen::lir::LirInst& inst,
                                 ValueMap& value_aliases,
                                 CompareMap& compare_exprs,
                                 std::vector<bir::Inst>* lowered_insts) const;
  bool resolve_select_chain_inst(const c4c::codegen::lir::LirInst& inst,
                                 ValueMap& value_aliases,
                                 CompareMap& compare_exprs,
                                 std::vector<bir::Inst>* lowered_insts) const;
  bool lower_canonical_select_entry_inst(const c4c::codegen::lir::LirInst& inst,
                                         ValueMap& value_aliases,
                                         CompareMap& compare_exprs,
                                         std::vector<bir::Inst>* lowered_insts) const;

  // Calling and signature helpers.
  static bool is_void_param_sentinel(const c4c::TypeSpec& type);
  static std::optional<bir::TypeKind> lower_param_type(const c4c::TypeSpec& type);
  static std::optional<LoweredReturnInfo> lower_return_info_from_type(
      std::string_view type_text,
      const TypeDeclMap& type_decls,
      const c4c::TargetProfile& target_profile);
  std::optional<LoweredReturnInfo> infer_function_return_info() const;
  static std::optional<LoweredReturnInfo> lower_signature_return_info(
      std::string_view signature_text,
      const TypeDeclMap& type_decls,
      const c4c::TargetProfile& target_profile);
  static bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                                    const c4c::TargetProfile& target_profile,
                                    const std::optional<LoweredReturnInfo>& return_info,
                                    const TypeDeclMap& type_decls,
                                    bir::Function* lowered);
  void note_function_lowering_family_failure(std::string_view family);
  void note_semantic_call_family_failure(std::string_view family);
  void note_runtime_intrinsic_family_failure(std::string_view family);
  bool lower_runtime_intrinsic_inst(const c4c::codegen::lir::LirInst& inst,
                                    const ValueMap& value_aliases,
                                    std::vector<bir::Inst>* lowered_insts);

  // Aggregate lowering helpers.
  std::vector<std::pair<std::size_t, std::string>> collect_sorted_leaf_slots(
      const LocalAggregateSlots& aggregate_slots) const;
  AggregateParamMap collect_aggregate_params() const;
  bool append_local_aggregate_scalar_slots(std::string_view type_text,
                                           std::string_view slot_prefix,
                                           std::size_t byte_offset,
                                           std::size_t align_bytes,
                                           LocalAggregateSlots* aggregate_slots);
  bool declare_local_aggregate_slots(std::string_view type_text,
                                     std::string_view slot_name,
                                     std::size_t align_bytes);
  bool append_local_aggregate_copy_from_slots(const LocalAggregateSlots& source_slots,
                                              const LocalAggregateSlots& target_slots,
                                              std::string_view temp_prefix,
                                              std::vector<bir::Inst>* lowered_insts) const;
  bool append_local_aggregate_copy_to_pointer(const LocalAggregateSlots& source_slots,
                                              const bir::Value& target_pointer,
                                              std::size_t target_align_bytes,
                                              std::string_view temp_prefix,
                                              std::vector<bir::Inst>* lowered_insts) const;
  bool materialize_aggregate_param_aliases(std::vector<bir::Inst>* lowered_insts);
  void seed_pointer_param_addresses();

  // CFG and phi helpers.
  BlockLookup make_block_lookup() const;
  static std::optional<BranchChain> follow_empty_branch_chain(const BlockLookup& blocks,
                                                              const std::string& start_label);
  static std::optional<BranchChain> follow_canonical_select_chain(
      const BlockLookup& blocks,
      const std::string& start_label);
  std::optional<PhiBlockPlanMap> collect_phi_lowering_plans() const;

  // Local/global memory helpers.
  static bool is_local_array_element_slot(std::string_view slot_name,
                                          const LocalArraySlotMap& local_array_slots);
  static std::optional<std::pair<std::size_t, bir::TypeKind>> parse_local_array_type(
      std::string_view text);
  static std::optional<bir::Value> lower_zero_initializer_value(bir::TypeKind type);
  static std::optional<bir::Value> lower_repeated_byte_initializer_value(
      bir::TypeKind type,
      std::uint8_t fill_byte);
  static std::optional<bir::Value> lower_typed_index_value(const ParsedTypedOperand& index_operand,
                                                           const ValueMap& value_aliases);
  static std::optional<bir::Value> make_index_immediate(bir::TypeKind type,
                                                        std::size_t value);
  static std::optional<bir::Value> synthesize_value_array_selects(
      std::string_view result_name,
      const std::vector<bir::Value>& element_values,
      const bir::Value& index_value,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bir::Value> synthesize_pointer_array_selects(
      std::string_view result_name,
      const std::vector<bir::Value>& element_values,
      const bir::Value& index_value,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<std::vector<bir::Value>> collect_local_pointer_values(
      const std::vector<std::string>& element_slots,
      const LocalPointerValueAliasMap& local_pointer_value_aliases);
  static std::optional<std::vector<bir::Value>> collect_global_array_pointer_values(
      const DynamicGlobalPointerArrayAccess& access,
      const GlobalTypes& global_types);
  static std::optional<AggregateArrayExtent> find_repeated_aggregate_extent_at_offset(
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
  static void record_pointer_global_object_alias(
      std::string_view result_name,
      const lir_to_bir_detail::GlobalInfo& global_info,
      const GlobalTypes& global_types,
      GlobalObjectPointerMap& global_object_pointer_slots);
  static std::optional<GlobalAddress> resolve_pointer_store_address(
      const c4c::codegen::lir::LirOperand& operand,
      const GlobalPointerMap& global_pointer_slots,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols);
  static std::optional<std::string> resolve_local_aggregate_gep_slot(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<std::vector<std::string>> resolve_local_aggregate_pointer_array_slots(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalPointerArrayAccess>
  resolve_local_aggregate_dynamic_pointer_array_access(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<LocalAggregateGepTarget> resolve_local_aggregate_gep_target(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalAggregateArrayAccess> resolve_local_dynamic_aggregate_array_access(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<bir::Value> resolve_local_aggregate_pointer_value_alias(
      const c4c::codegen::lir::LirOperand& operand,
      const ValueMap& value_aliases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      const FunctionSymbolSet& function_symbols);
  static std::optional<bir::Value> lower_call_pointer_arg_value(
      const c4c::codegen::lir::LirOperand& operand,
      const ValueMap& value_aliases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols);
  static std::optional<GlobalAddress> resolve_honest_pointer_base(
      const GlobalAddress& address,
      const GlobalTypes& global_types);
  static std::optional<GlobalAddress> resolve_honest_addressed_global_access(
      const GlobalAddress& address,
      bir::TypeKind accessed_type,
      const GlobalTypes& global_types);
  static GlobalPointerSlotKey make_global_pointer_slot_key(const GlobalAddress& address);
  bool ensure_local_scratch_slot(std::string_view slot_name,
                                 bir::TypeKind type,
                                 std::size_t align_bytes);

  // Function assembly flow.
  bool lower_scalar_or_local_memory_inst(const c4c::codegen::lir::LirInst& inst,
                                         std::vector<bir::Inst>* lowered_insts);

  std::optional<bir::Function> try_lower_canonical_select_function();
  std::optional<bir::Value> lower_select_chain_value(const BlockLookup& blocks,
                                                     const BranchChain& chain,
                                                     const c4c::codegen::lir::LirOperand& incoming,
                                                     bir::TypeKind expected_type,
                                                     const ValueMap& value_aliases,
                                                     std::vector<bir::Inst>* lowered_insts) const;
  bool canonicalize_compare_return_alias(const c4c::codegen::lir::LirOperand& ret_value,
                                         const bir::Value& lowered_value,
                                         bir::TypeKind return_type,
                                         std::vector<bir::Inst>* lowered_insts,
                                         bir::ReturnTerminator* lowered_ret) const;
  bool lower_alloca_insts();
  bool lower_block(const c4c::codegen::lir::LirBlock& block,
                   bool* emitted_hoisted_alloca_scratch);
  bool lower_block_phi_insts(const c4c::codegen::lir::LirBlock& block,
                             bir::Block* lowered_block);
  bool lower_block_insts(const c4c::codegen::lir::LirBlock& block,
                         bir::Block* lowered_block);
  bool lower_block_terminator(const c4c::codegen::lir::LirBlock& block,
                              bir::Block* lowered_block);

  // Lowering state.
  BirLoweringContext& context_;
  const c4c::codegen::lir::LirFunction& function_;
  const GlobalTypes& global_types_;
  const FunctionSymbolSet& function_symbols_;
  const TypeDeclMap& type_decls_;

  bir::Function lowered_function_;
  std::optional<LoweredReturnInfo> return_info_;
  PhiBlockPlanMap phi_plans_;
  AggregateParamMap aggregate_params_;
  ValueMap value_aliases_;
  CompareMap compare_exprs_;
  AggregateValueAliasMap aggregate_value_aliases_;
  LocalSlotTypes local_slot_types_;
  LocalPointerSlots local_pointer_slots_;
  LocalArraySlotMap local_array_slots_;
  LocalPointerArrayBaseMap local_pointer_array_bases_;
  DynamicLocalPointerArrayMap dynamic_local_pointer_arrays_;
  DynamicLocalAggregateArrayMap dynamic_local_aggregate_arrays_;
  DynamicPointerValueArrayMap dynamic_pointer_value_arrays_;
  LocalAggregateSlotMap local_aggregate_slots_;
  LocalAggregateFieldSet local_aggregate_field_slots_;
  LocalPointerValueAliasMap local_pointer_value_aliases_;
  LocalIndirectPointerSlotSet local_indirect_pointer_slots_;
  PointerAddressMap pointer_value_addresses_;
  PointerAddressMap local_pointer_slot_addresses_;
  LocalAddressSlots local_address_slots_;
  LocalSlotAddressSlots local_slot_address_slots_;
  LocalSlotPointerValues local_slot_pointer_values_;
  GlobalAddressSlots global_address_slots_;
  AddressedGlobalPointerSlots addressed_global_pointer_slots_;
  GlobalPointerMap global_pointer_slots_;
  DynamicGlobalPointerArrayMap dynamic_global_pointer_arrays_;
  DynamicGlobalAggregateArrayMap dynamic_global_aggregate_arrays_;
  DynamicGlobalScalarArrayMap dynamic_global_scalar_arrays_;
  GlobalObjectPointerMap global_object_pointer_slots_;
  GlobalAddressIntMap global_address_ints_;
  GlobalObjectAddressIntMap global_object_address_ints_;
  std::vector<bir::Inst> hoisted_alloca_scratch_;
};

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options);
std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
