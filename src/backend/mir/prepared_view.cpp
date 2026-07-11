#include "prepared_view.hpp"

#include "../prealloc/module.hpp"

#include <string>
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

std::string PreparedMirCoreView::canonical_dump() const {
  std::string out;
  const auto append_line = [&](std::string line) {
    out += std::move(line);
    out += '\n';
  };
  const auto bool_text = [](bool value) -> const char* {
    return value ? "1" : "0";
  };
  const auto id_text = [](auto id) {
    return std::to_string(static_cast<std::size_t>(id));
  };
  const auto block_label_text = [&](BlockLabelId id) {
    const std::string_view label = prepare::prepared_block_label(prepared_names(), id);
    return label.empty() ? std::string{"<invalid>"} : std::string{label};
  };
  const auto value_name_text = [&](ValueNameId id) {
    const std::string_view name = prepare::prepared_value_name(prepared_names(), id);
    return name.empty() ? std::string{"<invalid>"} : std::string{name};
  };

  append_line("prepared_mir_core_view schema=1");
  append_line("target triple=" + std::string(target_triple()) +
              " profile_triple=" + target_profile().triple +
              " arch=" + c4c::target_arch_name(target_profile().arch) +
              " os=" + c4c::target_os_name(target_profile().os) +
              " abi=" + c4c::backend_abi_name(target_profile().backend_abi) +
              " relocation=" +
              c4c::target_relocation_model_name(target_profile().relocation_model) +
              " float_args=" + bool_text(target_profile().has_float_arg_registers) +
              " float_returns=" + bool_text(target_profile().has_float_return_registers));
  append_line("module functions=" + std::to_string(functions().size()) +
              " defined_functions=" + std::to_string(defined_functions().size()) +
              " globals=" + std::to_string(globals().size()) +
              " strings=" + std::to_string(string_constants().size()));

  for (std::size_t index = 0; index < functions().size(); ++index) {
    const auto& function = functions()[index];
    const auto function_id = resolve_function_name(function.name);
    const bool has_view = function_id.has_value() && function_view(*function_id).has_value();
    append_line("function index=" + std::to_string(index) +
                " name=" + function.name +
                " id=" + (function_id.has_value() ? id_text(*function_id) : std::string{"invalid"}) +
                " declaration=" + bool_text(function.is_declaration) +
                " blocks=" + std::to_string(function.blocks.size()) +
                " view=" + bool_text(has_view));
  }

  for (std::size_t index = 0; index < defined_functions().size(); ++index) {
    const auto* function = defined_functions()[index];
    append_line("defined_function index=" + std::to_string(index) +
                " name=" + (function != nullptr ? function->name : std::string{"<null>"}));
  }

  for (std::size_t index = 0; index < globals().size(); ++index) {
    const auto& global = globals()[index];
    append_line("global index=" + std::to_string(index) +
                " name=" + global.name +
                " link_id=" + id_text(global.link_name_id) +
                " type=" + std::to_string(static_cast<unsigned>(global.type)) +
                " extern=" + bool_text(global.is_extern) +
                " constant=" + bool_text(global.is_constant) +
                " size=" + std::to_string(global.size_bytes) +
                " align=" + std::to_string(global.align_bytes));
  }

  for (std::size_t index = 0; index < string_constants().size(); ++index) {
    const auto& constant = string_constants()[index];
    append_line("string index=" + std::to_string(index) +
                " name=" + constant.name +
                " bytes=" + std::to_string(constant.bytes.size()) +
                " align=" + std::to_string(constant.align_bytes));
  }

  for (const auto& entry : function_entries_) {
    const PreparedMirFunctionView view{this, &entry};
    const auto& control_flow = view.control_flow();
    const auto& value_locations = view.value_locations();
    const auto& addressing = view.addressing();
    const auto& lookups = view.prepared_lookups();

    append_line("function_view name=" + std::string(view.function_name_text()) +
                " id=" + id_text(view.function_name()) +
                " bir_blocks=" + std::to_string(view.bir_function().blocks.size()) +
                " prepared_blocks=" + std::to_string(control_flow.blocks.size()));
    append_line("control_flow function=" + std::string(view.function_name_text()) +
                " blocks=" + std::to_string(control_flow.blocks.size()) +
                " branch_conditions=" + std::to_string(control_flow.branch_conditions.size()) +
                " join_transfers=" + std::to_string(control_flow.join_transfers.size()) +
                " parallel_copy_bundles=" +
                std::to_string(control_flow.parallel_copy_bundles.size()));
    append_line("value_locations function=" + std::string(view.function_name_text()) +
                " homes=" + std::to_string(value_locations.value_homes.size()) +
                " move_bundles=" + std::to_string(value_locations.move_bundles.size()));
    for (const auto& home : value_locations.value_homes) {
      append_line("value_home function=" + std::string(view.function_name_text()) +
                  " value_id=" + std::to_string(home.value_id) +
                  " value_name=" + value_name_text(home.value_name) +
                  " kind=" + std::string(prepare::prepared_value_home_kind_name(home.kind)));
    }
    append_line("stack_layout function=" + std::string(view.function_name_text()) +
                " objects=" + std::to_string(view.stack_layout().objects.size()) +
                " frame_slots=" + std::to_string(view.stack_layout().frame_slots.size()) +
                " frame_size=" + std::to_string(view.stack_layout().frame_size_bytes) +
                " frame_align=" + std::to_string(view.stack_layout().frame_alignment_bytes));
    append_line("addressing function=" + std::string(view.function_name_text()) +
                " accesses=" + std::to_string(addressing.accesses.size()) +
                " materializations=" +
                std::to_string(addressing.address_materializations.size()) +
                " frame_size=" + std::to_string(addressing.frame_size_bytes) +
                " frame_align=" + std::to_string(addressing.frame_alignment_bytes));
    append_line("lookups function=" + std::string(view.function_name_text()) +
                " calls=" + std::to_string(lookups.call_plans.calls_by_position.size()) +
                " address_blocks=" +
                std::to_string(lookups.address_materializations.materializations_by_block.size()) +
                " memory_positions=" +
                std::to_string(lookups.memory_accesses.accesses_by_position.size()) +
                " value_homes=" + std::to_string(lookups.value_homes.homes_by_id.size()) +
                " move_bundles=" +
                std::to_string(lookups.move_bundles.bundles_by_position.size()) +
                " edge_publications=" +
                std::to_string(lookups.edge_publications.publications.size()) +
                " edge_source_producers=" +
                std::to_string(lookups.edge_publication_source_producers.producers_by_value_name.size()) +
                " branch_stack_loads=" +
                std::to_string(lookups.branch_stack_load_authorities.records.size()));

    for (const auto& block : view.blocks()) {
      const std::size_t instruction_count =
          block.block != nullptr ? block.block->insts.size() : 0;
      append_line("block function=" + std::string(view.function_name_text()) +
                  " index=" + std::to_string(block.block_index) +
                  " label=" + block_label_text(block.block_label) +
                  " bir_present=" + bool_text(block.block != nullptr) +
                  " prepared_present=" + bool_text(block.control_flow != nullptr) +
                  " instructions=" + std::to_string(instruction_count));
      for (std::size_t instruction_index = 0; instruction_index < instruction_count;
           ++instruction_index) {
        const auto cursor = view.instruction(block.block_index, instruction_index);
        append_line("cursor function=" + std::string(view.function_name_text()) +
                    " block_index=" + std::to_string(block.block_index) +
                    " instruction_index=" + std::to_string(instruction_index) +
                    " label=" + block_label_text(block.block_label) +
                    " bound=" + bool_text(cursor.has_value()));
      }
    }
  }

  return out;
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
