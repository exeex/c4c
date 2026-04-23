#include "prepared_fast_path_operands.hpp"

#include "../lowering/frame_lowering.hpp"
#include "../lowering/memory_lowering.hpp"

namespace c4c::backend::x86 {

namespace {

std::size_t align_up(std::size_t value, std::size_t align) {
  if (align <= 1) {
    return value;
  }
  const auto remainder = value % align;
  return remainder == 0 ? value : value + (align - remainder);
}

std::int64_t prepared_x86_param_stack_offset(std::size_t class_stack_offset) {
  return 16 + static_cast<std::int64_t>(class_stack_offset);
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

std::optional<std::string_view> prepared_float_memory_operand_size_name(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::F32:
      return "DWORD";
    case c4c::backend::bir::TypeKind::F64:
      return "QWORD";
    default:
      return std::nullopt;
  }
}

}  // namespace

std::optional<std::string> choose_prepared_float_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  constexpr std::string_view kScratchRegister = "xmm15";
  if (prepared_value_homes_use_register(function_locations, kScratchRegister)) {
    return std::nullopt;
  }
  return std::string(kScratchRegister);
}

std::optional<std::string> render_prepared_named_float_source_into_register_if_supported(
    c4c::backend::bir::TypeKind type,
    std::string_view value_name,
    std::string_view destination_register,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  const auto move_mnemonic = type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
  const auto size_name = prepared_float_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }

  if (const auto* home =
          find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
      home != nullptr) {
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value()) {
      if (*home->register_name == destination_register) {
        return std::string{};
      }
      return "    " + std::string(move_mnemonic) + " " + std::string(destination_register) + ", " +
             *home->register_name + "\n";
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      return "    " + std::string(move_mnemonic) + " " + std::string(destination_register) + ", " +
             render_prepared_stack_memory_operand(*home->offset_bytes, *size_name) + "\n";
    }
  }

  if (local_layout != nullptr) {
    if (const auto frame_offset = find_prepared_value_home_frame_offset(
            *local_layout, prepared_names, function_locations, value_name);
        frame_offset.has_value()) {
      return "    " + std::string(move_mnemonic) + " " + std::string(destination_register) + ", " +
             render_prepared_stack_memory_operand(*frame_offset, *size_name) + "\n";
    }
  }

  return std::nullopt;
}

bool append_prepared_float_home_sync_if_supported(
    std::string* body,
    c4c::backend::bir::TypeKind type,
    std::string_view source_register,
    std::string_view value_name,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (body == nullptr) {
    return false;
  }
  const auto move_mnemonic = type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
  const auto size_name = prepared_float_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return false;
  }

  if (const auto* home =
          find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
      home != nullptr) {
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        home->register_name.has_value()) {
      if (*home->register_name != source_register) {
        *body += "    " + std::string(move_mnemonic) + " " + *home->register_name + ", " +
                 std::string(source_register) + "\n";
      }
      return true;
    }
    if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      *body += "    " + std::string(move_mnemonic) + " " +
               render_prepared_stack_memory_operand(*home->offset_bytes, *size_name) + ", " +
               std::string(source_register) + "\n";
      return true;
    }
  }

  if (local_layout != nullptr) {
    if (const auto frame_offset = find_prepared_value_home_frame_offset(
            *local_layout, prepared_names, function_locations, value_name);
        frame_offset.has_value()) {
      *body += "    " + std::string(move_mnemonic) + " " +
               render_prepared_stack_memory_operand(*frame_offset, *size_name) + ", " +
               std::string(source_register) + "\n";
      return true;
    }
  }

  return false;
}

std::optional<std::string> render_prepared_aggregate_slice_root_home_memory_operand_if_supported(
    c4c::backend::bir::TypeKind type,
    std::string_view slot_name,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr || slot_name.empty()) {
    return std::nullopt;
  }
  const auto slice = c4c::backend::prepare::parse_prepared_slot_slice_name(slot_name);
  if (!slice.has_value()) {
    return std::nullopt;
  }
  const PreparedModuleLocalSlotLayout empty_layout;
  const auto& layout = local_layout == nullptr ? empty_layout : *local_layout;

  std::optional<std::string_view> size_name;
  if (type == c4c::backend::bir::TypeKind::I8 || type == c4c::backend::bir::TypeKind::I32) {
    size_name = prepared_scalar_memory_operand_size_name(type);
  } else if (type == c4c::backend::bir::TypeKind::F32 || type == c4c::backend::bir::TypeKind::F64) {
    size_name = prepared_float_memory_operand_size_name(type);
  } else if (type == c4c::backend::bir::TypeKind::F128) {
    size_name = "TBYTE";
  }
  if (!size_name.has_value()) {
    return std::nullopt;
  }

  if (const auto exact_root_home_offset = find_prepared_value_home_frame_offset(
          layout, prepared_names, function_locations, slice->first);
      exact_root_home_offset.has_value()) {
    return render_prepared_stack_memory_operand(*exact_root_home_offset + slice->second, *size_name);
  }

  auto ancestor_name = slice->first;
  while (true) {
    const auto dot = ancestor_name.rfind('.');
    if (dot == std::string_view::npos) {
      break;
    }
    ancestor_name = ancestor_name.substr(0, dot);
    const auto stack_address = render_prepared_named_stack_address_if_supported(
        layout,
        static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
        static_cast<const c4c::backend::prepare::PreparedAddressingFunction*>(nullptr),
        prepared_names,
        function_locations,
        c4c::kInvalidFunctionName,
        ancestor_name,
        slice->second);
    if (stack_address.has_value()) {
      return std::string(*size_name) + " PTR " + *stack_address;
    }
  }

  if (const auto root_stack_address = render_prepared_named_stack_address_if_supported(
          layout,
          static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
          static_cast<const c4c::backend::prepare::PreparedAddressingFunction*>(nullptr),
          prepared_names,
          function_locations,
          c4c::kInvalidFunctionName,
          slice->first,
          slice->second);
      root_stack_address.has_value()) {
    return std::string(*size_name) + " PTR " + *root_stack_address;
  }

  return std::nullopt;
}

std::optional<std::string> render_prepared_named_f128_source_memory_operand_if_supported(
    std::string_view value_name,
    std::size_t stack_byte_bias,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (const auto* home =
          find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
      home != nullptr &&
      home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return render_prepared_stack_memory_operand(*home->offset_bytes + stack_byte_bias, "TBYTE");
  }

  if (local_layout != nullptr) {
    if (const auto frame_offset = find_prepared_value_home_frame_offset(
            *local_layout, prepared_names, function_locations, value_name);
        frame_offset.has_value()) {
      return render_prepared_stack_memory_operand(*frame_offset + stack_byte_bias, "TBYTE");
    }
  }

  if (function != nullptr) {
    std::size_t next_stack_offset = 0;
    for (const auto& param : function->params) {
      if (!param.abi.has_value()) {
        continue;
      }
      const auto& abi = *param.abi;
      const bool stack_passed =
          abi.passed_on_stack || abi.byval_copy || abi.sret_pointer ||
          abi.primary_class == c4c::backend::bir::AbiValueClass::Memory;
      if (!stack_passed) {
        continue;
      }
      const auto stack_align = abi.align_bytes != 0 ? abi.align_bytes : std::size_t{1};
      const auto stack_size = abi.size_bytes != 0 ? abi.size_bytes : std::size_t{1};
      next_stack_offset = align_up(next_stack_offset, stack_align);
      if (param.name == value_name && param.type == c4c::backend::bir::TypeKind::F128) {
        return "TBYTE PTR [rbp + " +
               std::to_string(prepared_x86_param_stack_offset(next_stack_offset)) + "]";
      }
      next_stack_offset += stack_size;
    }
  }

  return std::nullopt;
}

std::optional<std::string> render_prepared_named_f128_copy_into_memory_if_supported(
    std::string_view value_name,
    std::string_view destination_memory_operand,
    std::size_t stack_byte_bias,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Function* function,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  const auto source_memory_operand = render_prepared_named_f128_source_memory_operand_if_supported(
      value_name, stack_byte_bias, local_layout, function, prepared_names, function_locations);
  if (!source_memory_operand.has_value()) {
    return std::nullopt;
  }
  return "    fld " + *source_memory_operand + "\n    fstp " + std::string(destination_memory_operand) +
         "\n";
}

const c4c::backend::prepare::PreparedValueHome* find_prepared_direct_extern_named_value_home(
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_value_home(
      *prepared_names, *function_locations, value_name);
}

bool append_prepared_direct_extern_i32_move_into_register_if_supported(
    std::string* rendered_body,
    std::string_view destination_register,
    const PreparedDirectExternNamedI32Source& source) {
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
}

std::optional<PreparedDirectExternNamedI32Source>
select_prepared_direct_extern_named_i32_source_if_supported(
    std::string_view value_name,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i32.has_value() && current_i32->value_name == value_name) {
    return PreparedDirectExternNamedI32Source{
        .register_name = current_i32->register_name,
        .stack_operand = current_i32->stack_operand,
        .immediate_i32 = std::nullopt,
    };
  }
  const auto constant_it = i32_constants.find(value_name);
  if (constant_it != i32_constants.end()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = constant_it->second,
    };
  }
  const auto* home = find_prepared_direct_extern_named_value_home(
      prepared_names, function_locations, value_name);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = narrow_i32_register(*home->register_name),
        .stack_operand = std::nullopt,
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD"),
        .immediate_i32 = std::nullopt,
    };
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return PreparedDirectExternNamedI32Source{
        .register_name = std::nullopt,
        .stack_operand = std::nullopt,
        .immediate_i32 = *home->immediate_i32,
    };
  }
  return std::nullopt;
}

bool append_prepared_direct_extern_call_argument_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::size_t arg_index,
    std::size_t instruction_index,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::unordered_map<std::string_view, std::int64_t>& i32_constants,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>&
        find_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::unordered_set<std::string_view>* emitted_string_names,
    std::vector<const c4c::backend::bir::StringConstant*>* used_string_constants,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::string* body) {
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
    if (arg.kind != c4c::backend::bir::Value::Kind::Named || arg.name.empty() ||
        arg.name.front() != '@') {
      return false;
    }
    const std::string_view symbol_name(arg.name.data() + 1, arg.name.size() - 1);
    if (const auto* string_constant = find_string_constant(symbol_name); string_constant != nullptr) {
      if (emitted_string_names->insert(symbol_name).second) {
        used_string_constants->push_back(string_constant);
      }
      *body += "    lea ";
      *body += kArgRegs64[arg_index];
      *body += ", [rip + ";
      *body += render_private_data_label(arg.name);
      *body += "]\n";
      return true;
    }
    const auto* global = find_same_module_global(symbol_name);
    if (global == nullptr) {
      return false;
    }
    used_same_module_globals->insert(global->name);
    *body += "    lea ";
    *body += kArgRegs64[arg_index];
    *body += ", [rip + ";
    *body += render_asm_symbol_name(global->name);
    *body += "]\n";
    return true;
  }

  if (arg_type != c4c::backend::bir::TypeKind::I32) {
    return false;
  }

  PreparedDirectExternNamedI32Source source;
  if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
    source.immediate_i32 = static_cast<std::int32_t>(arg.immediate);
  } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
    const auto named_source = select_prepared_direct_extern_named_i32_source_if_supported(
        arg.name, current_i32, i32_constants, prepared_names, function_locations);
    if (!named_source.has_value()) {
      return false;
    }
    source = *named_source;
  } else {
    return false;
  }

  const auto destination_register = select_prepared_i32_call_argument_abi_register_if_supported(
      function_locations, instruction_index, arg_index);
  if (!destination_register.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  return append_prepared_direct_extern_i32_move_into_register_if_supported(
      body, *destination_register, source);
}

bool finalize_prepared_direct_extern_call_result_if_supported(
    const c4c::backend::bir::CallInst& call,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string* body,
    std::optional<PreparedDirectExternCurrentI32Carrier>* current_i32) {
  if (!call.result.has_value() || call.result->type != c4c::backend::bir::TypeKind::I32 ||
      call.result->kind != c4c::backend::bir::Value::Kind::Named) {
    *current_i32 = std::nullopt;
    return true;
  }

  const auto* result_home = find_prepared_direct_extern_named_value_home(
      prepared_names, function_locations, call.result->name);
  const auto call_result_selection =
      select_prepared_i32_call_result_abi_if_supported(
          function_locations, instruction_index, result_home);
  if (!call_result_selection.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  if (call_result_selection->move != nullptr) {
    if (call_result_selection->move->destination_storage_kind !=
        c4c::backend::prepare::PreparedMoveStorageKind::Register) {
      return false;
    }
    if (result_home == nullptr) {
      return false;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto home_register = narrow_i32_register(*result_home->register_name);
      if (!home_register.has_value()) {
        return false;
      }
      if (*home_register != call_result_selection->abi_register) {
        *body += "    mov " + *home_register + ", " + call_result_selection->abi_register + "\n";
      }
      *current_i32 = PreparedDirectExternCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = *home_register,
          .stack_operand = std::nullopt,
      };
      return true;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        result_home->offset_bytes.has_value()) {
      const auto stack_operand =
          render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD");
      *body += "    mov " + stack_operand + ", " + call_result_selection->abi_register + "\n";
      *current_i32 = PreparedDirectExternCurrentI32Carrier{
          .value_name = call.result->name,
          .register_name = std::nullopt,
          .stack_operand = stack_operand,
      };
      return true;
    }
    return false;
  }

  *current_i32 = PreparedDirectExternCurrentI32Carrier{
      .value_name = call.result->name,
      .register_name = call_result_selection->abi_register,
      .stack_operand = std::nullopt,
  };
  return true;
}

bool finalize_prepared_direct_extern_return_if_supported(
    const c4c::backend::bir::Value& returned,
    std::string_view return_register,
    const std::optional<PreparedDirectExternCurrentI32Carrier>& current_i32,
    const std::function<std::optional<std::int64_t>(const c4c::backend::bir::Value&)>&
        resolve_i32_constant,
    std::string* body) {
  if (returned.kind == c4c::backend::bir::Value::Kind::Named &&
      current_i32.has_value() && returned.name == current_i32->value_name &&
      current_i32->register_name.has_value() &&
      *current_i32->register_name == std::string(return_register)) {
    *body += "    add rsp, 8\n    ret\n";
    return true;
  }

  const auto returned_value = resolve_i32_constant(returned);
  if (!returned_value.has_value()) {
    return false;
  }
  *body += "    mov ";
  *body += std::string(return_register);
  *body += ", ";
  *body += std::to_string(static_cast<std::int32_t>(*returned_value));
  *body += "\n    add rsp, 8\n    ret\n";
  return true;
}

}  // namespace c4c::backend::x86
