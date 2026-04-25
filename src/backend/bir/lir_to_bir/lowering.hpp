#pragma once

#include "../lir_to_bir.hpp"
#include "../../../target_profile.hpp"
#include "../../../codegen/lir/call_args_ops.hpp"
#include "memory/memory_types.hpp"

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

struct BirLoweringContext {
  const c4c::codegen::lir::LirModule& lir_module;
  c4c::TargetProfile target_profile;
  BirLoweringOptions options;
  std::vector<BirLoweringNote> notes;

  void note(std::string phase, std::string message);
};

BirLoweringContext make_lowering_context(const c4c::codegen::lir::LirModule& module,
                                         const BirLoweringOptions& options);

BirModuleAnalysis analyze_module(BirLoweringContext& context);

std::optional<bir::Module> lower_module(BirLoweringContext& context,
                                        const BirModuleAnalysis& analysis);

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
  std::size_t runtime_element_count = 0;
  std::size_t runtime_element_stride_bytes = 0;
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

struct BackendStructuredLayoutEntry {
  std::string type_name;
  AggregateTypeLayout structured_layout;
  AggregateTypeLayout legacy_layout;
  bool legacy_found = false;
  bool structured_found = false;
  bool parity_checked = false;
  bool parity_matches = false;
};

using BackendStructuredLayoutTable = std::unordered_map<std::string, BackendStructuredLayoutEntry>;

struct IntegerArrayType {
  std::vector<std::size_t> extents;
  bir::TypeKind element_type = bir::TypeKind::Void;
};

TypeDeclMap build_type_decl_map(const std::vector<std::string>& type_decls);
BackendStructuredLayoutTable build_backend_structured_layout_table(
    const std::vector<c4c::codegen::lir::LirStructDecl>& struct_decls,
    const c4c::StructNameTable& struct_names,
    const TypeDeclMap& legacy_type_decls);
void report_backend_structured_layout_parity_notes(
    BirLoweringContext& context,
    const BackendStructuredLayoutTable& structured_layouts);
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
AggregateTypeLayout lookup_backend_aggregate_type_layout(
    std::string_view text,
    const TypeDeclMap& type_decls,
    const BackendStructuredLayoutTable& structured_layouts);
std::optional<IntegerArrayType> parse_integer_array_type(std::string_view text);
std::optional<GlobalAddress> parse_global_address_initializer(std::string_view text,
                                                              const TypeDeclMap& type_decls);
std::optional<bir::Value> lower_global_initializer(std::string_view text,
                                                   bir::TypeKind type);
std::optional<std::vector<bir::Value>> lower_integer_array_initializer(
    std::string_view init_text,
    std::string_view type_text);
std::string_view strip_typed_initializer_prefix(std::string_view init_text,
                                                std::string_view expected_type_text);
std::optional<std::vector<bir::Value>> lower_aggregate_initializer(
    std::string_view init_text,
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    std::unordered_map<std::size_t, GlobalAddress>* pointer_offsets);

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
                     const lir_to_bir_detail::TypeDeclMap& type_decls,
                     const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts);

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

  using GlobalPointerSlotKey = c4c::backend::GlobalPointerSlotKey;
  using GlobalPointerSlotKeyHash = c4c::backend::GlobalPointerSlotKeyHash;

  using GlobalPointerMap = c4c::backend::GlobalPointerMap;
  using GlobalObjectPointerMap = c4c::backend::GlobalObjectPointerMap;
  using GlobalAddressIntMap = c4c::backend::GlobalAddressIntMap;
  using GlobalObjectAddressIntMap = c4c::backend::GlobalObjectAddressIntMap;
  using LocalAddressSlots = c4c::backend::LocalAddressSlots;
  using LocalSlotAddress = c4c::backend::LocalSlotAddress;
  using LocalSlotAddressSlots = c4c::backend::LocalSlotAddressSlots;
  using LocalSlotPointerValues = c4c::backend::LocalSlotPointerValues;
  using GlobalAddressSlots = c4c::backend::GlobalAddressSlots;
  using AddressedGlobalPointerSlots = c4c::backend::AddressedGlobalPointerSlots;

  using LocalArraySlots = c4c::backend::LocalArraySlots;

  using LocalArraySlotMap = c4c::backend::LocalArraySlotMap;

  using DynamicLocalPointerArrayAccess = c4c::backend::DynamicLocalPointerArrayAccess;

  using DynamicLocalPointerArrayMap = c4c::backend::DynamicLocalPointerArrayMap;

  using DynamicLocalAggregateArrayAccess = c4c::backend::DynamicLocalAggregateArrayAccess;

  using DynamicLocalAggregateArrayMap = c4c::backend::DynamicLocalAggregateArrayMap;

  using DynamicPointerValueArrayAccess = c4c::backend::DynamicPointerValueArrayAccess;

  using DynamicPointerValueArrayMap = c4c::backend::DynamicPointerValueArrayMap;

  using LocalPointerArrayBase = c4c::backend::LocalPointerArrayBase;

  using LocalPointerArrayBaseMap = c4c::backend::LocalPointerArrayBaseMap;

  using DynamicGlobalPointerArrayAccess = c4c::backend::DynamicGlobalPointerArrayAccess;

  using DynamicGlobalPointerArrayMap = c4c::backend::DynamicGlobalPointerArrayMap;

  using DynamicGlobalAggregateArrayAccess = c4c::backend::DynamicGlobalAggregateArrayAccess;

  using DynamicGlobalAggregateArrayMap = c4c::backend::DynamicGlobalAggregateArrayMap;

  using DynamicGlobalScalarArrayAccess = c4c::backend::DynamicGlobalScalarArrayAccess;

  using DynamicGlobalScalarArrayMap = c4c::backend::DynamicGlobalScalarArrayMap;

  using LocalAggregateSlots = c4c::backend::LocalAggregateSlots;

  using LocalAggregateSlotMap = c4c::backend::LocalAggregateSlotMap;
  using LocalAggregateFieldSet = c4c::backend::LocalAggregateFieldSet;
  using LocalPointerValueAliasMap = c4c::backend::LocalPointerValueAliasMap;
  using PointerAddress = c4c::backend::PointerAddress;

  using PointerAddressMap = c4c::backend::PointerAddressMap;
  using PointerAddressIntMap = c4c::backend::PointerAddressIntMap;
  using GlobalPointerValueSlots = c4c::backend::GlobalPointerValueSlots;
  using AddressedGlobalPointerValueSlots = c4c::backend::AddressedGlobalPointerValueSlots;

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
    enum class Kind : unsigned char {
      ScalarValue,
      AggregateValue,
    };

    Kind kind = Kind::ScalarValue;
    std::string result_name;
    bir::TypeKind type = bir::TypeKind::Void;
    std::string type_text;
    std::size_t aggregate_align_bytes = 0;
    std::vector<std::pair<std::string, c4c::codegen::lir::LirOperand>> incomings;
  };

  using PhiBlockPlanMap = std::unordered_map<std::string, std::vector<PhiLoweringPlan>>;

  struct PendingAggregatePhiCopy {
    c4c::codegen::lir::LirOperand source;
    std::string target_slot_name;
    std::string temp_prefix;
  };

  using PendingAggregatePhiCopyMap =
      std::unordered_map<std::string, std::vector<PendingAggregatePhiCopy>>;

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

  using AggregateArrayExtent = c4c::backend::AggregateArrayExtent;
  using LocalAggregateGepTarget = c4c::backend::LocalAggregateGepTarget;

  enum class LocalSlotLoadResult {
    NotHandled,
    Lowered,
    Failed,
  };

  enum class LocalSlotStoreResult {
    NotHandled,
    Lowered,
    Failed,
  };

 private:
  // Scalar lowering helpers.
  static std::optional<AggregateTypeLayout> lower_byval_aggregate_layout(
      std::string_view text,
      const TypeDeclMap& type_decls);
  static std::optional<AggregateTypeLayout> lower_byval_aggregate_layout(
      std::string_view text,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
  static std::optional<AggregateTypeLayout> lower_intrinsic_aggregate_layout(
      std::string_view text,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
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
  static std::optional<std::pair<bir::Value, bir::Value>> lower_scalar_binop_operands(
      const c4c::codegen::lir::LirBinOp& bin,
      bir::TypeKind value_type,
      const ValueMap& value_aliases);
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
  std::optional<bool> lower_scalar_family_inst(const c4c::codegen::lir::LirInst& inst,
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
      const c4c::TargetProfile& target_profile,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
  std::optional<LoweredReturnInfo> infer_function_return_info() const;
  static std::optional<LoweredReturnInfo> lower_signature_return_info(
      std::string_view signature_text,
      const TypeDeclMap& type_decls,
      const c4c::TargetProfile& target_profile,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
  bool lower_function_params(const c4c::codegen::lir::LirFunction& function,
                             const c4c::TargetProfile& target_profile,
                             const std::optional<LoweredReturnInfo>& return_info,
                             const TypeDeclMap& type_decls,
                             bir::Function* lowered) const;
  static bool lower_function_params_fallback(
      const c4c::codegen::lir::LirFunction& function,
      const c4c::TargetProfile& target_profile,
      const std::optional<LoweredReturnInfo>& return_info,
      const TypeDeclMap& type_decls,
      bir::Function* lowered);
  static bool lower_function_params_with_layouts(
      const c4c::codegen::lir::LirFunction& function,
      const c4c::TargetProfile& target_profile,
      const std::optional<LoweredReturnInfo>& return_info,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      bir::Function* lowered);
  void note_function_lowering_family_failure(std::string_view family);
  void note_semantic_call_family_failure(std::string_view family);
  void note_runtime_intrinsic_family_failure(std::string_view family);
  bool lower_runtime_intrinsic_inst(const c4c::codegen::lir::LirInst& inst,
                                    const ValueMap& value_aliases,
                                    std::vector<bir::Inst>* lowered_insts);
  bool lower_call_inst(const c4c::codegen::lir::LirCallOp& call,
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
  bool initialize_aggregate_phi_state();
  bool apply_pending_aggregate_phi_copies(std::string_view predecessor_label,
                                          std::vector<bir::Inst>* lowered_insts);

  // Local/global memory helpers.
  static std::optional<std::vector<std::string>> collect_local_scalar_array_slots(
      std::string_view type_text,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<std::vector<std::string>> collect_local_scalar_array_slots(
      std::string_view type_text,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static bool is_local_array_element_slot(std::string_view slot_name,
                                          const LocalArraySlotMap& local_array_slots);
  static std::optional<std::pair<std::size_t, bir::TypeKind>> parse_local_array_type(
      std::string_view text);
  bool lower_local_memory_alloca_inst(const c4c::codegen::lir::LirAllocaOp& alloca,
                                      std::vector<bir::Inst>* lowered_insts);
  bool lower_memory_gep_inst(const c4c::codegen::lir::LirGepOp& gep,
                             std::vector<bir::Inst>* lowered_insts);
  bool lower_memory_store_inst(const c4c::codegen::lir::LirStoreOp& store,
                               std::vector<bir::Inst>* lowered_insts);
  bool lower_memory_load_inst(const c4c::codegen::lir::LirLoadOp& load,
                              std::vector<bir::Inst>* lowered_insts);
  static std::optional<AggregateArrayExtent> find_repeated_aggregate_extent_at_offset(
      std::string_view type_text,
      std::size_t target_offset,
      std::string_view repeated_type_text,
      const TypeDeclMap& type_decls);
  static std::optional<AggregateArrayExtent> find_repeated_aggregate_extent_at_offset(
      std::string_view type_text,
      std::size_t target_offset,
      std::string_view repeated_type_text,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts);
  static std::optional<AggregateArrayExtent> find_nested_repeated_aggregate_extent_at_offset(
      std::string_view type_text,
      std::size_t target_offset,
      std::string_view repeated_type_text,
      const TypeDeclMap& type_decls);
  static std::optional<AggregateArrayExtent> find_nested_repeated_aggregate_extent_at_offset(
      std::string_view type_text,
      std::size_t target_offset,
      std::string_view repeated_type_text,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts);
  static std::optional<LocalAggregateGepTarget> resolve_relative_gep_target(
      std::string_view type_text,
      std::int64_t base_byte_offset,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls);
  static std::optional<LocalAggregateGepTarget> resolve_relative_gep_target(
      std::string_view type_text,
      std::int64_t base_byte_offset,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
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
  static std::optional<std::string> resolve_local_aggregate_gep_slot(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<std::vector<std::string>> resolve_local_aggregate_pointer_array_slots(
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
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalPointerArrayAccess>
  resolve_local_aggregate_dynamic_pointer_array_access(
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
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<LocalAggregateGepTarget> resolve_local_aggregate_gep_target(
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
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalAggregateArrayAccess>
  resolve_local_dynamic_aggregate_array_access(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalAggregateArrayAccess>
  resolve_local_dynamic_aggregate_array_access(
      std::string_view base_type_text,
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalAggregateSlots& aggregate_slots);
  static std::optional<DynamicLocalAggregateArrayAccess>
  build_dynamic_local_aggregate_array_access(
      bir::TypeKind element_type,
      std::size_t byte_offset,
      const LocalAggregateSlots& aggregate_slots,
      const bir::Value& index,
      const TypeDeclMap& type_decls);
  static std::optional<DynamicLocalAggregateArrayAccess>
  build_dynamic_local_aggregate_array_access(
      bir::TypeKind element_type,
      std::size_t byte_offset,
      const LocalAggregateSlots& aggregate_slots,
      const bir::Value& index,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts);
  static std::optional<DynamicLocalPointerArrayAccess>
  resolve_dynamic_local_aggregate_gep_projection(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const DynamicLocalAggregateArrayAccess& access);
  static std::optional<DynamicLocalPointerArrayAccess>
  resolve_dynamic_local_aggregate_gep_projection(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const DynamicLocalAggregateArrayAccess& access);
  static std::optional<bool> try_lower_dynamic_local_aggregate_gep_projection(
      const c4c::codegen::lir::LirGepOp& gep,
      const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays);
  static std::optional<bool> try_lower_dynamic_local_aggregate_gep_projection(
      const c4c::codegen::lir::LirGepOp& gep,
      const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays);
  static std::optional<bool> try_lower_local_slot_pointer_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      LocalSlotPointerValues* local_slot_pointer_values);
  static std::optional<bool> try_lower_local_slot_pointer_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      LocalSlotPointerValues* local_slot_pointer_values);
  static std::optional<bool> try_lower_local_array_slot_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const LocalArraySlotMap& local_array_slots,
      LocalPointerSlots* local_pointer_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays);
  static std::optional<bool> try_lower_local_pointer_array_base_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const LocalSlotTypes& local_slot_types,
      LocalPointerSlots* local_pointer_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
      DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays,
      LocalSlotPointerValues* local_slot_pointer_values);
  static std::optional<bool> try_lower_local_pointer_array_base_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      LocalPointerSlots* local_pointer_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
      DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays,
      LocalSlotPointerValues* local_slot_pointer_values);
  static std::optional<bool> try_lower_local_pointer_slot_base_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      const LocalArraySlotMap& local_array_slots,
      const LocalAggregateSlotMap& local_aggregate_slots,
      LocalPointerSlots* local_pointer_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
      DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays);
  static std::optional<bool> try_lower_local_pointer_slot_base_gep(
      const c4c::codegen::lir::LirGepOp& gep,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      const LocalArraySlotMap& local_array_slots,
      const LocalAggregateSlotMap& local_aggregate_slots,
      LocalPointerSlots* local_pointer_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      DynamicLocalPointerArrayMap* dynamic_local_pointer_arrays,
      DynamicLocalAggregateArrayMap* dynamic_local_aggregate_arrays);
  static bool try_lower_immediate_local_memset(
      std::string_view dst_operand,
      std::uint8_t fill_byte,
      std::size_t fill_size_bytes,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      const LocalPointerSlots& local_pointer_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerArrayBaseMap& local_pointer_array_bases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      std::vector<bir::Inst>* lowered_insts);
  static bool try_lower_immediate_local_memset(
      std::string_view dst_operand,
      std::uint8_t fill_byte,
      std::size_t fill_size_bytes,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      const LocalPointerSlots& local_pointer_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerArrayBaseMap& local_pointer_array_bases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      std::vector<bir::Inst>* lowered_insts);
  static bool try_lower_immediate_local_memcpy(
      std::string_view dst_operand,
      std::string_view src_operand,
      std::size_t requested_size,
      const bir::Function& lowered_function,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      const LocalPointerSlots& local_pointer_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerArrayBaseMap& local_pointer_array_bases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      std::vector<bir::Inst>* lowered_insts);
  static bool try_lower_immediate_local_memcpy(
      std::string_view dst_operand,
      std::string_view src_operand,
      std::size_t requested_size,
      const bir::Function& lowered_function,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      const LocalPointerSlots& local_pointer_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerArrayBaseMap& local_pointer_array_bases,
      const LocalAggregateSlotMap& local_aggregate_slots,
      std::vector<bir::Inst>* lowered_insts);
  bool lower_memory_memcpy_inst(const c4c::codegen::lir::LirMemcpyOp& memcpy,
                                std::vector<bir::Inst>* lowered_insts);
  bool lower_memory_memset_inst(const c4c::codegen::lir::LirMemsetOp& memset,
                                std::vector<bir::Inst>* lowered_insts);
  std::optional<bool> try_lower_direct_memory_intrinsic_call(
      std::string_view symbol_name,
      const ParsedTypedCall& typed_call,
      const c4c::codegen::lir::LirCallOp& call,
      const LoweredReturnInfo& return_info,
      std::vector<bir::Inst>* lowered_insts);
  bool try_lower_local_slot_pointer_store(
      const LocalSlotAddress& local_slot_ptr,
      bir::TypeKind value_type,
      const bir::Value& value,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  bool try_lower_local_slot_pointer_load(
      std::string_view result_name,
      const LocalSlotAddress& local_slot_ptr,
      bir::TypeKind value_type,
      const LocalSlotTypes& local_slot_types,
      const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
      const LocalAddressSlots& local_address_slots,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      ValueMap* value_aliases,
      LocalSlotPointerValues* local_slot_pointer_values,
      GlobalPointerMap* global_pointer_slots,
      std::vector<bir::Inst>* lowered_insts);
  LocalSlotStoreResult try_lower_local_slot_store(
      std::string_view ptr_name,
      const c4c::codegen::lir::LirOperand& stored_operand,
      bir::TypeKind value_type,
      const bir::Value& value,
      const ValueMap& value_aliases,
      const TypeDeclMap& type_decls,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      const LocalPointerSlots& local_pointer_slots,
      const LocalSlotTypes& local_slot_types,
      const LocalAggregateFieldSet& local_aggregate_field_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalAggregateSlotMap& local_aggregate_slots,
      const LocalPointerArrayBaseMap& local_pointer_array_bases,
      const LocalSlotPointerValues& local_slot_pointer_values,
      const PointerAddressMap& pointer_value_addresses,
      const GlobalPointerMap& global_pointer_slots,
      const GlobalAddressIntMap& global_address_ints,
      LocalPointerValueAliasMap* local_pointer_value_aliases,
      LocalIndirectPointerSlotSet* local_indirect_pointer_slots,
      PointerAddressMap* local_pointer_slot_addresses,
      LocalSlotAddressSlots* local_slot_address_slots,
      LocalAddressSlots* local_address_slots,
      std::vector<bir::Inst>* lowered_insts);
  LocalSlotLoadResult try_lower_local_slot_load(
      std::string_view result_name,
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const LocalPointerSlots& local_pointer_slots,
      const LocalSlotTypes& local_slot_types,
      const LocalAggregateFieldSet& local_aggregate_field_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerValueAliasMap& local_pointer_value_aliases,
      const TypeDeclMap& type_decls,
      const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
      const LocalAddressSlots& local_address_slots,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const PointerAddressMap& local_pointer_slot_addresses,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      ValueMap* value_aliases,
      LocalSlotPointerValues* local_slot_pointer_values,
      LocalAggregateSlotMap* local_aggregate_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      GlobalPointerMap* global_pointer_slots,
      PointerAddressMap* pointer_value_addresses,
      GlobalAddressIntMap* global_address_ints,
      std::vector<bir::Inst>* lowered_insts);
  static bool try_lower_nonpointer_local_slot_load(
      std::string_view result_name,
      std::string_view slot_name,
      bir::TypeKind value_type,
      const LocalAddressSlots& local_address_slots,
      GlobalAddressIntMap* global_address_ints,
      std::vector<bir::Inst>* lowered_insts);
  bool try_lower_tracked_local_pointer_slot_load(
      std::string_view result_name,
      std::string_view slot_name,
      const LocalAggregateFieldSet& local_aggregate_field_slots,
      const LocalArraySlotMap& local_array_slots,
      const LocalPointerValueAliasMap& local_pointer_value_aliases,
      const TypeDeclMap& type_decls,
      const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
      const LocalAddressSlots& local_address_slots,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const PointerAddressMap& local_pointer_slot_addresses,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      ValueMap* value_aliases,
      LocalSlotPointerValues* local_slot_pointer_values,
      LocalAggregateSlotMap* local_aggregate_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases,
      GlobalPointerMap* global_pointer_slots,
      PointerAddressMap* pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
  static void record_loaded_local_pointer_slot_state(
      std::string_view result_name,
      std::string_view slot_name,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      LocalSlotPointerValues* local_slot_pointer_values,
      LocalAggregateSlotMap* local_aggregate_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases);
  static void record_loaded_local_pointer_slot_state(
      std::string_view result_name,
      std::string_view slot_name,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const TypeDeclMap& type_decls,
      LocalSlotPointerValues* local_slot_pointer_values,
      LocalAggregateSlotMap* local_aggregate_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases);
  static void record_loaded_local_pointer_slot_state(
      std::string_view result_name,
      std::string_view slot_name,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      LocalSlotPointerValues* local_slot_pointer_values,
      LocalAggregateSlotMap* local_aggregate_slots,
      LocalPointerArrayBaseMap* local_pointer_array_bases);
  static std::optional<bir::Value> load_dynamic_local_aggregate_array_value(
      std::string_view result_name,
      bir::TypeKind value_type,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bir::Value> load_dynamic_local_aggregate_array_value(
      std::string_view result_name,
      bir::TypeKind value_type,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bir::Value> load_dynamic_local_aggregate_array_value(
      std::string_view result_name,
      bir::TypeKind value_type,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bir::Value> load_dynamic_global_scalar_array_value(
      std::string_view result_name,
      bir::TypeKind value_type,
      const DynamicGlobalScalarArrayAccess& access,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bool> try_lower_dynamic_pointer_array_load(
      std::string_view result_name,
      std::string_view ptr_name,
      const DynamicLocalPointerArrayMap& dynamic_local_pointer_arrays,
      const DynamicGlobalPointerArrayMap& dynamic_global_pointer_arrays,
      const LocalPointerValueAliasMap& local_pointer_value_aliases,
      const GlobalTypes& global_types,
      ValueMap* value_aliases,
      std::vector<bir::Inst>* lowered_insts);
  std::optional<bir::Value> load_dynamic_pointer_value_array_value(
      std::string_view result_name,
      bir::TypeKind value_type,
      const DynamicPointerValueArrayAccess& access,
      std::vector<bir::Inst>* lowered_insts);
  static bool append_dynamic_local_aggregate_store(
      std::string_view scratch_prefix,
      bir::TypeKind value_type,
      const bir::Value& value,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  static bool append_dynamic_local_aggregate_store(
      std::string_view scratch_prefix,
      bir::TypeKind value_type,
      const bir::Value& value,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable* structured_layouts,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  static bool append_dynamic_local_aggregate_store(
      std::string_view scratch_prefix,
      bir::TypeKind value_type,
      const bir::Value& value,
      const DynamicLocalAggregateArrayAccess& access,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts);
  bool append_dynamic_pointer_value_array_store(
      std::string_view scratch_prefix,
      bir::TypeKind value_type,
      const bir::Value& value,
      const DynamicPointerValueArrayAccess& access,
      std::vector<bir::Inst>* lowered_insts);
  static bool try_lower_dynamic_local_aggregate_store(
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const bir::Value& value,
      const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      const LocalSlotTypes& local_slot_types,
      std::vector<bir::Inst>* lowered_insts,
      bool* handled);
  static bool try_lower_dynamic_local_aggregate_load(
      std::string_view result_name,
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const DynamicLocalAggregateArrayMap& dynamic_local_aggregate_arrays,
      const TypeDeclMap& type_decls,
      const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts,
      const LocalSlotTypes& local_slot_types,
      ValueMap* value_aliases,
      std::vector<bir::Inst>* lowered_insts,
      bool* handled);
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
  static std::optional<bool> try_lower_global_provenance_load(
      const c4c::codegen::lir::LirLoadOp& load,
      bir::TypeKind value_type,
      const GlobalTypes& global_types,
      const TypeDeclMap& type_decls,
      const GlobalAddressSlots& global_address_slots,
      const AddressedGlobalPointerSlots& addressed_global_pointer_slots,
      const GlobalPointerValueSlots& global_pointer_value_slots,
      const AddressedGlobalPointerValueSlots& addressed_global_pointer_value_slots,
      GlobalPointerMap* global_pointer_slots,
      GlobalObjectPointerMap* global_object_pointer_slots,
      PointerAddressMap* pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
  static std::optional<bool> try_lower_global_provenance_store(
      const c4c::codegen::lir::LirStoreOp& store,
      bir::TypeKind value_type,
      const bir::Value& value,
      const TypeDeclMap& type_decls,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      const GlobalPointerMap& global_pointer_slots,
      const GlobalObjectPointerMap& global_object_pointer_slots,
      const PointerAddressMap& pointer_value_addresses,
      GlobalAddressSlots* global_address_slots,
      AddressedGlobalPointerSlots* addressed_global_pointer_slots,
      GlobalPointerValueSlots* global_pointer_value_slots,
      AddressedGlobalPointerValueSlots* addressed_global_pointer_value_slots,
      std::vector<bir::Inst>* lowered_insts);
  std::optional<bool> try_lower_pointer_provenance_store(
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const bir::Value& value,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      const LocalSlotPointerValues& local_slot_pointer_values,
      const PointerAddressMap& pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
  std::optional<bool> try_lower_addressed_pointer_store(
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const bir::Value& value,
      const TypeDeclMap& type_decls,
      const PointerAddressMap& pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
  std::optional<bool> try_lower_pointer_provenance_load(
      std::string_view result_name,
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const TypeDeclMap& type_decls,
      const LocalSlotTypes& local_slot_types,
      const LocalIndirectPointerSlotSet& local_indirect_pointer_slots,
      const LocalAddressSlots& local_address_slots,
      const LocalSlotAddressSlots& local_slot_address_slots,
      const GlobalTypes& global_types,
      const FunctionSymbolSet& function_symbols,
      ValueMap* value_aliases,
      LocalSlotPointerValues* local_slot_pointer_values,
      GlobalPointerMap* global_pointer_slots,
      const PointerAddressMap& pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
  std::optional<bool> try_lower_addressed_pointer_load(
      std::string_view result_name,
      std::string_view ptr_name,
      bir::TypeKind value_type,
      const TypeDeclMap& type_decls,
      const PointerAddressMap& pointer_value_addresses,
      PointerAddressMap* loaded_pointer_value_addresses,
      std::vector<bir::Inst>* lowered_insts);
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
                              bir::Block* lowered_block,
                              std::vector<bir::Block>* trailing_blocks);

  // Lowering state.
  BirLoweringContext& context_;
  const c4c::codegen::lir::LirFunction& function_;
  const GlobalTypes& global_types_;
  const FunctionSymbolSet& function_symbols_;
  const TypeDeclMap& type_decls_;
  const lir_to_bir_detail::BackendStructuredLayoutTable& structured_layouts_;

  bir::Function lowered_function_;
  std::optional<LoweredReturnInfo> return_info_;
  PhiBlockPlanMap phi_plans_;
  PendingAggregatePhiCopyMap pending_aggregate_phi_copies_;
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
  ValueMap local_scalar_slot_values_;
  ValueMap loaded_local_scalar_immediates_;
  LocalIndirectPointerSlotSet local_indirect_pointer_slots_;
  PointerAddressMap pointer_value_addresses_;
  PointerAddressIntMap pointer_address_ints_;
  PointerAddressMap local_pointer_slot_addresses_;
  LocalAddressSlots local_address_slots_;
  LocalSlotAddressSlots local_slot_address_slots_;
  LocalSlotPointerValues local_slot_pointer_values_;
  GlobalAddressSlots global_address_slots_;
  AddressedGlobalPointerSlots addressed_global_pointer_slots_;
  GlobalPointerValueSlots global_pointer_value_slots_;
  AddressedGlobalPointerValueSlots addressed_global_pointer_value_slots_;
  GlobalPointerMap global_pointer_slots_;
  DynamicGlobalPointerArrayMap dynamic_global_pointer_arrays_;
  DynamicGlobalAggregateArrayMap dynamic_global_aggregate_arrays_;
  DynamicGlobalScalarArrayMap dynamic_global_scalar_arrays_;
  GlobalObjectPointerMap global_object_pointer_slots_;
  GlobalAddressIntMap global_address_ints_;
  GlobalObjectAddressIntMap global_object_address_ints_;
  std::vector<bir::Inst> hoisted_alloca_scratch_;
};

}  // namespace c4c::backend
