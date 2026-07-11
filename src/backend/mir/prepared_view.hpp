#pragma once

#include "../bir/bir.hpp"
#include "../prealloc/addressing.hpp"
#include "../prealloc/control_flow.hpp"
#include "../prealloc/names.hpp"
#include "../prealloc/prepared_lookups.hpp"
#include "../prealloc/stack_layout/stack_layout.hpp"
#include "../prealloc/value_locations.hpp"
#include "../../target_profile.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {
struct PreparedBirModule;
}  // namespace c4c::backend::prepare

namespace c4c::backend::mir::prepared {

class PreparedMirCoreView;

struct PreparedMirBlockView {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  const bir::Block* block = nullptr;
  const prepare::PreparedControlFlowBlock* control_flow = nullptr;
};

struct PreparedMirFunctionEntry {
  FunctionNameId function_name = kInvalidFunctionName;
  const bir::Function* bir_function = nullptr;
  const prepare::PreparedControlFlowFunction* control_flow = nullptr;
  const prepare::PreparedValueLocationFunction* value_locations = nullptr;
  const prepare::PreparedAddressingFunction* addressing = nullptr;
  prepare::PreparedFunctionLookups prepared_lookups;
  std::vector<PreparedMirBlockView> blocks;
};

struct PreparedMirInstructionCursor {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const bir::Function* function = nullptr;
  const bir::Block* block = nullptr;
  const bir::Inst* instruction = nullptr;
  const prepare::PreparedControlFlowBlock* prepared_block = nullptr;
};

struct PreparedMirCoreTargetSnapshot {
  std::string target_triple;
  std::string profile_triple;
  std::string arch;
  std::string os;
  std::string abi;
  std::string relocation;
  bool has_float_arg_registers = false;
  bool has_float_return_registers = false;
};

struct PreparedMirCoreFunctionSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t function_id = 0;
  bool has_function_id = false;
  bool is_declaration = false;
  std::size_t block_count = 0;
  bool has_function_view = false;
};

struct PreparedMirCoreGlobalSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t link_name_id = 0;
  std::size_t type = 0;
  bool is_extern = false;
  bool is_constant = false;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedMirCoreStringSnapshot {
  std::size_t index = 0;
  std::string name;
  std::size_t byte_count = 0;
  std::string bytes_hex;
  std::size_t align_bytes = 0;
};

struct PreparedMirCoreBlockSnapshot {
  std::size_t index = 0;
  std::string label;
  bool bir_present = false;
  bool prepared_present = false;
  std::size_t instruction_count = 0;
  std::size_t cursor_count = 0;
};

struct PreparedMirCoreFunctionViewSnapshot {
  std::string name;
  std::size_t function_id = 0;
  std::size_t bir_block_count = 0;
  std::size_t prepared_block_count = 0;
  std::size_t branch_condition_count = 0;
  std::size_t join_transfer_count = 0;
  std::size_t parallel_copy_bundle_count = 0;
  std::size_t value_home_count = 0;
  std::size_t move_bundle_count = 0;
  std::size_t stack_object_count = 0;
  std::size_t stack_frame_slot_count = 0;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  std::size_t memory_access_count = 0;
  std::size_t address_materialization_count = 0;
  std::size_t lookup_call_count = 0;
  std::size_t lookup_address_block_count = 0;
  std::size_t lookup_memory_position_count = 0;
  std::size_t lookup_value_home_count = 0;
  std::size_t lookup_move_bundle_count = 0;
  std::size_t lookup_edge_publication_count = 0;
  std::size_t lookup_edge_source_producer_count = 0;
  std::size_t lookup_branch_stack_load_count = 0;
  std::vector<PreparedMirCoreBlockSnapshot> blocks;
};

struct PreparedMirCoreSnapshot {
  std::size_t schema_version = 1;
  PreparedMirCoreTargetSnapshot target;
  std::vector<PreparedMirCoreFunctionSnapshot> functions;
  std::vector<std::string> defined_functions;
  std::vector<PreparedMirCoreGlobalSnapshot> globals;
  std::vector<PreparedMirCoreStringSnapshot> string_constants;
  std::vector<PreparedMirCoreFunctionViewSnapshot> function_views;
};

struct PreparedMirCoreComparisonOptions {
  bool compare_target_identity = true;
};

enum class PreparedMirCoreComparisonStatus {
  Equal,
  Different,
  Unsupported,
};

struct PreparedMirCoreComparisonReport {
  PreparedMirCoreComparisonStatus status = PreparedMirCoreComparisonStatus::Equal;
  std::vector<std::string> blocking_differences;
  std::vector<std::string> unsupported_reasons;

  [[nodiscard]] bool equal() const;
};

class PreparedMirFunctionView {
 public:
  PreparedMirFunctionView() = default;

  [[nodiscard]] explicit operator bool() const;

  [[nodiscard]] FunctionNameId function_name() const;
  [[nodiscard]] std::string_view function_name_text() const;

  [[nodiscard]] const bir::Function& bir_function() const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction& control_flow() const;
  [[nodiscard]] const prepare::PreparedValueLocationFunction& value_locations() const;
  [[nodiscard]] const prepare::PreparedStackLayout& stack_layout() const;
  [[nodiscard]] const prepare::PreparedAddressingFunction& addressing() const;
  [[nodiscard]] const prepare::PreparedFunctionLookups& prepared_lookups() const;

  [[nodiscard]] const std::vector<PreparedMirBlockView>& blocks() const;
  [[nodiscard]] std::optional<PreparedMirInstructionCursor> instruction(
      std::size_t block_index,
      std::size_t instruction_index) const;

 private:
  friend class PreparedMirCoreView;

  PreparedMirFunctionView(const PreparedMirCoreView* core,
                          const PreparedMirFunctionEntry* entry);

  const PreparedMirCoreView* core_ = nullptr;
  const PreparedMirFunctionEntry* entry_ = nullptr;
};

class PreparedMirCoreView {
 public:
  explicit PreparedMirCoreView(const prepare::PreparedBirModule& module);

  [[nodiscard]] const TargetProfile& target_profile() const;
  [[nodiscard]] std::string_view target_triple() const;

  [[nodiscard]] const bir::NameTables& bir_names() const;
  [[nodiscard]] const prepare::PreparedNameTables& prepared_names() const;

  [[nodiscard]] const std::vector<bir::Function>& functions() const;
  [[nodiscard]] const std::vector<const bir::Function*>& defined_functions() const;
  [[nodiscard]] const std::vector<bir::Global>& globals() const;
  [[nodiscard]] const std::vector<bir::StringConstant>& string_constants() const;

  [[nodiscard]] std::optional<FunctionNameId> resolve_function_name(
      std::string_view name) const;
  [[nodiscard]] std::string_view function_name(FunctionNameId id) const;
  [[nodiscard]] const bir::Function* bir_function(FunctionNameId id) const;
  [[nodiscard]] const prepare::PreparedControlFlowFunction* control_flow(
      FunctionNameId id) const;

  [[nodiscard]] std::optional<PreparedMirFunctionView> function_view(
      FunctionNameId id) const;
  [[nodiscard]] std::optional<PreparedMirFunctionView> function_view(
      std::string_view name) const;
  [[nodiscard]] PreparedMirCoreSnapshot structural_snapshot() const;
  [[nodiscard]] std::string canonical_dump() const;

 private:
  friend class PreparedMirFunctionView;

  struct BirFunctionBinding {
    FunctionNameId function_name = kInvalidFunctionName;
    const bir::Function* function = nullptr;
    bool duplicate = false;
  };

  [[nodiscard]] const PreparedMirFunctionEntry* entry(FunctionNameId id) const;
  [[nodiscard]] const BirFunctionBinding* bir_binding(FunctionNameId id) const;

  const prepare::PreparedBirModule* module_ = nullptr;
  std::vector<const bir::Function*> defined_functions_;
  std::vector<BirFunctionBinding> bir_bindings_;
  std::vector<PreparedMirFunctionEntry> function_entries_;
};

[[nodiscard]] PreparedMirCoreComparisonReport compare_prepared_mir_core_views(
    const PreparedMirCoreView& lhs,
    const PreparedMirCoreView& rhs,
    const PreparedMirCoreComparisonOptions& options = {});

}  // namespace c4c::backend::mir::prepared
