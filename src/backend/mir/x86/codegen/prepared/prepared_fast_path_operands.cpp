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

}  // namespace c4c::backend::x86
