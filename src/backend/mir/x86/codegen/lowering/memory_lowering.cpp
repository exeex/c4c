#include "memory_lowering.hpp"

#include "frame_lowering.hpp"

namespace c4c::backend::x86 {

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

bool prepared_value_homes_use_register(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view register_name) {
  if (function_locations == nullptr || register_name.empty()) {
    return false;
  }
  for (const auto& home : function_locations->value_homes) {
    if (home.kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home.register_name.has_value() && *home.register_name == register_name) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> choose_prepared_float_load_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  constexpr std::string_view kScratchRegister = "xmm15";
  if (prepared_value_homes_use_register(function_locations, kScratchRegister)) {
    return std::nullopt;
  }
  return std::string(kScratchRegister);
}

}  // namespace

std::optional<std::string_view> prepared_scalar_memory_operand_size_name(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::Ptr:
      return "QWORD";
    case c4c::backend::bir::TypeKind::I8:
      return "BYTE";
    case c4c::backend::bir::TypeKind::I32:
      return "DWORD";
    case c4c::backend::bir::TypeKind::F32:
      return "DWORD";
    case c4c::backend::bir::TypeKind::F64:
      return "QWORD";
    case c4c::backend::bir::TypeKind::F128:
      return "TBYTE";
    default:
      return std::nullopt;
  }
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
  if (prepared_names == nullptr) {
    return nullptr;
  }
  const auto* access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (access == nullptr ||
      access->address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      c4c::backend::prepare::prepared_link_name(*prepared_names, *access->address.symbol_name)
          .empty()) {
    return nullptr;
  }
  return access;
}

std::optional<std::string> render_prepared_i32_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::size_t inst_index,
    const c4c::backend::prepare::PreparedNameTables*,
    const c4c::backend::prepare::PreparedValueLocationFunction*) {
  const auto* prepared_access =
      find_prepared_frame_memory_access(function_addressing, block_label_id, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  return render_prepared_frame_slot_memory_operand_if_supported(
      local_layout, prepared_access->address, "DWORD");
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
  return render_prepared_stack_memory_operand(
      static_cast<std::size_t>(signed_byte_offset), size_name);
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

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_compatible_scalar_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name) {
  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }

  if (const auto* prepared_access =
          find_prepared_frame_memory_access(function_addressing, block_label, inst_index);
      prepared_access != nullptr) {
    const auto memory_operand = render_prepared_frame_slot_memory_operand_if_supported(
        local_layout, prepared_access->address, *size_name);
    if (!memory_operand.has_value()) {
      return std::nullopt;
    }
    return PreparedScalarMemoryAccessRender{
        .setup_asm = {},
        .memory_operand = std::move(*memory_operand),
    };
  }

  if (prepared_names == nullptr || render_asm_symbol_name == nullptr) {
    return std::nullopt;
  }
  const auto* prepared_access =
      find_prepared_symbol_memory_access(prepared_names, function_addressing, block_label, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  const auto memory_operand = render_prepared_symbol_memory_operand_if_supported(
      *prepared_names, prepared_access->address, *size_name, *render_asm_symbol_name);
  if (!memory_operand.has_value()) {
    return std::nullopt;
  }
  return PreparedScalarMemoryAccessRender{
      .setup_asm = {},
      .memory_operand = std::move(*memory_operand),
  };
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

std::optional<std::string> render_prepared_value_home_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  const auto frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      function_name,
      value_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset);
}

std::optional<std::string> render_prepared_named_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name,
    std::size_t stack_byte_bias) {
  auto frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      function_name,
      value_name);
  if (!frame_offset.has_value()) {
    frame_offset = find_prepared_value_home_frame_offset(
        local_layout, prepared_names, function_locations, value_name);
  }
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset + stack_byte_bias);
}

std::optional<std::string> render_prepared_named_payload_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view pointer_name,
    std::size_t stack_byte_bias) {
  auto frame_offset = resolve_prepared_named_payload_frame_offset_if_supported(
      local_layout,
      prepared_names,
      function_locations,
      stack_layout,
      function_name,
      pointer_name);
  if (!frame_offset.has_value()) {
    frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
        local_layout,
        stack_layout,
        function_addressing,
        prepared_names,
        function_locations,
        function_name,
        pointer_name);
  }
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset + stack_byte_bias);
}

std::optional<std::string> render_prepared_named_i32_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::string{};
  }

  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);
  if (!home.has_value()) {
    return std::string{};
  }
  if (home->register_name.has_value()) {
    const auto narrowed_register = narrow_i32_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return std::nullopt;
    }
    if (*narrowed_register == "eax") {
      return std::string{};
    }
    return "    mov " + *narrowed_register + ", eax\n";
  }
  if (home->frame_offset.has_value()) {
    return "    mov " + render_prepared_stack_memory_operand(*home->frame_offset, "DWORD") +
           ", eax\n";
  }
  return std::string{};
}

std::optional<PreparedNamedI32Source> select_prepared_named_i32_source_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32_name.has_value() && value_name == *current_i32_name) {
    return PreparedNamedI32Source{
        .register_name = std::string("eax"),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (previous_i32_name.has_value() && value_name == *previous_i32_name) {
    return PreparedNamedI32Source{
        .register_name = std::string("ecx"),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }

  const auto home = resolve_prepared_named_home_if_supported(
      PreparedModuleLocalSlotLayout{}, prepared_names, function_locations, value_name);
  if (!home.has_value()) {
    return std::nullopt;
  }
  if (home->register_name.has_value()) {
    const auto narrowed_register = narrow_i32_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return std::nullopt;
    }
    return PreparedNamedI32Source{
        .register_name = *narrowed_register,
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->frame_offset.has_value()) {
    return PreparedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = render_prepared_stack_memory_operand(*home->frame_offset, "DWORD"),
        .immediate_i32 = std::nullopt,
    };
  }
  const auto* direct_home =
      c4c::backend::prepare::find_prepared_value_home(
          *prepared_names, *function_locations, value_name);
  if (direct_home != nullptr &&
      direct_home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      direct_home->immediate_i32.has_value()) {
    return PreparedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = *direct_home->immediate_i32,
    };
  }
  return std::nullopt;
}

std::optional<PreparedNamedI32Source> select_prepared_i32_source_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return PreparedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = static_cast<std::int32_t>(value.immediate),
    };
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  return select_prepared_named_i32_source_if_supported(
      value.name, current_i32_name, previous_i32_name, prepared_names, function_locations);
}

std::optional<PreparedNamedI32Source> select_prepared_named_i32_block_source_if_supported(
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32_name.has_value() && value_name == *current_i32_name) {
    return PreparedNamedI32Source{
        .register_name = std::string("eax"),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (local_layout != nullptr) {
    if (block != nullptr && instruction_index <= block->insts.size()) {
      for (std::size_t prior_index = instruction_index; prior_index > 0; --prior_index) {
        const auto* load =
            std::get_if<c4c::backend::bir::LoadLocalInst>(&block->insts[prior_index - 1]);
        if (load != nullptr &&
            load->byte_offset == 0 &&
            load->result.kind == c4c::backend::bir::Value::Kind::Named &&
            load->result.type == c4c::backend::bir::TypeKind::I32 &&
            load->result.name == value_name) {
          const auto memory_operand = render_prepared_i32_memory_operand_for_inst_if_supported(
              *local_layout,
              function_addressing,
              block_label_id,
              prior_index - 1,
              prepared_names,
              function_locations);
          if (memory_operand.has_value()) {
            return PreparedNamedI32Source{
                .register_name = std::nullopt,
                .stack_operand = *memory_operand,
                .immediate_i32 = std::nullopt,
            };
          }
        }
        const auto* store =
            std::get_if<c4c::backend::bir::StoreLocalInst>(&block->insts[prior_index - 1]);
        if (store == nullptr || store->byte_offset != 0 ||
            store->value.kind != c4c::backend::bir::Value::Kind::Named ||
            store->value.type != c4c::backend::bir::TypeKind::I32 ||
            store->value.name != value_name) {
          continue;
        }
        const auto stack_operand = render_prepared_named_stack_memory_operand_if_supported(
            *local_layout, prepared_names, function_locations, store->slot_name, "DWORD");
        if (stack_operand.has_value()) {
          return PreparedNamedI32Source{
              .register_name = std::nullopt,
              .stack_operand = *stack_operand,
              .immediate_i32 = std::nullopt,
          };
        }
      }
    }
    const auto stack_operand = render_prepared_named_stack_memory_operand_if_supported(
        *local_layout, prepared_names, function_locations, value_name, "DWORD");
    if (stack_operand.has_value()) {
      return PreparedNamedI32Source{
          .register_name = std::nullopt,
          .stack_operand = *stack_operand,
          .immediate_i32 = std::nullopt,
      };
    }
  }
  if (previous_i32_name.has_value() && value_name == *previous_i32_name) {
    return PreparedNamedI32Source{
        .register_name = std::string("ecx"),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  return select_prepared_named_i32_source_if_supported(
      value_name, std::nullopt, std::nullopt, prepared_names, function_locations);
}

std::optional<PreparedNamedI32Source> select_prepared_i32_block_source_if_supported(
    const c4c::backend::bir::Value& value,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return PreparedNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = static_cast<std::int32_t>(value.immediate),
    };
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  return select_prepared_named_i32_block_source_if_supported(
      local_layout,
      block,
      instruction_index,
      function_addressing,
      block_label_id,
      value.name,
      current_i32_name,
      previous_i32_name,
      prepared_names,
      function_locations);
}

std::optional<std::string> render_prepared_i32_operand_from_source_if_supported(
    const PreparedNamedI32Source& source) {
  if (source.immediate_i32.has_value()) {
    return std::to_string(static_cast<std::int32_t>(*source.immediate_i32));
  }
  if (source.register_name.has_value()) {
    return *source.register_name;
  }
  return source.stack_operand;
}

std::optional<PreparedI32BinaryRender> render_prepared_i32_binary_from_sources_in_eax_if_supported(
    c4c::backend::bir::BinaryOpcode opcode,
    PreparedNamedI32Source lhs_source,
    PreparedNamedI32Source rhs_source,
    std::optional<std::string_view> authoritative_lhs_value_name,
    std::optional<std::string_view> rhs_value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (rhs_source.register_name.has_value() && *rhs_source.register_name == "eax" &&
      lhs_source.register_name.has_value() && *lhs_source.register_name == "ecx" &&
      authoritative_lhs_value_name.has_value()) {
    if (const auto authoritative_lhs = select_prepared_named_i32_source_if_supported(
            *authoritative_lhs_value_name,
            std::nullopt,
            std::nullopt,
            prepared_names,
            function_locations);
        authoritative_lhs.has_value() &&
        (!authoritative_lhs->register_name.has_value() ||
         *authoritative_lhs->register_name != "eax")) {
      lhs_source = *authoritative_lhs;
    }
  }

  bool rhs_now_in_ecx = false;
  std::string rendered_binary;
  if (rhs_source.register_name.has_value() && *rhs_source.register_name == "eax" &&
      (!lhs_source.register_name.has_value() || *lhs_source.register_name != "eax")) {
    rendered_binary += "    mov ecx, eax\n";
    rhs_source.register_name = std::string("ecx");
    rhs_now_in_ecx = rhs_value_name.has_value();
  }
  if (!append_prepared_named_i32_move_into_register_if_supported(
          &rendered_binary, "eax", lhs_source)) {
    return std::nullopt;
  }
  const auto rhs_operand = render_prepared_i32_operand_from_source_if_supported(rhs_source);
  if (!rhs_operand.has_value()) {
    return std::nullopt;
  }

  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      rendered_binary += "    add eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::Sub:
      rendered_binary += "    sub eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::Mul:
      rendered_binary += "    imul eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::And:
      rendered_binary += "    and eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::Or:
      rendered_binary += "    or eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::Xor:
      rendered_binary += "    xor eax, " + *rhs_operand + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::Shl:
      if (!rhs_source.immediate_i32.has_value()) {
        return std::nullopt;
      }
      rendered_binary +=
          "    shl eax, " + std::to_string(static_cast<std::int32_t>(*rhs_source.immediate_i32)) + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::LShr:
      if (!rhs_source.immediate_i32.has_value()) {
        return std::nullopt;
      }
      rendered_binary +=
          "    shr eax, " + std::to_string(static_cast<std::int32_t>(*rhs_source.immediate_i32)) + "\n";
      break;
    case c4c::backend::bir::BinaryOpcode::AShr:
      if (!rhs_source.immediate_i32.has_value()) {
        return std::nullopt;
      }
      rendered_binary +=
          "    sar eax, " + std::to_string(static_cast<std::int32_t>(*rhs_source.immediate_i32)) + "\n";
      break;
    default:
      return std::nullopt;
  }

  return PreparedI32BinaryRender{
      .body = std::move(rendered_binary),
      .rhs_now_in_ecx = rhs_now_in_ecx,
  };
}

std::optional<std::string> render_prepared_i32_select_from_sources_if_supported(
    const PreparedNamedI32Source& compared_source,
    std::int64_t compared_immediate,
    const PreparedNamedI32Source& true_source,
    const PreparedNamedI32Source& false_source,
    std::string_view false_label,
    std::string_view done_label) {
  std::string body;
  const auto true_operand = render_prepared_i32_operand_from_source_if_supported(true_source);
  if (!true_operand.has_value()) {
    return std::nullopt;
  }

  if (compared_source.register_name.has_value() && *compared_source.register_name == "eax") {
    body += "    cmp eax, " + std::to_string(static_cast<std::int32_t>(compared_immediate)) + "\n";
    body += "    jne " + std::string(false_label) + "\n";
    body += "    mov eax, " + *true_operand + "\n";
    body += "    jmp " + std::string(done_label) + "\n";
    body += std::string(false_label) + ":\n";
    if (!append_prepared_named_i32_move_into_register_if_supported(&body, "eax", false_source)) {
      return std::nullopt;
    }
  } else {
    if (!append_prepared_named_i32_move_into_register_if_supported(&body, "eax", false_source)) {
      return std::nullopt;
    }
    if (compared_source.immediate_i32.has_value()) {
      if (static_cast<std::int32_t>(*compared_source.immediate_i32) ==
          static_cast<std::int32_t>(compared_immediate)) {
        body += "    mov eax, " + *true_operand + "\n";
      }
    } else {
      const auto compared_operand =
          render_prepared_i32_operand_from_source_if_supported(compared_source);
      if (!compared_operand.has_value()) {
        return std::nullopt;
      }
      body += "    cmp " + *compared_operand + ", " +
              std::to_string(static_cast<std::int32_t>(compared_immediate)) + "\n";
      body += "    jne " + std::string(done_label) + "\n";
      body += "    mov eax, " + *true_operand + "\n";
    }
  }

  body += std::string(done_label) + ":\n";
  return body;
}

std::optional<std::string> render_prepared_named_i32_operand_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  const auto source = select_prepared_named_i32_source_if_supported(
      value_name, current_i32_name, previous_i32_name, prepared_names, function_locations);
  if (!source.has_value()) {
    return std::nullopt;
  }
  return render_prepared_i32_operand_from_source_if_supported(*source);
}

std::optional<std::string> render_prepared_named_i32_block_operand_if_supported(
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  const auto source = select_prepared_named_i32_block_source_if_supported(
      local_layout,
      block,
      instruction_index,
      function_addressing,
      block_label_id,
      value_name,
      current_i32_name,
      previous_i32_name,
      prepared_names,
      function_locations);
  if (!source.has_value()) {
    return std::nullopt;
  }
  return render_prepared_i32_operand_from_source_if_supported(*source);
}

bool append_prepared_named_i32_move_into_register_if_supported(
    std::string* body,
    std::string_view destination_register,
    const PreparedNamedI32Source& source) {
  if (body == nullptr) {
    return false;
  }
  if (source.immediate_i32.has_value()) {
    *body += "    mov " + std::string(destination_register) + ", " +
             std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
    return true;
  }
  if (source.register_name.has_value()) {
    if (*source.register_name != destination_register) {
      *body += "    mov " + std::string(destination_register) + ", " + *source.register_name + "\n";
    }
    return true;
  }
  if (source.stack_operand.has_value()) {
    *body += "    mov " + std::string(destination_register) + ", " + *source.stack_operand + "\n";
    return true;
  }
  return false;
}

bool append_prepared_named_i32_home_sync_if_supported(
    std::string* body,
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const PreparedNamedI32Source& source,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (body == nullptr || prepared_names == nullptr || function_locations == nullptr) {
    return true;
  }

  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);
  if (!home.has_value()) {
    return true;
  }
  if (home->register_name.has_value()) {
    const auto narrowed_register = narrow_i32_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return false;
    }
    return append_prepared_named_i32_move_into_register_if_supported(
        body, *narrowed_register, source);
  }
  if (home->frame_offset.has_value()) {
    const auto home_operand =
        render_prepared_stack_memory_operand(*home->frame_offset, "DWORD");
    if (source.immediate_i32.has_value()) {
      *body += "    mov " + home_operand + ", " +
               std::to_string(static_cast<std::int32_t>(*source.immediate_i32)) + "\n";
      return true;
    }
    if (source.register_name.has_value()) {
      *body += "    mov " + home_operand + ", " + *source.register_name + "\n";
      return true;
    }
    if (source.stack_operand.has_value()) {
      if (*source.stack_operand == home_operand) {
        return true;
      }
      *body += "    mov eax, " + *source.stack_operand + "\n";
      *body += "    mov " + home_operand + ", eax\n";
      return true;
    }
    return false;
  }
  return true;
}

std::optional<std::string> finish_prepared_named_i32_load_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string rendered_load,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr) {
    return std::nullopt;
  }

  const auto home_sync = render_prepared_named_i32_stack_home_sync_if_supported(
      local_layout, value_name, prepared_names, function_locations);
  if (!home_sync.has_value()) {
    return std::nullopt;
  }
  rendered_load += *home_sync;
  if (current_i32_name->has_value()) {
    rendered_load = "    mov ecx, eax\n" + rendered_load;
    *previous_i32_name = *current_i32_name;
  } else {
    *previous_i32_name = std::nullopt;
  }
  *current_i32_name = value_name;
  *current_i8_name = std::nullopt;
  return rendered_load;
}

std::optional<std::string> publish_prepared_named_i32_result_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string rendered_value,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view> published_previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr) {
    return std::nullopt;
  }

  const auto synced_home = render_prepared_named_i32_home_sync_if_supported(
      local_layout, value_name, prepared_names, function_locations);
  if (!synced_home.has_value()) {
    return std::nullopt;
  }
  rendered_value += *synced_home;
  *current_i32_name = value_name;
  *previous_i32_name = published_previous_i32_name;
  *current_i8_name = std::nullopt;
  return rendered_value;
}

std::optional<std::string> render_prepared_named_i32_stack_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::string{};
  }

  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);
  if (!home.has_value() || !home->frame_offset.has_value()) {
    return std::string{};
  }
  return "    mov " + render_prepared_stack_memory_operand(*home->frame_offset, "DWORD") +
         ", eax\n";
}

std::optional<std::string> render_prepared_named_ptr_home_sync_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::string{};
  }

  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);
  if (!home.has_value()) {
    return std::string{};
  }
  if (home->register_name.has_value()) {
    if (*home->register_name == "rax") {
      return std::string{};
    }
    return "    mov " + *home->register_name + ", rax\n";
  }
  if (home->frame_offset.has_value()) {
    return "    mov " + render_prepared_stack_memory_operand(*home->frame_offset, "QWORD") +
           ", rax\n";
  }
  return std::string{};
}

std::optional<std::string> render_prepared_named_qword_load_from_memory_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view value_name,
    std::string_view source_memory,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view scratch_register) {
  if (source_memory.empty()) {
    return std::nullopt;
  }

  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);
  if (home.has_value() && home->register_name.has_value()) {
    return "    mov " + *home->register_name + ", " + std::string(source_memory) + "\n";
  }
  if (home.has_value() && home->frame_offset.has_value()) {
    const auto destination_memory =
        render_prepared_stack_memory_operand(*home->frame_offset, "QWORD");
    if (destination_memory == source_memory) {
      return std::string{};
    }
    return "    mov " + std::string(scratch_register) + ", " + std::string(source_memory) +
           "\n    mov " + destination_memory + ", " + std::string(scratch_register) + "\n";
  }
  return "    mov " + std::string(scratch_register) + ", " + std::string(source_memory) + "\n";
}

std::optional<PreparedNamedFloatLoadRender> render_prepared_named_float_load_from_memory_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    c4c::backend::bir::TypeKind type,
    std::string_view value_name,
    std::string_view source_memory,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (source_memory.empty() ||
      (type != c4c::backend::bir::TypeKind::F32 &&
       type != c4c::backend::bir::TypeKind::F64)) {
    return std::nullopt;
  }

  const auto move_mnemonic =
      type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
  const auto home = resolve_prepared_named_home_if_supported(
      local_layout, prepared_names, function_locations, value_name);

  std::string destination_register;
  if (home.has_value() && home->register_name.has_value()) {
    destination_register = *home->register_name;
  } else {
    const auto scratch_register =
        choose_prepared_float_load_scratch_register_if_supported(function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    destination_register = *scratch_register;
  }

  PreparedNamedFloatLoadRender rendered{
      .destination_register = destination_register,
  };
  rendered.body = "    " + std::string(move_mnemonic) + " " + destination_register + ", " +
                  std::string(source_memory) + "\n";
  if (home.has_value() && home->frame_offset.has_value()) {
    const auto destination_memory =
        render_prepared_stack_memory_operand(*home->frame_offset,
                                             type == c4c::backend::bir::TypeKind::F32 ? "DWORD"
                                                                                       : "QWORD");
    if (destination_memory != source_memory) {
      rendered.body += "    " + std::string(move_mnemonic) + " " + destination_memory + ", " +
                       destination_register + "\n";
    }
  }
  return rendered;
}

}  // namespace c4c::backend::x86
