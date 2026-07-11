#include "prepared_view.hpp"

#include "../prealloc/module.hpp"

#include <utility>

namespace c4c::backend::mir::prepared {

PreparedMirFunctionView::PreparedMirFunctionView(const PreparedMirCoreView* core,
                                                 const PreparedMirFunctionEntry* entry)
    : core_(core), entry_(entry) {}

PreparedMirFunctionView::operator bool() const {
  return core_ != nullptr && entry_ != nullptr;
}

FunctionNameId PreparedMirFunctionView::function_name() const {
  return entry_->function_name;
}

std::string_view PreparedMirFunctionView::function_name_text() const {
  return core_->function_name(entry_->function_name);
}

const bir::Function& PreparedMirFunctionView::bir_function() const {
  return *entry_->bir_function;
}

const prepare::PreparedControlFlowFunction& PreparedMirFunctionView::control_flow() const {
  return *entry_->control_flow;
}

const prepare::PreparedValueLocationFunction& PreparedMirFunctionView::value_locations() const {
  return *entry_->value_locations;
}

const prepare::PreparedStackLayout& PreparedMirFunctionView::stack_layout() const {
  return core_->module_->stack_layout;
}

const prepare::PreparedAddressingFunction& PreparedMirFunctionView::addressing() const {
  return *entry_->addressing;
}

const prepare::PreparedFunctionLookups& PreparedMirFunctionView::prepared_lookups() const {
  return entry_->prepared_lookups;
}

const std::vector<PreparedMirBlockView>& PreparedMirFunctionView::blocks() const {
  return entry_->blocks;
}

std::optional<PreparedMirInstructionCursor> PreparedMirFunctionView::instruction(
    std::size_t block_index,
    std::size_t instruction_index) const {
  if (!*this || block_index >= entry_->blocks.size()) {
    return std::nullopt;
  }
  const auto& block_view = entry_->blocks[block_index];
  if (block_view.block == nullptr ||
      instruction_index >= block_view.block->insts.size()) {
    return std::nullopt;
  }
  return PreparedMirInstructionCursor{
      .function_name = entry_->function_name,
      .block_label = block_view.block_label,
      .block_index = block_index,
      .instruction_index = instruction_index,
      .function = entry_->bir_function,
      .block = block_view.block,
      .instruction = &block_view.block->insts[instruction_index],
      .prepared_block = block_view.control_flow,
  };
}

PreparedMirCoreView::PreparedMirCoreView(const prepare::PreparedBirModule& module)
    : module_(&module) {
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    defined_functions_.push_back(&function);

    const auto function_name_id = module.names.function_names.find(function.name);
    if (function_name_id == kInvalidFunctionName) {
      continue;
    }

    auto* existing = const_cast<BirFunctionBinding*>(bir_binding(function_name_id));
    if (existing != nullptr) {
      existing->function = nullptr;
      existing->duplicate = true;
      continue;
    }
    bir_bindings_.push_back(BirFunctionBinding{
        .function_name = function_name_id,
        .function = &function,
    });
  }

  for (const auto& binding : bir_bindings_) {
    if (binding.duplicate || binding.function == nullptr) {
      continue;
    }

    const auto* control_flow =
        prepare::find_prepared_control_flow_function(module.control_flow,
                                                     binding.function_name);
    const auto* value_locations =
        prepare::find_prepared_value_location_function(module.value_locations,
                                                       binding.function_name);
    const auto* addressing =
        prepare::find_prepared_addressing_function(module.addressing,
                                                   binding.function_name);
    if (control_flow == nullptr || value_locations == nullptr || addressing == nullptr) {
      continue;
    }

    PreparedMirFunctionEntry entry{
        .function_name = binding.function_name,
        .bir_function = binding.function,
        .control_flow = control_flow,
        .value_locations = value_locations,
        .addressing = addressing,
        .prepared_lookups = prepare::make_prepared_function_lookups(module,
                                                                    *control_flow),
    };

    for (std::size_t block_index = 0; block_index < control_flow->blocks.size();
         ++block_index) {
      const bir::Block* bir_block = nullptr;
      if (block_index < binding.function->blocks.size()) {
        bir_block = &binding.function->blocks[block_index];
      }
      const auto& prepared_block = control_flow->blocks[block_index];
      entry.blocks.push_back(PreparedMirBlockView{
          .function_name = binding.function_name,
          .block_label = prepared_block.block_label,
          .block_index = block_index,
          .block = bir_block,
          .control_flow = &prepared_block,
      });
    }

    function_entries_.push_back(std::move(entry));
  }
}

const TargetProfile& PreparedMirCoreView::target_profile() const {
  return module_->target_profile;
}

std::string_view PreparedMirCoreView::target_triple() const {
  return module_->module.target_triple;
}

const bir::NameTables& PreparedMirCoreView::bir_names() const {
  return module_->module.names;
}

const prepare::PreparedNameTables& PreparedMirCoreView::prepared_names() const {
  return module_->names;
}

const std::vector<bir::Function>& PreparedMirCoreView::functions() const {
  return module_->module.functions;
}

const std::vector<const bir::Function*>& PreparedMirCoreView::defined_functions() const {
  return defined_functions_;
}

const std::vector<bir::Global>& PreparedMirCoreView::globals() const {
  return module_->module.globals;
}

const std::vector<bir::StringConstant>& PreparedMirCoreView::string_constants() const {
  return module_->module.string_constants;
}

std::optional<FunctionNameId> PreparedMirCoreView::resolve_function_name(
    std::string_view name) const {
  const FunctionNameId id = module_->names.function_names.find(name);
  if (id == kInvalidFunctionName || prepare::prepared_function_name(module_->names, id) != name) {
    return std::nullopt;
  }
  return id;
}

std::string_view PreparedMirCoreView::function_name(FunctionNameId id) const {
  if (id == kInvalidFunctionName) {
    return {};
  }
  return prepare::prepared_function_name(module_->names, id);
}

const bir::Function* PreparedMirCoreView::bir_function(FunctionNameId id) const {
  const auto* binding = bir_binding(id);
  if (binding == nullptr || binding->duplicate) {
    return nullptr;
  }
  return binding->function;
}

const prepare::PreparedControlFlowFunction* PreparedMirCoreView::control_flow(
    FunctionNameId id) const {
  return prepare::find_prepared_control_flow_function(module_->control_flow, id);
}

std::optional<PreparedMirFunctionView> PreparedMirCoreView::function_view(
    FunctionNameId id) const {
  const auto* function_entry = entry(id);
  if (function_entry == nullptr) {
    return std::nullopt;
  }
  return PreparedMirFunctionView{this, function_entry};
}

std::optional<PreparedMirFunctionView> PreparedMirCoreView::function_view(
    std::string_view name) const {
  const auto id = resolve_function_name(name);
  if (!id.has_value()) {
    return std::nullopt;
  }
  return function_view(*id);
}

const PreparedMirFunctionEntry* PreparedMirCoreView::entry(
    FunctionNameId id) const {
  for (const auto& function_entry : function_entries_) {
    if (function_entry.function_name == id) {
      return &function_entry;
    }
  }
  return nullptr;
}

const PreparedMirCoreView::BirFunctionBinding* PreparedMirCoreView::bir_binding(
    FunctionNameId id) const {
  for (const auto& binding : bir_bindings_) {
    if (binding.function_name == id) {
      return &binding;
    }
  }
  return nullptr;
}

}  // namespace c4c::backend::mir::prepared
