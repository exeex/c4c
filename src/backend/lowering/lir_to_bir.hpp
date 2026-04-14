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

}  // namespace lir_to_bir_detail

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options);
std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
