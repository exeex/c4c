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

}  // namespace c4c::backend::mir::prepared
