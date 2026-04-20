#include "x86_codegen.hpp"

#include <algorithm>

namespace c4c::backend::x86 {

namespace {

std::size_t align_up(std::size_t value, std::size_t align) {
  if (align <= 1) {
    return value;
  }
  const auto remainder = value % align;
  return remainder == 0 ? value : value + (align - remainder);
}

}  // namespace

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
  const c4c::FunctionNameId function_name_id =
      prepared_names == nullptr ? c4c::kInvalidFunctionName
                                : c4c::backend::prepare::resolve_prepared_function_name_id(
                                      *prepared_names, function.name)
                                      .value_or(c4c::kInvalidFunctionName);
  for (const auto& slot : function.local_slots) {
    if (slot.type != c4c::backend::bir::TypeKind::I8 &&
        slot.type != c4c::backend::bir::TypeKind::I16 &&
        slot.type != c4c::backend::bir::TypeKind::I32 &&
        slot.type != c4c::backend::bir::TypeKind::I64 &&
        slot.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto slot_size = slot.size_bytes != 0
                               ? slot.size_bytes
                               : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                  : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                  : 1u);
    const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
    if (slot_size == 0 || slot_size > 8 || slot_align > 16) {
      return std::nullopt;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.offsets.emplace(slot.name, next_offset);
    next_offset += slot_size;
    max_align = std::max(max_align, slot_align);
  }
  if (stack_layout != nullptr) {
    for (const auto& frame_slot : stack_layout->frame_slots) {
      if (frame_slot.function_name != function_name_id) {
        continue;
      }
      layout.frame_slot_offsets.emplace(frame_slot.slot_id, frame_slot.offset_bytes);
    }
  }
  if (function_addressing != nullptr) {
    if (function_name_id == c4c::kInvalidFunctionName ||
        function_addressing->function_name != function_name_id) {
      return std::nullopt;
    }
    const auto required_frame_alignment =
        std::max<std::size_t>(16, function_addressing->frame_alignment_bytes);
    layout.frame_alignment = required_frame_alignment;
    layout.frame_size = align_up(function_addressing->frame_size_bytes, required_frame_alignment);
    return layout;
  }
  layout.frame_alignment = max_align;
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

namespace {

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_function_memory_access(
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  if (function_addressing == nullptr || block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_memory_access(
      *function_addressing, block_label, inst_index);
}

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_frame_memory_access(
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  const auto* access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return nullptr;
  }
  return access;
}

const c4c::backend::prepare::PreparedMemoryAccess* find_prepared_symbol_memory_access(
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index) {
  const auto* access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      c4c::backend::prepare::prepared_link_name(*prepared_names, *access->address.symbol_name).empty()) {
    return nullptr;
  }
  return access;
}

bool prepared_frame_memory_accesses_match(
    const c4c::backend::prepare::PreparedMemoryAccess* lhs,
    const c4c::backend::prepare::PreparedMemoryAccess* rhs) {
  return lhs != nullptr && rhs != nullptr && lhs->address.frame_slot_id.has_value() &&
         rhs->address.frame_slot_id.has_value() &&
         *lhs->address.frame_slot_id == *rhs->address.frame_slot_id &&
         lhs->address.byte_offset == rhs->address.byte_offset;
}

std::optional<std::string> render_prepared_frame_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name) {
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot ||
      !address.frame_slot_id.has_value()) {
    return std::nullopt;
  }
  const auto frame_slot_it = local_layout.frame_slot_offsets.find(*address.frame_slot_id);
  if (frame_slot_it == local_layout.frame_slot_offsets.end()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(frame_slot_it->second) + address.byte_offset;
  if (signed_byte_offset < 0) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset),
                                              size_name);
}

std::optional<std::string> render_prepared_symbol_memory_operand_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !address.symbol_name.has_value()) {
    return std::nullopt;
  }
  const std::string_view symbol_name =
      c4c::backend::prepare::prepared_link_name(prepared_names, *address.symbol_name);
  if (symbol_name.empty()) {
    return std::nullopt;
  }
  std::string memory = std::string(size_name) + " PTR [rip + " +
                       render_asm_symbol_name(symbol_name);
  if (address.byte_offset > 0) {
    memory += " + " + std::to_string(address.byte_offset);
  } else if (address.byte_offset < 0) {
    memory += " - " + std::to_string(-address.byte_offset);
  }
  memory += "]";
  return memory;
}

std::optional<std::size_t> find_prepared_named_stack_object_frame_offset(
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::FunctionNameId function_name,
    std::string_view object_name) {
  if (stack_layout == nullptr || prepared_names == nullptr || function_name == c4c::kInvalidFunctionName ||
      object_name.empty()) {
    return std::nullopt;
  }

  const auto* matched_object = [&]() -> const c4c::backend::prepare::PreparedStackObject* {
    for (const auto& object : stack_layout->objects) {
      if (object.function_name != function_name) {
        continue;
      }
      if (c4c::backend::prepare::prepared_stack_object_name(*prepared_names, object) ==
          object_name) {
        return &object;
      }
    }
    return nullptr;
  }();
  if (matched_object == nullptr) {
    return std::nullopt;
  }

  for (const auto& frame_slot : stack_layout->frame_slots) {
    if (frame_slot.function_name == function_name && frame_slot.object_id == matched_object->object_id) {
      return frame_slot.offset_bytes;
    }
  }
  return std::nullopt;
}

std::optional<std::size_t> find_prepared_value_stack_object_frame_offset(
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  if (stack_layout == nullptr || prepared_names == nullptr || function_name == c4c::kInvalidFunctionName ||
      value_name.empty()) {
    return std::nullopt;
  }

  const auto value_name_id =
      c4c::backend::prepare::resolve_prepared_value_name_id(*prepared_names, value_name);
  if (!value_name_id.has_value()) {
    return std::nullopt;
  }

  const auto* matched_object = [&]() -> const c4c::backend::prepare::PreparedStackObject* {
    for (const auto& object : stack_layout->objects) {
      if (object.function_name != function_name || object.value_name != value_name_id) {
        continue;
      }
      return &object;
    }
    return nullptr;
  }();
  if (matched_object == nullptr) {
    return std::nullopt;
  }

  for (const auto& frame_slot : stack_layout->frame_slots) {
    if (frame_slot.function_name == function_name && frame_slot.object_id == matched_object->object_id) {
      return frame_slot.offset_bytes;
    }
  }
  return std::nullopt;
}

const c4c::backend::prepare::PreparedBranchCondition*
find_required_prepared_guard_branch_condition(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id) {
  if (function_control_flow == nullptr || prepared_names == nullptr ||
      block_label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  if (c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow,
                                                              block_label_id) == nullptr) {
    return nullptr;
  }
  const auto* branch_condition =
      c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow, block_label_id);
  if (branch_condition == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
  }
  return branch_condition;
}

bool has_authoritative_prepared_control_flow_block(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::BlockLabelId block_label_id) {
  if (function_control_flow == nullptr || block_label_id == c4c::kInvalidBlockLabel) {
    return false;
  }
  return c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow,
                                                                 block_label_id) != nullptr;
}

bool has_authoritative_prepared_short_circuit_continuation(
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation) {
  if (!continuation.has_value()) {
    return false;
  }
  return continuation->incoming_label != c4c::kInvalidBlockLabel ||
         continuation->true_label != c4c::kInvalidBlockLabel ||
         continuation->false_label != c4c::kInvalidBlockLabel;
}

const c4c::backend::bir::Block* resolve_authoritative_prepared_branch_target(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Block& source_block,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (function_control_flow == nullptr || prepared_names == nullptr ||
      block_label_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const auto* prepared_block =
      c4c::backend::prepare::find_prepared_control_flow_block(*function_control_flow, block_label_id);
  if (prepared_block == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }
  if (prepared_block->terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
      prepared_block->branch_target_label == c4c::kInvalidBlockLabel) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  const auto* target = find_block(c4c::backend::prepare::prepared_block_label(
      *prepared_names, prepared_block->branch_target_label));
  if (target == nullptr || target == &source_block) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }
  return target;
}

}  // namespace

std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string render_prepared_stack_address_expr(std::size_t byte_offset) {
  if (byte_offset == 0) {
    return "[rsp]";
  }
  return "[rsp + " + std::to_string(byte_offset) + "]";
}

std::optional<std::string> render_prepared_named_stack_object_address_if_supported(
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::FunctionNameId function_name,
    std::string_view object_name) {
  const auto frame_offset = find_prepared_named_stack_object_frame_offset(
      stack_layout, prepared_names, function_name, object_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset);
}

std::optional<std::string> render_prepared_value_stack_object_address_if_supported(
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  const auto frame_offset = find_prepared_value_stack_object_frame_offset(
      stack_layout, prepared_names, function_name, value_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset);
}

std::optional<std::string> render_prepared_named_stack_object_memory_operand_if_supported(
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::FunctionNameId function_name,
    const std::optional<c4c::backend::bir::MemoryAddress>& address,
    std::string_view size_name) {
  if (!address.has_value() ||
      address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto frame_offset = find_prepared_named_stack_object_frame_offset(
      stack_layout, prepared_names, function_name, address->base_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(*frame_offset) + address->byte_offset;
  if (signed_byte_offset < 0) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset),
                                              size_name);
}

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name) {
  const auto slot_it = local_layout.offsets.find(slot_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(slot_it->second + stack_byte_bias, size_name);
}

std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_names,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers) {
  if (!function.params.empty() || function.blocks.empty() || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  const c4c::FunctionNameId function_name_id =
      prepared_names == nullptr ? c4c::kInvalidFunctionName
                                : c4c::backend::prepare::resolve_prepared_function_name_id(
                                      *prepared_names, function.name)
                                      .value_or(c4c::kInvalidFunctionName);
  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  const auto render_function_return =
      [&](std::int32_t returned_imm) -> std::string {
    std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
    if (layout->frame_size != 0) {
      rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    rendered += "    ret\n";
    return rendered;
  };
  std::unordered_set<std::string_view> same_module_global_names;

  std::unordered_set<std::string_view> rendered_blocks;
  std::function<std::optional<std::string>(
      const c4c::backend::bir::Block&,
      const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>
      render_block =
          [&](const c4c::backend::bir::Block& block,
              const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
                  continuation) -> std::optional<std::string> {
    if (!rendered_blocks.insert(block.label).second &&
        block.terminator.kind != c4c::backend::bir::TerminatorKind::Return) {
      return std::nullopt;
    }
    const c4c::BlockLabelId block_label_id =
        prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                  : c4c::backend::prepare::resolve_prepared_block_label_id(
                                        *prepared_names, block.label)
                                        .value_or(c4c::kInvalidBlockLabel);

    auto rendered_load_or_store =
        [&](const c4c::backend::bir::Inst& inst,
            std::size_t inst_index,
            std::optional<std::string_view>* current_i32_name,
            std::optional<std::string_view>* previous_i32_name,
            std::optional<std::string_view>* current_i8_name,
            std::optional<std::string_view>* current_ptr_name,
            std::optional<MaterializedI32Compare>* current_materialized_compare)
        -> std::optional<std::string> {
      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
        const auto* prepared_access =
            find_prepared_function_memory_access(function_addressing, block_label_id, inst_index);
        std::optional<std::string> memory;
        if (prepared_access != nullptr) {
          memory = render_prepared_frame_slot_memory_operand_if_supported(
              *layout,
              prepared_access->address,
              load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
              : load->result.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                     : "DWORD");
        }
        if (!memory.has_value()) {
          return std::nullopt;
        }
        *current_materialized_compare = std::nullopt;
        if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_i8_name = std::nullopt;
          *current_ptr_name = load->result.name;
          return "    mov rax, " + *memory + "\n";
        }
        if (load->result.type == c4c::backend::bir::TypeKind::I32) {
          *current_i32_name = load->result.name;
          *previous_i32_name = std::nullopt;
          *current_i8_name = std::nullopt;
          *current_ptr_name = std::nullopt;
          return "    mov eax, " + *memory + "\n";
        }
        if (load->result.type == c4c::backend::bir::TypeKind::I8) {
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_i8_name = load->result.name;
          *current_ptr_name = std::nullopt;
          return "    movsx eax, " + *memory + "\n";
        }
        return std::nullopt;
      }

      if (const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst)) {
        if ((load->result.type != c4c::backend::bir::TypeKind::I32 &&
             load->result.type != c4c::backend::bir::TypeKind::Ptr) ||
            load->result.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto* prepared_access =
            find_prepared_symbol_memory_access(
                prepared_names, function_addressing, block_label_id, inst_index);
        if (prepared_access == nullptr) {
          return std::nullopt;
        }
        auto memory = render_prepared_symbol_memory_operand_if_supported(
            *prepared_names,
            prepared_access->address,
            load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD",
            render_asm_symbol_name);
        if (!memory.has_value()) {
          return std::nullopt;
        }
        const std::string_view resolved_global_name =
            c4c::backend::prepare::prepared_link_name(
                *prepared_names, *prepared_access->address.symbol_name);
        const auto* global = find_same_module_global(resolved_global_name);
        const auto global_byte_offset = prepared_access->address.byte_offset;
        if (global == nullptr ||
            !same_module_global_supports_scalar_load(*global, load->result.type, global_byte_offset)) {
          return std::nullopt;
        }
        same_module_global_names.insert(global->name);
        *current_materialized_compare = std::nullopt;
        *current_i8_name = std::nullopt;
        if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_ptr_name = load->result.name;
          return "    mov rax, " + *memory + "\n";
        }
        *current_i32_name = load->result.name;
        *previous_i32_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov eax, " + *memory + "\n";
      }

      if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
        if (store->address.has_value() || store->value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        const auto* prepared_access =
            find_prepared_symbol_memory_access(
                prepared_names, function_addressing, block_label_id, inst_index);
        if (prepared_access == nullptr) {
          return std::nullopt;
        }
        auto memory = render_prepared_symbol_memory_operand_if_supported(
            *prepared_names, prepared_access->address, "DWORD", render_asm_symbol_name);
        if (!memory.has_value()) {
          return std::nullopt;
        }
        const std::string_view resolved_global_name =
            c4c::backend::prepare::prepared_link_name(
                *prepared_names, *prepared_access->address.symbol_name);
        const auto global_byte_offset = prepared_access->address.byte_offset;
        const auto* global = find_same_module_global(resolved_global_name);
        if (global == nullptr || global->type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        same_module_global_names.insert(global->name);
        *current_materialized_compare = std::nullopt;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          return "    mov " + *memory + ", " +
                 std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
            current_i32_name->has_value() && *current_i32_name == store->value.name) {
          return "    mov " + *memory + ", eax\n";
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
            previous_i32_name->has_value() && *previous_i32_name == store->value.name) {
          return "    mov " + *memory + ", ecx\n";
        }
        return std::nullopt;
      }

      const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store == nullptr || store->byte_offset != 0) {
        return std::nullopt;
      }
      const auto* prepared_access =
          find_prepared_function_memory_access(function_addressing, block_label_id, inst_index);
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            *layout,
            prepared_access->address,
            store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
            : store->value.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                   : "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      *current_materialized_compare = std::nullopt;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I32) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov " + *memory + ", " +
               std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::I32 &&
          current_i32_name->has_value() && *current_i32_name == store->value.name) {
        *current_i32_name = store->value.name;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov " + *memory + ", eax\n";
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::I32 &&
          previous_i32_name->has_value() && *previous_i32_name == store->value.name) {
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov " + *memory + ", ecx\n";
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I8) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov " + *memory + ", " +
               std::to_string(static_cast<std::int8_t>(store->value.immediate)) + "\n";
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
        *current_i8_name = std::nullopt;
        if (current_ptr_name->has_value() && *current_ptr_name == store->value.name) {
          *current_ptr_name = std::nullopt;
          return "    mov " + *memory + ", rax\n";
        }
        const auto pointee_address = render_prepared_value_stack_object_address_if_supported(
            stack_layout, prepared_names, function_name_id, store->value.name);
        if (!pointee_address.has_value()) {
          *current_ptr_name = std::nullopt;
          return std::string{};
        }
        *current_ptr_name = std::nullopt;
        return "    lea rax, " + *pointee_address + "\n    mov " + *memory + ", rax\n";
      }
      return std::nullopt;
    };

    std::string body;
    std::optional<std::string_view> current_i32_name;
    std::optional<std::string_view> previous_i32_name;
    std::optional<std::string_view> current_i8_name;
    std::optional<std::string_view> current_ptr_name;
    std::optional<MaterializedI32Compare> current_materialized_compare;
    std::size_t compare_index = block.insts.size();
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
      if (block.insts.empty()) {
        return std::nullopt;
      }
      compare_index = block.insts.size() - 1;
    } else if (continuation.has_value() &&
               block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch &&
               !block.insts.empty()) {
      const auto* branch_compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
      if (branch_compare != nullptr &&
          (branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
           branch_compare->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
          branch_compare->operand_type == c4c::backend::bir::TypeKind::I32 &&
          (branch_compare->result.type == c4c::backend::bir::TypeKind::I1 ||
           branch_compare->result.type == c4c::backend::bir::TypeKind::I32)) {
        compare_index = block.insts.size() - 1;
      }
    }

    for (std::size_t index = 0; index < compare_index; ++index) {
      const auto rendered_inst =
          rendered_load_or_store(block.insts[index],
                                 index,
                                 &current_i32_name,
                                 &previous_i32_name,
                                 &current_i8_name,
                                 &current_ptr_name,
                                 &current_materialized_compare);
      if (rendered_inst.has_value()) {
        body += *rendered_inst;
        continue;
      }

      if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&block.insts[index])) {
        if (call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
            call->is_variadic || call->args.size() != call->arg_types.size() || call->args.size() > 6 ||
            bounded_same_module_helper_names.find(call->callee) ==
                bounded_same_module_helper_names.end()) {
          return std::nullopt;
        }
        std::string rendered_call;
        for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
          if (call->arg_types[arg_index] != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          const auto& arg = call->args[arg_index];
          if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
            rendered_call += "    mov ";
            rendered_call += kArgRegs32[arg_index];
            rendered_call += ", ";
            rendered_call += std::to_string(static_cast<std::int32_t>(arg.immediate));
            rendered_call += "\n";
            continue;
          }
          if (arg.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_i32_name.has_value() || arg.name != *current_i32_name) {
            return std::nullopt;
          }
          rendered_call += "    mov ";
          rendered_call += kArgRegs32[arg_index];
          rendered_call += ", eax\n";
        }
        rendered_call += "    xor eax, eax\n";
        rendered_call += "    call ";
        rendered_call += render_asm_symbol_name(call->callee);
        rendered_call += "\n";
        current_materialized_compare = std::nullopt;
        previous_i32_name = std::nullopt;
        current_i8_name = std::nullopt;
        current_ptr_name = std::nullopt;
        current_i32_name =
            call->result.has_value() && call->result->type == c4c::backend::bir::TypeKind::I32
                ? std::optional<std::string_view>(call->result->name)
                : std::nullopt;
        body += rendered_call;
        continue;
      }

      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index]);
      if (binary != nullptr &&
          (binary->opcode == c4c::backend::bir::BinaryOpcode::Add ||
           binary->opcode == c4c::backend::bir::BinaryOpcode::Sub) &&
          binary->operand_type == c4c::backend::bir::TypeKind::I32 &&
          binary->result.type == c4c::backend::bir::TypeKind::I32 &&
          current_i32_name.has_value()) {
        const bool lhs_is_current_rhs_is_imm =
            binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
            binary->lhs.name == *current_i32_name &&
            binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
            binary->rhs.type == c4c::backend::bir::TypeKind::I32;
        const bool rhs_is_current_lhs_is_imm =
            binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
            binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
            binary->rhs.name == *current_i32_name &&
            binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
            binary->lhs.type == c4c::backend::bir::TypeKind::I32;
        if (lhs_is_current_rhs_is_imm || rhs_is_current_lhs_is_imm) {
          const auto immediate = lhs_is_current_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
          body += "    mov ecx, eax\n";
          body += std::string("    ") +
                  (binary->opcode == c4c::backend::bir::BinaryOpcode::Add ? "add" : "sub") +
                  " eax, " + std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
          current_materialized_compare = std::nullopt;
          previous_i32_name = current_i32_name;
          current_i32_name = binary->result.name;
          current_i8_name = std::nullopt;
          current_ptr_name = std::nullopt;
          continue;
        }
      }
      if (binary != nullptr &&
          (binary->opcode == c4c::backend::bir::BinaryOpcode::Eq ||
           binary->opcode == c4c::backend::bir::BinaryOpcode::Ne) &&
          binary->operand_type == c4c::backend::bir::TypeKind::I32 &&
          binary->result.type == c4c::backend::bir::TypeKind::I1) {
        auto compare_setup = [&]() -> std::optional<std::string> {
          if (current_i32_name.has_value()) {
            const bool lhs_is_current_rhs_is_imm =
                binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                binary->lhs.name == *current_i32_name &&
                binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                binary->rhs.type == c4c::backend::bir::TypeKind::I32;
            const bool rhs_is_current_lhs_is_imm =
                binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                binary->rhs.name == *current_i32_name &&
                binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                binary->lhs.type == c4c::backend::bir::TypeKind::I32;
            if (lhs_is_current_rhs_is_imm || rhs_is_current_lhs_is_imm) {
              const auto compare_immediate =
                  lhs_is_current_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
              if (compare_immediate == 0) {
                return std::string("    test eax, eax\n");
              }
              return "    cmp eax, " +
                     std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
            }
          }
          if (binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
              binary->lhs.type == c4c::backend::bir::TypeKind::I32 &&
              binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
              binary->rhs.type == c4c::backend::bir::TypeKind::I32) {
            return "    mov eax, " +
                   std::to_string(static_cast<std::int32_t>(binary->lhs.immediate)) +
                   "\n    cmp eax, " +
                   std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
          }
          return std::nullopt;
        }();
        if (!compare_setup.has_value()) {
          return std::nullopt;
        }
        current_materialized_compare = MaterializedI32Compare{
            .i1_name = binary->result.name,
            .opcode = binary->opcode,
            .compare_setup = *compare_setup,
        };
        current_i32_name = std::nullopt;
        previous_i32_name = std::nullopt;
        current_i8_name = std::nullopt;
        current_ptr_name = std::nullopt;
        continue;
      }

      const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&block.insts[index]);
      if (cast != nullptr && cast->opcode == c4c::backend::bir::CastOpcode::ZExt &&
          cast->operand.type == c4c::backend::bir::TypeKind::I1 &&
          cast->result.type == c4c::backend::bir::TypeKind::I32 &&
          cast->operand.kind == c4c::backend::bir::Value::Kind::Named &&
          current_materialized_compare.has_value() &&
          current_materialized_compare->i1_name == cast->operand.name) {
        current_materialized_compare->i32_name = cast->result.name;
        current_i32_name = cast->result.name;
        previous_i32_name = std::nullopt;
        current_i8_name = std::nullopt;
        current_ptr_name = std::nullopt;
        continue;
      }
      if (cast == nullptr || cast->opcode != c4c::backend::bir::CastOpcode::SExt ||
          cast->operand.type != c4c::backend::bir::TypeKind::I8 ||
          cast->result.type != c4c::backend::bir::TypeKind::I32 ||
          cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
          !current_i8_name.has_value() || *current_i8_name != cast->operand.name) {
        return std::nullopt;
      }
      current_materialized_compare = std::nullopt;
      current_i8_name = std::nullopt;
      current_i32_name = cast->result.name;
      previous_i32_name = std::nullopt;
      current_ptr_name = std::nullopt;
    }

    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Return) {
      if (compare_index != block.insts.size() || !block.terminator.value.has_value()) {
        return std::nullopt;
      }
      const auto& returned = *block.terminator.value;
      if (returned.kind == c4c::backend::bir::Value::Kind::Immediate &&
          returned.type == c4c::backend::bir::TypeKind::I32) {
        return body + render_function_return(static_cast<std::int32_t>(returned.immediate));
      }
      if (returned.kind == c4c::backend::bir::Value::Kind::Named &&
          returned.type == c4c::backend::bir::TypeKind::I32 &&
          current_i32_name.has_value() && *current_i32_name == returned.name) {
        if (layout->frame_size != 0) {
          body += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
        }
        body += "    ret\n";
        return body;
      }
      return std::nullopt;
    }

    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
      if (continuation.has_value()) {
        if (prepared_names == nullptr) {
          return std::nullopt;
        }
        const auto compare_join_render_plan =
            c4c::backend::x86::build_prepared_compare_join_entry_render_plan(
                *prepared_names,
                function_control_flow,
                function,
                block,
                *continuation,
                compare_index,
                current_materialized_compare,
                current_i32_name,
                [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan)
                    -> std::optional<ShortCircuitPlan> {
                  return c4c::backend::x86::build_prepared_short_circuit_plan(
                      *prepared_names, prepared_plan, find_block);
                });
        if (compare_join_render_plan.has_value()) {
          return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
              function.name,
              body,
              *compare_join_render_plan,
              prepared_names,
              find_block,
              render_block);
        }
        if (has_authoritative_prepared_short_circuit_continuation(continuation)) {
          throw std::invalid_argument(
              "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
        }
      }
      if (compare_index != block.insts.size()) {
        return std::nullopt;
      }
      const auto* target = resolve_authoritative_prepared_branch_target(
          function_control_flow, prepared_names, block_label_id, block, find_block);
      if (target == nullptr) {
        target = find_block(block.terminator.target_label);
        if (target == nullptr || target == &block) {
          return std::nullopt;
        }
      }
      const auto rendered_target = render_block(*target, continuation);
      if (!rendered_target.has_value()) {
        return std::nullopt;
      }
      return body + *rendered_target;
    }

    if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        compare_index != block.insts.size() - 1) {
      return std::nullopt;
    }

    const auto try_render_short_circuit_plan =
        [&]() -> std::optional<std::string> {
      if (function_control_flow == nullptr || prepared_names == nullptr) {
        return std::nullopt;
      }
      const auto block_label_id =
          c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, block.label);
      if (!block_label_id.has_value()) {
        return std::nullopt;
      }

      const auto join_context =
          c4c::backend::x86::find_prepared_short_circuit_join_context_if_supported(
              *prepared_names, *function_control_flow, function, *block_label_id);
      if (!join_context.has_value()) {
        return std::nullopt;
      }

      const auto short_circuit_render_plan =
          c4c::backend::x86::build_prepared_short_circuit_entry_render_plan(
              *prepared_names,
              function_control_flow,
              function,
              block,
              *join_context,
              compare_index,
              current_materialized_compare,
              current_i32_name,
              [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan& prepared_plan)
                  -> std::optional<ShortCircuitPlan> {
                return c4c::backend::x86::build_prepared_short_circuit_plan(
                    *prepared_names, prepared_plan, find_block);
              });
      if (!short_circuit_render_plan.has_value()) {
        return std::nullopt;
      }
      return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
          function.name,
          body,
          *short_circuit_render_plan,
          prepared_names,
          find_block,
          render_block);
    };
    if (const auto rendered_short_circuit = try_render_short_circuit_plan();
        rendered_short_circuit.has_value()) {
      return rendered_short_circuit;
    }
    if (has_authoritative_prepared_control_flow_block(function_control_flow, block_label_id) &&
        c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                              block_label_id) == nullptr) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }
    if (function_control_flow != nullptr &&
        prepared_names != nullptr &&
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            *prepared_names, *function_control_flow, block_label_id)
            .has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
    }

    if (prepared_names == nullptr) {
      return std::nullopt;
    }
    const auto plain_cond_render_plan =
        c4c::backend::x86::build_prepared_plain_cond_entry_render_plan(
            *prepared_names,
            function_control_flow,
            block,
            compare_index,
            current_materialized_compare,
            current_i32_name,
            find_block);
    if (!plain_cond_render_plan.has_value()) {
      return std::nullopt;
    }
    return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
        function.name,
        body,
        *plain_cond_render_plan,
        nullptr,
        find_block,
        render_block);
  };

  auto asm_text = std::string(asm_prefix);
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const auto rendered_entry = render_block(entry, std::nullopt);
  if (!rendered_entry.has_value()) {
    return std::nullopt;
  }
  std::string rendered_same_module_globals;
  for (const auto& global : module.globals) {
    if (same_module_global_names.find(global.name) == same_module_global_names.end() &&
        bounded_same_module_helper_global_names.find(global.name) ==
            bounded_same_module_helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_same_module_globals += *rendered_global_data;
  }
  asm_text += *rendered_entry;
  asm_text += rendered_same_module_globals;
  return prepend_bounded_same_module_helpers(std::move(asm_text));
}

std::optional<std::string> render_prepared_local_i32_arithmetic_guard_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (!function.params.empty() || function.blocks.size() != 3 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      prepared_arch != c4c::TargetArch::X86_64 || entry.insts.empty()) {
    return std::nullopt;
  }

  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  struct NamedLocalI32Expr {
    enum class Kind { LocalLoad, Add, Sub };
    Kind kind = Kind::LocalLoad;
    std::string operand;
    c4c::backend::bir::Value lhs;
    c4c::backend::bir::Value rhs;
  };
  std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
  const auto render_guard_return = [&](std::int32_t returned_imm) -> std::string {
    std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
    if (layout->frame_size != 0) {
      rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    rendered += "    ret\n";
    return rendered;
  };

  std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_operand;
  std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_value;
  render_i32_operand = [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      return std::to_string(static_cast<std::int32_t>(value.immediate));
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    const auto expr_it = named_i32_exprs.find(value.name);
    if (expr_it == named_i32_exprs.end() ||
        expr_it->second.kind != NamedLocalI32Expr::Kind::LocalLoad) {
      return std::nullopt;
    }
    return expr_it->second.operand;
  };
  render_i32_value = [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (const auto operand = render_i32_operand(value); operand.has_value()) {
      return "    mov eax, " + *operand + "\n";
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    const auto expr_it = named_i32_exprs.find(value.name);
    if (expr_it == named_i32_exprs.end()) {
      return std::nullopt;
    }
    const auto& expr = expr_it->second;
    if (expr.kind == NamedLocalI32Expr::Kind::Add) {
      if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
        if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
          return *lhs_render + "    add eax, " + *rhs_operand + "\n";
        }
      }
      if (const auto rhs_render = render_i32_value(expr.rhs); rhs_render.has_value()) {
        if (const auto lhs_operand = render_i32_operand(expr.lhs); lhs_operand.has_value()) {
          return *rhs_render + "    add eax, " + *lhs_operand + "\n";
        }
      }
      return std::nullopt;
    }
    if (expr.kind == NamedLocalI32Expr::Kind::Sub) {
      if (const auto lhs_render = render_i32_value(expr.lhs); lhs_render.has_value()) {
        if (const auto rhs_operand = render_i32_operand(expr.rhs); rhs_operand.has_value()) {
          return *lhs_render + "    sub eax, " + *rhs_operand + "\n";
        }
      }
      return std::nullopt;
    }
    return "    mov eax, " + expr.operand + "\n";
  };

  auto asm_text = std::string(asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);

  for (std::size_t index = 0; index + 1 < entry.insts.size(); ++index) {
    const auto& inst = entry.insts[index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(function_addressing, entry_label_id, index);
      if (store->byte_offset != 0 || store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            *layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      asm_text += "    mov " + *memory + ", " +
                  std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
      continue;
    }

    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(function_addressing, entry_label_id, index);
      if (load->byte_offset != 0 || load->result.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            *layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      named_i32_exprs[load->result.name] = NamedLocalI32Expr{
          .kind = NamedLocalI32Expr::Kind::LocalLoad,
          .operand = *memory,
      };
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        (binary->opcode != c4c::backend::bir::BinaryOpcode::Add &&
         binary->opcode != c4c::backend::bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    named_i32_exprs[binary->result.name] = NamedLocalI32Expr{
        .kind = binary->opcode == c4c::backend::bir::BinaryOpcode::Add
                    ? NamedLocalI32Expr::Kind::Add
                    : NamedLocalI32Expr::Kind::Sub,
        .lhs = binary->lhs,
        .rhs = binary->rhs,
    };
  }

  const auto* prepared_branch_condition =
      find_required_prepared_guard_branch_condition(
          function_control_flow, prepared_names, entry_label_id);
  if (function_control_flow != nullptr && prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *prepared_names, *function_control_flow, entry_label_id)
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  c4c::backend::bir::Value compared_value;
  std::int64_t compare_immediate = 0;
  auto compare_opcode = c4c::backend::bir::BinaryOpcode::Eq;
  std::string true_label = entry.terminator.true_label;
  std::string false_label = entry.terminator.false_label;

  if (prepared_branch_condition != nullptr) {
    const auto* matched_prepared_immediate_branch =
        static_cast<const c4c::backend::prepare::PreparedBranchCondition*>(nullptr);
    const auto* matched_prepared_compared_value =
        static_cast<const c4c::backend::bir::Value*>(nullptr);
    for (const auto& [value_name, _] : named_i32_exprs) {
      const auto value_name_id =
          c4c::backend::prepare::resolve_prepared_value_name_id(*prepared_names, value_name);
      if (!value_name_id.has_value()) {
        continue;
      }
      const auto* candidate = c4c::backend::prepare::find_prepared_i32_immediate_branch_condition(
          *prepared_names, *function_control_flow, entry_label_id, *value_name_id);
      if (candidate == nullptr) {
        continue;
      }

      const auto* candidate_value =
          candidate->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
                  candidate->lhs->name == value_name
              ? &*candidate->lhs
              : &*candidate->rhs;
      if (matched_prepared_immediate_branch != nullptr &&
          candidate_value->name != matched_prepared_compared_value->name) {
        return std::nullopt;
      }
      matched_prepared_immediate_branch = candidate;
      matched_prepared_compared_value = candidate_value;
    }
    const auto* prepared_immediate_branch = matched_prepared_immediate_branch;
    const auto* prepared_compared_value = matched_prepared_compared_value;
    if (prepared_immediate_branch == nullptr || prepared_compared_value == nullptr) {
      return std::nullopt;
    }

    const bool lhs_is_value_rhs_is_imm =
        prepared_immediate_branch->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        prepared_immediate_branch->rhs->type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_value_lhs_is_imm =
        prepared_immediate_branch->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        prepared_immediate_branch->lhs->type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
      return std::nullopt;
    }

    compared_value = *prepared_compared_value;
    compare_immediate = lhs_is_value_rhs_is_imm ? prepared_immediate_branch->rhs->immediate
                                                : prepared_immediate_branch->lhs->immediate;
    compare_opcode = *prepared_immediate_branch->predicate;
    const auto target_labels =
        c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
            *prepared_names,
            function_control_flow,
            entry_label_id,
            entry,
            *prepared_immediate_branch);
    if (!target_labels.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }
    true_label = std::string(
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->true_label));
    false_label = std::string(
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->false_label));
  } else {
    const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.back());
    if (compare == nullptr ||
        (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
         compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
        compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
        (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
         compare->result.type != c4c::backend::bir::TypeKind::I32) ||
        entry.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
        entry.terminator.condition.name != compare->result.name) {
      return std::nullopt;
    }

    const auto* raw_compared_value = [&]() -> const c4c::backend::bir::Value* {
      const bool lhs_is_value_rhs_is_imm =
          compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare->rhs.type == c4c::backend::bir::TypeKind::I32;
      const bool rhs_is_value_lhs_is_imm =
          compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare->lhs.type == c4c::backend::bir::TypeKind::I32;
      if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
        return static_cast<const c4c::backend::bir::Value*>(nullptr);
      }
      return &(lhs_is_value_rhs_is_imm ? compare->lhs : compare->rhs);
    }();
    if (raw_compared_value == nullptr) {
      return std::nullopt;
    }

    compared_value = *raw_compared_value;
    compare_immediate =
        raw_compared_value == &compare->lhs ? compare->rhs.immediate : compare->lhs.immediate;
    compare_opcode = compare->opcode;
  }

  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  const auto* true_block = find_block(true_label);
  const auto* false_block = find_block(false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto compared_render = render_i32_value(compared_value);
  if (!compared_render.has_value()) {
    return std::nullopt;
  }
  asm_text += *compared_render;
  asm_text += compare_immediate == 0
                  ? "    test eax, eax\n"
                  : "    cmp eax, " +
                        std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
  asm_text += std::string("    ") +
              (compare_opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je") + " .L" +
              function.name + "_" + false_block->label + "\n";

  const auto render_return_block =
      [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
    const auto& value = *block.terminator.value;
    if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    return render_guard_return(static_cast<std::int32_t>(value.immediate));
  };
  const auto rendered_true = render_return_block(*true_block);
  const auto rendered_false = render_return_block(*false_block);
  if (!rendered_true.has_value() || !rendered_false.has_value()) {
    return std::nullopt;
  }
  asm_text += *rendered_true;
  asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
  asm_text += *rendered_false;
  return asm_text;
}

std::optional<std::string> render_prepared_local_i16_arithmetic_guard_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (!function.params.empty() || function.blocks.size() != 3 ||
      prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      entry.insts.size() != 9) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const c4c::backend::bir::Block* true_block = find_block(entry.terminator.true_label);
  const c4c::backend::bir::Block* false_block = find_block(entry.terminator.false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto* store_zero = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[0]);
  const auto* load_initial = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[1]);
  const auto* sext_initial = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[2]);
  const auto* add_one = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[3]);
  const auto* trunc_updated = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[4]);
  const auto* store_updated = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[5]);
  const auto* load_updated = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[6]);
  const auto* sext_updated = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[7]);
  const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[8]);
  if (store_zero == nullptr || load_initial == nullptr || sext_initial == nullptr ||
      add_one == nullptr || trunc_updated == nullptr || store_updated == nullptr ||
      load_updated == nullptr || sext_updated == nullptr || compare == nullptr) {
    return std::nullopt;
  }
  if (store_zero->byte_offset != 0 || load_initial->byte_offset != 0 ||
      store_updated->byte_offset != 0 || load_updated->byte_offset != 0 ||
      store_zero->address.has_value() || load_initial->address.has_value() ||
      store_updated->address.has_value() || load_updated->address.has_value()) {
    return std::nullopt;
  }
  if (store_zero->slot_name != load_initial->slot_name ||
      store_zero->slot_name != store_updated->slot_name ||
      store_zero->slot_name != load_updated->slot_name) {
    return std::nullopt;
  }
  if (store_zero->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_zero->value.type != c4c::backend::bir::TypeKind::I16 ||
      load_initial->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_initial->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_initial->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_initial->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_initial->operand.name != load_initial->result.name ||
      sext_initial->result.type != c4c::backend::bir::TypeKind::I32 ||
      add_one->opcode != c4c::backend::bir::BinaryOpcode::Add ||
      add_one->operand_type != c4c::backend::bir::TypeKind::I32 ||
      add_one->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      add_one->lhs.name != sext_initial->result.name ||
      add_one->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      add_one->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      add_one->rhs.immediate != 1 ||
      add_one->result.type != c4c::backend::bir::TypeKind::I32 ||
      trunc_updated->opcode != c4c::backend::bir::CastOpcode::Trunc ||
      trunc_updated->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      trunc_updated->operand.type != c4c::backend::bir::TypeKind::I32 ||
      trunc_updated->operand.name != add_one->result.name ||
      trunc_updated->result.type != c4c::backend::bir::TypeKind::I16 ||
      store_updated->value.kind != c4c::backend::bir::Value::Kind::Named ||
      store_updated->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_updated->value.name != trunc_updated->result.name ||
      load_updated->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_updated->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_updated->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_updated->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_updated->operand.name != load_updated->result.name ||
      sext_updated->result.type != c4c::backend::bir::TypeKind::I32 ||
      (compare->opcode != c4c::backend::bir::BinaryOpcode::Eq &&
       compare->opcode != c4c::backend::bir::BinaryOpcode::Ne) ||
      compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
      compare->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      compare->lhs.name != sext_updated->result.name ||
      compare->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      compare->rhs.type != c4c::backend::bir::TypeKind::I32 ||
      compare->result.kind != c4c::backend::bir::Value::Kind::Named ||
      compare->result.name != entry.terminator.condition.name) {
    return std::nullopt;
  }

  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  std::optional<std::string> short_memory;
  if (const auto* prepared_access =
          find_prepared_function_memory_access(function_addressing, entry_label_id, 0);
      prepared_access != nullptr) {
    short_memory =
        render_prepared_frame_slot_memory_operand_if_supported(*layout,
                                                               prepared_access->address,
                                                               "WORD");
  }
  if (!short_memory.has_value()) {
    return std::nullopt;
  }
  const auto render_return_block =
      [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
    const auto& value = *block.terminator.value;
    if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    std::string rendered =
        "    mov eax, " + std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
    if (layout->frame_size != 0) {
      rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    rendered += "    ret\n";
    return rendered;
  };
  const auto* prepared_branch_condition =
      find_required_prepared_guard_branch_condition(
          function_control_flow, prepared_names, entry_label_id);
  if (function_control_flow != nullptr && prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *prepared_names, *function_control_flow, entry_label_id)
          .has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
  }

  auto compare_opcode = compare->opcode;
  std::int64_t compare_immediate = compare->rhs.immediate;
  std::string true_label = entry.terminator.true_label;
  std::string false_label = entry.terminator.false_label;
  if (prepared_branch_condition != nullptr) {
    if (!prepared_branch_condition->predicate.has_value() ||
        !prepared_branch_condition->compare_type.has_value() ||
        !prepared_branch_condition->lhs.has_value() ||
        !prepared_branch_condition->rhs.has_value() ||
        *prepared_branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
        prepared_branch_condition->condition_value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }

    const bool lhs_is_value_rhs_is_imm =
        prepared_branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
        prepared_branch_condition->lhs->name == sext_updated->result.name &&
        prepared_branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        prepared_branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_value_lhs_is_imm =
        prepared_branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
        prepared_branch_condition->rhs->name == sext_updated->result.name &&
        prepared_branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        prepared_branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
      return std::nullopt;
    }

    compare_opcode = *prepared_branch_condition->predicate;
    compare_immediate =
        lhs_is_value_rhs_is_imm ? prepared_branch_condition->rhs->immediate
                                : prepared_branch_condition->lhs->immediate;
    const auto target_labels =
        c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
            *prepared_names,
            function_control_flow,
            entry_label_id,
            entry,
            *prepared_branch_condition);
    if (!target_labels.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
    }
    true_label = std::string(
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->true_label));
    false_label = std::string(
        c4c::backend::prepare::prepared_block_label(*prepared_names, target_labels->false_label));
  }
  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  true_block = find_block(true_label);
  false_block = find_block(false_label);
  if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
      false_block == &entry ||
      true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  const auto rendered_true = render_return_block(*true_block);
  const auto rendered_false = render_return_block(*false_block);
  if (!rendered_true.has_value() || !rendered_false.has_value()) {
    return std::nullopt;
  }

  std::string asm_text = std::string(asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    mov " + *short_memory + ", " +
              std::to_string(static_cast<std::int16_t>(store_zero->value.immediate)) + "\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += "    add eax, 1\n";
  asm_text += "    mov " + *short_memory + ", ax\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += compare_immediate == 0
                  ? "    test eax, eax\n"
                  : "    cmp eax, " +
                        std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
  asm_text += "    " +
              std::string(compare_opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je") +
              " .L" + function.name + "_" + false_block->label + "\n";
  asm_text += *rendered_true;
  asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
  asm_text += *rendered_false;
  return asm_text;
}

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  if (!function.params.empty() || function.blocks.size() != 1 ||
      prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value() || entry.insts.size() != 10) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const auto* store_short = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[0]);
  const auto* store_long = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[1]);
  const auto* load_long = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[2]);
  const auto* load_short = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[3]);
  const auto* sext_short = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[4]);
  const auto* sub = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[5]);
  const auto* trunc_result = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[6]);
  const auto* store_result = std::get_if<c4c::backend::bir::StoreLocalInst>(&entry.insts[7]);
  const auto* load_result = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[8]);
  const auto* sext_result = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[9]);
  if (store_short == nullptr || store_long == nullptr || load_long == nullptr ||
      load_short == nullptr || sext_short == nullptr || sub == nullptr ||
      trunc_result == nullptr || store_result == nullptr || load_result == nullptr ||
      sext_result == nullptr) {
    return std::nullopt;
  }
  if (store_short->byte_offset != 0 || store_long->byte_offset != 0 ||
      load_long->byte_offset != 0 || load_short->byte_offset != 0 ||
      store_result->byte_offset != 0 || load_result->byte_offset != 0 ||
      store_short->address.has_value() || store_long->address.has_value() ||
      load_long->address.has_value() || load_short->address.has_value() ||
      store_result->address.has_value() || load_result->address.has_value()) {
    return std::nullopt;
  }
  const auto& returned = *entry.terminator.value;
  if (store_short->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_short->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_long->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
      store_long->value.type != c4c::backend::bir::TypeKind::I64 ||
      load_long->result.type != c4c::backend::bir::TypeKind::I64 ||
      load_short->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_short->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_short->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_short->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_short->operand.name != load_short->result.name ||
      sext_short->result.type != c4c::backend::bir::TypeKind::I64 ||
      sub->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
      sub->operand_type != c4c::backend::bir::TypeKind::I64 ||
      sub->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->lhs.name != sext_short->result.name ||
      sub->rhs.kind != c4c::backend::bir::Value::Kind::Named ||
      sub->rhs.name != load_long->result.name ||
      sub->rhs.type != c4c::backend::bir::TypeKind::I64 ||
      sub->result.type != c4c::backend::bir::TypeKind::I64 ||
      trunc_result->opcode != c4c::backend::bir::CastOpcode::Trunc ||
      trunc_result->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      trunc_result->operand.type != c4c::backend::bir::TypeKind::I64 ||
      trunc_result->operand.name != sub->result.name ||
      trunc_result->result.type != c4c::backend::bir::TypeKind::I16 ||
      store_result->value.kind != c4c::backend::bir::Value::Kind::Named ||
      store_result->value.type != c4c::backend::bir::TypeKind::I16 ||
      store_result->value.name != trunc_result->result.name ||
      load_result->result.type != c4c::backend::bir::TypeKind::I16 ||
      sext_result->opcode != c4c::backend::bir::CastOpcode::SExt ||
      sext_result->operand.kind != c4c::backend::bir::Value::Kind::Named ||
      sext_result->operand.type != c4c::backend::bir::TypeKind::I16 ||
      sext_result->operand.name != load_result->result.name ||
      sext_result->result.type != c4c::backend::bir::TypeKind::I32 ||
      returned.kind != c4c::backend::bir::Value::Kind::Named ||
      returned.name != sext_result->result.name) {
    return std::nullopt;
  }

  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  const auto* prepared_store_short =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 0);
  const auto* prepared_store_long =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 1);
  const auto* prepared_load_long =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 2);
  const auto* prepared_load_short =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 3);
  const auto* prepared_store_result =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 7);
  const auto* prepared_load_result =
      find_prepared_frame_memory_access(function_addressing, entry_label_id, 8);
  if (!prepared_frame_memory_accesses_match(prepared_store_short, prepared_load_short) ||
      !prepared_frame_memory_accesses_match(prepared_store_short, prepared_store_result) ||
      !prepared_frame_memory_accesses_match(prepared_store_short, prepared_load_result) ||
      !prepared_frame_memory_accesses_match(prepared_store_long, prepared_load_long)) {
    return std::nullopt;
  }

  const auto short_memory = render_prepared_frame_slot_memory_operand_if_supported(
      *layout, prepared_store_short->address, "WORD");
  const auto long_memory = render_prepared_frame_slot_memory_operand_if_supported(
      *layout, prepared_store_long->address, "QWORD");
  if (!short_memory.has_value() || !long_memory.has_value()) {
    return std::nullopt;
  }

  std::string asm_text = std::string(asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    mov " + *short_memory + ", " +
              std::to_string(static_cast<std::int16_t>(store_short->value.immediate)) + "\n";
  asm_text += "    mov " + *long_memory + ", " +
              std::to_string(static_cast<std::int64_t>(store_long->value.immediate)) + "\n";
  asm_text += "    movsx rax, " + *short_memory + "\n";
  asm_text += "    sub rax, " + *long_memory + "\n";
  asm_text += "    mov " + *short_memory + ", ax\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  if (layout->frame_size != 0) {
    asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register) {
  if (!function.params.empty() || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      prepared_arch != c4c::TargetArch::X86_64 || stack_layout == nullptr ||
      function_addressing == nullptr || prepared_names == nullptr) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto wrap_i32 = [](std::int64_t value) -> std::int32_t {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
  };
  const auto function_name_id =
      c4c::backend::prepare::resolve_prepared_function_name_id(*prepared_names, function.name);
  const auto entry_label_id =
      c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, entry.label);
  if (!function_name_id.has_value() || !entry_label_id.has_value()) {
    return std::nullopt;
  }
  const auto layout = build_prepared_module_local_slot_layout(
      function, stack_layout, function_addressing, prepared_names, prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  struct ConstantValue {
    c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
    std::int64_t value = 0;
  };

  std::unordered_map<std::string_view, ConstantValue> named_values;
  std::unordered_map<std::size_t, ConstantValue> local_memory;

  const auto resolve_i32_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::int32_t> {
        if (value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return static_cast<std::int32_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it == named_values.end() ||
            value_it->second.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        return static_cast<std::int32_t>(value_it->second.value);
      };

  const auto resolve_ptr_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::size_t> {
        if (value.type != c4c::backend::bir::TypeKind::Ptr) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          if (value.immediate < 0) {
            return std::nullopt;
          }
          return static_cast<std::size_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it != named_values.end()) {
          if (value_it->second.type != c4c::backend::bir::TypeKind::Ptr ||
              value_it->second.value < 0) {
            return std::nullopt;
          }
          return static_cast<std::size_t>(value_it->second.value);
        }
        return find_prepared_named_stack_object_frame_offset(
            stack_layout, prepared_names, *function_name_id, value.name);
      };

  const auto resolve_prepared_memory_address = [&](std::size_t inst_index)
      -> std::optional<std::size_t> {
    const auto* prepared_access =
        find_prepared_function_memory_access(function_addressing, *entry_label_id, inst_index);
    if (prepared_access == nullptr) {
      return std::nullopt;
    }

    std::optional<std::size_t> base_offset;
    switch (prepared_access->address.base_kind) {
      case c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot: {
        if (!prepared_access->address.frame_slot_id.has_value()) {
          return std::nullopt;
        }
        const auto frame_slot_it =
            layout->frame_slot_offsets.find(*prepared_access->address.frame_slot_id);
        if (frame_slot_it == layout->frame_slot_offsets.end()) {
          return std::nullopt;
        }
        base_offset = frame_slot_it->second;
        break;
      }
      case c4c::backend::prepare::PreparedAddressBaseKind::PointerValue: {
        if (!prepared_access->address.pointer_value_name.has_value() ||
            !prepared_access->address.can_use_base_plus_offset) {
          return std::nullopt;
        }
        const auto pointer_name =
            c4c::backend::prepare::prepared_value_name(
                *prepared_names, *prepared_access->address.pointer_value_name);
        if (pointer_name.empty()) {
          return std::nullopt;
        }
        base_offset = resolve_ptr_value(c4c::backend::bir::Value{
            .kind = c4c::backend::bir::Value::Kind::Named,
            .type = c4c::backend::bir::TypeKind::Ptr,
            .name = std::string(pointer_name),
        });
        break;
      }
      default:
        return std::nullopt;
    }

    if (!base_offset.has_value()) {
      return std::nullopt;
    }
    const auto signed_offset = static_cast<std::int64_t>(*base_offset) +
                               prepared_access->address.byte_offset;
    if (signed_offset < 0) {
      return std::nullopt;
    }
    return static_cast<std::size_t>(signed_offset);
  };

  const auto fold_binary_i32 =
      [&](const c4c::backend::bir::BinaryInst& binary) -> std::optional<std::int32_t> {
        const auto lhs = resolve_i32_value(binary.lhs);
        const auto rhs = resolve_i32_value(binary.rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        switch (binary.opcode) {
          case c4c::backend::bir::BinaryOpcode::Add:
            return wrap_i32(static_cast<std::int64_t>(*lhs) + *rhs);
          case c4c::backend::bir::BinaryOpcode::Sub:
            return wrap_i32(static_cast<std::int64_t>(*lhs) - *rhs);
          case c4c::backend::bir::BinaryOpcode::Mul:
            return wrap_i32(static_cast<std::int64_t>(*lhs) * *rhs);
          case c4c::backend::bir::BinaryOpcode::And:
            return static_cast<std::int32_t>(*lhs & *rhs);
          case c4c::backend::bir::BinaryOpcode::Or:
            return static_cast<std::int32_t>(*lhs | *rhs);
          case c4c::backend::bir::BinaryOpcode::Xor:
            return static_cast<std::int32_t>(*lhs ^ *rhs);
          case c4c::backend::bir::BinaryOpcode::Shl:
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(*lhs) <<
                                             (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::LShr:
            return static_cast<std::int32_t>(static_cast<std::uint32_t>(*lhs) >>
                                             (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::AShr:
            return static_cast<std::int32_t>(*lhs >> (static_cast<std::uint32_t>(*rhs) & 31u));
          case c4c::backend::bir::BinaryOpcode::SDiv:
            if (*rhs == 0) {
              return std::nullopt;
            }
            return static_cast<std::int32_t>(*lhs / *rhs);
          case c4c::backend::bir::BinaryOpcode::SRem:
            if (*rhs == 0) {
              return std::nullopt;
            }
            return static_cast<std::int32_t>(*lhs % *rhs);
          default:
            return std::nullopt;
        }
      };

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); ++inst_index) {
    const auto& inst = entry.insts[inst_index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto address = resolve_prepared_memory_address(inst_index);
      if (!address.has_value()) {
        return std::nullopt;
      }
      if (store->value.type == c4c::backend::bir::TypeKind::I32) {
        const auto stored_value = resolve_i32_value(store->value);
        if (!stored_value.has_value()) {
          return std::nullopt;
        }
        local_memory[*address] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::I32,
            .value = *stored_value,
        };
        continue;
      }
      if (store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        const auto stored_value = resolve_ptr_value(store->value);
        local_memory[*address] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::Ptr,
            .value = stored_value.has_value() ? static_cast<std::int64_t>(*stored_value) : -1,
        };
        continue;
      }
      return std::nullopt;
    }

    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      const auto address = resolve_prepared_memory_address(inst_index);
      if (!address.has_value()) {
        return std::nullopt;
      }
      const auto value_it = local_memory.find(*address);
      if (value_it == local_memory.end() || value_it->second.type != load->result.type) {
        return std::nullopt;
      }
      named_values[load->result.name] = value_it->second;
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    const auto folded = fold_binary_i32(*binary);
    if (!folded.has_value()) {
      return std::nullopt;
    }
    named_values[binary->result.name] = ConstantValue{
        .type = c4c::backend::bir::TypeKind::I32,
        .value = *folded,
    };
  }

  const auto folded_return = resolve_i32_value(*entry.terminator.value);
  if (!folded_return.has_value()) {
    return std::nullopt;
  }
  return std::string(asm_prefix) + "    mov " + std::string(return_register) + ", " +
         std::to_string(static_cast<std::int32_t>(*folded_return)) + "\n    ret\n";
}

std::optional<std::string> render_prepared_param_derived_i32_value_if_supported(
    std::string_view return_register,
    const c4c::backend::bir::Value& value,
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
        named_binaries,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named || param.type != value.type ||
      param.is_varargs || param.is_sret || param.is_byval) {
    return std::nullopt;
  }

  if (const auto param_register = minimal_param_register(param);
      value.name == param.name && param_register.has_value()) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n";
  }

  const auto binary_it = named_binaries.find(value.name);
  if (binary_it == named_binaries.end()) {
    return std::nullopt;
  }

  const auto& binary = *binary_it->second;
  if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      binary.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }

  const bool lhs_is_param_rhs_is_imm =
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.lhs.name == param.name &&
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_param_lhs_is_imm =
      binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      binary.rhs.name == param.name &&
      binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      binary.lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_param_rhs_is_imm && !rhs_is_param_lhs_is_imm) {
    return std::nullopt;
  }

  const auto param_register = minimal_param_register(param);
  if (!param_register.has_value()) {
    return std::nullopt;
  }
  const auto immediate =
      lhs_is_param_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    add " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sub " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    return "    mov " + std::string(return_register) + ", " + *param_register +
           "\n    imul " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    and " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    or " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    xor " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shl " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shr " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sar " +
           std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  if (function.blocks.size() != 1 || entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& returned = *entry.terminator.value;
  if (returned.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (entry.insts.empty() && function.params.empty() &&
      returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return std::string(asm_prefix) + "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n    ret\n";
  }
  if (prepared_arch != c4c::TargetArch::X86_64 || function.params.size() != 1) {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
  if (entry.insts.empty()) {
    const auto value_render = render_prepared_param_derived_i32_value_if_supported(
        return_register, returned, named_binaries, param, minimal_param_register);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return std::string(asm_prefix) + render_prepared_return_body(*value_render);
  }

  if (entry.insts.size() != 1 || returned.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
  if (binary == nullptr || returned.name != binary->result.name) {
    return std::nullopt;
  }
  named_binaries.emplace(binary->result.name, binary);

  const auto value_render = render_prepared_param_derived_i32_value_if_supported(
      return_register, returned, named_binaries, param, minimal_param_register);
  if (!value_render.has_value()) {
    return std::nullopt;
  }
  return std::string(asm_prefix) + render_prepared_return_body(*value_render);
}

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  if (function.params.empty() == false || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      function.blocks.front().insts.empty()) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto& returned = *entry.terminator.value;
  const auto* final_load = std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts.back());
  if (final_load == nullptr || returned.kind != c4c::backend::bir::Value::Kind::Named ||
      returned.type != c4c::backend::bir::TypeKind::I32 ||
      final_load->result.name != returned.name ||
      final_load->result.type != c4c::backend::bir::TypeKind::I32 ||
      final_load->byte_offset != 0) {
    return std::nullopt;
  }

  auto asm_text = std::string(asm_prefix);
  const c4c::FunctionNameId function_name_id =
      prepared_names == nullptr ? c4c::kInvalidFunctionName
                                : c4c::backend::prepare::resolve_prepared_function_name_id(
                                      *prepared_names, function.name)
                                      .value_or(c4c::kInvalidFunctionName);
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); ++inst_index) {
    const auto& inst = entry.insts[inst_index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (store->byte_offset != 0) {
        return std::nullopt;
      }
      const auto* prepared_access =
          find_prepared_function_memory_access(function_addressing, entry_label_id, inst_index);
      auto memory = prepared_access == nullptr
                        ? std::nullopt
                        : render_prepared_frame_slot_memory_operand_if_supported(
                              *layout,
                              prepared_access->address,
                              store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                                                                                     : "DWORD");
      if (!memory.has_value()) {
        return std::nullopt;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I32) {
        asm_text += "    mov " + *memory + ", " +
                    std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
        continue;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        const auto pointee_address = render_prepared_value_stack_object_address_if_supported(
            stack_layout, prepared_names, function_name_id, store->value.name);
        if (!pointee_address.has_value()) {
          return std::nullopt;
        }
        asm_text += "    lea rax, " + *pointee_address + "\n";
        asm_text += "    mov " + *memory + ", rax\n";
        continue;
      }
      return std::nullopt;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0) {
      return std::nullopt;
    }
    const auto* prepared_access =
        find_prepared_function_memory_access(function_addressing, entry_label_id, inst_index);
    auto memory = prepared_access == nullptr
                      ? std::nullopt
                      : render_prepared_frame_slot_memory_operand_if_supported(
                            *layout,
                            prepared_access->address,
                            load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                                                                                   : "DWORD");
    if (!memory.has_value()) {
      return std::nullopt;
    }
    if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
      asm_text += "    mov rax, " + *memory + "\n";
      continue;
    }
    if (load->result.type == c4c::backend::bir::TypeKind::I32) {
      asm_text += "    mov eax, " + *memory + "\n";
      continue;
    }
    return std::nullopt;
  }

  if (layout->frame_size != 0) {
    asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

std::optional<std::string> render_prepared_trivial_defined_function_if_supported(
    const c4c::backend::bir::Function& candidate,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix) {
  if (prepared_arch != c4c::TargetArch::X86_64 || !candidate.params.empty() ||
      !candidate.local_slots.empty() || candidate.blocks.size() != 1) {
    return std::nullopt;
  }
  const auto& candidate_entry = candidate.blocks.front();
  if (!candidate_entry.insts.empty() ||
      candidate_entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return) {
    return std::nullopt;
  }
  if (candidate.return_type == c4c::backend::bir::TypeKind::Void &&
      !candidate_entry.terminator.value.has_value()) {
    return minimal_function_asm_prefix(candidate) + "    ret\n";
  }
  if (candidate.return_type != c4c::backend::bir::TypeKind::I32 ||
      !candidate_entry.terminator.value.has_value() ||
      candidate_entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto candidate_return_register = minimal_function_return_register(candidate);
  if (!candidate_return_register.has_value()) {
    return std::nullopt;
  }
  return minimal_function_asm_prefix(candidate) + "    mov " + *candidate_return_register + ", " +
         std::to_string(static_cast<std::int32_t>(candidate_entry.terminator.value->immediate)) +
         "\n    ret\n";
}

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers) {
  if (prepared_arch != c4c::TargetArch::X86_64 || !function.params.empty() ||
      !function.local_slots.empty() || function.blocks.size() != 1 || entry.label != "entry" ||
      entry.insts.empty() ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  struct NamedI32Source {
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
    std::optional<std::int64_t> immediate_i32;
  };
  struct CurrentI32Carrier {
    std::string_view value_name;
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
  };
  std::unordered_map<std::string_view, std::int64_t> i32_constants;
  std::unordered_set<std::string_view> emitted_string_names;
  std::vector<const c4c::backend::bir::StringConstant*> used_string_constants;
  std::unordered_set<std::string_view> used_same_module_globals;
  std::string body = "    sub rsp, 8\n";
  bool saw_call = false;
  std::optional<CurrentI32Carrier> current_i32;

  const auto resolve_i32_constant =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::int64_t> {
        if (value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return static_cast<std::int32_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto constant_it = i32_constants.find(value.name);
        if (constant_it == i32_constants.end()) {
          return std::nullopt;
        }
        return constant_it->second;
      };
  const auto fold_binary_immediate =
      [&](const c4c::backend::bir::BinaryInst& binary) -> std::optional<std::int64_t> {
        const auto lhs = resolve_i32_constant(binary.lhs);
        const auto rhs = resolve_i32_constant(binary.rhs);
        if (!lhs.has_value() || !rhs.has_value()) {
          return std::nullopt;
        }
        switch (binary.opcode) {
          case c4c::backend::bir::BinaryOpcode::Add:
            return static_cast<std::int32_t>(*lhs + *rhs);
          case c4c::backend::bir::BinaryOpcode::Sub:
            return static_cast<std::int32_t>(*lhs - *rhs);
          case c4c::backend::bir::BinaryOpcode::Mul:
            return static_cast<std::int32_t>(*lhs * *rhs);
          case c4c::backend::bir::BinaryOpcode::And:
            return static_cast<std::int32_t>(*lhs & *rhs);
          case c4c::backend::bir::BinaryOpcode::Or:
            return static_cast<std::int32_t>(*lhs | *rhs);
          case c4c::backend::bir::BinaryOpcode::Xor:
            return static_cast<std::int32_t>(*lhs ^ *rhs);
          case c4c::backend::bir::BinaryOpcode::Shl:
            return static_cast<std::int32_t>(
                static_cast<std::uint32_t>(*lhs) << static_cast<std::uint32_t>(*rhs));
          case c4c::backend::bir::BinaryOpcode::LShr:
            return static_cast<std::int32_t>(
                static_cast<std::uint32_t>(*lhs) >> static_cast<std::uint32_t>(*rhs));
          case c4c::backend::bir::BinaryOpcode::AShr:
            return static_cast<std::int32_t>(*lhs >> *rhs);
          default:
            return std::nullopt;
        }
      };
  const auto narrow_i32_register = [](std::string_view wide_register) -> std::optional<std::string> {
    if (wide_register == "rax") return std::string("eax");
    if (wide_register == "rbx") return std::string("ebx");
    if (wide_register == "rcx") return std::string("ecx");
    if (wide_register == "rdx") return std::string("edx");
    if (wide_register == "rdi") return std::string("edi");
    if (wide_register == "rsi") return std::string("esi");
    if (wide_register == "rbp") return std::string("ebp");
    if (wide_register == "rsp") return std::string("esp");
    if (wide_register == "r8") return std::string("r8d");
    if (wide_register == "r9") return std::string("r9d");
    if (wide_register == "r10") return std::string("r10d");
    if (wide_register == "r11") return std::string("r11d");
    if (wide_register == "r12") return std::string("r12d");
    if (wide_register == "r13") return std::string("r13d");
    if (wide_register == "r14") return std::string("r14d");
    if (wide_register == "r15") return std::string("r15d");
    return std::string(wide_register);
  };
  const auto find_named_value_home =
      [&](std::string_view value_name) -> const c4c::backend::prepare::PreparedValueHome* {
    if (prepared_names == nullptr || function_locations == nullptr) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_value_home(
        *prepared_names, *function_locations, value_name);
  };
  const auto source_for_named_i32 =
      [&](std::string_view value_name) -> std::optional<NamedI32Source> {
    if (current_i32.has_value() && current_i32->value_name == value_name) {
      return NamedI32Source{
          .register_name = current_i32->register_name,
          .stack_operand = current_i32->stack_operand,
          .immediate_i32 = std::nullopt,
      };
    }
    const auto constant_it = i32_constants.find(value_name);
    if (constant_it != i32_constants.end()) {
      return NamedI32Source{
          .register_name = std::nullopt,
          .stack_operand = std::nullopt,
          .immediate_i32 = constant_it->second,
      };
    }
    const auto* home = find_named_value_home(value_name);
    if (home == nullptr) {
      return std::nullopt;
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value()) {
      return NamedI32Source{
          .register_name = narrow_i32_register(*home->register_name),
          .stack_operand = std::nullopt,
          .immediate_i32 = std::nullopt,
      };
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      return NamedI32Source{
          .register_name = std::nullopt,
          .stack_operand =
              render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD"),
          .immediate_i32 = std::nullopt,
      };
    }
    if (home->kind ==
            c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
        home->immediate_i32.has_value()) {
      return NamedI32Source{
          .register_name = std::nullopt,
          .stack_operand = std::nullopt,
          .immediate_i32 = *home->immediate_i32,
      };
    }
    return std::nullopt;
  };
  const auto append_move_into_register =
      [&](std::string* rendered_body,
          std::string_view destination_register,
          const NamedI32Source& source) -> bool {
    if (source.immediate_i32.has_value()) {
      *rendered_body += "    mov " + std::string(destination_register) + ", " +
                        std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
      return true;
    }
    if (source.register_name.has_value()) {
      if (*source.register_name != destination_register) {
        *rendered_body +=
            "    mov " + std::string(destination_register) + ", " + *source.register_name + "\n";
      }
      return true;
    }
    if (source.stack_operand.has_value()) {
      *rendered_body += "    mov " + std::string(destination_register) + ", " +
                        *source.stack_operand + "\n";
      return true;
    }
    return false;
  };

  for (const auto& inst : entry.insts) {
    if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
      if (binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
          binary->result.type != c4c::backend::bir::TypeKind::I32 ||
          binary->operand_type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto folded = fold_binary_immediate(*binary);
      if (!folded.has_value()) {
        return std::nullopt;
      }
      i32_constants.emplace(binary->result.name, *folded);
      current_i32 = std::nullopt;
      continue;
    }

    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr || call->is_indirect || call->callee.empty() ||
        call->callee_value.has_value() || call->args.size() != call->arg_types.size() ||
        call->args.size() > 6) {
      return std::nullopt;
    }
    saw_call = true;
    const auto instruction_index =
        static_cast<std::size_t>(&inst - entry.insts.data());
    const auto* before_call_bundle =
        function_locations == nullptr
            ? nullptr
            : c4c::backend::prepare::find_prepared_move_bundle(
                  *function_locations,
                  c4c::backend::prepare::PreparedMovePhase::BeforeCall,
                  0,
                  instruction_index);

    for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
      const auto& arg = call->args[arg_index];
      if (call->arg_types[arg_index] == c4c::backend::bir::TypeKind::Ptr) {
        if (arg.kind != c4c::backend::bir::Value::Kind::Named || arg.name.empty() ||
            arg.name.front() != '@') {
          return std::nullopt;
        }
        const std::string_view symbol_name(arg.name.data() + 1, arg.name.size() - 1);
        if (const auto* string_constant = find_string_constant(symbol_name);
            string_constant != nullptr) {
          if (emitted_string_names.insert(symbol_name).second) {
            used_string_constants.push_back(string_constant);
          }
          body += "    lea ";
          body += kArgRegs64[arg_index];
          body += ", [rip + ";
          body += render_private_data_label(arg.name);
          body += "]\n";
          continue;
        }
        const auto* global = find_same_module_global(symbol_name);
        if (global == nullptr) {
          return std::nullopt;
        }
        used_same_module_globals.insert(global->name);
        body += "    lea ";
        body += kArgRegs64[arg_index];
        body += ", [rip + ";
        body += render_asm_symbol_name(global->name);
        body += "]\n";
        continue;
      }

      if (call->arg_types[arg_index] != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      NamedI32Source source;
      if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
        source.immediate_i32 = static_cast<std::int32_t>(arg.immediate);
      } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
        const auto named_source = source_for_named_i32(arg.name);
        if (!named_source.has_value()) {
          return std::nullopt;
        }
        source = *named_source;
      } else {
        return std::nullopt;
      }
      std::optional<std::string> destination_register;
      if (before_call_bundle != nullptr) {
        for (const auto& move : before_call_bundle->moves) {
          if (move.destination_kind !=
                  c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
              move.destination_storage_kind !=
                  c4c::backend::prepare::PreparedMoveStorageKind::Register ||
              move.destination_abi_index != std::optional<std::size_t>{arg_index} ||
              !move.destination_register_name.has_value()) {
            continue;
          }
          destination_register = narrow_i32_register(*move.destination_register_name);
          break;
        }
      }
      if (!destination_register.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff");
      }
      if (!append_move_into_register(&body, *destination_register, source)) {
        return std::nullopt;
      }
    }

    body += "    xor eax, eax\n";
    body += "    call ";
    body += render_asm_symbol_name(call->callee);
    body += "\n";
    if (call->result.has_value() && call->result->type == c4c::backend::bir::TypeKind::I32 &&
        call->result->kind == c4c::backend::bir::Value::Kind::Named) {
      const auto* after_call_bundle =
          function_locations == nullptr
              ? nullptr
              : c4c::backend::prepare::find_prepared_move_bundle(
                    *function_locations,
                    c4c::backend::prepare::PreparedMovePhase::AfterCall,
                    0,
                    instruction_index);
      const auto* result_home = find_named_value_home(call->result->name);
      const auto* after_call_move =
          [&]() -> const c4c::backend::prepare::PreparedMoveResolution* {
        if (after_call_bundle == nullptr) {
          return nullptr;
        }
        if (result_home != nullptr) {
          for (const auto& move : after_call_bundle->moves) {
            if (move.destination_kind ==
                    c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi &&
                move.to_value_id == result_home->value_id) {
              return &move;
            }
          }
        }
        for (const auto& move : after_call_bundle->moves) {
          if (move.destination_kind ==
              c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi) {
            return &move;
          }
        }
        return nullptr;
      }();
      if (after_call_move == nullptr || !after_call_move->destination_register_name.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff");
      }
      const auto abi_result_register =
          narrow_i32_register(*after_call_move->destination_register_name);
      if (!abi_result_register.has_value()) {
        return std::nullopt;
      }
      if (after_call_move != nullptr) {
        if (after_call_move->destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::Register) {
          return std::nullopt;
        }
        if (result_home == nullptr) {
          return std::nullopt;
        }
        if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
            result_home->register_name.has_value()) {
          const auto home_register = narrow_i32_register(*result_home->register_name);
          if (!home_register.has_value()) {
            return std::nullopt;
          }
          if (*home_register != *abi_result_register) {
            body += "    mov " + *home_register + ", " + *abi_result_register + "\n";
          }
          current_i32 = CurrentI32Carrier{
              .value_name = call->result->name,
              .register_name = *home_register,
              .stack_operand = std::nullopt,
          };
        } else if (result_home->kind ==
                       c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                   result_home->offset_bytes.has_value()) {
          const auto stack_operand =
              render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD");
          body += "    mov " + stack_operand + ", " + *abi_result_register + "\n";
          current_i32 = CurrentI32Carrier{
              .value_name = call->result->name,
              .register_name = std::nullopt,
              .stack_operand = stack_operand,
          };
        } else {
          return std::nullopt;
        }
      } else {
        current_i32 = CurrentI32Carrier{
            .value_name = call->result->name,
            .register_name = *abi_result_register,
            .stack_operand = std::nullopt,
        };
      }
    } else {
      current_i32 = std::nullopt;
    }
  }

  if (!saw_call) {
    return std::nullopt;
  }

  if (entry.terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
      current_i32.has_value() && entry.terminator.value->name == current_i32->value_name &&
      current_i32->register_name.has_value() &&
      *current_i32->register_name == std::string(return_register)) {
    body += "    add rsp, 8\n    ret\n";
  } else {
    const auto returned_value = resolve_i32_constant(*entry.terminator.value);
    if (!returned_value.has_value()) {
      return std::nullopt;
    }
    body += "    mov ";
    body += std::string(return_register);
    body += ", ";
    body += std::to_string(static_cast<std::int32_t>(*returned_value));
    body += "\n    add rsp, 8\n    ret\n";
  }

  std::string rendered_data;
  for (const auto* string_constant : used_string_constants) {
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& global : module.globals) {
    if (used_same_module_globals.find(global.name) == used_same_module_globals.end() &&
        bounded_same_module_helper_global_names.find(global.name) ==
            bounded_same_module_helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_data += *rendered_global_data;
  }
  return prepend_bounded_same_module_helpers(std::string(asm_prefix) + body + rendered_data);
}

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const c4c::backend::bir::Module& module,
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data,
    const std::function<std::string(std::string)>& prepend_bounded_same_module_helpers,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  if (const auto rendered_direct_calls =
          render_prepared_minimal_direct_extern_call_sequence_if_supported(
              module, function, entry, prepared_names, function_locations,
              prepared_arch, asm_prefix, return_register,
              bounded_same_module_helper_global_names, find_string_constant,
              find_same_module_global, render_private_data_label, render_asm_symbol_name,
              emit_string_constant_data, emit_same_module_global_data,
              prepend_bounded_same_module_helpers);
      rendered_direct_calls.has_value()) {
    return *rendered_direct_calls;
  }
  if (const auto rendered_local_slot =
          render_prepared_minimal_local_slot_return_if_supported(
              function, stack_layout, function_addressing, prepared_names, prepared_arch, asm_prefix);
      rendered_local_slot.has_value()) {
    return *rendered_local_slot;
  }
  if (const auto rendered_constant_folded =
          render_prepared_constant_folded_single_block_return_if_supported(
              function, stack_layout, function_addressing, prepared_names,
              prepared_arch, asm_prefix, return_register);
      rendered_constant_folded.has_value()) {
    return *rendered_constant_folded;
  }
  if (const auto rendered_local_i16_i64_return =
          render_prepared_local_i16_i64_sub_return_if_supported(
              function, entry, stack_layout, function_addressing, prepared_names, prepared_arch, asm_prefix);
      rendered_local_i16_i64_return.has_value()) {
    return *rendered_local_i16_i64_return;
  }
  return render_prepared_minimal_immediate_or_param_return_if_supported(
      function, entry, prepared_arch, asm_prefix, return_register, minimal_param_register);
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }

  PreparedBoundedMultiDefinedCallLaneModuleRender rendered;
  std::unordered_set<std::string> emitted_string_names;
  std::unordered_set<std::string> emitted_same_module_global_names;
  bool saw_bounded_entry = false;

  for (const auto* candidate : defined_functions) {
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered.rendered_functions += *rendered_trivial;
      continue;
    }

    if (saw_bounded_entry || candidate->return_type != c4c::backend::bir::TypeKind::I32 ||
        !candidate->params.empty() || candidate->blocks.size() != 1 ||
        candidate->blocks.front().label != "entry" ||
        candidate->blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !candidate->blocks.front().terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto candidate_return_register = minimal_function_return_register(*candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }

    const auto candidate_layout = build_prepared_module_local_slot_layout(
        *candidate,
        static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
        prepared_arch);
    if (!candidate_layout.has_value()) {
      return std::nullopt;
    }
    const auto* function_locations =
        c4c::backend::prepare::find_prepared_value_location_function(module, candidate->name);
    if (function_locations == nullptr) {
      return std::nullopt;
    }
    const auto rendered_candidate =
        render_prepared_bounded_multi_defined_call_lane_body_if_supported(
            module, *function_locations, *candidate, defined_functions, *candidate_layout,
            *candidate_return_register,
            has_string_constant, has_same_module_global, render_private_data_label,
            render_asm_symbol_name);
    if (!rendered_candidate.has_value()) {
      return std::nullopt;
    }
    for (const auto& string_name : rendered_candidate->used_string_names) {
      if (emitted_string_names.insert(string_name).second) {
        rendered.used_string_names.push_back(string_name);
      }
    }
    for (const auto& global_name : rendered_candidate->used_same_module_global_names) {
      if (emitted_same_module_global_names.insert(global_name).second) {
        rendered.used_same_module_global_names.push_back(global_name);
      }
    }
    rendered.rendered_functions +=
        minimal_function_asm_prefix(*candidate) + rendered_candidate->body;
    saw_bounded_entry = true;
  }

  if (!saw_bounded_entry) {
    return std::nullopt;
  }

  return rendered;
}

std::optional<PreparedBoundedSameModuleHelperPrefixRender>
render_prepared_bounded_same_module_helper_prefix_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || entry_function.name != "main") {
    return std::nullopt;
  }

  struct BoundedSameModuleHelperRender {
    std::string asm_text;
    std::unordered_set<std::string_view> used_same_module_globals;
  };

  const auto render_bounded_same_module_helper_function_if_supported =
      [&](const c4c::backend::bir::Function& candidate)
      -> std::optional<BoundedSameModuleHelperRender> {
    if (prepared_arch != c4c::TargetArch::X86_64 || candidate.local_slots.empty() == false ||
        candidate.blocks.size() != 1 || candidate.blocks.front().label != "entry" ||
        candidate.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !candidate.blocks.front().terminator.value.has_value() || candidate.params.size() > 6) {
      return std::nullopt;
    }

    std::unordered_map<std::string_view, std::string> param_registers;
    for (std::size_t param_index = 0; param_index < candidate.params.size(); ++param_index) {
      const auto& param = candidate.params[param_index];
      if (param.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto param_register = minimal_param_register_at(param, param_index);
      if (!param_register.has_value()) {
        return std::nullopt;
      }
      param_registers.emplace(param.name, *param_register);
    }

    const auto candidate_return_register = minimal_function_return_register(candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }
    const auto candidate_function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, candidate.name)
            .value_or(c4c::kInvalidFunctionName);
    const auto* candidate_function_addressing =
        c4c::backend::prepare::find_prepared_addressing(module, candidate_function_name_id);
    const c4c::BlockLabelId entry_block_label_id =
        c4c::backend::prepare::resolve_prepared_block_label_id(
            module.names, candidate.blocks.front().label)
            .value_or(c4c::kInvalidBlockLabel);

    const auto render_value_to_eax =
        [&](const c4c::backend::bir::Value& value,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          if (value.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            return "    mov eax, " +
                   std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named) {
            return std::nullopt;
          }
          if (current_i32_name.has_value() && value.name == *current_i32_name) {
            return std::string{};
          }
          const auto param_it = param_registers.find(value.name);
          if (param_it == param_registers.end()) {
            return std::nullopt;
          }
          return "    mov eax, " + param_it->second + "\n";
        };
    const auto render_i32_operand =
        [&](const c4c::backend::bir::Value& value,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          if (value.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            return std::to_string(static_cast<std::int32_t>(value.immediate));
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named) {
            return std::nullopt;
          }
          if (current_i32_name.has_value() && value.name == *current_i32_name) {
            return std::string("eax");
          }
          const auto param_it = param_registers.find(value.name);
          if (param_it == param_registers.end()) {
            return std::nullopt;
          }
          return param_it->second;
        };
    const auto apply_binary_in_eax =
        [&](const c4c::backend::bir::BinaryInst& binary,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
              binary.result.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }

          const auto render_rhs =
              [&](const c4c::backend::bir::Value& rhs) -> std::optional<std::string> {
            return render_i32_operand(rhs, current_i32_name);
          };
          const auto render_commutative =
              [&](const c4c::backend::bir::Value& lhs,
                  const c4c::backend::bir::Value& rhs) -> std::optional<std::string> {
                const auto setup = render_value_to_eax(lhs, current_i32_name);
                const auto rhs_operand = render_rhs(rhs);
                if (!setup.has_value() || !rhs_operand.has_value()) {
                  return std::nullopt;
                }
                std::string rendered = *setup;
                switch (binary.opcode) {
                  case c4c::backend::bir::BinaryOpcode::Add:
                    rendered += "    add eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::Sub:
                    rendered += "    sub eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::Mul:
                    rendered += "    imul eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::And:
                    rendered += "    and eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::Or:
                    rendered += "    or eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::Xor:
                    rendered += "    xor eax, " + *rhs_operand + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::Shl:
                    if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                        rhs.type != c4c::backend::bir::TypeKind::I32) {
                      return std::nullopt;
                    }
                    rendered += "    shl eax, " +
                                std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::LShr:
                    if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                        rhs.type != c4c::backend::bir::TypeKind::I32) {
                      return std::nullopt;
                    }
                    rendered += "    shr eax, " +
                                std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
                    return rendered;
                  case c4c::backend::bir::BinaryOpcode::AShr:
                    if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                        rhs.type != c4c::backend::bir::TypeKind::I32) {
                      return std::nullopt;
                    }
                    rendered += "    sar eax, " +
                                std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
                    return rendered;
                  default:
                    return std::nullopt;
                }
              };

          if (const auto rendered = render_commutative(binary.lhs, binary.rhs);
              rendered.has_value()) {
            return rendered;
          }
          switch (binary.opcode) {
            case c4c::backend::bir::BinaryOpcode::Add:
            case c4c::backend::bir::BinaryOpcode::Mul:
            case c4c::backend::bir::BinaryOpcode::And:
            case c4c::backend::bir::BinaryOpcode::Or:
            case c4c::backend::bir::BinaryOpcode::Xor:
              return render_commutative(binary.rhs, binary.lhs);
            default:
              return std::nullopt;
          }
        };

    std::unordered_set<std::string_view> used_same_module_globals;
    std::string body;
    std::optional<std::string_view> current_i32_name;
    for (const auto& inst : candidate.blocks.front().insts) {
      const auto inst_index = static_cast<std::size_t>(&inst - candidate.blocks.front().insts.data());
      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
      if (binary != nullptr) {
        if (binary->result.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto rendered_binary = apply_binary_in_eax(*binary, current_i32_name);
        if (!rendered_binary.has_value()) {
          return std::nullopt;
        }
        body += *rendered_binary;
        current_i32_name = binary->result.name;
        continue;
      }

      const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst);
      if (store == nullptr || store->address.has_value() || store->byte_offset != 0 ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto* prepared_access = find_prepared_symbol_memory_access(
          &module.names, candidate_function_addressing, entry_block_label_id, inst_index);
      if (prepared_access == nullptr) {
        return std::nullopt;
      }
      auto memory = render_prepared_symbol_memory_operand_if_supported(
          module.names, prepared_access->address, "DWORD", render_asm_symbol_name);
      if (!memory.has_value()) {
        return std::nullopt;
      }
      const std::string_view resolved_global_name = c4c::backend::prepare::prepared_link_name(
          module.names, *prepared_access->address.symbol_name);
      const auto* global = find_same_module_global(resolved_global_name);
      if (global == nullptr || global->type != c4c::backend::bir::TypeKind::I32 ||
          prepared_access->address.byte_offset != 0) {
        return std::nullopt;
      }
      used_same_module_globals.insert(global->name);

      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
        body += "    mov " + *memory + ", ";
        body += std::to_string(static_cast<std::int32_t>(store->value.immediate));
        body += "\n";
        continue;
      }
      if (store->value.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      body += "    mov " + *memory + ", ";
      if (current_i32_name.has_value() && store->value.name == *current_i32_name) {
        body += "eax\n";
        continue;
      }
      const auto param_it = param_registers.find(store->value.name);
      if (param_it == param_registers.end()) {
        return std::nullopt;
      }
      body += param_it->second;
      body += "\n";
    }

    const auto& returned = *candidate.blocks.front().terminator.value;
    if (returned.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
      body += "    mov " + *candidate_return_register + ", " +
              std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n";
    } else if (returned.kind == c4c::backend::bir::Value::Kind::Named) {
      if (current_i32_name.has_value() && returned.name == *current_i32_name) {
        if (*candidate_return_register != "eax") {
          body += "    mov " + *candidate_return_register + ", eax\n";
        }
      } else {
        const auto param_it = param_registers.find(returned.name);
        if (param_it == param_registers.end()) {
          return std::nullopt;
        }
        body += "    mov " + *candidate_return_register + ", " + param_it->second + "\n";
      }
    } else {
      return std::nullopt;
    }
    body += "    ret\n";
    return BoundedSameModuleHelperRender{
        .asm_text = minimal_function_asm_prefix(candidate) + body,
        .used_same_module_globals = std::move(used_same_module_globals),
    };
  };

  PreparedBoundedSameModuleHelperPrefixRender rendered_helpers;
  for (const auto* candidate : defined_functions) {
    if (candidate == &entry_function) {
      continue;
    }
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered_helpers.helper_names.insert(candidate->name);
      rendered_helpers.helper_prefix += *rendered_trivial;
      continue;
    }
    const auto rendered_helper =
        render_bounded_same_module_helper_function_if_supported(*candidate);
    if (!rendered_helper.has_value()) {
      return std::nullopt;
    }
    for (const auto global_name : rendered_helper->used_same_module_globals) {
      rendered_helpers.helper_global_names.insert(global_name);
    }
    rendered_helpers.helper_names.insert(candidate->name);
    rendered_helpers.helper_prefix += rendered_helper->asm_text;
  }

  return rendered_helpers;
}

std::optional<std::string>
render_prepared_bounded_multi_defined_call_lane_data_if_supported(
    const PreparedBoundedMultiDefinedCallLaneModuleRender& rendered_module,
    const c4c::backend::bir::Module& module,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  std::string rendered_data;
  std::unordered_set<std::string_view> used_same_module_globals(
      rendered_module.used_same_module_global_names.begin(),
      rendered_module.used_same_module_global_names.end());
  for (const auto& string_name : rendered_module.used_string_names) {
    const auto* string_constant = find_string_constant(string_name);
    if (string_constant == nullptr) {
      return std::nullopt;
    }
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& global : module.globals) {
    if (used_same_module_globals.find(global.name) == used_same_module_globals.end() &&
        helper_global_names.find(global.name) == helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_data += *rendered_global_data;
  }
  return rendered_data;
}

std::optional<std::string> render_prepared_bounded_multi_defined_call_lane_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  const auto rendered_module = render_prepared_bounded_multi_defined_call_lane_module_if_supported(
      module, defined_functions, prepared_arch, render_trivial_defined_function,
      minimal_function_return_register, minimal_function_asm_prefix, has_string_constant,
      has_same_module_global, render_private_data_label, render_asm_symbol_name);
  if (!rendered_module.has_value()) {
    return std::nullopt;
  }

  const auto rendered_data = render_prepared_bounded_multi_defined_call_lane_data_if_supported(
      *rendered_module, module.module, helper_global_names, find_string_constant,
      emit_string_constant_data,
      emit_same_module_global_data);
  if (!rendered_data.has_value()) {
    return std::nullopt;
  }
  return rendered_module->rendered_functions + *rendered_data;
}

PreparedModuleMultiDefinedDispatchState build_prepared_module_multi_defined_dispatch_state(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function* entry_function,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  PreparedModuleMultiDefinedDispatchState state;
  if (entry_function != nullptr) {
    if (const auto helpers = render_prepared_bounded_same_module_helper_prefix_if_supported(
            module, defined_functions, *entry_function, prepared_arch,
            render_trivial_defined_function, minimal_function_return_register,
            minimal_function_asm_prefix, find_same_module_global, minimal_param_register_at,
            render_asm_symbol_name);
        helpers.has_value()) {
      state.helper_prefix = helpers->helper_prefix;
      state.helper_names = std::move(helpers->helper_names);
      state.helper_global_names = std::move(helpers->helper_global_names);
      state.has_bounded_same_module_helpers = true;
    }
  }
  state.rendered_module = render_prepared_bounded_multi_defined_call_lane_if_supported(
      module, defined_functions, prepared_arch, state.helper_global_names,
      render_trivial_defined_function, minimal_function_return_register, minimal_function_asm_prefix,
      has_string_constant, has_same_module_global, render_private_data_label, render_asm_symbol_name,
      find_string_constant, emit_string_constant_data, emit_same_module_global_data);
  return state;
}

}  // namespace c4c::backend::x86
