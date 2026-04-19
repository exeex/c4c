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
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
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
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

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

std::optional<std::string> render_prepared_local_address_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const std::optional<c4c::backend::bir::MemoryAddress>& address,
    std::string_view size_name) {
  if (!address.has_value() ||
      address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto slot_it = local_layout.offsets.find(address->base_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(slot_it->second) + address->byte_offset;
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
  const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }
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

    auto rendered_load_or_store =
        [&](const c4c::backend::bir::Inst& inst,
            std::optional<std::string_view>* current_i32_name,
            std::optional<std::string_view>* previous_i32_name,
            std::optional<std::string_view>* current_i8_name,
            std::optional<std::string_view>* current_ptr_name,
            std::optional<MaterializedI32Compare>* current_materialized_compare)
        -> std::optional<std::string> {
      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
        std::optional<std::string> memory;
        if (load->address.has_value()) {
          memory = render_prepared_local_address_operand_if_supported(
              *layout,
              load->address,
              load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
              : load->result.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                     : "DWORD");
        } else {
          if (load->byte_offset != 0) {
            return std::nullopt;
          }
          const auto slot_it = layout->offsets.find(load->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_prepared_stack_memory_operand(
              slot_it->second,
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
        const auto* global = find_same_module_global(load->global_name);
        if (global == nullptr ||
            !same_module_global_supports_scalar_load(*global, load->result.type, load->byte_offset)) {
          return std::nullopt;
        }
        same_module_global_names.insert(global->name);
        *current_materialized_compare = std::nullopt;
        *current_i8_name = std::nullopt;
        std::string memory =
            (load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD PTR [rip + "
                                                                    : "DWORD PTR [rip + ") +
            render_asm_symbol_name(load->global_name);
        if (load->byte_offset != 0) {
          memory += " + " + std::to_string(load->byte_offset);
        }
        memory += "]";
        if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_ptr_name = load->result.name;
          return "    mov rax, " + memory + "\n";
        }
        *current_i32_name = load->result.name;
        *previous_i32_name = std::nullopt;
        *current_ptr_name = std::nullopt;
        return "    mov eax, " + memory + "\n";
      }

      if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
        if (store->address.has_value() || store->byte_offset != 0 ||
            store->value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        const auto* global = find_same_module_global(store->global_name);
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
          return "    mov DWORD PTR [rip + " + render_asm_symbol_name(store->global_name) + "], " +
                 std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
            current_i32_name->has_value() && *current_i32_name == store->value.name) {
          return "    mov DWORD PTR [rip + " + render_asm_symbol_name(store->global_name) +
                 "], eax\n";
        }
        if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
            previous_i32_name->has_value() && *previous_i32_name == store->value.name) {
          return "    mov DWORD PTR [rip + " + render_asm_symbol_name(store->global_name) +
                 "], ecx\n";
        }
        return std::nullopt;
      }

      const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
      if (store == nullptr || store->byte_offset != 0) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (store->address.has_value()) {
        memory = render_prepared_local_address_operand_if_supported(
            *layout,
            store->address,
            store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
            : store->value.type == c4c::backend::bir::TypeKind::I8 ? "BYTE"
                                                                   : "DWORD");
      } else {
        const auto slot_it = layout->offsets.find(store->slot_name);
        if (slot_it == layout->offsets.end()) {
          return std::nullopt;
        }
        memory = render_prepared_stack_memory_operand(
            slot_it->second,
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
        const auto pointee_slot_it = layout->offsets.find(store->value.name);
        if (pointee_slot_it == layout->offsets.end()) {
          *current_ptr_name = std::nullopt;
          return std::string{};
        }
        *current_ptr_name = std::nullopt;
        return "    lea rax, " + render_prepared_stack_address_expr(pointee_slot_it->second) +
               "\n    mov " + *memory + ", rax\n";
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
        const auto compare_join_render_plan =
            c4c::backend::x86::build_prepared_compare_join_entry_render_plan(
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
                      prepared_plan, find_block);
                });
        if (compare_join_render_plan.has_value()) {
          return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
              function.name, body, *compare_join_render_plan, find_block, render_block);
        }
      }
      if (compare_index != block.insts.size()) {
        return std::nullopt;
      }
      const auto* target = find_block(block.terminator.target_label);
      if (target == nullptr || target == &block) {
        return std::nullopt;
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
      if (function_control_flow == nullptr) {
        return std::nullopt;
      }

      const auto join_context =
          c4c::backend::x86::find_prepared_short_circuit_join_context_if_supported(
              *function_control_flow, function, block.label);
      if (!join_context.has_value()) {
        return std::nullopt;
      }

      const auto short_circuit_render_plan =
          c4c::backend::x86::build_prepared_short_circuit_entry_render_plan(
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
                    prepared_plan, find_block);
              });
      if (!short_circuit_render_plan.has_value()) {
        return std::nullopt;
      }
      return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
          function.name, body, *short_circuit_render_plan, find_block, render_block);
    };
    if (const auto rendered_short_circuit = try_render_short_circuit_plan();
        rendered_short_circuit.has_value()) {
      return rendered_short_circuit;
    }
    if (function_control_flow != nullptr &&
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            *function_control_flow, block.label)
            .has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires the authoritative prepared short-circuit handoff through the canonical prepared-module handoff");
    }

    const auto plain_cond_render_plan =
        c4c::backend::x86::build_prepared_plain_cond_entry_render_plan(
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
        function.name, body, *plain_cond_render_plan, find_block, render_block);
  };

  auto asm_text = std::string(asm_prefix);
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

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register) {
  if (!function.params.empty() || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto wrap_i32 = [](std::int64_t value) -> std::int32_t {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
  };

  PreparedModuleLocalSlotLayout constant_layout;
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  {
    struct SyntheticRootSlot {
      std::size_t size_bytes = 0;
      std::size_t align_bytes = 1;
    };
    std::size_t next_offset = 0;
    std::size_t max_align = 16;
    std::vector<std::string_view> local_slot_names;
    std::unordered_map<std::string, SyntheticRootSlot> synthetic_root_slots;
    const auto note_numeric_suffix_root =
        [&](std::string_view slot_name, std::size_t size_bytes, std::size_t align_bytes) {
          const auto dot = slot_name.rfind('.');
          if (dot == std::string_view::npos || dot + 1 >= slot_name.size()) {
            return;
          }
          std::size_t suffix_offset = 0;
          for (std::size_t index = dot + 1; index < slot_name.size(); ++index) {
            const char ch = slot_name[index];
            if (ch < '0' || ch > '9') {
              return;
            }
            suffix_offset = suffix_offset * 10 + static_cast<std::size_t>(ch - '0');
          }
          auto& root_slot = synthetic_root_slots[std::string(slot_name.substr(0, dot))];
          root_slot.size_bytes = std::max(root_slot.size_bytes, suffix_offset + size_bytes);
          root_slot.align_bytes = std::max(root_slot.align_bytes, align_bytes);
        };
    local_slot_names.reserve(function.local_slots.size());
    for (const auto& slot : function.local_slots) {
      local_slot_names.push_back(slot.name);
    }
    for (const auto& inst : entry.insts) {
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (!store->address.has_value()) {
          const auto stored_size = store->value.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                   : store->value.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                   : store->value.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                   : store->value.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                   : store->value.type == c4c::backend::bir::TypeKind::I8  ? 1u
                                                                                             : 0u;
          if (stored_size != 0) {
            note_numeric_suffix_root(store->slot_name, stored_size,
                                     std::max<std::size_t>(1, stored_size));
          }
        }
        if (store->address.has_value() &&
            store->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot &&
            store->address->size_bytes != 0) {
          note_numeric_suffix_root(store->address->base_name, store->address->size_bytes,
                                   std::max<std::size_t>(1, store->address->align_bytes));
        }
        continue;
      }
      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
          load != nullptr && load->address.has_value() &&
          load->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot &&
          load->address->size_bytes != 0) {
        note_numeric_suffix_root(load->address->base_name, load->address->size_bytes,
                                 std::max<std::size_t>(1, load->address->align_bytes));
      }
    }
    for (const auto& slot : function.local_slots) {
      const auto slot_size = slot.size_bytes != 0
                                 ? slot.size_bytes
                                 : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                    : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                    : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                    : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                    : slot.type == c4c::backend::bir::TypeKind::I8 ? 1u
                                                                                  : 0u);
      const auto slot_align =
          slot.align_bytes != 0 ? slot.align_bytes
                                : (slot_size != 0 ? slot_size : static_cast<std::size_t>(1));
      if (slot_size == 0 || slot_align > 16) {
        continue;
      }
      const bool has_descendant_slots = [&]() {
        const std::string prefix = slot.name + ".";
        for (const auto candidate_name : local_slot_names) {
          if (candidate_name == slot.name) {
            continue;
          }
          if (candidate_name.rfind(prefix, 0) == 0) {
            return true;
          }
        }
        return false;
      }();
      const bool scalar_like_slot = slot.type == c4c::backend::bir::TypeKind::I8 ||
                                    slot.type == c4c::backend::bir::TypeKind::I16 ||
                                    slot.type == c4c::backend::bir::TypeKind::I32 ||
                                    slot.type == c4c::backend::bir::TypeKind::I64 ||
                                    slot.type == c4c::backend::bir::TypeKind::Ptr;
      if (!scalar_like_slot && slot_size > 8) {
        next_offset = align_up(next_offset, slot_align);
        constant_layout.offsets.emplace(slot.name, next_offset);
        if (!has_descendant_slots) {
          next_offset += slot_size;
        }
        max_align = std::max(max_align, slot_align);
        continue;
      }
      next_offset = align_up(next_offset, slot_align);
      constant_layout.offsets.emplace(slot.name, next_offset);
      next_offset += slot_size;
      max_align = std::max(max_align, slot_align);
    }
    for (const auto& [root_name, root_slot] : synthetic_root_slots) {
      if (constant_layout.offsets.find(root_name) != constant_layout.offsets.end() ||
          root_slot.size_bytes == 0 || root_slot.align_bytes > 16) {
        continue;
      }
      std::optional<std::size_t> aggregate_offset;
      const std::string prefix = root_name + ".";
      for (const auto& [candidate_name, candidate_offset] : constant_layout.offsets) {
        if (candidate_name.rfind(prefix, 0) != 0) {
          continue;
        }
        if (!aggregate_offset.has_value() || candidate_offset < *aggregate_offset) {
          aggregate_offset = candidate_offset;
        }
      }
      if (aggregate_offset.has_value()) {
        constant_layout.offsets.emplace(root_name, *aggregate_offset);
        continue;
      }
      next_offset = align_up(next_offset, root_slot.align_bytes);
      constant_layout.offsets.emplace(root_name, next_offset);
      next_offset += root_slot.size_bytes;
      max_align = std::max(max_align, root_slot.align_bytes);
    }
    constant_layout.frame_size = align_up(next_offset, max_align);
  }

  struct ConstantValue {
    c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
    std::int64_t value = 0;
  };

  std::unordered_map<std::string_view, ConstantValue> named_values;
  std::unordered_map<std::size_t, ConstantValue> local_memory;

  std::function<std::optional<std::size_t>(std::string_view)> resolve_local_slot_base_offset =
      [&](std::string_view slot_name) -> std::optional<std::size_t> {
        if (const auto slot_it = constant_layout.offsets.find(slot_name);
            slot_it != constant_layout.offsets.end()) {
          return slot_it->second;
        }
        if (const auto dot = slot_name.rfind('.'); dot != std::string_view::npos &&
            dot + 1 < slot_name.size()) {
          std::size_t suffix_offset = 0;
          bool numeric_suffix = true;
          for (std::size_t index = dot + 1; index < slot_name.size(); ++index) {
            const char ch = slot_name[index];
            if (ch < '0' || ch > '9') {
              numeric_suffix = false;
              break;
            }
            suffix_offset = suffix_offset * 10 + static_cast<std::size_t>(ch - '0');
          }
          if (numeric_suffix) {
            const auto base_offset = resolve_local_slot_base_offset(slot_name.substr(0, dot));
            if (base_offset.has_value()) {
              return *base_offset + suffix_offset;
            }
          }
        }
        std::optional<std::size_t> aggregate_offset;
        const std::string prefix = std::string(slot_name) + ".";
        for (const auto& [candidate_name, candidate_offset] : constant_layout.offsets) {
          if (candidate_name.rfind(prefix, 0) != 0) {
            continue;
          }
          const auto suffix = candidate_name.substr(prefix.size());
          if (suffix.empty()) {
            continue;
          }
          bool numeric_suffix = true;
          for (const char ch : suffix) {
            if (ch < '0' || ch > '9') {
              numeric_suffix = false;
              break;
            }
          }
          if (!numeric_suffix) {
            continue;
          }
          if (!aggregate_offset.has_value() || candidate_offset < *aggregate_offset) {
            aggregate_offset = candidate_offset;
          }
        }
        return aggregate_offset;
      };

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
        if (const auto slot_offset = resolve_local_slot_base_offset(value.name);
            slot_offset.has_value()) {
          return *slot_offset;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it == named_values.end() ||
            value_it->second.type != c4c::backend::bir::TypeKind::Ptr ||
            value_it->second.value < 0) {
          return std::nullopt;
        }
        return static_cast<std::size_t>(value_it->second.value);
      };

  const auto resolve_local_address =
      [&](std::string_view slot_name,
          std::size_t byte_offset,
          const std::optional<c4c::backend::bir::MemoryAddress>& address)
      -> std::optional<std::size_t> {
        if (!address.has_value()) {
          const auto slot_offset = resolve_local_slot_base_offset(slot_name);
          if (!slot_offset.has_value()) {
            return std::nullopt;
          }
          return *slot_offset + byte_offset;
        }

        std::optional<std::size_t> base_offset;
        if (address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
          base_offset = resolve_local_slot_base_offset(address->base_name);
          if (!base_offset.has_value()) {
            return std::nullopt;
          }
        } else if (address->base_kind ==
                   c4c::backend::bir::MemoryAddress::BaseKind::PointerValue) {
          base_offset = resolve_ptr_value(address->base_value);
        } else {
          return std::nullopt;
        }

        if (!base_offset.has_value()) {
          return std::nullopt;
        }
        const auto signed_offset = static_cast<std::int64_t>(*base_offset) +
                                   static_cast<std::int64_t>(byte_offset) + address->byte_offset;
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

  for (const auto& inst : entry.insts) {
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto address =
          resolve_local_address(store->slot_name, store->byte_offset, store->address);
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
      const auto address = resolve_local_address(load->slot_name, load->byte_offset, load->address);
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

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  if (function.params.empty() == false || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      function.blocks.front().insts.empty()) {
    return std::nullopt;
  }
  const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
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
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }

  for (const auto& inst : entry.insts) {
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      if (store->byte_offset != 0) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (store->address.has_value()) {
        memory = render_prepared_local_address_operand_if_supported(
            *layout, store->address,
            store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
      } else {
        const auto slot_it = layout->offsets.find(store->slot_name);
        if (slot_it == layout->offsets.end()) {
          return std::nullopt;
        }
        memory = render_prepared_stack_memory_operand(
            slot_it->second,
            store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
      }
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
        const auto pointee_slot_it = layout->offsets.find(store->value.name);
        if (pointee_slot_it == layout->offsets.end()) {
          return std::nullopt;
        }
        asm_text += "    lea rax, " +
                    render_prepared_stack_memory_operand(pointee_slot_it->second, "QWORD")
                        .substr(10) +
                    "\n";
        asm_text += "    mov " + *memory + ", rax\n";
        continue;
      }
      return std::nullopt;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0) {
      return std::nullopt;
    }
    std::optional<std::string> memory;
    if (load->address.has_value()) {
      memory = render_prepared_local_address_operand_if_supported(
          *layout, load->address,
          load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
    } else {
      const auto slot_it = layout->offsets.find(load->slot_name);
      if (slot_it == layout->offsets.end()) {
        return std::nullopt;
      }
      memory = render_prepared_stack_memory_operand(
          slot_it->second,
          load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
    }
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

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
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

    const auto candidate_layout =
        build_prepared_module_local_slot_layout(*candidate, prepared_arch);
    if (!candidate_layout.has_value()) {
      return std::nullopt;
    }
    const auto rendered_candidate =
        render_prepared_bounded_multi_defined_call_lane_body_if_supported(
            *candidate, defined_functions, *candidate_layout, *candidate_return_register,
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
      const auto* global = find_same_module_global(store->global_name);
      if (global == nullptr || global->type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      used_same_module_globals.insert(global->name);

      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
        body += "    mov DWORD PTR [rip + ";
        body += render_asm_symbol_name(store->global_name);
        body += "], ";
        body += std::to_string(static_cast<std::int32_t>(store->value.immediate));
        body += "\n";
        continue;
      }
      if (store->value.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      body += "    mov DWORD PTR [rip + ";
      body += render_asm_symbol_name(store->global_name);
      body += "], ";
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
    const c4c::backend::bir::Module& module,
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
      defined_functions, prepared_arch, render_trivial_defined_function,
      minimal_function_return_register, minimal_function_asm_prefix, has_string_constant,
      has_same_module_global, render_private_data_label, render_asm_symbol_name);
  if (!rendered_module.has_value()) {
    return std::nullopt;
  }

  const auto rendered_data = render_prepared_bounded_multi_defined_call_lane_data_if_supported(
      *rendered_module, module, helper_global_names, find_string_constant, emit_string_constant_data,
      emit_same_module_global_data);
  if (!rendered_data.has_value()) {
    return std::nullopt;
  }
  return rendered_module->rendered_functions + *rendered_data;
}

}  // namespace c4c::backend::x86
