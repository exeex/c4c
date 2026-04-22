#include "x86_codegen.hpp"
#include "lowering/frame_lowering.hpp"
#include "lowering/memory_lowering.hpp"

#include <algorithm>
#include <array>

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

std::optional<std::string_view> inst_result_name(const c4c::backend::bir::Inst& inst) {
  if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
    return binary->result.name;
  }
  if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
    return cast->result.name;
  }
  if (const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&inst)) {
    return select->result.name;
  }
  if (const auto* load_local = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
    return load_local->result.name;
  }
  if (const auto* load_global = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst)) {
    return load_global->result.name;
  }
  return std::nullopt;
}

bool block_defines_named_value(const c4c::backend::bir::Block& block, std::string_view value_name) {
  for (const auto& inst : block.insts) {
    if (const auto result_name = inst_result_name(inst);
        result_name.has_value() && *result_name == value_name) {
      return true;
    }
  }
  return false;
}

bool short_circuit_join_has_join_defined_named_incoming(
    const c4c::backend::prepare::PreparedShortCircuitJoinContext& join_context) {
  if (join_context.join_block == nullptr) {
    return false;
  }

  const auto named_incoming_is_join_defined =
      [&](const c4c::backend::prepare::PreparedEdgeValueTransfer* transfer) {
        return transfer != nullptr &&
               transfer->incoming_value.kind == c4c::backend::bir::Value::Kind::Named &&
               block_defines_named_value(*join_context.join_block, transfer->incoming_value.name);
      };

  return named_incoming_is_join_defined(join_context.true_transfer) ||
         named_incoming_is_join_defined(join_context.false_transfer);
}

}  // namespace

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

struct PreparedI32ValueSelection {
  std::optional<std::int32_t> immediate;
  std::optional<std::string> operand;
  bool in_eax = false;
};

struct PreparedCurrentFloatCarrier {
  std::string_view value_name;
  std::string register_name;
};

struct PreparedScalarMemoryAccessRender;

std::optional<std::string_view> prepared_scalar_memory_operand_size_name(
    c4c::backend::bir::TypeKind type);
std::optional<std::string> render_prepared_symbol_memory_operand_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name);
const c4c::backend::bir::Block* resolve_authoritative_prepared_branch_target(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::bir::Block& block,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block);
std::optional<PreparedScalarMemoryAccessRender>
render_prepared_scalar_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name,
    const std::optional<std::string_view>& current_ptr_name);
bool has_authoritative_prepared_control_flow_block(
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    c4c::BlockLabelId block_label_id);
bool has_authoritative_prepared_short_circuit_continuation(
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation);

struct PreparedSameModuleScalarMemorySelection {
  const c4c::backend::bir::Global* global = nullptr;
  std::string memory_operand;
};

struct PreparedScalarMemoryAccessRender {
  std::string setup_asm;
  std::string memory_operand;
};

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

const c4c::backend::prepare::PreparedValueHome* find_prepared_named_value_home_if_supported(
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_value_home(
      *prepared_names, *function_locations, value_name);
}

std::optional<std::string> choose_prepared_float_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  constexpr std::string_view kScratchRegister = "xmm15";
  if (prepared_value_homes_use_register(function_locations, kScratchRegister)) {
    return std::nullopt;
  }
  return std::string(kScratchRegister);
}

std::optional<std::string> choose_prepared_pointer_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  static constexpr std::array<std::string_view, 4> kScratchRegisters = {"r11", "r10", "r9", "r8"};
  for (const auto candidate : kScratchRegisters) {
    if (!prepared_value_homes_use_register(function_locations, candidate)) {
      return std::string(candidate);
    }
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_named_ptr_into_register_if_supported(
    std::string_view value_name,
    std::string_view destination_register,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::optional<std::string_view>& current_ptr_name,
    std::unordered_set<std::string_view>* active_names = nullptr) {
  if (prepared_names == nullptr || function_locations == nullptr || value_name.empty()) {
    return std::nullopt;
  }

  std::unordered_set<std::string_view> local_active_names;
  auto* recursion_guard = active_names == nullptr ? &local_active_names : active_names;
  if (!recursion_guard->insert(value_name).second) {
    return std::nullopt;
  }

  auto erase_active_name = [&]() { recursion_guard->erase(value_name); };
  if (current_ptr_name.has_value() && *current_ptr_name == value_name) {
    erase_active_name();
    if (destination_register == "rax") {
      return std::string{};
    }
    return "    mov " + std::string(destination_register) + ", rax\n";
  }

  const auto* home =
      find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
  if (home == nullptr) {
    erase_active_name();
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    erase_active_name();
    if (*home->register_name == destination_register) {
      return std::string{};
    }
    return "    mov " + std::string(destination_register) + ", " + *home->register_name + "\n";
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    erase_active_name();
    return "    mov " + std::string(destination_register) + ", " +
           render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD") + "\n";
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    erase_active_name();
    return "    mov " + std::string(destination_register) + ", " +
           std::to_string(*home->immediate_i32) + "\n";
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset &&
      home->pointer_base_value_name.has_value() && home->pointer_byte_delta.has_value()) {
    const auto base_name = c4c::backend::prepare::prepared_value_name(
        *prepared_names, *home->pointer_base_value_name);
    if (base_name.empty()) {
      erase_active_name();
      return std::nullopt;
    }
    auto rendered_base = render_prepared_named_ptr_into_register_if_supported(base_name,
                                                                              destination_register,
                                                                              prepared_names,
                                                                              function_locations,
                                                                              current_ptr_name,
                                                                              recursion_guard);
    erase_active_name();
    if (!rendered_base.has_value()) {
      return std::nullopt;
    }
    if (*home->pointer_byte_delta == 0) {
      return rendered_base;
    }
    std::string rendered = std::move(*rendered_base);
    const auto delta = *home->pointer_byte_delta;
    const auto magnitude = delta < 0 ? -delta : delta;
    rendered += "    ";
    rendered += delta < 0 ? "sub " : "add ";
    rendered += std::string(destination_register) + ", " + std::to_string(magnitude) + "\n";
    return rendered;
  }

  erase_active_name();
  return std::nullopt;
}

bool prepared_pointer_value_matches_family_if_supported(
    std::string_view value_name,
    std::string_view family_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::unordered_set<std::string_view>* active_names = nullptr) {
  if (prepared_names == nullptr || function_locations == nullptr ||
      value_name.empty() || family_name.empty()) {
    return false;
  }
  if (value_name == family_name) {
    return true;
  }

  std::unordered_set<std::string_view> local_active_names;
  auto* recursion_guard = active_names == nullptr ? &local_active_names : active_names;
  if (!recursion_guard->insert(value_name).second) {
    return false;
  }

  auto erase_active_name = [&]() { recursion_guard->erase(value_name); };
  const auto* home =
      find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
  if (home == nullptr ||
      home->kind != c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !home->pointer_base_value_name.has_value()) {
    erase_active_name();
    return false;
  }
  const auto base_name =
      c4c::backend::prepare::prepared_value_name(*prepared_names, *home->pointer_base_value_name);
  const bool matches = !base_name.empty() &&
                       prepared_pointer_value_matches_family_if_supported(base_name,
                                                                         family_name,
                                                                         prepared_names,
                                                                         function_locations,
                                                                         recursion_guard);
  erase_active_name();
  return matches;
}

std::string_view prepared_current_ptr_carrier_name(
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::unordered_set<std::string_view>* active_names = nullptr) {
  if (prepared_names == nullptr || function_locations == nullptr || value_name.empty()) {
    return value_name;
  }
  std::unordered_set<std::string_view> local_active_names;
  auto* recursion_guard = active_names == nullptr ? &local_active_names : active_names;
  if (!recursion_guard->insert(value_name).second) {
    return value_name;
  }
  const auto* home =
      find_prepared_named_value_home_if_supported(prepared_names, function_locations, value_name);
  if (home == nullptr ||
      home->kind != c4c::backend::prepare::PreparedValueHomeKind::PointerBasePlusOffset ||
      !home->pointer_base_value_name.has_value()) {
    return value_name;
  }
  const auto base_name =
      c4c::backend::prepare::prepared_value_name(*prepared_names, *home->pointer_base_value_name);
  if (base_name.empty()) {
    return value_name;
  }
  return prepared_current_ptr_carrier_name(
      base_name, prepared_names, function_locations, recursion_guard);
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

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_local_slot_memory_operand_from_handoff_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    std::string_view slot_name,
    std::size_t byte_offset,
    bool use_prepared_access,
    const std::optional<std::string_view>& current_ptr_name,
    std::unordered_set<std::string_view>* used_same_module_globals) {
  if (block_context.function_context == nullptr || block_context.local_layout == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (use_prepared_access) {
    const auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
        *block_context.local_layout,
        function_context.prepared_names,
        function_context.function_locations,
        function_context.function_addressing,
        block_context.block_label_id,
        inst_index,
        type,
        &function_context.render_asm_symbol_name,
        current_ptr_name);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    if (type != c4c::backend::bir::TypeKind::F128 &&
        used_same_module_globals != nullptr &&
        function_context.prepared_names != nullptr) {
      if (const auto* symbol_access = find_prepared_symbol_memory_access(
              function_context.prepared_names,
              function_context.function_addressing,
              block_context.block_label_id,
              inst_index);
          symbol_access != nullptr) {
        const auto symbol_name = c4c::backend::prepare::prepared_link_name(
            *function_context.prepared_names, *symbol_access->address.symbol_name);
        if (!symbol_name.empty()) {
          if (const auto* global = function_context.find_same_module_global(symbol_name);
              global != nullptr) {
            used_same_module_globals->insert(global->name);
          }
        }
      }
    }
    return memory;
  }

  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }
  const auto base_offset = resolve_prepared_local_slot_base_offset_if_supported(
      *block_context.local_layout,
      function_context.prepared_names,
      function_context.function_locations,
      slot_name);
  if (!base_offset.has_value()) {
    return std::nullopt;
  }
  return PreparedScalarMemoryAccessRender{
      .setup_asm = {},
      .memory_operand =
          render_prepared_stack_memory_operand(*base_offset + byte_offset, *size_name),
  };
}

std::optional<std::string> render_prepared_float_load_or_cast_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::size_t inst_index,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<PreparedCurrentFloatCarrier>* current_f32_value,
    std::optional<PreparedCurrentFloatCarrier>* current_f64_value,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || block_context.local_layout == nullptr ||
      current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_f32_value == nullptr || current_f64_value == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  const auto emit_float_load =
      [&](c4c::backend::bir::TypeKind type,
          std::string_view result_name,
          std::string_view memory_operand,
          std::string setup_asm = {}) -> std::optional<std::string> {
    const auto rendered_load = render_prepared_named_float_load_from_memory_if_supported(
        *block_context.local_layout,
        type,
        result_name,
        memory_operand,
        function_context.prepared_names,
        function_context.function_locations);
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    std::string rendered = std::move(setup_asm);
    rendered += rendered_load->body;
    *current_materialized_compare = std::nullopt;
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    if (type == c4c::backend::bir::TypeKind::F32) {
      *current_f32_value = PreparedCurrentFloatCarrier{
          .value_name = result_name,
          .register_name = rendered_load->destination_register,
      };
      *current_f64_value = std::nullopt;
    } else {
      *current_f32_value = std::nullopt;
      *current_f64_value = PreparedCurrentFloatCarrier{
          .value_name = result_name,
          .register_name = rendered_load->destination_register,
      };
    }
    return rendered;
  };

  if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
    if (load->result.kind != c4c::backend::bir::Value::Kind::Named ||
        (load->result.type != c4c::backend::bir::TypeKind::F32 &&
         load->result.type != c4c::backend::bir::TypeKind::F64)) {
      return std::nullopt;
    }
    const auto memory = render_prepared_local_slot_memory_operand_from_handoff_if_supported(
        block_context,
        inst_index,
        load->result.type,
        load->slot_name,
        load->byte_offset,
        load->address.has_value(),
        *current_ptr_name,
        used_same_module_globals);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    return emit_float_load(
        load->result.type, load->result.name, memory->memory_operand, memory->setup_asm);
  }

  const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
  if (cast == nullptr || cast->opcode != c4c::backend::bir::CastOpcode::FPExt ||
      cast->operand.type != c4c::backend::bir::TypeKind::F32 ||
      cast->result.type != c4c::backend::bir::TypeKind::F64 ||
      cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
      cast->operand.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto* result_home = find_prepared_named_value_home_if_supported(
      function_context.prepared_names, function_context.function_locations, cast->result.name);
  std::string destination_register;
  if (result_home != nullptr &&
      result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      result_home->register_name.has_value()) {
    destination_register = *result_home->register_name;
  } else {
    const auto scratch_register = choose_prepared_float_scratch_register_if_supported(
        function_context.function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    destination_register = *scratch_register;
  }
  const auto rendered_operand = render_prepared_named_float_source_into_register_if_supported(
      c4c::backend::bir::TypeKind::F32,
      cast->operand.name,
      destination_register,
      block_context.local_layout,
      function_context.prepared_names,
      function_context.function_locations);
  if (!rendered_operand.has_value()) {
    return std::nullopt;
  }
  std::string rendered = *rendered_operand;
  rendered += "    cvtss2sd " + destination_register + ", " + destination_register + "\n";
  if (!append_prepared_float_home_sync_if_supported(
          &rendered,
          c4c::backend::bir::TypeKind::F64,
          destination_register,
          cast->result.name,
          block_context.local_layout,
          function_context.prepared_names,
          function_context.function_locations)) {
    return std::nullopt;
  }
  *current_materialized_compare = std::nullopt;
  *current_i32_name = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  *current_f32_value = std::nullopt;
  *current_f64_value = PreparedCurrentFloatCarrier{
      .value_name = cast->result.name,
      .register_name = destination_register,
  };
  return rendered;
}

std::optional<std::string> render_prepared_float_store_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::size_t inst_index,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<PreparedCurrentFloatCarrier>* current_f32_value,
    std::optional<PreparedCurrentFloatCarrier>* current_f64_value,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || current_i32_name == nullptr ||
      previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_f32_value == nullptr ||
      current_f64_value == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
  if (store == nullptr || store->value.kind != c4c::backend::bir::Value::Kind::Named ||
      (store->value.type != c4c::backend::bir::TypeKind::F32 &&
       store->value.type != c4c::backend::bir::TypeKind::F64)) {
    return std::nullopt;
  }
  const auto memory = render_prepared_local_slot_memory_operand_from_handoff_if_supported(
      block_context,
      inst_index,
      store->value.type,
      store->slot_name,
      store->byte_offset,
      store->address.has_value(),
      *current_ptr_name,
      used_same_module_globals);
  if (!memory.has_value()) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  const auto move_mnemonic =
      store->value.type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
  const auto effective_slice_slot_name =
      [&]() -> std::optional<std::string> {
        if (store->address.has_value()) {
          return std::nullopt;
        }
        if (c4c::backend::prepare::parse_prepared_slot_slice_name(store->slot_name).has_value()) {
          return std::string(store->slot_name);
        }
        return std::string(store->slot_name) + "." + std::to_string(store->byte_offset);
      }();
  std::string source_register;
  std::string rendered;
  const auto* current_value =
      store->value.type == c4c::backend::bir::TypeKind::F32 ? current_f32_value
                                                            : current_f64_value;
  if (current_value->has_value() && (*current_value)->value_name == store->value.name) {
    source_register = (*current_value)->register_name;
  } else if (const auto* home = find_prepared_named_value_home_if_supported(
                 function_context.prepared_names,
                 function_context.function_locations,
                 store->value.name);
             home != nullptr &&
             home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
             home->register_name.has_value()) {
    source_register = *home->register_name;
  } else {
    const auto scratch_register =
        choose_prepared_float_scratch_register_if_supported(function_context.function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    source_register = *scratch_register;
  }

  const auto rendered_source = render_prepared_named_float_source_into_register_if_supported(
      store->value.type,
      store->value.name,
      source_register,
      block_context.local_layout,
      function_context.prepared_names,
      function_context.function_locations);
  if (!rendered_source.has_value()) {
    return std::nullopt;
  }
  rendered = memory->setup_asm + *rendered_source;
  rendered +=
      "    " + std::string(move_mnemonic) + " " + memory->memory_operand + ", " + source_register +
      "\n";
  if (effective_slice_slot_name.has_value()) {
    if (const auto aggregate_root_memory =
            render_prepared_aggregate_slice_root_home_memory_operand_if_supported(
                store->value.type,
                *effective_slice_slot_name,
                block_context.local_layout,
                function_context.prepared_names,
                function_context.function_locations);
        aggregate_root_memory.has_value() && *aggregate_root_memory != memory->memory_operand) {
      rendered += "    " + std::string(move_mnemonic) + " " + *aggregate_root_memory + ", " +
                  source_register + "\n";
    }
  }
  *current_materialized_compare = std::nullopt;
  *current_i32_name = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return rendered;
}

std::optional<std::string> render_prepared_f128_local_copy_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::size_t inst_index,
    std::unordered_set<std::string_view>* used_same_module_globals,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || block_context.local_layout == nullptr ||
      current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;

  if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
    if (load->result.kind != c4c::backend::bir::Value::Kind::Named ||
        load->result.type != c4c::backend::bir::TypeKind::F128) {
      return std::nullopt;
    }
    const auto source_memory = render_prepared_local_slot_memory_operand_from_handoff_if_supported(
        block_context,
        inst_index,
        load->result.type,
        load->slot_name,
        load->byte_offset,
        load->address.has_value(),
        *current_ptr_name,
        used_same_module_globals);
    const auto destination_memory_operand =
        render_prepared_named_f128_source_memory_operand_if_supported(
            load->result.name,
            0,
            block_context.local_layout,
            function_context.function,
            function_context.prepared_names,
            function_context.function_locations);
    if (!source_memory.has_value() || !destination_memory_operand.has_value()) {
      return std::nullopt;
    }
    *current_materialized_compare = std::nullopt;
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return source_memory->setup_asm + "    fld " + source_memory->memory_operand +
           "\n    fstp " + *destination_memory_operand + "\n";
  }

  const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
  if (store == nullptr || store->value.kind != c4c::backend::bir::Value::Kind::Named ||
      store->value.type != c4c::backend::bir::TypeKind::F128) {
    return std::nullopt;
  }
  const auto destination_memory = render_prepared_local_slot_memory_operand_from_handoff_if_supported(
      block_context,
      inst_index,
      store->value.type,
      store->slot_name,
      store->byte_offset,
      store->address.has_value(),
      *current_ptr_name,
      used_same_module_globals);
  if (!destination_memory.has_value()) {
    return std::nullopt;
  }
  const auto rendered_copy = render_prepared_named_f128_copy_into_memory_if_supported(
      store->value.name,
      destination_memory->memory_operand,
      0,
      block_context.local_layout,
      function_context.function,
      function_context.prepared_names,
      function_context.function_locations);
  if (!rendered_copy.has_value()) {
    return std::nullopt;
  }
  const auto effective_slice_slot_name =
      [&]() -> std::optional<std::string> {
        if (store->address.has_value()) {
          return std::nullopt;
        }
        if (c4c::backend::prepare::parse_prepared_slot_slice_name(store->slot_name).has_value()) {
          return std::string(store->slot_name);
        }
        return std::string(store->slot_name) + "." + std::to_string(store->byte_offset);
      }();
  std::string rendered = destination_memory->setup_asm + *rendered_copy;
  if (effective_slice_slot_name.has_value()) {
    if (const auto aggregate_root_memory =
            render_prepared_aggregate_slice_root_home_memory_operand_if_supported(
                store->value.type,
                *effective_slice_slot_name,
                block_context.local_layout,
                function_context.prepared_names,
                function_context.function_locations);
        aggregate_root_memory.has_value() && *aggregate_root_memory != destination_memory->memory_operand) {
      const auto rendered_root_copy = render_prepared_named_f128_copy_into_memory_if_supported(
          store->value.name,
          *aggregate_root_memory,
          0,
          block_context.local_layout,
          function_context.function,
          function_context.prepared_names,
          function_context.function_locations);
      if (!rendered_root_copy.has_value()) {
        return std::nullopt;
      }
      rendered += *rendered_root_copy;
    }
  }
  *current_materialized_compare = std::nullopt;
  *current_i32_name = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return rendered;
}

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_pointer_value_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name,
    const std::optional<std::string_view>& current_ptr_name) {
  if (prepared_names == nullptr || function_locations == nullptr ||
      address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::PointerValue ||
      !address.pointer_value_name.has_value() || !address.can_use_base_plus_offset) {
    return std::nullopt;
  }

  const std::string_view pointer_name =
      c4c::backend::prepare::prepared_value_name(*prepared_names, *address.pointer_value_name);
  if (pointer_name.empty()) {
    return std::nullopt;
  }

  const auto frame_offset = render_prepared_value_home_stack_address_if_supported(
      local_layout,
      static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
      function_addressing,
      prepared_names,
      function_locations,
      c4c::kInvalidFunctionName,
      pointer_name);

  std::string_view base_register;
  std::string setup_asm;
  if (!frame_offset.has_value() && current_ptr_name.has_value() &&
      prepared_pointer_value_matches_family_if_supported(*current_ptr_name,
                                                         pointer_name,
                                                         prepared_names,
                                                         function_locations)) {
    base_register = "rax";
  } else {
    const auto scratch_register =
        choose_prepared_pointer_scratch_register_if_supported(function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    if (frame_offset.has_value()) {
      setup_asm = "    lea " + *scratch_register + ", " + *frame_offset + "\n";
    } else {
      const auto rendered_pointer = render_prepared_named_ptr_into_register_if_supported(
          pointer_name,
          *scratch_register,
          prepared_names,
          function_locations,
          current_ptr_name);
      if (!rendered_pointer.has_value()) {
        return std::nullopt;
      }
      setup_asm = std::move(*rendered_pointer);
    }
    base_register = *scratch_register;
  }

  std::string memory_operand = std::string(size_name) + " PTR [" + std::string(base_register);
  if (address.byte_offset > 0) {
    memory_operand += " + " + std::to_string(address.byte_offset);
  } else if (address.byte_offset < 0) {
    memory_operand += " - " + std::to_string(-address.byte_offset);
  }
  memory_operand += "]";
  return PreparedScalarMemoryAccessRender{
      .setup_asm = std::move(setup_asm),
      .memory_operand = std::move(memory_operand),
  };
}

template <class ValidateGlobal>
std::optional<PreparedSameModuleScalarMemorySelection>
select_prepared_same_module_scalar_memory_for_inst_if_supported(
    const c4c::backend::prepare::PreparedNameTables& prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    ValidateGlobal&& validate_global) {
  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }
  const auto* prepared_access =
      find_prepared_symbol_memory_access(&prepared_names, function_addressing, block_label, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  auto memory_operand = render_prepared_symbol_memory_operand_if_supported(
      prepared_names, prepared_access->address, *size_name, render_asm_symbol_name);
  if (!memory_operand.has_value()) {
    return std::nullopt;
  }
  const std::string_view resolved_global_name =
      c4c::backend::prepare::prepared_link_name(
          prepared_names, *prepared_access->address.symbol_name);
  const auto* global = find_same_module_global(resolved_global_name);
  if (global == nullptr || !validate_global(*global, prepared_access->address.byte_offset)) {
    return std::nullopt;
  }
  return PreparedSameModuleScalarMemorySelection{
      .global = global,
      .memory_operand = std::move(*memory_operand),
  };
}

template <class ResolveNamedOperand>
std::optional<PreparedI32ValueSelection> select_prepared_i32_value_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return PreparedI32ValueSelection{
        .immediate = static_cast<std::int32_t>(value.immediate),
        .operand = std::nullopt,
        .in_eax = false,
    };
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  if (current_i32_name.has_value() && value.name == *current_i32_name) {
    return PreparedI32ValueSelection{
        .immediate = std::nullopt,
        .operand = std::nullopt,
        .in_eax = true,
    };
  }
  const auto operand = resolve_named_operand(value.name);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  return PreparedI32ValueSelection{
      .immediate = std::nullopt,
      .operand = std::move(*operand),
      .in_eax = false,
  };
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_operand_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto selection = select_prepared_i32_value_if_supported(
      value, current_i32_name, resolve_named_operand);
  if (!selection.has_value()) {
    return std::nullopt;
  }
  if (selection->immediate.has_value()) {
    return std::to_string(*selection->immediate);
  }
  if (selection->in_eax) {
    return std::string("eax");
  }
  if (!selection->operand.has_value()) {
    return std::nullopt;
  }
  return *selection->operand;
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_move_to_register_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    std::string_view target_register,
    ResolveNamedOperand&& resolve_named_operand) {
  const auto operand = render_prepared_i32_operand_if_supported(
      value, current_i32_name, resolve_named_operand);
  if (!operand.has_value()) {
    return std::nullopt;
  }
  if (*operand == target_register) {
    return std::string{};
  }
  return "    mov " + std::string(target_register) + ", " + *operand + "\n";
}

bool prepared_pointer_value_has_authoritative_memory_use(
    std::string_view value_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing) {
  if (prepared_names == nullptr || function_addressing == nullptr) {
    return false;
  }
  const auto value_name_id =
      c4c::backend::prepare::resolve_prepared_value_name_id(*prepared_names, value_name);
  if (!value_name_id.has_value()) {
    return false;
  }
  return std::any_of(
      function_addressing->accesses.begin(),
      function_addressing->accesses.end(),
      [&](const c4c::backend::prepare::PreparedMemoryAccess& access) {
        return access.address.base_kind ==
                   c4c::backend::prepare::PreparedAddressBaseKind::PointerValue &&
               access.address.pointer_value_name == value_name_id;
      });
}

bool prepared_i32_binary_opcode_is_commutative(c4c::backend::bir::BinaryOpcode opcode) {
  return opcode == c4c::backend::bir::BinaryOpcode::Add ||
         opcode == c4c::backend::bir::BinaryOpcode::Mul ||
         opcode == c4c::backend::bir::BinaryOpcode::And ||
         opcode == c4c::backend::bir::BinaryOpcode::Or ||
         opcode == c4c::backend::bir::BinaryOpcode::Xor;
}

bool i32_register_is_caller_saved(std::string_view register_name) {
  return register_name == "eax" || register_name == "ecx" || register_name == "edx" ||
         register_name == "esi" || register_name == "edi" || register_name == "r8d" ||
         register_name == "r9d" || register_name == "r10d" || register_name == "r11d";
}

inline constexpr std::array<std::string_view, 5> kPreparedCalleeSavedWideRegisters{
    "rbx", "r13", "r14", "r15", "r12"};

bool prepared_wide_register_is_callee_saved(std::string_view register_name) {
  return std::find(kPreparedCalleeSavedWideRegisters.begin(),
                   kPreparedCalleeSavedWideRegisters.end(),
                   register_name) != kPreparedCalleeSavedWideRegisters.end();
}

struct PreparedI32ParamRegisterPreservationPlan {
  std::vector<std::string_view> borrowed_wide_registers;
  std::string save_asm;
  std::string restore_asm;
};

std::optional<PreparedI32ParamRegisterPreservationPlan>
select_prepared_i32_param_register_preservation_plan_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (prepared_names == nullptr || function_locations == nullptr) {
    return PreparedI32ParamRegisterPreservationPlan{};
  }

  std::size_t next_save_register = 0;
  PreparedI32ParamRegisterPreservationPlan plan;
  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs ||
        param.is_sret || param.is_byval) {
      continue;
    }
    const auto* home =
        c4c::backend::prepare::find_prepared_value_home(
            *prepared_names, *function_locations, param.name);
    if (home == nullptr ||
        home->kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
        !home->register_name.has_value()) {
      continue;
    }
    const auto home_register = narrow_i32_register(*home->register_name);
    if (!home_register.has_value() || !i32_register_is_caller_saved(*home_register)) {
      continue;
    }
    while (next_save_register < kPreparedCalleeSavedWideRegisters.size() &&
           prepared_value_homes_use_register(function_locations,
                                             kPreparedCalleeSavedWideRegisters[next_save_register])) {
      ++next_save_register;
    }
    if (next_save_register == kPreparedCalleeSavedWideRegisters.size()) {
      return std::nullopt;
    }
    const auto save_register =
        narrow_i32_register(kPreparedCalleeSavedWideRegisters[next_save_register]);
    if (!save_register.has_value()) {
      return std::nullopt;
    }
    plan.borrowed_wide_registers.push_back(kPreparedCalleeSavedWideRegisters[next_save_register]);
    plan.save_asm += "    mov " + *save_register + ", " + *home_register + "\n";
    plan.restore_asm += "    mov " + *home_register + ", " + *save_register + "\n";
    ++next_save_register;
  }
  return plan;
}

bool prepared_function_has_same_module_call(const c4c::backend::bir::Module& module,
                                            const c4c::backend::bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
          call->is_variadic || call->args.size() != call->arg_types.size() || call->args.size() > 6) {
        continue;
      }
      for (const auto& candidate : module.functions) {
        if (!candidate.is_declaration && candidate.name == call->callee) {
          return true;
        }
      }
    }
  }
  return false;
}

bool prepared_function_is_same_module_call_callee(const c4c::backend::bir::Module& module,
                                                  std::string_view callee_name) {
  if (callee_name.empty()) {
    return false;
  }
  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        if (call == nullptr || call->is_indirect || call->callee.empty() ||
            call->callee_value.has_value() || call->is_variadic ||
            call->args.size() != call->arg_types.size() || call->args.size() > 6) {
          continue;
        }
        if (call->callee == callee_name) {
          return true;
        }
      }
    }
  }
  return false;
}

std::vector<std::string_view> collect_prepared_function_callee_saved_registers(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::vector<std::string_view>& borrowed_wide_registers) {
  std::vector<std::string_view> registers;
  const auto append_if_missing = [&](std::string_view register_name) {
    if (std::find(registers.begin(), registers.end(), register_name) == registers.end()) {
      registers.push_back(register_name);
    }
  };

  if (function_locations != nullptr) {
    for (const auto& home : function_locations->value_homes) {
      if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !home.register_name.has_value() ||
          !prepared_wide_register_is_callee_saved(*home.register_name)) {
        continue;
      }
      append_if_missing(*home.register_name);
    }
  }
  for (const auto register_name : borrowed_wide_registers) {
    append_if_missing(register_name);
  }
  if ((registers.size() % 2) != 0) {
    for (const auto register_name : kPreparedCalleeSavedWideRegisters) {
      if (std::find(registers.begin(), registers.end(), register_name) == registers.end()) {
        registers.push_back(register_name);
        break;
      }
    }
  }
  return registers;
}

std::string render_prepared_function_callee_saved_save_asm(
    const std::vector<std::string_view>& registers) {
  std::string asm_text;
  for (const auto register_name : registers) {
    asm_text += "    push " + std::string(register_name) + "\n";
  }
  return asm_text;
}

std::string render_prepared_function_callee_saved_restore_asm(
    const std::vector<std::string_view>& registers) {
  std::string asm_text;
  for (auto it = registers.rbegin(); it != registers.rend(); ++it) {
    asm_text += "    pop " + std::string(*it) + "\n";
  }
  return asm_text;
}

const char* prepared_i32_setcc_opcode_if_supported(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
      return "sete";
    case c4c::backend::bir::BinaryOpcode::Ne:
      return "setne";
    case c4c::backend::bir::BinaryOpcode::Sgt:
      return "setg";
    case c4c::backend::bir::BinaryOpcode::Sge:
      return "setge";
    case c4c::backend::bir::BinaryOpcode::Slt:
      return "setl";
    case c4c::backend::bir::BinaryOpcode::Sle:
      return "setle";
    default:
      return nullptr;
  }
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
  return c4c::backend::x86::select_prepared_named_i32_block_source_if_supported(
      local_layout,
      block,
      instruction_index,
      value_name,
      current_i32_name,
      previous_i32_name,
      prepared_names,
      function_locations,
      [&](std::size_t prior_index) -> std::optional<std::string> {
        if (local_layout == nullptr) {
          return std::nullopt;
        }
        const auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
            *local_layout,
            prepared_names,
            function_locations,
            function_addressing,
            block_label_id,
            prior_index,
            c4c::backend::bir::TypeKind::I32,
            nullptr,
            std::nullopt);
        if (!memory.has_value() || !memory->setup_asm.empty()) {
          return std::nullopt;
        }
        return memory->memory_operand;
      });
}

std::optional<std::string> render_prepared_named_i32_operand_if_supported(
    std::string_view value_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  return c4c::backend::x86::render_prepared_named_i32_operand_if_supported(
      value_name, current_i32_name, previous_i32_name, prepared_names, function_locations);
}

template <class ResolveNamedOperand>
std::optional<std::string> render_prepared_i32_setup_in_eax_if_supported(
    const c4c::backend::bir::Value& value,
    const std::optional<std::string_view>& current_i32_name,
    ResolveNamedOperand&& resolve_named_operand) {
  if (value.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return "    mov eax, " + std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
  }
  return render_prepared_i32_move_to_register_if_supported(
      value, current_i32_name, "eax", std::forward<ResolveNamedOperand>(resolve_named_operand));
}

}  // namespace

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs) {
  const bool lhs_is_value_rhs_is_imm =
      lhs.kind == c4c::backend::bir::Value::Kind::Named &&
      rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      rhs.type == c4c::backend::bir::TypeKind::I32;
  const bool rhs_is_value_lhs_is_imm =
      rhs.kind == c4c::backend::bir::Value::Kind::Named &&
      lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
      lhs.type == c4c::backend::bir::TypeKind::I32;
  if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
    return std::nullopt;
  }
  return PreparedI32NamedImmediateCompareSelection{
      .named_value = lhs_is_value_rhs_is_imm ? &lhs : &rhs,
      .immediate = lhs_is_value_rhs_is_imm ? rhs.immediate : lhs.immediate,
  };
}

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_value_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string_view compared_value_name) {
  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_if_supported(lhs, rhs);
  if (!selected_compare.has_value() ||
      selected_compare->named_value->name != compared_value_name) {
    return std::nullopt;
  }
  return selected_compare;
}

bool prepared_i32_compare_value_name_matches_any_candidate(
    std::string_view compared_value_name,
    const std::vector<std::string_view>& candidate_compare_value_names) {
  return std::find(candidate_compare_value_names.begin(),
                   candidate_compare_value_names.end(),
                   compared_value_name) != candidate_compare_value_names.end();
}

std::optional<PreparedI32NamedImmediateCompareSelection>
select_prepared_i32_named_immediate_compare_for_candidates_if_supported(
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    const std::vector<std::string_view>& candidate_compare_value_names) {
  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_if_supported(lhs, rhs);
  if (!selected_compare.has_value() ||
      !prepared_i32_compare_value_name_matches_any_candidate(
          selected_compare->named_value->name, candidate_compare_value_names)) {
    return std::nullopt;
  }
  return selected_compare;
}

std::string render_prepared_i32_eax_immediate_compare_setup(std::int64_t compare_immediate) {
  if (compare_immediate == 0) {
    return std::string("    test eax, eax\n");
  }
  return "    cmp eax, " +
         std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
}

namespace {

struct PreparedI32ImmediateBranchPlan {
  std::int64_t compare_immediate = 0;
  c4c::backend::bir::BinaryOpcode compare_opcode = c4c::backend::bir::BinaryOpcode::Eq;
  std::string true_label;
  std::string false_label;
};

struct PreparedI32ComparedImmediateBranchPlan {
  c4c::backend::bir::Value compared_value;
  PreparedI32ImmediateBranchPlan branch_plan;
};

struct NamedLocalI32Expr {
  enum class Kind { LocalLoad, Add, Sub };
  Kind kind = Kind::LocalLoad;
  std::string operand;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

struct PreparedLocalI32GuardExpressionSurface {
  std::string setup_asm;
  std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
  std::vector<std::string_view> candidate_compare_value_names;
};

std::optional<std::string> render_prepared_frame_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedAddress& address,
    std::string_view size_name);

std::optional<PreparedLocalI32GuardExpressionSurface>
select_prepared_local_i32_guard_expression_surface_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    const PreparedModuleLocalSlotLayout& layout,
    c4c::BlockLabelId entry_label_id) {
  if (context.entry == nullptr) {
    return std::nullopt;
  }

  PreparedLocalI32GuardExpressionSurface selection;
  const auto& entry = *context.entry;
  for (std::size_t index = 0; index + 1 < entry.insts.size(); ++index) {
    const auto& inst = entry.insts[index];
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, index);
      if (store->byte_offset != 0 || store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      selection.setup_asm += "    mov " + *memory + ", " +
                             std::to_string(static_cast<std::int32_t>(store->value.immediate)) +
                             "\n";
      continue;
    }

    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
      const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, index);
      if (load->byte_offset != 0 || load->result.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::optional<std::string> memory;
      if (prepared_access != nullptr) {
        memory = render_prepared_frame_slot_memory_operand_if_supported(
            layout, prepared_access->address, "DWORD");
      }
      if (!memory.has_value()) {
        return std::nullopt;
      }
      selection.named_i32_exprs[load->result.name] = NamedLocalI32Expr{
          .kind = NamedLocalI32Expr::Kind::LocalLoad,
          .operand = *memory,
      };
      selection.candidate_compare_value_names.push_back(load->result.name);
      continue;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32 ||
        (binary->opcode != c4c::backend::bir::BinaryOpcode::Add &&
         binary->opcode != c4c::backend::bir::BinaryOpcode::Sub)) {
      return std::nullopt;
    }
    selection.named_i32_exprs[binary->result.name] = NamedLocalI32Expr{
        .kind = binary->opcode == c4c::backend::bir::BinaryOpcode::Add
                    ? NamedLocalI32Expr::Kind::Add
                    : NamedLocalI32Expr::Kind::Sub,
        .lhs = binary->lhs,
        .rhs = binary->rhs,
    };
    selection.candidate_compare_value_names.push_back(binary->result.name);
  }

  return selection;
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_raw_i32_compared_immediate_branch_plan_if_supported(
    const c4c::backend::bir::Block& entry,
    const std::vector<std::string_view>& compared_value_names) {
  const auto* compare = entry.insts.empty()
                            ? nullptr
                            : std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.back());
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

  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_for_candidates_if_supported(
          compare->lhs, compare->rhs, compared_value_names);
  if (!selected_compare.has_value()) {
    return std::nullopt;
  }

  return PreparedI32ComparedImmediateBranchPlan{
      .compared_value = *selected_compare->named_value,
      .branch_plan =
          PreparedI32ImmediateBranchPlan{
              .compare_immediate = selected_compare->immediate,
              .compare_opcode = compare->opcode,
              .true_label = entry.terminator.true_label,
              .false_label = entry.terminator.false_label,
          },
  };
}

std::optional<PreparedI32ImmediateBranchPlan>
select_raw_i32_immediate_branch_plan_if_supported(
    const c4c::backend::bir::Block& entry,
    std::string_view compared_value_name) {
  const auto raw_branch_plan = select_raw_i32_compared_immediate_branch_plan_if_supported(
      entry, {compared_value_name});
  if (!raw_branch_plan.has_value()) {
    return std::nullopt;
  }
  return raw_branch_plan->branch_plan;
}

std::optional<PreparedI32ImmediateBranchPlan>
select_prepared_i32_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition& prepared_branch_condition,
    std::string_view compared_value_name) {
  if (!context.prepared_names || !prepared_branch_condition.predicate.has_value() ||
      !prepared_branch_condition.compare_type.has_value() ||
      !prepared_branch_condition.lhs.has_value() || !prepared_branch_condition.rhs.has_value() ||
      *prepared_branch_condition.compare_type != c4c::backend::bir::TypeKind::I32 ||
      prepared_branch_condition.condition_value.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto selected_compare =
      select_prepared_i32_named_immediate_compare_for_value_if_supported(
          *prepared_branch_condition.lhs,
          *prepared_branch_condition.rhs,
          compared_value_name);
  if (!selected_compare.has_value()) {
    return std::nullopt;
  }

  const auto target_labels = c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
      *context.prepared_names,
      context.function_control_flow,
      entry_label_id,
      entry,
      prepared_branch_condition);
  if (!target_labels.has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires the authoritative prepared guard-chain handoff through the canonical prepared-module handoff");
  }

  return PreparedI32ImmediateBranchPlan{
      .compare_immediate = selected_compare->immediate,
      .compare_opcode = *prepared_branch_condition.predicate,
      .true_label = std::string(c4c::backend::prepare::prepared_block_label(
          *context.prepared_names, target_labels->true_label)),
      .false_label = std::string(c4c::backend::prepare::prepared_block_label(
          *context.prepared_names, target_labels->false_label)),
  };
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_prepared_i32_compared_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition& prepared_branch_condition,
    const std::vector<std::string_view>& compared_value_names) {
  const auto* matched_prepared_immediate_branch =
      static_cast<const c4c::backend::prepare::PreparedBranchCondition*>(nullptr);
  const auto* matched_prepared_compared_value = static_cast<const c4c::backend::bir::Value*>(nullptr);
  for (const auto value_name : compared_value_names) {
    const auto value_name_id =
        c4c::backend::prepare::resolve_prepared_value_name_id(*context.prepared_names, value_name);
    if (!value_name_id.has_value()) {
      continue;
    }
    const auto* candidate = c4c::backend::prepare::find_prepared_i32_immediate_branch_condition(
        *context.prepared_names, *context.function_control_flow, entry_label_id, *value_name_id);
    if (candidate == nullptr) {
      continue;
    }

    const auto selected_compare =
        select_prepared_i32_named_immediate_compare_for_value_if_supported(
            *candidate->lhs, *candidate->rhs, value_name);
    if (!selected_compare.has_value()) {
      return std::nullopt;
    }
    const auto* candidate_value = selected_compare->named_value;
    if (matched_prepared_immediate_branch != nullptr &&
        candidate_value->name != matched_prepared_compared_value->name) {
      return std::nullopt;
    }
    matched_prepared_immediate_branch = candidate;
    matched_prepared_compared_value = candidate_value;
  }

  if (matched_prepared_immediate_branch == nullptr || matched_prepared_compared_value == nullptr) {
    return std::nullopt;
  }

  const auto prepared_branch_plan = select_prepared_i32_immediate_branch_plan_if_supported(
      context,
      entry_label_id,
      entry,
      prepared_branch_condition,
      matched_prepared_compared_value->name);
  if (!prepared_branch_plan.has_value()) {
    return std::nullopt;
  }

  return PreparedI32ComparedImmediateBranchPlan{
      .compared_value = *matched_prepared_compared_value,
      .branch_plan = *prepared_branch_plan,
  };
}

std::optional<PreparedI32ImmediateBranchPlan>
select_prepared_or_raw_i32_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition* prepared_branch_condition,
    std::string_view compared_value_name) {
  if (prepared_branch_condition != nullptr) {
    return select_prepared_i32_immediate_branch_plan_if_supported(
        context, entry_label_id, entry, *prepared_branch_condition, compared_value_name);
  }

  const auto raw_branch_plan =
      select_raw_i32_immediate_branch_plan_if_supported(entry, compared_value_name);
  if (!raw_branch_plan.has_value()) {
    return std::nullopt;
  }
  return raw_branch_plan;
}

std::optional<PreparedI32ComparedImmediateBranchPlan>
select_prepared_or_raw_i32_compared_immediate_branch_plan_if_supported(
    const PreparedX86FunctionDispatchContext& context,
    c4c::BlockLabelId entry_label_id,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedBranchCondition* prepared_branch_condition,
    const std::vector<std::string_view>& compared_value_names) {
  if (prepared_branch_condition != nullptr) {
    return select_prepared_i32_compared_immediate_branch_plan_if_supported(
        context, entry_label_id, entry, *prepared_branch_condition, compared_value_names);
  }
  return select_raw_i32_compared_immediate_branch_plan_if_supported(entry, compared_value_names);
}

std::string render_prepared_i32_compare_and_branch(c4c::backend::bir::BinaryOpcode compare_opcode,
                                                   std::int64_t compare_immediate,
                                                   std::string_view function_name,
                                                   std::string_view false_label) {
  std::string rendered =
      render_prepared_i32_eax_immediate_compare_setup(compare_immediate);
  rendered += "    ";
  rendered += compare_opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je";
  rendered += " .L";
  rendered += function_name;
  rendered += "_";
  rendered += false_label;
  rendered += "\n";
  return rendered;
}

std::optional<c4c::backend::bir::BinaryOpcode> invert_prepared_compare_opcode_if_supported(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
      return c4c::backend::bir::BinaryOpcode::Ne;
    case c4c::backend::bir::BinaryOpcode::Ne:
      return c4c::backend::bir::BinaryOpcode::Eq;
    case c4c::backend::bir::BinaryOpcode::Sgt:
      return c4c::backend::bir::BinaryOpcode::Sle;
    case c4c::backend::bir::BinaryOpcode::Sge:
      return c4c::backend::bir::BinaryOpcode::Slt;
    case c4c::backend::bir::BinaryOpcode::Slt:
      return c4c::backend::bir::BinaryOpcode::Sge;
    case c4c::backend::bir::BinaryOpcode::Sle:
      return c4c::backend::bir::BinaryOpcode::Sgt;
    default:
      return std::nullopt;
  }
}

std::optional<std::string> render_prepared_i32_binary_inst_if_supported(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor &&
      binary.operand_type == c4c::backend::bir::TypeKind::I1 &&
      binary.result.type == c4c::backend::bir::TypeKind::I1 &&
      binary.result.kind == c4c::backend::bir::Value::Kind::Named &&
      current_materialized_compare->has_value()) {
    const auto match_inverted_compare_operand =
        [&](const c4c::backend::bir::Value& lhs,
            const c4c::backend::bir::Value& rhs) -> bool {
          return lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                 lhs.type == c4c::backend::bir::TypeKind::I1 &&
                 lhs.name == (*current_materialized_compare)->i1_name &&
                 rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                 rhs.type == c4c::backend::bir::TypeKind::I1 &&
                 rhs.immediate == 1;
        };
    if (match_inverted_compare_operand(binary.lhs, binary.rhs) ||
        match_inverted_compare_operand(binary.rhs, binary.lhs)) {
      const auto inverted_opcode =
          invert_prepared_compare_opcode_if_supported((*current_materialized_compare)->opcode);
      if (!inverted_opcode.has_value()) {
        return std::nullopt;
      }
      (*current_materialized_compare)->i1_name = binary.result.name;
      (*current_materialized_compare)->i32_name = std::nullopt;
      (*current_materialized_compare)->opcode = *inverted_opcode;
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      return std::string{};
    }
  }

  if (binary.operand_type == c4c::backend::bir::TypeKind::I8 &&
      (binary.result.type == c4c::backend::bir::TypeKind::I1 ||
       binary.result.type == c4c::backend::bir::TypeKind::I32) &&
      binary.result.kind == c4c::backend::bir::Value::Kind::Named &&
      prepared_i32_setcc_opcode_if_supported(binary.opcode) != nullptr) {
    const auto compare_context = render_prepared_guard_false_branch_compare(
        binary,
        *current_materialized_compare,
        *current_i32_name,
        prepared_names,
        function_locations);
    if (!compare_context.has_value()) {
      return std::nullopt;
    }
    if (binary.result.type == c4c::backend::bir::TypeKind::I32) {
      if (local_layout == nullptr) {
        return std::nullopt;
      }
      const auto synced_home = render_prepared_named_i32_home_sync_if_supported(
          *local_layout, binary.result.name, prepared_names, function_locations);
      if (!synced_home.has_value()) {
        return std::nullopt;
      }
      *current_materialized_compare = MaterializedI32Compare{
          .i1_name = binary.result.name,
          .i32_name = binary.result.name,
          .opcode = binary.opcode,
          .compare_setup = compare_context->first,
      };
      *current_i32_name = binary.result.name;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      return compare_context->first + "    " +
             std::string(prepared_i32_setcc_opcode_if_supported(binary.opcode)) +
             " al\n    movzx eax, al\n" + *synced_home;
    }
    *current_materialized_compare = MaterializedI32Compare{
        .i1_name = binary.result.name,
        .i32_name = std::nullopt,
        .opcode = binary.opcode,
        .compare_setup = compare_context->first,
    };
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return std::string{};
  }

  const auto select_i32_source =
      [&](const c4c::backend::bir::Value& value) -> std::optional<PreparedNamedI32Source> {
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
        if (block != nullptr) {
          return select_prepared_named_i32_block_source_if_supported(
              local_layout,
              block,
              instruction_index,
              function_addressing,
              block_label_id,
              value.name,
              *current_i32_name,
              *previous_i32_name,
              prepared_names,
              function_locations);
        }
        return select_prepared_named_i32_source_if_supported(
            value.name,
            *current_i32_name,
            *previous_i32_name,
            prepared_names,
            function_locations);
      };

  const auto render_binary_in_eax =
      [&](const c4c::backend::bir::Value& lhs,
          const c4c::backend::bir::Value& rhs) -> std::optional<std::pair<std::string, std::optional<std::string_view>>> {
      const auto lhs_source = select_i32_source(lhs);
      const auto rhs_source = select_i32_source(rhs);
      if (!lhs_source.has_value() || !rhs_source.has_value()) {
        return std::nullopt;
      }

      PreparedNamedI32Source effective_lhs = *lhs_source;
      PreparedNamedI32Source effective_rhs = *rhs_source;
      std::optional<std::string_view> rhs_name =
          rhs.kind == c4c::backend::bir::Value::Kind::Named
              ? std::optional<std::string_view>(rhs.name)
              : std::nullopt;
      std::optional<std::string_view> ecx_value_name;
      std::string rendered_binary;
      if (effective_rhs.register_name.has_value() && *effective_rhs.register_name == "eax" &&
          effective_lhs.register_name.has_value() && *effective_lhs.register_name == "ecx" &&
          lhs.kind == c4c::backend::bir::Value::Kind::Named) {
        if (const auto authoritative_lhs = block != nullptr
                                               ? select_prepared_named_i32_source_if_supported(
                                                     lhs.name,
                                                     std::nullopt,
                                                     std::nullopt,
                                                     prepared_names,
                                                     function_locations)
                                               : select_prepared_named_i32_source_if_supported(
                                                     lhs.name,
                                                     std::nullopt,
                                                     std::nullopt,
                                                     prepared_names,
                                                     function_locations);
            authoritative_lhs.has_value() &&
            (!authoritative_lhs->register_name.has_value() ||
             *authoritative_lhs->register_name != "eax")) {
          effective_lhs = *authoritative_lhs;
        }
      }
      if (effective_rhs.register_name.has_value() && *effective_rhs.register_name == "eax" &&
          (!effective_lhs.register_name.has_value() || *effective_lhs.register_name != "eax")) {
        rendered_binary += "    mov ecx, eax\n";
        effective_rhs.register_name = std::string("ecx");
        ecx_value_name = rhs_name;
      }
      if (!append_prepared_named_i32_move_into_register_if_supported(
              &rendered_binary, "eax", effective_lhs)) {
        return std::nullopt;
      }
      const auto rhs_operand = render_prepared_i32_operand_from_source_if_supported(effective_rhs);
      if (!rhs_operand.has_value()) {
        return std::nullopt;
        }

        switch (binary.opcode) {
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
            if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                rhs.type != c4c::backend::bir::TypeKind::I32) {
              return std::nullopt;
            }
            rendered_binary += "    shl eax, " +
                               std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
            break;
          case c4c::backend::bir::BinaryOpcode::LShr:
            if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                rhs.type != c4c::backend::bir::TypeKind::I32) {
              return std::nullopt;
            }
            rendered_binary += "    shr eax, " +
                               std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
            break;
          case c4c::backend::bir::BinaryOpcode::AShr:
            if (rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
                rhs.type != c4c::backend::bir::TypeKind::I32) {
              return std::nullopt;
            }
            rendered_binary += "    sar eax, " +
                               std::to_string(static_cast<std::int32_t>(rhs.immediate)) + "\n";
            break;
          default:
            return std::nullopt;
        }
        return std::pair<std::string, std::optional<std::string_view>>{
            std::move(rendered_binary),
            ecx_value_name,
        };
      };

  if (binary.operand_type == c4c::backend::bir::TypeKind::I32 &&
      binary.result.type == c4c::backend::bir::TypeKind::I32) {
    auto rendered_binary = render_binary_in_eax(binary.lhs, binary.rhs);
    if (!rendered_binary.has_value() &&
        prepared_i32_binary_opcode_is_commutative(binary.opcode)) {
      rendered_binary = render_binary_in_eax(binary.rhs, binary.lhs);
    }
    if (rendered_binary.has_value()) {
      if (local_layout == nullptr) {
        return std::nullopt;
      }
      const auto synced_home = render_prepared_named_i32_home_sync_if_supported(
          *local_layout, binary.result.name, prepared_names, function_locations);
      if (!synced_home.has_value()) {
        return std::nullopt;
      }
      auto rendered = std::move(rendered_binary->first);
      rendered += *synced_home;
      *current_materialized_compare = std::nullopt;
      *current_i32_name = binary.result.name;
      *previous_i32_name = rendered_binary->second;
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      return rendered;
    }
  }

  if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
      (binary.result.type != c4c::backend::bir::TypeKind::I1 &&
       binary.result.type != c4c::backend::bir::TypeKind::I32)) {
    return std::nullopt;
  }

  const auto setcc_opcode = prepared_i32_setcc_opcode_if_supported(binary.opcode);
  if (setcc_opcode == nullptr) {
    return std::nullopt;
  }

  const auto render_compare_setup =
      [&](const c4c::backend::bir::Value& lhs,
          const c4c::backend::bir::Value& rhs) -> std::optional<std::string> {
        const auto lhs_source = select_i32_source(lhs);
        const auto rhs_source = select_i32_source(rhs);
        if (!lhs_source.has_value() || !rhs_source.has_value()) {
          return std::nullopt;
        }

        PreparedNamedI32Source effective_rhs = *rhs_source;
        std::string compare_setup;
        if (effective_rhs.register_name.has_value() && *effective_rhs.register_name == "eax" &&
            (!lhs_source->register_name.has_value() || *lhs_source->register_name != "eax")) {
          compare_setup += "    mov ecx, eax\n";
          effective_rhs.register_name = std::string("ecx");
        }
        if (!append_prepared_named_i32_move_into_register_if_supported(
                &compare_setup, "eax", *lhs_source)) {
          return std::nullopt;
        }
        const auto rhs_operand = render_prepared_i32_operand_from_source_if_supported(effective_rhs);
        if (!rhs_operand.has_value()) {
          return std::nullopt;
        }
        compare_setup += "    cmp eax, " + *rhs_operand + "\n";
        return compare_setup;
      };

  const auto compare_setup = render_compare_setup(binary.lhs, binary.rhs);
  if (!compare_setup.has_value()) {
    return std::nullopt;
  }

  if (binary.result.type == c4c::backend::bir::TypeKind::I32) {
    if (local_layout == nullptr) {
      return std::nullopt;
    }
    const auto synced_home = render_prepared_named_i32_home_sync_if_supported(
        *local_layout, binary.result.name, prepared_names, function_locations);
    if (!synced_home.has_value()) {
      return std::nullopt;
    }
    *current_materialized_compare = MaterializedI32Compare{
        .i1_name = binary.result.name,
        .i32_name = binary.result.name,
        .opcode = binary.opcode,
        .compare_setup = *compare_setup,
    };
    *current_i32_name = binary.result.name;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return *compare_setup + "    " + setcc_opcode + " al\n    movzx eax, al\n" + *synced_home;
  }

  *current_materialized_compare = MaterializedI32Compare{
      .i1_name = binary.result.name,
      .opcode = binary.opcode,
      .compare_setup = *compare_setup,
  };
  if (const auto selected_compare =
          select_prepared_i32_named_immediate_compare_if_supported(binary.lhs, binary.rhs);
      selected_compare.has_value() && selected_compare->named_value == &binary.lhs) {
    *current_i32_name = binary.lhs.name;
  } else {
    *current_i32_name = std::nullopt;
  }
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return std::string{};
}

std::optional<std::string> render_prepared_ptr_binary_inst_if_supported(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedModuleLocalSlotLayout* local_layout,
    const c4c::backend::bir::Block* block,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::unordered_map<std::string_view, std::string_view>& i64_i32_aliases,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr || binary.result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary.result.type != c4c::backend::bir::TypeKind::Ptr ||
      (binary.opcode != c4c::backend::bir::BinaryOpcode::Add &&
       binary.opcode != c4c::backend::bir::BinaryOpcode::Sub)) {
    return std::nullopt;
  }
  if (local_layout == nullptr) {
    return std::nullopt;
  }

  const auto select_i32_source =
      [&](std::string_view value_name) -> std::optional<PreparedNamedI32Source> {
        if (block != nullptr) {
          return select_prepared_named_i32_block_source_if_supported(
              local_layout,
              block,
              instruction_index,
              function_addressing,
              block_label_id,
              value_name,
              *current_i32_name,
              *previous_i32_name,
              prepared_names,
              function_locations);
        }
        return select_prepared_named_i32_source_if_supported(
            value_name,
            *current_i32_name,
            *previous_i32_name,
            prepared_names,
            function_locations);
      };

  const auto render_signed_offset_into_register =
      [&](const c4c::backend::bir::Value& value,
          std::string_view destination_register) -> std::optional<std::string> {
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate &&
            (value.type == c4c::backend::bir::TypeKind::I32 ||
             value.type == c4c::backend::bir::TypeKind::I64)) {
          return "    mov " + std::string(destination_register) + ", " +
                 std::to_string(value.type == c4c::backend::bir::TypeKind::I32
                                    ? static_cast<std::int32_t>(value.immediate)
                                    : value.immediate) +
                 "\n";
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }

        std::string_view i32_value_name;
        if (value.type == c4c::backend::bir::TypeKind::I32) {
          i32_value_name = value.name;
        } else if (value.type == c4c::backend::bir::TypeKind::I64) {
          const auto alias_it = i64_i32_aliases.find(value.name);
          if (alias_it == i64_i32_aliases.end()) {
            return std::nullopt;
          }
          i32_value_name = alias_it->second;
        } else {
          return std::nullopt;
        }

        const auto source = select_i32_source(i32_value_name);
        if (!source.has_value()) {
          return std::nullopt;
        }
        if (source->immediate_i32.has_value()) {
          return "    mov " + std::string(destination_register) + ", " +
                 std::to_string(static_cast<std::int32_t>(*source->immediate_i32)) + "\n";
        }
        if (source->register_name.has_value()) {
          return "    movsxd " + std::string(destination_register) + ", " +
                 *source->register_name + "\n";
        }
        if (source->stack_operand.has_value()) {
          return "    movsxd " + std::string(destination_register) + ", " +
                 *source->stack_operand + "\n";
        }
        return std::nullopt;
      };

  const auto is_supported_offset_value =
      [&](const c4c::backend::bir::Value& value) -> bool {
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return value.type == c4c::backend::bir::TypeKind::I32 ||
                 value.type == c4c::backend::bir::TypeKind::I64;
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return false;
        }
        return value.type == c4c::backend::bir::TypeKind::I32 ||
               (value.type == c4c::backend::bir::TypeKind::I64 &&
                i64_i32_aliases.find(value.name) != i64_i32_aliases.end());
      };

  const auto is_named_ptr =
      [](const c4c::backend::bir::Value& value) {
        return value.kind == c4c::backend::bir::Value::Kind::Named &&
               value.type == c4c::backend::bir::TypeKind::Ptr;
      };

  const c4c::backend::bir::Value* base_value = nullptr;
  const c4c::backend::bir::Value* offset_value = nullptr;
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    if (is_named_ptr(binary.lhs) && is_supported_offset_value(binary.rhs)) {
      base_value = &binary.lhs;
      offset_value = &binary.rhs;
    } else if (is_named_ptr(binary.rhs) && is_supported_offset_value(binary.lhs)) {
      base_value = &binary.rhs;
      offset_value = &binary.lhs;
    }
  } else if (is_named_ptr(binary.lhs) && is_supported_offset_value(binary.rhs)) {
    base_value = &binary.lhs;
    offset_value = &binary.rhs;
  }
  if (base_value == nullptr || offset_value == nullptr) {
    return std::nullopt;
  }

  const auto offset_register =
      choose_prepared_pointer_scratch_register_if_supported(function_locations);
  if (!offset_register.has_value()) {
    return std::nullopt;
  }
  const auto rendered_offset =
      render_signed_offset_into_register(*offset_value, *offset_register);
  if (!rendered_offset.has_value()) {
    return std::nullopt;
  }
  const auto rendered_base = render_prepared_named_ptr_into_register_if_supported(
      base_value->name,
      "rax",
      prepared_names,
      function_locations,
      *current_ptr_name);
  if (!rendered_base.has_value()) {
    return std::nullopt;
  }

  std::string rendered = *rendered_offset + *rendered_base;
  rendered += "    ";
  rendered += binary.opcode == c4c::backend::bir::BinaryOpcode::Add ? "add " : "sub ";
  rendered += "rax, " + *offset_register + "\n";
  if (const auto home_sync = render_prepared_named_ptr_home_sync_if_supported(
          *local_layout, binary.result.name, prepared_names, function_locations);
      home_sync.has_value()) {
    rendered += *home_sync;
  } else {
    return std::nullopt;
  }

  *current_materialized_compare = std::nullopt;
  *current_i32_name = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name =
      prepared_current_ptr_carrier_name(binary.result.name, prepared_names, function_locations);
  return rendered;
}

std::optional<std::string> render_prepared_block_return_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    const c4c::backend::bir::Value& returned,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& previous_i32_name,
    std::string_view function_callee_saved_restore_asm) {
  if (block_context.local_layout == nullptr || block_context.function_context == nullptr) {
    return std::nullopt;
  }
  const auto resolve_named_operand =
      [&](std::string_view value_name) -> std::optional<std::string> {
        return render_prepared_named_i32_operand_if_supported(
            value_name,
            current_i32_name,
            previous_i32_name,
            block_context.function_context->prepared_names,
            block_context.function_context->function_locations);
      };
  const auto selected_return = select_prepared_i32_value_if_supported(
      returned, current_i32_name, resolve_named_operand);
  if (!selected_return.has_value()) {
    return std::nullopt;
  }
  if (selected_return->immediate.has_value()) {
    body += "    mov eax, " + std::to_string(*selected_return->immediate) + "\n";
    if (block_context.local_layout->frame_size != 0) {
      body += "    add rsp, " + std::to_string(block_context.local_layout->frame_size) + "\n";
    }
    body += function_callee_saved_restore_asm;
    body += "    ret\n";
    return body;
  }
  if (!selected_return->in_eax) {
    if (!selected_return->operand.has_value()) {
      return std::nullopt;
    }
    body += "    mov eax, " + *selected_return->operand + "\n";
  }
  if (block_context.local_layout->frame_size != 0) {
    body += "    add rsp, " + std::to_string(block_context.local_layout->frame_size) + "\n";
  }
  body += function_callee_saved_restore_asm;
  body += "    ret\n";
  return body;
}

std::optional<std::string> render_prepared_void_block_return_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::string_view function_callee_saved_restore_asm) {
  if (block_context.local_layout == nullptr) {
    return std::nullopt;
  }
  if (block_context.local_layout->frame_size != 0) {
    body += "    add rsp, " + std::to_string(block_context.local_layout->frame_size) + "\n";
  }
  body += function_callee_saved_restore_asm;
  body += "    ret\n";
  return body;
}

std::optional<std::string> render_prepared_cast_inst_if_supported(
    const c4c::backend::bir::CastInst& cast,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare,
    std::unordered_map<std::string_view, std::string_view>* i64_i32_aliases) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr ||
      current_i8_name == nullptr || current_ptr_name == nullptr ||
      current_materialized_compare == nullptr || i64_i32_aliases == nullptr) {
    return std::nullopt;
  }

  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I1 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32 &&
      cast.operand.kind == c4c::backend::bir::Value::Kind::Named &&
      current_materialized_compare->has_value() &&
      (*current_materialized_compare)->i1_name == cast.operand.name) {
    (*current_materialized_compare)->i32_name = cast.result.name;
    *current_i32_name = cast.result.name;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return std::string{};
  }

  if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt &&
      cast.operand.type == c4c::backend::bir::TypeKind::I32 &&
      cast.result.type == c4c::backend::bir::TypeKind::I32 &&
      cast.operand.kind == c4c::backend::bir::Value::Kind::Named &&
      cast.result.kind == c4c::backend::bir::Value::Kind::Named &&
      ((current_i32_name->has_value() && **current_i32_name == cast.operand.name) ||
       (current_materialized_compare->has_value() &&
        (*current_materialized_compare)->i32_name.has_value() &&
        *(*current_materialized_compare)->i32_name == cast.operand.name))) {
    if (current_materialized_compare->has_value() &&
        (*current_materialized_compare)->i32_name.has_value() &&
        *(*current_materialized_compare)->i32_name == cast.operand.name) {
      (*current_materialized_compare)->i32_name = cast.result.name;
    }
    *current_i32_name = cast.result.name;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return std::string{};
  }

  if ((cast.opcode == c4c::backend::bir::CastOpcode::SExt ||
       cast.opcode == c4c::backend::bir::CastOpcode::ZExt) &&
      cast.operand.type == c4c::backend::bir::TypeKind::I32 &&
      cast.result.type == c4c::backend::bir::TypeKind::I64 &&
      cast.operand.kind == c4c::backend::bir::Value::Kind::Named &&
      cast.result.kind == c4c::backend::bir::Value::Kind::Named) {
    if (prepared_names != nullptr && function_locations != nullptr) {
      const auto source = select_prepared_named_i32_source_if_supported(
          cast.operand.name,
          *current_i32_name,
          *previous_i32_name,
          prepared_names,
          function_locations);
      const auto* result_home = c4c::backend::prepare::find_prepared_value_home(
          *prepared_names, *function_locations, cast.result.name);
      if (source.has_value() && result_home != nullptr) {
        const auto render_into_wide_register =
            [&](std::string_view destination_register) -> std::optional<std::string> {
              if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt) {
                const auto narrow_destination = narrow_i32_register(std::string(destination_register));
                if (!narrow_destination.has_value()) {
                  return std::nullopt;
                }
                if (source->immediate_i32.has_value()) {
                  return "    mov " + *narrow_destination + ", " +
                         std::to_string(static_cast<std::int32_t>(*source->immediate_i32)) + "\n";
                }
                if (source->register_name.has_value()) {
                  if (*source->register_name == *narrow_destination) {
                    return std::string{};
                  }
                  return "    mov " + *narrow_destination + ", " + *source->register_name + "\n";
                }
                if (source->stack_operand.has_value()) {
                  return "    mov " + *narrow_destination + ", " + *source->stack_operand + "\n";
                }
                return std::nullopt;
              }
              if (source->immediate_i32.has_value()) {
                return "    mov " + std::string(destination_register) + ", " +
                       std::to_string(static_cast<std::int64_t>(
                           static_cast<std::int32_t>(*source->immediate_i32))) +
                       "\n";
              }
              if (source->register_name.has_value()) {
                return "    movsxd " + std::string(destination_register) + ", " +
                       *source->register_name + "\n";
              }
              if (source->stack_operand.has_value()) {
                return "    movsxd " + std::string(destination_register) + ", " +
                       *source->stack_operand + "\n";
              }
              return std::nullopt;
            };
        if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
            result_home->register_name.has_value()) {
          const auto rendered = render_into_wide_register(*result_home->register_name);
          if (!rendered.has_value()) {
            return std::nullopt;
          }
          (*i64_i32_aliases)[cast.result.name] = cast.operand.name;
          *current_materialized_compare = std::nullopt;
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_i8_name = std::nullopt;
          *current_ptr_name = std::nullopt;
          return *rendered;
        }
        if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
            result_home->offset_bytes.has_value()) {
          std::string rendered;
          if (cast.opcode == c4c::backend::bir::CastOpcode::ZExt) {
            if (source->immediate_i32.has_value()) {
              rendered += "    mov eax, " +
                          std::to_string(static_cast<std::int32_t>(*source->immediate_i32)) + "\n";
            } else if (source->register_name.has_value()) {
              if (*source->register_name != "eax") {
                rendered += "    mov eax, " + *source->register_name + "\n";
              }
            } else if (source->stack_operand.has_value()) {
              rendered += "    mov eax, " + *source->stack_operand + "\n";
            } else {
              return std::nullopt;
            }
          } else {
            if (source->immediate_i32.has_value()) {
              rendered += "    mov rax, " +
                          std::to_string(static_cast<std::int64_t>(
                              static_cast<std::int32_t>(*source->immediate_i32))) +
                          "\n";
            } else if (source->register_name.has_value()) {
              rendered += "    movsxd rax, " + *source->register_name + "\n";
            } else if (source->stack_operand.has_value()) {
              rendered += "    movsxd rax, " + *source->stack_operand + "\n";
            } else {
              return std::nullopt;
            }
          }
          rendered += "    mov " +
                      render_prepared_stack_memory_operand(*result_home->offset_bytes, "QWORD") +
                      ", rax\n";
          (*i64_i32_aliases)[cast.result.name] = cast.operand.name;
          *current_materialized_compare = std::nullopt;
          *current_i32_name = std::nullopt;
          *previous_i32_name = std::nullopt;
          *current_i8_name = std::nullopt;
          *current_ptr_name = std::nullopt;
          return rendered;
        }
      }
    }
    (*i64_i32_aliases)[cast.result.name] = cast.operand.name;
    *current_materialized_compare = std::nullopt;
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    return std::string{};
  }

  if (cast.opcode != c4c::backend::bir::CastOpcode::SExt ||
      cast.operand.type != c4c::backend::bir::TypeKind::I8 ||
      cast.result.type != c4c::backend::bir::TypeKind::I32 ||
      cast.operand.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const bool operand_is_current_i8 =
      current_i8_name->has_value() && **current_i8_name == cast.operand.name;
  *current_materialized_compare = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_i32_name = cast.result.name;
  *previous_i32_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  if (operand_is_current_i8) {
    return std::string{};
  }
  if (prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(
          *prepared_names, *function_locations, cast.operand.name);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto narrowed_register = narrow_i8_register(*home->register_name);
    if (!narrowed_register.has_value()) {
      return std::nullopt;
    }
    return "    movsx eax, " + *narrowed_register + "\n";
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    return "    movsx eax, " +
           render_prepared_stack_memory_operand(*home->offset_bytes, "BYTE") + "\n";
  }
  if (home->kind ==
          c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
      home->immediate_i32.has_value()) {
    return "    mov eax, " +
           std::to_string(static_cast<std::int32_t>(
               static_cast<std::int8_t>(*home->immediate_i32))) +
           "\n";
  }
  return std::nullopt;
}

std::optional<std::string> render_prepared_select_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t inst_index,
    const c4c::backend::bir::SelectInst& select,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const std::unordered_map<std::string_view, std::string_view>& i64_i32_aliases,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || block_context.block == nullptr ||
      current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  if (block_context.local_layout == nullptr) {
    return std::nullopt;
  }
  if (prepared_names != nullptr && block_context.function_context->function_control_flow != nullptr &&
      select.result.kind == c4c::backend::bir::Value::Kind::Named) {
    const auto* join_transfer = c4c::backend::prepare::find_prepared_join_transfer(
        *prepared_names,
        *block_context.function_context->function_control_flow,
        block_context.block_label_id,
        select.result.name);
    if (join_transfer != nullptr &&
        c4c::backend::prepare::effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
            c4c::backend::prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
      *current_materialized_compare = std::nullopt;
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      return std::string{};
    }
  }
  if (select.predicate != c4c::backend::bir::BinaryOpcode::Eq ||
      select.compare_type != c4c::backend::bir::TypeKind::I64 ||
      select.result.type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (select.lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      select.rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      select.rhs.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }

  const auto alias_it = i64_i32_aliases.find(select.lhs.name);
  if (alias_it == i64_i32_aliases.end()) {
    return std::nullopt;
  }
  const auto select_i32_source =
      [&](const c4c::backend::bir::Value& value) -> std::optional<PreparedNamedI32Source> {
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
        if (block_context.block != nullptr) {
          return select_prepared_named_i32_block_source_if_supported(
              block_context.local_layout,
              block_context.block,
              inst_index,
              block_context.function_context->function_addressing,
              block_context.block_label_id,
              value.name,
              *current_i32_name,
              *previous_i32_name,
              prepared_names,
              function_locations);
        }
        return select_prepared_named_i32_source_if_supported(
            value.name,
            *current_i32_name,
            *previous_i32_name,
            prepared_names,
            function_locations);
      };
  const auto compared_source = select_i32_source(
      c4c::backend::bir::Value{
          .kind = c4c::backend::bir::Value::Kind::Named,
          .type = c4c::backend::bir::TypeKind::I32,
          .name = std::string(alias_it->second),
      });
  const auto true_source = select_i32_source(select.true_value);
  const auto false_source = select_i32_source(select.false_value);
  if (!compared_source.has_value() || !true_source.has_value() || !false_source.has_value()) {
    return std::nullopt;
  }

  std::string body;
  const auto true_operand = render_prepared_i32_operand_from_source_if_supported(*true_source);
  if (!true_operand.has_value()) {
    return std::nullopt;
  }
  const auto done_label = ".L" + block_context.function_context->function->name + "_" +
                          block_context.block->label + "_select_" +
                          std::to_string(inst_index) + "_done";
  if (compared_source->register_name.has_value() && *compared_source->register_name == "eax") {
    body += "    cmp eax, " + std::to_string(static_cast<std::int32_t>(select.rhs.immediate)) + "\n";
    const auto false_label = ".L" + block_context.function_context->function->name + "_" +
                             block_context.block->label + "_select_" +
                             std::to_string(inst_index) + "_false";
    body += "    jne " + false_label + "\n";
    body += "    mov eax, " + *true_operand + "\n";
    body += "    jmp " + done_label + "\n";
    body += false_label + ":\n";
    if (!append_prepared_named_i32_move_into_register_if_supported(&body, "eax", *false_source)) {
      return std::nullopt;
    }
  } else {
    if (!append_prepared_named_i32_move_into_register_if_supported(&body, "eax", *false_source)) {
      return std::nullopt;
    }
    if (compared_source->immediate_i32.has_value()) {
      if (static_cast<std::int32_t>(*compared_source->immediate_i32) ==
          static_cast<std::int32_t>(select.rhs.immediate)) {
        body += "    mov eax, " + *true_operand + "\n";
      }
    } else {
      const auto compared_operand =
          render_prepared_i32_operand_from_source_if_supported(*compared_source);
      if (!compared_operand.has_value()) {
        return std::nullopt;
      }
      body += "    cmp " + *compared_operand + ", " +
              std::to_string(static_cast<std::int32_t>(select.rhs.immediate)) + "\n";
      body += "    jne " + done_label + "\n";
      body += "    mov eax, " + *true_operand + "\n";
    }
  }
  body += done_label + ":\n";
  const auto synced_home = render_prepared_named_i32_home_sync_if_supported(
      *block_context.local_layout, select.result.name, prepared_names, function_locations);
  if (!synced_home.has_value()) {
    return std::nullopt;
  }
  body += *synced_home;
  *current_materialized_compare = std::nullopt;
  *current_i32_name = select.result.name;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  return body;
}


std::optional<std::string> render_prepared_block_plain_branch_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::size_t compare_index,
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>& continuation,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  const auto& block = *block_context.block;
  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
      compare_index != block.insts.size()) {
    return std::nullopt;
  }
  const auto* target = resolve_authoritative_prepared_branch_target(
      function_context.function_control_flow,
      function_context.prepared_names,
      block_context.block_label_id,
      block,
      find_block);
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

std::optional<std::size_t> select_prepared_block_terminator_compare_index_if_supported(
    const c4c::backend::bir::Block& block,
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation) {
  const auto is_supported_guard_compare =
      [](const c4c::backend::bir::BinaryInst& compare) {
        const bool supported_opcode =
            compare.opcode == c4c::backend::bir::BinaryOpcode::Eq ||
            compare.opcode == c4c::backend::bir::BinaryOpcode::Ne ||
            compare.opcode == c4c::backend::bir::BinaryOpcode::Sgt ||
            compare.opcode == c4c::backend::bir::BinaryOpcode::Sge ||
            compare.opcode == c4c::backend::bir::BinaryOpcode::Slt ||
            compare.opcode == c4c::backend::bir::BinaryOpcode::Sle;
        const bool supported_operand_type =
            compare.operand_type == c4c::backend::bir::TypeKind::I32 ||
            compare.operand_type == c4c::backend::bir::TypeKind::I8;
        const bool supported_result_type =
            compare.result.type == c4c::backend::bir::TypeKind::I1 ||
            compare.result.type == c4c::backend::bir::TypeKind::I32;
        return supported_opcode && supported_operand_type && supported_result_type;
      };
  if (block.terminator.kind == c4c::backend::bir::TerminatorKind::CondBranch) {
    if (block.insts.empty()) {
      return std::nullopt;
    }
    return block.insts.size() - 1;
  }

  if (continuation.has_value() &&
      block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch &&
      !block.insts.empty()) {
    const auto* branch_compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
    if (branch_compare != nullptr && is_supported_guard_compare(*branch_compare)) {
      return block.insts.size() - 1;
    }
  }

  return block.insts.size();
}

struct PreparedBlockCondBranchRenderSelection {
  CompareDrivenBranchRenderPlan render_plan;
  const c4c::backend::prepare::PreparedNameTables* prepared_names = nullptr;
};

struct PreparedBlockBranchRenderSelection {
  std::optional<CompareDrivenBranchRenderPlan> compare_join_render_plan;
  const c4c::backend::prepare::PreparedNameTables* prepared_names = nullptr;
};

std::optional<std::pair<std::string, std::string>>
render_prepared_guard_false_branch_compare_with_current_i8_if_supported(
    const c4c::backend::bir::BinaryInst& compare,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations) {
  if (current_i8_name.has_value() && compare.operand_type == c4c::backend::bir::TypeKind::I8) {
    const auto current_i8_compare =
        [&]() -> std::optional<std::pair<bool, std::int64_t>> {
      if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.lhs.name == *current_i8_name &&
          compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<bool, std::int64_t>{true, compare.rhs.immediate};
      }
      if (compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
          compare.rhs.type == c4c::backend::bir::TypeKind::I8 &&
          compare.rhs.name == *current_i8_name &&
          compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          compare.lhs.type == c4c::backend::bir::TypeKind::I8) {
        return std::pair<bool, std::int64_t>{false, compare.lhs.immediate};
      }
      return std::nullopt;
    }();
    if (current_i8_compare.has_value()) {
      const auto branch_opcode_for_current_immediate =
          [&](bool current_is_lhs) -> const char* {
        switch (compare.opcode) {
          case c4c::backend::bir::BinaryOpcode::Eq:
            return "jne";
          case c4c::backend::bir::BinaryOpcode::Ne:
            return "je";
          case c4c::backend::bir::BinaryOpcode::Sgt:
            return current_is_lhs ? "jle" : "jge";
          case c4c::backend::bir::BinaryOpcode::Sge:
            return current_is_lhs ? "jl" : "jg";
          case c4c::backend::bir::BinaryOpcode::Slt:
            return current_is_lhs ? "jge" : "jle";
          case c4c::backend::bir::BinaryOpcode::Sle:
            return current_is_lhs ? "jg" : "jl";
          default:
            return nullptr;
        }
      };
      const char* branch_opcode =
          branch_opcode_for_current_immediate(current_i8_compare->first);
      if (branch_opcode != nullptr) {
        return std::pair<std::string, std::string>{
            "    cmp al, " +
                std::to_string(static_cast<std::int32_t>(
                    static_cast<std::int8_t>(current_i8_compare->second))) +
                "\n",
            branch_opcode,
        };
      }
    }
  }

  return c4c::backend::x86::render_prepared_guard_false_branch_compare(
      compare,
      current_materialized_compare,
      current_i32_name,
      prepared_names,
      function_locations);
}

std::optional<PreparedBlockCondBranchRenderSelection>
select_prepared_short_circuit_cond_branch_render_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.function == nullptr) {
    return std::nullopt;
  }

  const auto& function = *function_context.function;
  const auto& block = *block_context.block;
  const auto* function_control_flow = function_context.function_control_flow;
  const auto* prepared_names = function_context.prepared_names;
  if (function_control_flow == nullptr || prepared_names == nullptr) {
    return std::nullopt;
  }

  const auto resolved_block_label_id =
      c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, block.label);
  if (!resolved_block_label_id.has_value()) {
    return std::nullopt;
  }
  const auto join_context =
      c4c::backend::x86::find_prepared_short_circuit_join_context_if_supported(
          *prepared_names, *function_control_flow, function, *resolved_block_label_id);
  if (!join_context.has_value()) {
    return std::nullopt;
  }
  if (c4c::backend::prepare::find_prepared_short_circuit_continuation_targets(
          *prepared_names, *function_control_flow, function, *resolved_block_label_id)
          .has_value()) {
    const auto* source_branch_condition =
        c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                              *resolved_block_label_id);
    if (source_branch_condition == nullptr) {
      return std::nullopt;
    }
    const auto direct_targets =
        c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(*prepared_names,
                                                                             function_control_flow,
                                                                             *resolved_block_label_id,
                                                                             block,
                                                                             *source_branch_condition);
    if (!direct_targets.has_value()) {
      return std::nullopt;
    }
    const auto rhs_entry_label = join_context->classified_incoming.short_circuit_on_compare_true
                                     ? direct_targets->false_label
                                     : direct_targets->true_label;
    const auto* rhs_entry_block =
        find_block(c4c::backend::prepare::prepared_block_label(*prepared_names, rhs_entry_label));
    if (rhs_entry_block == nullptr) {
      return std::nullopt;
    }
    const auto rhs_compare_index = select_prepared_block_terminator_compare_index_if_supported(
        *rhs_entry_block,
        c4c::backend::prepare::PreparedShortCircuitContinuationLabels{
            .incoming_label = rhs_entry_label,
            .true_label = join_context->continuation_true_label,
            .false_label = join_context->continuation_false_label,
        });
    if ((!rhs_compare_index.has_value() || *rhs_compare_index >= rhs_entry_block->insts.size()) &&
        !short_circuit_join_has_join_defined_named_incoming(*join_context)) {
      return std::nullopt;
    }
  }

  if (current_i8_name.has_value() && compare_index < block.insts.size()) {
    const auto* source_branch_condition =
        c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                              *resolved_block_label_id);
    const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[compare_index]);
    const auto compare_context =
        compare != nullptr && compare->operand_type == c4c::backend::bir::TypeKind::I8 &&
                source_branch_condition != nullptr
            ? render_prepared_guard_false_branch_compare_with_current_i8_if_supported(
                  *compare,
                  current_i8_name,
                  current_materialized_compare,
                  current_i32_name,
                  prepared_names,
                  function_context.function_locations)
            : std::nullopt;
    const auto prepared_target_labels =
        compare_context.has_value() && source_branch_condition != nullptr
            ? c4c::backend::prepare::resolve_prepared_compare_branch_target_labels(
                  *prepared_names,
                  function_control_flow,
                  *resolved_block_label_id,
                  block,
                  *source_branch_condition)
            : std::nullopt;
    const auto prepared_short_circuit_plan =
        prepared_target_labels.has_value()
            ? c4c::backend::prepare::find_prepared_short_circuit_branch_plan(
                  *prepared_names, *join_context, *prepared_target_labels)
            : std::nullopt;
    const auto short_circuit_plan =
        prepared_short_circuit_plan.has_value()
            ? c4c::backend::x86::build_prepared_short_circuit_plan(
                  *prepared_names, *prepared_short_circuit_plan, find_block)
            : std::nullopt;
    if (compare_context.has_value() && short_circuit_plan.has_value()) {
      return PreparedBlockCondBranchRenderSelection{
          .render_plan =
              CompareDrivenBranchRenderPlan{
                  .branch_plan = std::move(*short_circuit_plan),
                  .compare_setup = std::move(compare_context->first),
                  .false_branch_opcode = std::move(compare_context->second),
              },
          .prepared_names = prepared_names,
      };
    }
  }

  const auto short_circuit_render_plan =
      c4c::backend::x86::build_prepared_short_circuit_entry_render_plan(
          *prepared_names,
          function_control_flow,
          function_context.function_locations,
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
  return PreparedBlockCondBranchRenderSelection{
      .render_plan = std::move(*short_circuit_render_plan),
      .prepared_names = prepared_names,
  };
}

std::optional<PreparedBlockCondBranchRenderSelection>
select_prepared_plain_cond_branch_render_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  const auto& block = *block_context.block;
  const auto* function_control_flow = function_context.function_control_flow;
  const auto* prepared_names = function_context.prepared_names;
  const auto block_label_id = block_context.block_label_id;

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
          function_context.function_locations,
          block,
          compare_index,
          current_materialized_compare,
          current_i32_name,
          find_block);
  if (!plain_cond_render_plan.has_value()) {
    return std::nullopt;
  }
  return PreparedBlockCondBranchRenderSelection{
      .render_plan = std::move(*plain_cond_render_plan),
      .prepared_names = nullptr,
  };
}

std::optional<PreparedBlockBranchRenderSelection>
select_prepared_block_branch_render_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.function == nullptr || block_context.block->terminator.kind !=
                                                  c4c::backend::bir::TerminatorKind::Branch) {
    return std::nullopt;
  }

  const auto& function = *function_context.function;
  const auto& block = *block_context.block;
  const auto* function_control_flow = function_context.function_control_flow;
  const auto* prepared_names = function_context.prepared_names;

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
      return PreparedBlockBranchRenderSelection{
          .compare_join_render_plan = std::move(compare_join_render_plan),
          .prepared_names = prepared_names,
      };
    }

    if (compare_index < block.insts.size()) {
      const auto* prepared_branch_condition =
          function_control_flow != nullptr
              ? c4c::backend::prepare::find_prepared_branch_condition(
                    *function_control_flow, block_context.block_label_id)
              : nullptr;
      const auto prepared_branch_plan =
          c4c::backend::prepare::find_prepared_compare_join_entry_branch_plan(
              *prepared_names, function_control_flow, function, block, *continuation);
      const auto short_circuit_plan =
          prepared_branch_plan.has_value()
              ? c4c::backend::x86::build_prepared_short_circuit_plan(
                    *prepared_names, *prepared_branch_plan, find_block)
              : std::nullopt;
      const auto* compare = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[compare_index]);
      const auto authoritative_compare =
          prepared_branch_condition != nullptr && prepared_branch_condition->predicate.has_value() &&
                  prepared_branch_condition->lhs.has_value() &&
                  prepared_branch_condition->rhs.has_value()
              ? std::optional<c4c::backend::bir::BinaryInst>{
                    c4c::backend::bir::BinaryInst{
                        .opcode = *prepared_branch_condition->predicate,
                        .result = prepared_branch_condition->condition_value,
                        .operand_type = prepared_branch_condition->compare_type.value_or(
                            c4c::backend::bir::TypeKind::I32),
                        .lhs = *prepared_branch_condition->lhs,
                        .rhs = *prepared_branch_condition->rhs,
                    }}
              : std::nullopt;
      const auto compare_context =
          authoritative_compare.has_value()
              ? render_prepared_guard_false_branch_compare_with_current_i8_if_supported(
                    *authoritative_compare,
                    current_i8_name,
                    current_materialized_compare,
                    current_i32_name,
                    prepared_names,
                    function_context.function_locations)
              : prepared_branch_condition != nullptr
                    ? std::nullopt
                    : compare != nullptr
                          ? render_prepared_guard_false_branch_compare_with_current_i8_if_supported(
                                *compare,
                                current_i8_name,
                                current_materialized_compare,
                                current_i32_name,
                                prepared_names,
                                function_context.function_locations)
                          : std::nullopt;
      if (short_circuit_plan.has_value() && compare_context.has_value()) {
        return PreparedBlockBranchRenderSelection{
            .compare_join_render_plan =
                CompareDrivenBranchRenderPlan{
                    .branch_plan = std::move(*short_circuit_plan),
                    .compare_setup = std::move(compare_context->first),
                    .false_branch_opcode = std::move(compare_context->second),
                },
            .prepared_names = prepared_names,
        };
      }
    }
  }

  return PreparedBlockBranchRenderSelection{};
}

std::optional<PreparedBlockCondBranchRenderSelection>
select_prepared_block_cond_branch_render_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block) {
  if (block_context.function_context == nullptr || block_context.block == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.function == nullptr) {
    return std::nullopt;
  }
  const auto& block = *block_context.block;

  if (block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      compare_index != block.insts.size() - 1) {
    return std::nullopt;
  }

  const auto short_circuit_render =
      select_prepared_short_circuit_cond_branch_render_if_supported(block_context,
                                                                    compare_index,
                                                                    current_materialized_compare,
                                                                    current_i32_name,
                                                                    current_i8_name,
                                                                    find_block);
  if (short_circuit_render.has_value()) {
    return short_circuit_render;
  }

  return select_prepared_plain_cond_branch_render_if_supported(block_context,
                                                               compare_index,
                                                               current_materialized_compare,
                                                               current_i32_name,
                                                               find_block);
}

std::optional<std::string> render_prepared_block_cond_branch_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block) {
  if (block_context.function_context == nullptr ||
      block_context.function_context->function == nullptr) {
    return std::nullopt;
  }
  const auto selected_render =
      select_prepared_block_cond_branch_render_if_supported(block_context,
                                                            compare_index,
                                                            current_materialized_compare,
                                                            current_i32_name,
                                                            current_i8_name,
                                                            find_block);
  if (!selected_render.has_value()) {
    return std::nullopt;
  }
  const auto& function = *block_context.function_context->function;
  return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
      function.name,
      body,
      selected_render->render_plan,
      selected_render->prepared_names,
      find_block,
      render_block);
}

std::optional<std::string> render_prepared_block_branch_terminator_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    std::string body,
    std::size_t compare_index,
    const std::optional<MaterializedI32Compare>& current_materialized_compare,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_i8_name,
    const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
        continuation,
    const std::function<const c4c::backend::bir::Block*(std::string_view)>& find_block,
    const std::function<std::optional<std::string>(
        const c4c::backend::bir::Block&,
        const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>&
        render_block) {
  const auto selected_render =
      select_prepared_block_branch_render_if_supported(block_context,
                                                       compare_index,
                                                       current_materialized_compare,
                                                       current_i32_name,
                                                       current_i8_name,
                                                       continuation,
                                                       find_block);
  if (!selected_render.has_value()) {
    return std::nullopt;
  }
  if (selected_render->compare_join_render_plan.has_value()) {
    if (block_context.function_context == nullptr ||
        block_context.function_context->function == nullptr) {
      return std::nullopt;
    }
    const auto& function = *block_context.function_context->function;
    return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
        function.name,
        body,
        *selected_render->compare_join_render_plan,
        selected_render->prepared_names,
        find_block,
        render_block);
  }
  return render_prepared_block_plain_branch_terminator_if_supported(
      block_context, std::move(body), compare_index, continuation, find_block, render_block);
}

std::optional<std::string> render_prepared_scalar_load_inst_if_supported(
    const c4c::backend::bir::Inst& inst,
    const PreparedModuleLocalSlotLayout& layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::size_t inst_index,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&, c4c::backend::bir::TypeKind,
                             std::int64_t)>& same_module_global_supports_scalar_load,
    std::unordered_set<std::string_view>* same_module_global_names,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (same_module_global_names == nullptr || current_i32_name == nullptr ||
      previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  auto finish_rendered_load =
      [&](const c4c::backend::bir::Value& result,
          std::optional<std::string> rendered_load) -> std::optional<std::string> {
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    *current_materialized_compare = std::nullopt;
    if (result.type == c4c::backend::bir::TypeKind::Ptr) {
      if (prepared_pointer_value_has_authoritative_memory_use(
              result.name, prepared_names, function_addressing)) {
        const auto home_sync = render_prepared_named_ptr_home_sync_if_supported(
            layout, result.name, prepared_names, function_locations);
        if (!home_sync.has_value()) {
          return std::nullopt;
        }
        rendered_load = *rendered_load + *home_sync;
      }
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      *current_ptr_name =
          prepared_current_ptr_carrier_name(result.name, prepared_names, function_locations);
      return rendered_load;
    }
    *current_ptr_name = std::nullopt;
    if (result.type == c4c::backend::bir::TypeKind::I8) {
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = result.name;
      return rendered_load;
    }
    if (result.type == c4c::backend::bir::TypeKind::I32) {
      const auto home_sync = render_prepared_named_i32_stack_home_sync_if_supported(
          layout, result.name, prepared_names, function_locations);
      if (!home_sync.has_value()) {
        return std::nullopt;
      }
      rendered_load = *rendered_load + *home_sync;
      if (current_i32_name->has_value()) {
        rendered_load = std::string("    mov ecx, eax\n") + *rendered_load;
        *previous_i32_name = *current_i32_name;
      } else {
        *previous_i32_name = std::nullopt;
      }
      *current_i32_name = result.name;
      *current_i8_name = std::nullopt;
      return rendered_load;
    }
    return std::nullopt;
  };

  if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
    const auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
        layout,
        prepared_names,
        function_locations,
        function_addressing,
        block_label_id,
        inst_index,
        load->result.type,
        &render_asm_symbol_name,
        *current_ptr_name);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    const auto rendered_load =
        render_prepared_scalar_load_from_memory_if_supported(load->result.type, memory->memory_operand);
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    return finish_rendered_load(load->result, memory->setup_asm + *rendered_load);
  }

  const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst);
  if (load == nullptr ||
      (load->result.type != c4c::backend::bir::TypeKind::I32 &&
       load->result.type != c4c::backend::bir::TypeKind::I8 &&
       load->result.type != c4c::backend::bir::TypeKind::Ptr) ||
      load->result.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto selected_global_memory =
      select_prepared_same_module_scalar_memory_for_inst_if_supported(
          *prepared_names,
          function_addressing,
          block_label_id,
          inst_index,
          load->result.type,
          render_asm_symbol_name,
          find_same_module_global,
          [&](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
            return same_module_global_supports_scalar_load(global, load->result.type, byte_offset);
          });
  if (!selected_global_memory.has_value()) {
    return std::nullopt;
  }
  same_module_global_names->insert(selected_global_memory->global->name);
  return finish_rendered_load(
      load->result,
      render_prepared_scalar_load_from_memory_if_supported(
          load->result.type, selected_global_memory->memory_operand));
}

std::optional<std::string> render_prepared_scalar_store_inst_if_supported(
    const c4c::backend::bir::Inst& inst,
    const PreparedModuleLocalSlotLayout& layout,
    const c4c::backend::bir::Block* block,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label_id,
    std::size_t inst_index,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::function<const c4c::backend::bir::Global*(std::string_view)>& find_same_module_global,
    const std::function<bool(const c4c::backend::bir::Global&, c4c::backend::bir::TypeKind,
                             std::int64_t)>& same_module_global_supports_scalar_load,
    std::unordered_set<std::string_view>* same_module_global_names,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (same_module_global_names == nullptr || current_i32_name == nullptr ||
      previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  auto resolve_previous_i32_operand =
      [&](std::string_view value_name) -> std::optional<std::string> {
    if (block != nullptr) {
      const auto source = select_prepared_named_i32_block_source_if_supported(
          &layout,
          block,
          inst_index,
          function_addressing,
          block_label_id,
          value_name,
          *current_i32_name,
          *previous_i32_name,
          prepared_names,
          function_locations);
      if (!source.has_value()) {
        return std::nullopt;
      }
      return render_prepared_i32_operand_from_source_if_supported(*source);
    }
    return render_prepared_named_i32_operand_if_supported(
        value_name,
        *current_i32_name,
        *previous_i32_name,
        prepared_names,
        function_locations);
  };

  auto render_current_i32_operand =
      [&](const c4c::backend::bir::Value& value,
          const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
    return render_prepared_i32_operand_if_supported(value, current_name, resolve_previous_i32_operand);
  };
  auto render_value_home_stack_address =
      [&](std::string_view value_name) -> std::optional<std::string> {
    return render_prepared_value_home_stack_address_if_supported(
        layout,
        static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
        function_addressing,
        prepared_names,
        function_locations,
        c4c::kInvalidFunctionName,
        value_name);
  };
  auto render_named_ptr_store =
      [&](const c4c::backend::bir::Value& value,
          std::string_view memory_operand) -> std::optional<std::string> {
    if (value.kind != c4c::backend::bir::Value::Kind::Named ||
        value.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    if (current_ptr_name->has_value() && **current_ptr_name == value.name) {
      return "    mov " + std::string(memory_operand) + ", rax\n";
    }
    if (const auto rendered_source = render_prepared_named_ptr_into_register_if_supported(
            value.name, "rax", prepared_names, function_locations, *current_ptr_name);
        rendered_source.has_value()) {
      return *rendered_source + "    mov " + std::string(memory_operand) + ", rax\n";
    }
    return render_prepared_ptr_store_to_memory_if_supported(
        value,
        *current_ptr_name,
        memory_operand,
        render_value_home_stack_address);
  };
  auto append_aggregate_slice_root_home_sync_if_supported =
      [&](std::string_view slot_name,
          c4c::backend::bir::TypeKind type,
          std::string_view destination_memory_operand,
          std::string_view source_operand,
          std::string* rendered_body) -> bool {
        if (rendered_body == nullptr) {
          return false;
        }
        const auto aggregate_root_memory =
            render_prepared_aggregate_slice_root_home_memory_operand_if_supported(
                type, slot_name, &layout, prepared_names, function_locations);
        if (!aggregate_root_memory.has_value() || *aggregate_root_memory == destination_memory_operand) {
          return true;
        }
        *rendered_body += "    mov " + *aggregate_root_memory + ", " + std::string(source_operand) + "\n";
        return true;
      };

  if (const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst)) {
    if (store->address.has_value() ||
        (store->value.type != c4c::backend::bir::TypeKind::I32 &&
         store->value.type != c4c::backend::bir::TypeKind::I8 &&
         store->value.type != c4c::backend::bir::TypeKind::Ptr)) {
      return std::nullopt;
    }
    const auto store_type = store->value.type;
    const auto selected_global_memory =
        select_prepared_same_module_scalar_memory_for_inst_if_supported(
            *prepared_names,
            function_addressing,
            block_label_id,
            inst_index,
            store_type,
            render_asm_symbol_name,
            find_same_module_global,
            [&](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
              if (store_type == c4c::backend::bir::TypeKind::I32) {
                return same_module_global_supports_scalar_load(global, store_type, byte_offset);
              }
              if (store_type == c4c::backend::bir::TypeKind::Ptr) {
                return byte_offset == 0 && global.type == store_type;
              }
              return byte_offset == 0 && global.type == store_type;
            });
    if (!selected_global_memory.has_value()) {
      return std::nullopt;
    }
    same_module_global_names->insert(selected_global_memory->global->name);
    *current_materialized_compare = std::nullopt;
    if (store_type == c4c::backend::bir::TypeKind::I32) {
      *current_i8_name = std::nullopt;
      *current_ptr_name = std::nullopt;
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
      }
      return render_prepared_i32_store_to_memory_if_supported(
          store->value,
          *current_i32_name,
          selected_global_memory->memory_operand,
          render_current_i32_operand);
    }
    if (store_type == c4c::backend::bir::TypeKind::Ptr) {
      *current_i32_name = std::nullopt;
      *previous_i32_name = std::nullopt;
      *current_i8_name = std::nullopt;
      const auto rendered_store =
          render_named_ptr_store(store->value, selected_global_memory->memory_operand);
      *current_ptr_name = std::nullopt;
      return rendered_store;
    }
    const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
        store->value,
        *current_i8_name,
        selected_global_memory->memory_operand,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
          if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            return std::to_string(static_cast<std::int8_t>(value.immediate));
          }
          if (value.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_name.has_value() || *current_name != value.name) {
            return std::nullopt;
          }
          return "al";
        });
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    if (rendered_store.has_value() &&
        store->value.kind == c4c::backend::bir::Value::Kind::Named) {
      *current_i8_name = store->value.name;
    } else {
      *current_i8_name = std::nullopt;
    }
    return rendered_store;
  }

  const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
  if (store == nullptr || store->byte_offset != 0) {
    return std::nullopt;
  }
  auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
      layout,
      prepared_names,
      function_locations,
      function_addressing,
      block_label_id,
      inst_index,
      store->value.type,
      &render_asm_symbol_name,
      *current_ptr_name);
  if (!memory.has_value() && !store->address.has_value()) {
    const auto size_name = prepared_scalar_memory_operand_size_name(store->value.type);
    if (size_name.has_value()) {
      std::optional<std::size_t> base_offset;
      if (const auto slot_it = layout.offsets.find(store->slot_name); slot_it != layout.offsets.end()) {
        base_offset = slot_it->second;
      } else {
        base_offset = find_prepared_value_home_frame_offset(
            layout, prepared_names, function_locations, store->slot_name);
      }
      if (base_offset.has_value()) {
        memory = PreparedScalarMemoryAccessRender{
            .setup_asm = {},
            .memory_operand = render_prepared_stack_memory_operand(
                *base_offset + store->byte_offset, *size_name),
        };
      }
    }
  }
  if (!memory.has_value()) {
    return std::nullopt;
  }
  *current_materialized_compare = std::nullopt;
  if (store->value.type == c4c::backend::bir::TypeKind::I32) {
    const auto selected_store_value = select_prepared_i32_value_if_supported(
        store->value, *current_i32_name, resolve_previous_i32_operand);
    if (selected_store_value.has_value()) {
      if (selected_store_value->immediate.has_value()) {
        *current_i32_name = std::nullopt;
        *previous_i32_name = std::nullopt;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      } else if (selected_store_value->in_eax) {
        *current_i32_name = store->value.name;
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      } else {
        *current_i8_name = std::nullopt;
        *current_ptr_name = std::nullopt;
      }
      const auto rendered_store = render_prepared_i32_store_to_memory_if_supported(
          store->value,
          *current_i32_name,
          memory->memory_operand,
          [&](const c4c::backend::bir::Value& value,
              const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
            return render_current_i32_operand(value, current_name);
          });
      if (!rendered_store.has_value()) {
        return std::nullopt;
      }
      return memory->setup_asm + *rendered_store;
    }
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
      store->value.type == c4c::backend::bir::TypeKind::I8) {
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
        store->value,
        std::nullopt,
        memory->memory_operand,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>&) -> std::optional<std::string> {
          if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
            return std::nullopt;
          }
          return std::to_string(static_cast<std::int8_t>(value.immediate));
        });
    if (!rendered_store.has_value()) {
      return std::nullopt;
    }
    std::string rendered = memory->setup_asm + *rendered_store;
    if (!append_aggregate_slice_root_home_sync_if_supported(
            store->slot_name,
            store->value.type,
            memory->memory_operand,
            std::to_string(static_cast<std::int8_t>(store->value.immediate)),
            &rendered)) {
      return std::nullopt;
    }
    return rendered;
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
      store->value.type == c4c::backend::bir::TypeKind::I8) {
    const auto rendered_store = render_prepared_i8_store_to_memory_if_supported(
        store->value,
        *current_i8_name,
        memory->memory_operand,
        [](const c4c::backend::bir::Value& value,
           const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
          if (value.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_name.has_value() || *current_name != value.name) {
            return std::nullopt;
          }
          return "al";
        });
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_ptr_name = std::nullopt;
    if (rendered_store.has_value()) {
      *current_i8_name = store->value.name;
      std::string rendered = memory->setup_asm + *rendered_store;
      if (!append_aggregate_slice_root_home_sync_if_supported(
              store->slot_name, store->value.type, memory->memory_operand, "al", &rendered)) {
        return std::nullopt;
      }
      return rendered;
    } else {
      *current_i8_name = std::nullopt;
    }
    return std::nullopt;
  }
  if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
      store->value.type == c4c::backend::bir::TypeKind::Ptr) {
    *current_i32_name = std::nullopt;
    *previous_i32_name = std::nullopt;
    *current_i8_name = std::nullopt;
    const auto rendered_store = render_named_ptr_store(store->value, memory->memory_operand);
    *current_ptr_name = std::nullopt;
    if (!rendered_store.has_value()) {
      return std::string{};
    }
    return memory->setup_asm + *rendered_store;
  }
  return std::nullopt;
}

std::optional<std::string> select_prepared_i32_call_argument_move_if_supported(
    const c4c::backend::bir::Value& arg,
    c4c::backend::bir::TypeKind arg_type,
    std::string_view abi_register,
    const std::optional<std::string_view>& current_i32_name) {
  if (arg_type != c4c::backend::bir::TypeKind::I32) {
    return std::nullopt;
  }
  return render_prepared_i32_move_to_register_if_supported(
      arg,
      current_i32_name,
      abi_register,
      [](std::string_view) -> std::optional<std::string> { return std::nullopt; });
}

bool append_prepared_named_ptr_argument_move_into_register_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    c4c::FunctionNameId function_name_id,
    std::string_view pointer_name,
    std::string_view destination_register,
    std::size_t stack_byte_bias,
    const std::optional<std::string_view>& current_ptr_name,
    std::string* body);
bool append_prepared_small_byval_payload_move_into_register_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    c4c::FunctionNameId function_name_id,
    std::string_view pointer_name,
    std::string_view destination_register,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::size_t stack_byte_bias,
    const std::optional<std::string_view>& current_ptr_name,
    std::string* body);

struct PreparedSameModuleHelperCallRenderSelection {
  const c4c::backend::bir::CallInst* call = nullptr;
  std::string rendered_arg_moves;
  std::string rendered_callee;
};

std::optional<PreparedSameModuleHelperCallRenderSelection>
select_prepared_bounded_same_module_helper_call_render_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_names,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    const std::optional<std::string_view>& current_i32_name,
    const std::optional<std::string_view>& current_ptr_name) {
  if (block_context.function_context == nullptr) {
    return std::nullopt;
  }
  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
      call->is_variadic || call->args.size() != call->arg_types.size() || call->args.size() > 6 ||
      bounded_same_module_helper_names.find(call->callee) == bounded_same_module_helper_names.end()) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.prepared_names == nullptr || function_context.function == nullptr) {
    return std::nullopt;
  }
  const auto function_name_id =
      c4c::backend::prepare::resolve_prepared_function_name_id(
          *function_context.prepared_names, function_context.function->name)
          .value_or(c4c::kInvalidFunctionName);
  const auto instruction_index =
      block_context.block == nullptr
          ? std::size_t{0}
          : static_cast<std::size_t>(&inst - block_context.block->insts.data());

  static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

  std::string rendered_arg_moves;
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto& arg = call->args[arg_index];
    const auto arg_type = call->arg_types[arg_index];
    if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
      const bool is_byval_payload =
          arg_index < call->arg_abi.size() && call->arg_abi[arg_index].byval_copy &&
          call->arg_abi[arg_index].size_bytes != 0;
      if (arg.kind != c4c::backend::bir::Value::Kind::Named ||
          !(is_byval_payload
                ? append_prepared_small_byval_payload_move_into_register_if_supported(
                      block_context,
                      function_name_id,
                      arg.name,
                      kArgRegs64[arg_index],
                      call->arg_abi[arg_index].size_bytes,
                      call->arg_abi[arg_index].align_bytes,
                      0,
                      current_ptr_name,
                      &rendered_arg_moves)
                : append_prepared_named_ptr_argument_move_into_register_if_supported(
                      block_context,
                      function_name_id,
                      arg.name,
                      kArgRegs64[arg_index],
                      0,
                      current_ptr_name,
                      &rendered_arg_moves))) {
        return std::nullopt;
      }
      continue;
    }
    const auto selected_arg_move = select_prepared_i32_call_argument_move_if_supported(
        arg, arg_type, kArgRegs32[arg_index], current_i32_name);
    if (!selected_arg_move.has_value()) {
      return std::nullopt;
    }
    rendered_arg_moves += *selected_arg_move;
  }

  return PreparedSameModuleHelperCallRenderSelection{
      .call = call,
      .rendered_arg_moves = std::move(rendered_arg_moves),
      .rendered_callee = render_asm_symbol_name(call->callee),
  };
}

void finalize_prepared_same_module_helper_call_state(
    const c4c::backend::bir::CallInst& call,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  *current_materialized_compare = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  *current_i32_name =
      call.result.has_value() && call.result->type == c4c::backend::bir::TypeKind::I32
          ? std::optional<std::string_view>(call.result->name)
          : std::nullopt;
}

std::optional<std::string> render_prepared_bounded_same_module_helper_call_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    const std::unordered_set<std::string_view>& bounded_same_module_helper_names,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_materialized_compare == nullptr) {
    return std::nullopt;
  }

  const auto selected_call = select_prepared_bounded_same_module_helper_call_render_if_supported(
      block_context,
      inst,
      bounded_same_module_helper_names,
      render_asm_symbol_name,
      *current_i32_name,
      *current_ptr_name);
  if (!selected_call.has_value() || selected_call->call == nullptr) {
    return std::nullopt;
  }

  std::string rendered_call = selected_call->rendered_arg_moves;
  const bool needs_call_alignment_pad =
      block_context.local_layout != nullptr &&
      (block_context.local_layout->frame_size % 16) == 0;
  if (needs_call_alignment_pad) {
    rendered_call += "    sub rsp, 8\n";
  }
  rendered_call += "    xor eax, eax\n";
  rendered_call += "    call ";
  rendered_call += selected_call->rendered_callee;
  rendered_call += "\n";
  if (needs_call_alignment_pad) {
    rendered_call += "    add rsp, 8\n";
  }

  finalize_prepared_same_module_helper_call_state(
      *selected_call->call,
      current_i32_name,
      previous_i32_name,
      current_i8_name,
      current_ptr_name,
      current_materialized_compare);
  return rendered_call;
}

std::optional<std::string> render_prepared_block_same_module_helper_call_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr) {
    return std::nullopt;
  }
  static const std::unordered_set<std::string_view> kEmptyHelperNames;
  const auto& function_context = *block_context.function_context;
  const auto& bounded_same_module_helper_names =
      function_context.bounded_same_module_helper_names == nullptr
          ? kEmptyHelperNames
          : *function_context.bounded_same_module_helper_names;
  return render_prepared_bounded_same_module_helper_call_if_supported(
      block_context,
      inst,
      bounded_same_module_helper_names,
      function_context.render_asm_symbol_name,
      current_i32_name,
      previous_i32_name,
      current_i8_name,
      current_ptr_name,
      current_materialized_compare);
}

bool append_prepared_named_ptr_argument_move_into_register_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    c4c::FunctionNameId function_name_id,
    std::string_view pointer_name,
    std::string_view destination_register,
    std::size_t stack_byte_bias,
    const std::optional<std::string_view>& current_ptr_name,
    std::string* body) {
  if (block_context.function_context == nullptr || body == nullptr || pointer_name.empty()) {
    return false;
  }
  const auto& function_context = *block_context.function_context;

  if (pointer_name.front() == '@') {
    const std::string_view symbol_name(pointer_name.data() + 1, pointer_name.size() - 1);
    if (function_context.find_string_constant != nullptr) {
      if (const auto* string_constant = function_context.find_string_constant(symbol_name);
          string_constant != nullptr) {
        if (function_context.used_string_names != nullptr) {
          function_context.used_string_names->insert(symbol_name);
        }
        *body += "    lea ";
        *body += destination_register;
        *body += ", [rip + ";
        *body += function_context.render_private_data_label(symbol_name);
        *body += "]\n";
        return true;
      }
    }
    if (function_context.find_same_module_global != nullptr) {
      if (const auto* global = function_context.find_same_module_global(symbol_name);
          global != nullptr) {
        if (function_context.used_same_module_global_names != nullptr) {
          function_context.used_same_module_global_names->insert(global->name);
        }
        *body += "    lea ";
        *body += destination_register;
        *body += ", [rip + ";
        *body += function_context.render_asm_symbol_name(global->name);
        *body += "]\n";
        return true;
      }
    }
    return false;
  }

  if (current_ptr_name.has_value() && *current_ptr_name == pointer_name) {
    if (destination_register != "rax") {
      *body += "    mov ";
      *body += destination_register;
      *body += ", rax\n";
    }
    return true;
  }

  const PreparedModuleLocalSlotLayout empty_layout;
  const auto& local_layout =
      block_context.local_layout == nullptr ? empty_layout : *block_context.local_layout;
  if (const auto stack_address = render_prepared_named_stack_address_if_supported(
          local_layout,
          function_context.stack_layout,
          function_context.function_addressing,
          function_context.prepared_names,
          function_context.function_locations,
          function_name_id,
          pointer_name,
          stack_byte_bias);
      stack_address.has_value()) {
    *body += "    lea ";
    *body += destination_register;
    *body += ", ";
    *body += *stack_address;
    *body += "\n";
    return true;
  }

  const auto* home = c4c::backend::prepare::find_prepared_value_home(
      *function_context.prepared_names, *function_context.function_locations, pointer_name);
  if (home == nullptr) {
    return false;
  }
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    if (*home->register_name != destination_register) {
      *body += "    mov ";
      *body += destination_register;
      *body += ", ";
      *body += *home->register_name;
      *body += "\n";
    }
    return true;
  }
  return false;
}

std::optional<std::string> choose_prepared_pointer_payload_address_scratch_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view excluded_register) {
  static constexpr std::array<std::string_view, 4> kScratchRegisters = {"r11", "r10", "r9", "r8"};
  for (const auto candidate : kScratchRegisters) {
    if (candidate == excluded_register || prepared_value_homes_use_register(function_locations, candidate)) {
      continue;
    }
    return std::string(candidate);
  }
  return std::nullopt;
}

bool append_prepared_small_byval_payload_move_into_register_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    c4c::FunctionNameId function_name_id,
    std::string_view pointer_name,
    std::string_view destination_register,
    std::size_t size_bytes,
    std::size_t align_bytes,
    std::size_t stack_byte_bias,
    const std::optional<std::string_view>& current_ptr_name,
    std::string* body) {
  if (block_context.function_context == nullptr || body == nullptr || size_bytes == 0) {
    return false;
  }

  if (!pointer_name.empty() && pointer_name.front() == '@') {
    return append_prepared_named_ptr_argument_move_into_register_if_supported(
        block_context,
        function_name_id,
        pointer_name,
        destination_register,
        stack_byte_bias,
        current_ptr_name,
        body);
  }

  bool rendered_payload_base = false;
  if (const auto& function_context = *block_context.function_context;
      function_context.prepared_names != nullptr) {
    static_cast<void>(align_bytes);
    const PreparedModuleLocalSlotLayout empty_layout;
    const auto& local_layout =
        block_context.local_layout == nullptr ? empty_layout : *block_context.local_layout;
    if (const auto stack_address = render_prepared_named_payload_stack_address_if_supported(
            local_layout,
            function_context.stack_layout,
            function_context.function_addressing,
            function_context.prepared_names,
            function_context.function_locations,
            function_name_id,
            pointer_name,
            stack_byte_bias);
        stack_address.has_value()) {
      *body += "    lea ";
      *body += destination_register;
      *body += ", ";
      *body += *stack_address;
      *body += "\n";
      rendered_payload_base = true;
    }
  }
  if (!rendered_payload_base &&
      !append_prepared_named_ptr_argument_move_into_register_if_supported(
          block_context,
          function_name_id,
          pointer_name,
          destination_register,
          stack_byte_bias,
          current_ptr_name,
          body)) {
    return false;
  }
  return true;
}

std::optional<std::string> render_prepared_block_same_module_call_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<PreparedCurrentFloatCarrier>* current_f32_value,
    std::optional<PreparedCurrentFloatCarrier>* current_f64_value,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || block_context.block == nullptr ||
      current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_f32_value == nullptr || current_f64_value == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.module == nullptr || function_context.prepared_names == nullptr ||
      function_context.function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
      call->args.size() != call->arg_types.size()) {
    return std::nullopt;
  }
  bool found_defined_callee = false;
  for (const auto& candidate : function_context.module->functions) {
    if (!candidate.is_declaration && candidate.name == call->callee) {
      found_defined_callee = true;
      break;
    }
  }
  if (!found_defined_callee) {
    return std::nullopt;
  }

  const auto instruction_index =
      static_cast<std::size_t>(&inst - block_context.block->insts.data());
  struct PreparedNamedI64Source {
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
    std::optional<std::int64_t> immediate_i64;
  };
  const auto select_prepared_named_i64_source_if_supported =
      [&](std::string_view value_name) -> std::optional<PreparedNamedI64Source> {
        const auto* home = c4c::backend::prepare::find_prepared_value_home(
            *function_context.prepared_names, *function_context.function_locations, value_name);
        if (home == nullptr) {
          return std::nullopt;
        }
        if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
            home->register_name.has_value()) {
          return PreparedNamedI64Source{
              .register_name = *home->register_name,
          };
        }
        if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
            home->offset_bytes.has_value()) {
          return PreparedNamedI64Source{
              .stack_operand =
                  render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD"),
          };
        }
        return std::nullopt;
      };
  const auto append_prepared_named_i64_move_into_register_if_supported =
      [&](std::string_view destination_register,
          const PreparedNamedI64Source& source,
          std::string* rendered_body) -> bool {
        if (source.immediate_i64.has_value()) {
          *rendered_body += "    mov " + std::string(destination_register) + ", " +
                            std::to_string(*source.immediate_i64) + "\n";
          return true;
        }
        if (source.register_name.has_value()) {
          if (*source.register_name != destination_register) {
            *rendered_body += "    mov " + std::string(destination_register) + ", " +
                              *source.register_name + "\n";
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
  std::size_t stack_arg_bytes = 0;
  std::size_t float_register_args = 0;
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_stack_offset.has_value()) {
      continue;
    }
    const auto arg_type = call->arg_types[arg_index];
    const auto arg_size =
        arg_type == c4c::backend::bir::TypeKind::F128 ? std::size_t{16}
        : arg_type == c4c::backend::bir::TypeKind::Ptr ||
                arg_type == c4c::backend::bir::TypeKind::I64 ||
                arg_type == c4c::backend::bir::TypeKind::F64
            ? std::size_t{8}
        : arg_type == c4c::backend::bir::TypeKind::F32 ? std::size_t{4}
                                                       : std::size_t{0};
    if (arg_size == 0) {
      return std::nullopt;
    }
    stack_arg_bytes = std::max(stack_arg_bytes, *destination_stack_offset + arg_size);
  }
  if (stack_arg_bytes % 16 != 0) {
    stack_arg_bytes += 8;
  }
  const bool needs_call_alignment_pad =
      block_context.local_layout != nullptr &&
      ((block_context.local_layout->frame_size + stack_arg_bytes) % 16) == 0;
  const std::size_t call_stack_reserve =
      stack_arg_bytes + (needs_call_alignment_pad ? std::size_t{8} : std::size_t{0});

  std::string body;
  const auto preserved_param_registers =
      function_context.function == nullptr
          ? std::optional<PreparedI32ParamRegisterPreservationPlan>(
                PreparedI32ParamRegisterPreservationPlan{})
          : select_prepared_i32_param_register_preservation_plan_if_supported(
                *function_context.function,
                function_context.prepared_names,
                function_context.function_locations);
  if (!preserved_param_registers.has_value()) {
    return std::nullopt;
  }
  body += preserved_param_registers->save_asm;
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (destination_stack_offset.has_value()) {
      continue;
    }
    const auto& arg = call->args[arg_index];
    if (call->arg_types[arg_index] == c4c::backend::bir::TypeKind::Ptr) {
      const auto function_name_id =
          c4c::backend::prepare::resolve_prepared_function_name_id(
              *function_context.prepared_names, function_context.function->name)
              .value_or(c4c::kInvalidFunctionName);
      const auto destination_register = select_prepared_call_argument_abi_register_if_supported(
          function_context.function_locations,
          block_context.block_index,
          instruction_index,
          arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      const bool is_byval_payload =
          arg_index < call->arg_abi.size() && call->arg_abi[arg_index].byval_copy &&
          call->arg_abi[arg_index].size_bytes != 0;
      if (arg.kind != c4c::backend::bir::Value::Kind::Named ||
          !(is_byval_payload
                ? append_prepared_small_byval_payload_move_into_register_if_supported(
                      block_context,
                      function_name_id,
                      arg.name,
                      *destination_register,
                      call->arg_abi[arg_index].size_bytes,
                      call->arg_abi[arg_index].align_bytes,
                      0,
                      *current_ptr_name,
                      &body)
                : append_prepared_named_ptr_argument_move_into_register_if_supported(
                      block_context,
                      function_name_id,
                      arg.name,
                      *destination_register,
                      0,
                      *current_ptr_name,
                      &body))) {
        return std::nullopt;
      }
      continue;
    }

    if (call->arg_types[arg_index] == c4c::backend::bir::TypeKind::I64) {
      const auto destination_register = select_prepared_call_argument_abi_register_if_supported(
          function_context.function_locations,
          block_context.block_index,
          instruction_index,
          arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      PreparedNamedI64Source source;
      if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
        source.immediate_i64 = arg.immediate;
      } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
        const auto selected_source = select_prepared_named_i64_source_if_supported(arg.name);
        if (!selected_source.has_value()) {
          return std::nullopt;
        }
        source = *selected_source;
      } else {
        return std::nullopt;
      }
      if (!append_prepared_named_i64_move_into_register_if_supported(
              *destination_register, source, &body)) {
        return std::nullopt;
      }
      continue;
    }

    if (call->arg_types[arg_index] == c4c::backend::bir::TypeKind::F32 ||
        call->arg_types[arg_index] == c4c::backend::bir::TypeKind::F64) {
      if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto destination_register = select_prepared_call_argument_abi_register_if_supported(
          function_context.function_locations,
          block_context.block_index,
          instruction_index,
          arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      const auto* current_float_value =
          call->arg_types[arg_index] == c4c::backend::bir::TypeKind::F32 ? current_f32_value
                                                                         : current_f64_value;
      if (current_float_value->has_value() &&
          current_float_value->value().value_name == arg.name) {
        const auto move_mnemonic =
            call->arg_types[arg_index] == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
        if (current_float_value->value().register_name != *destination_register) {
          body += "    " + std::string(move_mnemonic) + " " + *destination_register + ", " +
                  current_float_value->value().register_name + "\n";
        }
      } else {
        const auto rendered_arg = render_prepared_named_float_source_into_register_if_supported(
            call->arg_types[arg_index],
            arg.name,
            *destination_register,
            block_context.local_layout,
            function_context.prepared_names,
            function_context.function_locations);
        if (!rendered_arg.has_value()) {
          return std::nullopt;
        }
        body += *rendered_arg;
      }
      if (destination_register->rfind("xmm", 0) == 0) {
        float_register_args += 1;
      }
      continue;
    }

    if (call->arg_types[arg_index] != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    PreparedNamedI32Source source;
    if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
      source.immediate_i32 = static_cast<std::int32_t>(arg.immediate);
    } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
      const auto selected_source = select_prepared_named_i32_block_source_if_supported(
          block_context.local_layout,
          block_context.block,
          instruction_index,
          function_context.function_addressing,
          block_context.block_label_id,
          arg.name,
          *current_i32_name,
          *previous_i32_name,
          function_context.prepared_names,
          function_context.function_locations);
      if (!selected_source.has_value()) {
        return std::nullopt;
      }
      source = *selected_source;
      if (block_context.local_layout == nullptr) {
        return std::nullopt;
      }
      if (!append_prepared_named_i32_home_sync_if_supported(
              &body,
              *block_context.local_layout,
              arg.name,
              source,
              function_context.prepared_names,
              function_context.function_locations)) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }

    const auto destination_register = select_prepared_i32_call_argument_abi_register_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_register.has_value()) {
      throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
    }
    if (!append_prepared_named_i32_move_into_register_if_supported(
            &body, *destination_register, source)) {
      return std::nullopt;
    }
  }

  if (stack_arg_bytes != 0) {
    body += "    sub rsp, " + std::to_string(stack_arg_bytes) + "\n";
  }
  if (needs_call_alignment_pad) {
    body += "    sub rsp, 8\n";
  }
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_stack_offset.has_value()) {
      continue;
    }
    const auto& arg = call->args[arg_index];
    const auto arg_type = call->arg_types[arg_index];
    const auto scratch_register =
        choose_prepared_pointer_scratch_register_if_supported(function_context.function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    const auto stack_memory_operand =
        render_prepared_stack_memory_operand(*destination_stack_offset, "QWORD");
    if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
      const auto function_name_id =
          c4c::backend::prepare::resolve_prepared_function_name_id(
              *function_context.prepared_names, function_context.function->name)
              .value_or(c4c::kInvalidFunctionName);
      if (arg.kind != c4c::backend::bir::Value::Kind::Named ||
          !append_prepared_named_ptr_argument_move_into_register_if_supported(
              block_context,
              function_name_id,
              arg.name,
              *scratch_register,
              call_stack_reserve,
              *current_ptr_name,
              &body)) {
        return std::nullopt;
      }
      body += "    mov " + stack_memory_operand + ", " + *scratch_register + "\n";
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::I64 &&
        (arg.kind == c4c::backend::bir::Value::Kind::Immediate ||
         arg.kind == c4c::backend::bir::Value::Kind::Named)) {
      PreparedNamedI64Source source;
      if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
        source.immediate_i64 = arg.immediate;
      } else {
        const auto selected_source = select_prepared_named_i64_source_if_supported(arg.name);
        if (!selected_source.has_value()) {
          return std::nullopt;
        }
        source = *selected_source;
      }
      if (!append_prepared_named_i64_move_into_register_if_supported(
              *scratch_register, source, &body)) {
        return std::nullopt;
      }
      body += "    mov " + stack_memory_operand + ", " + *scratch_register + "\n";
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::F128 &&
        arg.kind == c4c::backend::bir::Value::Kind::Named) {
      const auto rendered_arg = render_prepared_named_f128_copy_into_memory_if_supported(
          arg.name,
          render_prepared_stack_memory_operand(*destination_stack_offset, "TBYTE"),
          call_stack_reserve,
          block_context.local_layout,
          function_context.function,
          function_context.prepared_names,
          function_context.function_locations);
      if (!rendered_arg.has_value()) {
        return std::nullopt;
      }
      body += *rendered_arg;
      continue;
    }
    if ((arg_type == c4c::backend::bir::TypeKind::F32 ||
         arg_type == c4c::backend::bir::TypeKind::F64) &&
        arg.kind == c4c::backend::bir::Value::Kind::Named) {
      const auto move_mnemonic = arg_type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
      const auto stack_size_name =
          arg_type == c4c::backend::bir::TypeKind::F32 ? "DWORD" : "QWORD";
      const auto* current_float_value =
          arg_type == c4c::backend::bir::TypeKind::F32 ? current_f32_value : current_f64_value;
      if (current_float_value->has_value() && current_float_value->value().value_name == arg.name) {
        body += "    " + std::string(move_mnemonic) + " " +
                render_prepared_stack_memory_operand(*destination_stack_offset, stack_size_name) +
                ", " + current_float_value->value().register_name + "\n";
        continue;
      }
      const auto float_scratch_register =
          choose_prepared_float_scratch_register_if_supported(function_context.function_locations);
      if (!float_scratch_register.has_value()) {
        return std::nullopt;
      }
      const auto rendered_arg = render_prepared_named_float_source_into_register_if_supported(
          arg_type,
          arg.name,
          *float_scratch_register,
          block_context.local_layout,
          function_context.prepared_names,
          function_context.function_locations);
      if (!rendered_arg.has_value()) {
        return std::nullopt;
      }
      body += *rendered_arg;
      body += "    " + std::string(move_mnemonic) + " " +
              render_prepared_stack_memory_operand(*destination_stack_offset, stack_size_name) +
              ", " + *float_scratch_register + "\n";
      continue;
    }
    return std::nullopt;
  }

  if (call->is_variadic) {
    body += "    mov eax, " + std::to_string(float_register_args) + "\n";
  } else {
    body += "    xor eax, eax\n";
  }
  body += "    call ";
  body += function_context.render_asm_symbol_name(call->callee);
  body += "\n";
  if (needs_call_alignment_pad) {
    body += "    add rsp, 8\n";
  }
  if (stack_arg_bytes != 0) {
    body += "    add rsp, " + std::to_string(stack_arg_bytes) + "\n";
  }
  body += preserved_param_registers->restore_asm;

  *current_materialized_compare = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  *current_f32_value = std::nullopt;
  *current_f64_value = std::nullopt;
  *current_i32_name = std::nullopt;
  if (!call->result.has_value() || call->result->kind != c4c::backend::bir::Value::Kind::Named) {
    return body;
  }
  if (call->result->type != c4c::backend::bir::TypeKind::I32) {
    return body;
  }

  const auto* result_home = c4c::backend::prepare::find_prepared_value_home(
      *function_context.prepared_names, *function_context.function_locations, call->result->name);
  const auto call_result_selection = select_prepared_i32_call_result_abi_if_supported(
      function_context.function_locations,
      block_context.block_index,
      instruction_index,
      result_home);
  if (!call_result_selection.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  if (call_result_selection->move != nullptr) {
    if (result_home == nullptr) {
      return std::nullopt;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto home_register = narrow_i32_register(*result_home->register_name);
      if (!home_register.has_value()) {
        return std::nullopt;
      }
      if (*home_register != call_result_selection->abi_register) {
        body += "    mov " + *home_register + ", " + call_result_selection->abi_register + "\n";
      }
      if (*home_register == "eax") {
        *current_i32_name = call->result->name;
      }
      return body;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        result_home->offset_bytes.has_value()) {
      body += "    mov " +
              render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD") + ", " +
              call_result_selection->abi_register + "\n";
      return body;
    }
    return std::nullopt;
  }

  if (call_result_selection->abi_register == "eax") {
    *current_i32_name = call->result->name;
  }
  return body;
}

std::optional<std::string> render_prepared_block_direct_extern_call_inst_if_supported(
    const PreparedX86BlockDispatchContext& block_context,
    const c4c::backend::bir::Inst& inst,
    std::optional<std::string_view>* current_i32_name,
    std::optional<std::string_view>* previous_i32_name,
    std::optional<std::string_view>* current_i8_name,
    std::optional<std::string_view>* current_ptr_name,
    std::optional<PreparedCurrentFloatCarrier>* current_f32_value,
    std::optional<PreparedCurrentFloatCarrier>* current_f64_value,
    std::optional<MaterializedI32Compare>* current_materialized_compare) {
  if (block_context.function_context == nullptr || block_context.block == nullptr ||
      current_i32_name == nullptr || previous_i32_name == nullptr || current_i8_name == nullptr ||
      current_ptr_name == nullptr || current_f32_value == nullptr || current_f64_value == nullptr ||
      current_materialized_compare == nullptr) {
    return std::nullopt;
  }
  const auto& function_context = *block_context.function_context;
  if (function_context.module == nullptr || function_context.prepared_names == nullptr ||
      function_context.function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
  if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
      call->args.size() != call->arg_types.size()) {
    return std::nullopt;
  }
  if (call->result.has_value() &&
      call->result->type != c4c::backend::bir::TypeKind::I32 &&
      call->result->type != c4c::backend::bir::TypeKind::Ptr &&
      call->result->type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }
  for (const auto& candidate : function_context.module->functions) {
    if (!candidate.is_declaration && candidate.name == call->callee) {
      return std::nullopt;
    }
  }

  const auto instruction_index =
      static_cast<std::size_t>(&inst - block_context.block->insts.data());
  const c4c::FunctionNameId function_name_id =
      c4c::backend::prepare::resolve_prepared_function_name_id(
          *function_context.prepared_names, function_context.function->name)
          .value_or(c4c::kInvalidFunctionName);
  const auto normalize_prepared_gp_register_family_if_supported =
      [&](std::string_view register_name) -> std::optional<std::string> {
        if (register_name == "rax" || register_name == "eax") return std::string("rax");
        if (register_name == "rbx" || register_name == "ebx") return std::string("rbx");
        if (register_name == "rcx" || register_name == "ecx") return std::string("rcx");
        if (register_name == "rdx" || register_name == "edx") return std::string("rdx");
        if (register_name == "rdi" || register_name == "edi") return std::string("rdi");
        if (register_name == "rsi" || register_name == "esi") return std::string("rsi");
        if (register_name == "rbp" || register_name == "ebp") return std::string("rbp");
        if (register_name == "rsp" || register_name == "esp") return std::string("rsp");
        if (register_name == "r8" || register_name == "r8d") return std::string("r8");
        if (register_name == "r9" || register_name == "r9d") return std::string("r9");
        if (register_name == "r10" || register_name == "r10d") return std::string("r10");
        if (register_name == "r11" || register_name == "r11d") return std::string("r11");
        if (register_name == "r12" || register_name == "r12d") return std::string("r12");
        if (register_name == "r13" || register_name == "r13d") return std::string("r13");
        if (register_name == "r14" || register_name == "r14d") return std::string("r14");
        if (register_name == "r15" || register_name == "r15d") return std::string("r15");
        return std::nullopt;
      };
  const auto render_prepared_gp_register_family_for_source =
      [&](std::string_view scratch_family, std::string_view source_register) -> std::string {
        const auto source_family =
            normalize_prepared_gp_register_family_if_supported(source_register);
        if (source_family.has_value() && source_register == *source_family) {
          return std::string(scratch_family);
        }
        if (source_family.has_value()) {
          const auto narrowed = narrow_i32_register(scratch_family);
          if (narrowed.has_value()) {
            return *narrowed;
          }
        }
        return std::string(scratch_family);
      };
  struct PreparedNamedI64Source {
    std::optional<std::string> register_name;
    std::optional<std::string> stack_operand;
    std::optional<std::int64_t> immediate_i64;
  };
  const auto select_prepared_named_i64_source_if_supported =
      [&](std::string_view value_name) -> std::optional<PreparedNamedI64Source> {
        const auto* home = c4c::backend::prepare::find_prepared_value_home(
            *function_context.prepared_names, *function_context.function_locations, value_name);
        if (home == nullptr) {
          return std::nullopt;
        }
        if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
            home->register_name.has_value()) {
          return PreparedNamedI64Source{
              .register_name = *home->register_name,
          };
        }
        if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
            home->offset_bytes.has_value()) {
          return PreparedNamedI64Source{
              .stack_operand =
                  render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD"),
          };
        }
        return std::nullopt;
      };
  const auto append_prepared_named_i64_move_into_register_if_supported =
      [&](std::string_view destination_register,
          const PreparedNamedI64Source& source,
          std::string* rendered_body) -> bool {
        if (source.immediate_i64.has_value()) {
          *rendered_body += "    mov " + std::string(destination_register) + ", " +
                            std::to_string(*source.immediate_i64) + "\n";
          return true;
        }
        if (source.register_name.has_value()) {
          if (*source.register_name != destination_register) {
            *rendered_body += "    mov " + std::string(destination_register) + ", " +
                              *source.register_name + "\n";
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
  const auto select_prepared_named_ptr_source_register_if_supported =
      [&](std::string_view pointer_name) -> std::optional<std::string> {
        if (pointer_name.empty()) {
          return std::nullopt;
        }
        if (pointer_name.front() == '@') {
          return std::nullopt;
        }
        if (current_ptr_name->has_value() && **current_ptr_name == pointer_name) {
          return std::string("rax");
        }
        const PreparedModuleLocalSlotLayout empty_layout;
        const auto& local_layout =
            block_context.local_layout == nullptr ? empty_layout : *block_context.local_layout;
        if (render_prepared_named_stack_address_if_supported(
                local_layout,
                function_context.stack_layout,
                function_context.function_addressing,
                function_context.prepared_names,
                function_context.function_locations,
                function_name_id,
                pointer_name,
                0)
                .has_value()) {
          return std::nullopt;
        }
        const auto* home = c4c::backend::prepare::find_prepared_value_home(
            *function_context.prepared_names, *function_context.function_locations, pointer_name);
        if (home == nullptr ||
            home->kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
            !home->register_name.has_value()) {
          return std::nullopt;
        }
        return *home->register_name;
      };
  std::unordered_map<std::string, std::size_t> pending_gp_source_family_uses;
  std::unordered_set<std::string> referenced_gp_source_families;
  std::unordered_set<std::string> referenced_gp_destination_families;
  const auto note_prepared_direct_extern_pending_source_family =
      [&](const std::optional<std::string>& source_register) {
        if (!source_register.has_value()) {
          return;
        }
        const auto family =
            normalize_prepared_gp_register_family_if_supported(*source_register);
        if (!family.has_value()) {
          return;
        }
        referenced_gp_source_families.insert(*family);
        pending_gp_source_family_uses[*family] += 1;
      };
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto& arg = call->args[arg_index];
    const auto arg_type = call->arg_types[arg_index];
    if (const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
            function_context.function_locations,
            block_context.block_index,
            instruction_index,
            arg_index);
        !destination_stack_offset.has_value()) {
      const auto destination_register = arg_type == c4c::backend::bir::TypeKind::I32
                                            ? select_prepared_i32_call_argument_abi_register_if_supported(
                                                  function_context.function_locations,
                                                  block_context.block_index,
                                                  instruction_index,
                                                  arg_index)
                                            : select_prepared_call_argument_abi_register_if_supported(
                                                  function_context.function_locations,
                                                  block_context.block_index,
                                                  instruction_index,
                                                  arg_index);
      if (destination_register.has_value()) {
        const auto family =
            normalize_prepared_gp_register_family_if_supported(*destination_register);
        if (family.has_value()) {
          referenced_gp_destination_families.insert(*family);
        }
      }
    }
    if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
      note_prepared_direct_extern_pending_source_family(
          select_prepared_named_ptr_source_register_if_supported(arg.name));
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::I64) {
      const auto selected_source = select_prepared_named_i64_source_if_supported(arg.name);
      note_prepared_direct_extern_pending_source_family(
          selected_source.has_value() ? selected_source->register_name : std::nullopt);
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::I32) {
      const auto selected_source = select_prepared_named_i32_block_source_if_supported(
          block_context.local_layout,
          block_context.block,
          instruction_index,
          function_context.function_addressing,
          block_context.block_label_id,
          arg.name,
          *current_i32_name,
          *previous_i32_name,
          function_context.prepared_names,
          function_context.function_locations);
      note_prepared_direct_extern_pending_source_family(
          selected_source.has_value() ? selected_source->register_name : std::nullopt);
    }
  }
  std::vector<std::string> available_gp_preservation_scratch_families;
  for (const auto candidate : {"r11", "r10", "r9", "r8"}) {
    if (prepared_value_homes_use_register(function_context.function_locations, candidate)) {
      continue;
    }
    if (referenced_gp_source_families.find(candidate) != referenced_gp_source_families.end()) {
      continue;
    }
    if (referenced_gp_destination_families.find(candidate) != referenced_gp_destination_families.end()) {
      continue;
    }
    available_gp_preservation_scratch_families.push_back(candidate);
  }
  std::unordered_map<std::string, std::string> preserved_gp_source_families;
  const auto remap_prepared_direct_extern_source_register =
      [&](std::string_view source_register) -> std::string {
        const auto family =
            normalize_prepared_gp_register_family_if_supported(source_register);
        if (!family.has_value()) {
          return std::string(source_register);
        }
        const auto preserved = preserved_gp_source_families.find(*family);
        if (preserved == preserved_gp_source_families.end()) {
          return std::string(source_register);
        }
        return render_prepared_gp_register_family_for_source(
            preserved->second, source_register);
      };
  const auto consume_prepared_direct_extern_source_register =
      [&](const std::optional<std::string>& source_register) {
        if (!source_register.has_value()) {
          return;
        }
        const auto family =
            normalize_prepared_gp_register_family_if_supported(*source_register);
        if (!family.has_value()) {
          return;
        }
        const auto it = pending_gp_source_family_uses.find(*family);
        if (it == pending_gp_source_family_uses.end() || it->second == 0) {
          return;
        }
        it->second -= 1;
      };
  const auto preserve_prepared_direct_extern_source_family_if_needed =
      [&](std::string_view destination_register,
          std::string* rendered_body) -> bool {
        const auto family =
            normalize_prepared_gp_register_family_if_supported(destination_register);
        if (!family.has_value()) {
          return true;
        }
        const auto pending_it = pending_gp_source_family_uses.find(*family);
        if (pending_it == pending_gp_source_family_uses.end() || pending_it->second == 0) {
          return true;
        }
        if (preserved_gp_source_families.find(*family) != preserved_gp_source_families.end()) {
          return true;
        }
        if (available_gp_preservation_scratch_families.empty()) {
          return false;
        }
        const auto scratch_family = available_gp_preservation_scratch_families.back();
        available_gp_preservation_scratch_families.pop_back();
        *rendered_body += "    mov " + scratch_family + ", " + *family + "\n";
        preserved_gp_source_families.emplace(*family, scratch_family);
        return true;
      };
  const auto append_prepared_named_ptr_move_into_register_with_preserved_sources_if_supported =
      [&](std::string_view pointer_name,
          std::string_view destination_register,
          std::size_t stack_byte_bias,
          std::string* rendered_body) -> bool {
        if (pointer_name.empty()) {
          return false;
        }
        if (pointer_name.front() == '@') {
          const std::string_view symbol_name(pointer_name.data() + 1, pointer_name.size() - 1);
          if (function_context.find_string_constant != nullptr) {
            if (const auto* string_constant = function_context.find_string_constant(symbol_name);
                string_constant != nullptr) {
              if (function_context.used_string_names != nullptr) {
                function_context.used_string_names->insert(symbol_name);
              }
              *rendered_body += "    lea ";
              *rendered_body += destination_register;
              *rendered_body += ", [rip + ";
              *rendered_body += function_context.render_private_data_label(symbol_name);
              *rendered_body += "]\n";
              return true;
            }
          }
          if (function_context.find_same_module_global != nullptr) {
            if (const auto* global = function_context.find_same_module_global(symbol_name);
                global != nullptr) {
              if (function_context.used_same_module_global_names != nullptr) {
                function_context.used_same_module_global_names->insert(global->name);
              }
              *rendered_body += "    lea ";
              *rendered_body += destination_register;
              *rendered_body += ", [rip + ";
              *rendered_body += function_context.render_asm_symbol_name(global->name);
              *rendered_body += "]\n";
              return true;
            }
          }
          return false;
        }

        if (current_ptr_name->has_value() && **current_ptr_name == pointer_name) {
          const auto remapped_source = remap_prepared_direct_extern_source_register("rax");
          if (destination_register != remapped_source) {
            *rendered_body += "    mov ";
            *rendered_body += destination_register;
            *rendered_body += ", ";
            *rendered_body += remapped_source;
            *rendered_body += "\n";
          }
          return true;
        }

        const PreparedModuleLocalSlotLayout empty_layout;
        const auto& local_layout =
            block_context.local_layout == nullptr ? empty_layout : *block_context.local_layout;
        if (const auto stack_address = render_prepared_named_stack_address_if_supported(
                local_layout,
                function_context.stack_layout,
                function_context.function_addressing,
                function_context.prepared_names,
                function_context.function_locations,
                function_name_id,
                pointer_name,
                stack_byte_bias);
            stack_address.has_value()) {
          *rendered_body += "    lea ";
          *rendered_body += destination_register;
          *rendered_body += ", ";
          *rendered_body += *stack_address;
          *rendered_body += "\n";
          return true;
        }

        const auto* home = c4c::backend::prepare::find_prepared_value_home(
            *function_context.prepared_names, *function_context.function_locations, pointer_name);
        if (home == nullptr) {
          return false;
        }
        if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
            home->register_name.has_value()) {
          const auto remapped_source =
              remap_prepared_direct_extern_source_register(*home->register_name);
          if (destination_register != remapped_source) {
            *rendered_body += "    mov ";
            *rendered_body += destination_register;
            *rendered_body += ", ";
            *rendered_body += remapped_source;
            *rendered_body += "\n";
          }
          return true;
        }
        return false;
      };
  std::string body;
  static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
  std::size_t float_register_args = 0;
  const auto select_prepared_direct_extern_float_argument_register =
      [&](std::size_t arg_index) -> std::optional<std::string> {
        if (call->is_variadic) {
          return std::string("xmm") + std::to_string(float_register_args);
        }
        return select_prepared_call_argument_abi_register_if_supported(
            function_context.function_locations,
            block_context.block_index,
            instruction_index,
            arg_index);
      };
  std::size_t stack_arg_bytes = 0;
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_stack_offset.has_value()) {
      continue;
    }
    const auto arg_type = call->arg_types[arg_index];
    const auto arg_size =
        arg_type == c4c::backend::bir::TypeKind::F128 ? std::size_t{16}
        : arg_type == c4c::backend::bir::TypeKind::Ptr ||
                arg_type == c4c::backend::bir::TypeKind::I64
            ? std::size_t{8}
            : std::size_t{0};
    if (arg_size == 0) {
      return std::nullopt;
    }
    stack_arg_bytes = std::max(stack_arg_bytes, *destination_stack_offset + arg_size);
  }
  if (stack_arg_bytes % 16 != 0) {
    stack_arg_bytes += 8;
  }
  const bool needs_call_alignment_pad =
      block_context.local_layout != nullptr &&
      ((block_context.local_layout->frame_size + stack_arg_bytes) % 16) == 0;
  const std::size_t call_stack_reserve =
      stack_arg_bytes + (needs_call_alignment_pad ? std::size_t{8} : std::size_t{0});
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto& arg = call->args[arg_index];
    const auto arg_type = call->arg_types[arg_index];
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (destination_stack_offset.has_value()) {
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::I64) {
      PreparedNamedI64Source source;
      std::optional<std::string> source_register;
      if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
        source.immediate_i64 = arg.immediate;
      } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
        const auto selected_source = select_prepared_named_i64_source_if_supported(arg.name);
        if (!selected_source.has_value()) {
          return std::nullopt;
        }
        source = *selected_source;
        source_register = source.register_name;
      } else {
        return std::nullopt;
      }
      const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
          *function_context.function_locations,
          c4c::backend::prepare::PreparedMovePhase::BeforeCall,
          block_context.block_index,
          instruction_index);
      if (before_call_bundle == nullptr) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      const auto destination_register = select_prepared_call_argument_abi_register_if_supported(
          function_context.function_locations,
          block_context.block_index,
          instruction_index,
          arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      consume_prepared_direct_extern_source_register(source_register);
      if (!preserve_prepared_direct_extern_source_family_if_needed(
              *destination_register, &body)) {
        return std::nullopt;
      }
      if (source.register_name.has_value()) {
        source.register_name =
            remap_prepared_direct_extern_source_register(*source.register_name);
      }
      if (!append_prepared_named_i64_move_into_register_if_supported(
              *destination_register, source, &body)) {
        return std::nullopt;
      }
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
      const auto destination_register = select_prepared_call_argument_abi_register_if_supported(
          function_context.function_locations,
          block_context.block_index,
          instruction_index,
          arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      consume_prepared_direct_extern_source_register(
          select_prepared_named_ptr_source_register_if_supported(arg.name));
      if (!preserve_prepared_direct_extern_source_family_if_needed(
              *destination_register, &body)) {
        return std::nullopt;
      }
      if (!append_prepared_named_ptr_move_into_register_with_preserved_sources_if_supported(
              arg.name, *destination_register, 0, &body)) {
        return std::nullopt;
      }
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::F32 ||
        arg_type == c4c::backend::bir::TypeKind::F64) {
      if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      const auto destination_register =
          select_prepared_direct_extern_float_argument_register(arg_index);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
      }
      const auto* current_float_value =
          arg_type == c4c::backend::bir::TypeKind::F32 ? current_f32_value : current_f64_value;
      if (current_float_value->has_value() && current_float_value->value().value_name == arg.name) {
        const auto move_mnemonic = arg_type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
        if (current_float_value->value().register_name != *destination_register) {
          body += "    " + std::string(move_mnemonic) + " " + *destination_register + ", " +
                  current_float_value->value().register_name + "\n";
        }
      } else {
      const auto rendered_arg = render_prepared_named_float_source_into_register_if_supported(
          arg_type,
          arg.name,
          *destination_register,
          block_context.local_layout,
          function_context.prepared_names,
          function_context.function_locations);
        if (!rendered_arg.has_value()) {
          return std::nullopt;
        }
        body += *rendered_arg;
      }
      if (destination_register->rfind("xmm", 0) == 0) {
        float_register_args += 1;
      }
      continue;
    }
    if (arg_type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    PreparedNamedI32Source source;
    std::optional<std::string> source_register;
    if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
      source.immediate_i32 = static_cast<std::int32_t>(arg.immediate);
    } else if (arg.kind == c4c::backend::bir::Value::Kind::Named) {
      const auto selected_source = select_prepared_named_i32_block_source_if_supported(
          block_context.local_layout,
          block_context.block,
          instruction_index,
          function_context.function_addressing,
          block_context.block_label_id,
          arg.name,
          *current_i32_name,
          *previous_i32_name,
          function_context.prepared_names,
          function_context.function_locations);
      if (!selected_source.has_value()) {
        return std::nullopt;
      }
      source = *selected_source;
      source_register = source.register_name;
    } else {
      return std::nullopt;
    }

    const auto destination_register = select_prepared_i32_call_argument_abi_register_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_register.has_value()) {
      throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
    }
    consume_prepared_direct_extern_source_register(source_register);
    if (!preserve_prepared_direct_extern_source_family_if_needed(
            *destination_register, &body)) {
      return std::nullopt;
    }
    if (source.register_name.has_value()) {
      source.register_name =
          remap_prepared_direct_extern_source_register(*source.register_name);
    }
    if (!append_prepared_named_i32_move_into_register_if_supported(
            &body, *destination_register, source)) {
      return std::nullopt;
    }
  }

  if (stack_arg_bytes != 0) {
    body += "    sub rsp, " + std::to_string(stack_arg_bytes) + "\n";
  }
  if (needs_call_alignment_pad) {
    body += "    sub rsp, 8\n";
  }
  for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
    const auto destination_stack_offset = select_prepared_call_argument_abi_stack_offset_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        arg_index);
    if (!destination_stack_offset.has_value()) {
      continue;
    }
    const auto& arg = call->args[arg_index];
    const auto arg_type = call->arg_types[arg_index];
    if (arg_type == c4c::backend::bir::TypeKind::F128 &&
        arg.kind == c4c::backend::bir::Value::Kind::Named) {
      const auto rendered_arg = render_prepared_named_f128_copy_into_memory_if_supported(
          arg.name,
          render_prepared_stack_memory_operand(*destination_stack_offset, "TBYTE"),
          call_stack_reserve,
          block_context.local_layout,
          function_context.function,
          function_context.prepared_names,
          function_context.function_locations);
      if (!rendered_arg.has_value()) {
        return std::nullopt;
      }
      body += *rendered_arg;
      continue;
    }
    const auto scratch_register =
        choose_prepared_pointer_scratch_register_if_supported(function_context.function_locations);
    if (!scratch_register.has_value()) {
      return std::nullopt;
    }
    const auto stack_memory_operand =
        render_prepared_stack_memory_operand(*destination_stack_offset, "QWORD");
    if (arg_type == c4c::backend::bir::TypeKind::Ptr) {
      if (arg.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      consume_prepared_direct_extern_source_register(
          select_prepared_named_ptr_source_register_if_supported(arg.name));
      if (!append_prepared_named_ptr_move_into_register_with_preserved_sources_if_supported(
              arg.name,
              *scratch_register,
              call_stack_reserve,
              &body)) {
        return std::nullopt;
      }
      body += "    mov " + stack_memory_operand + ", " + *scratch_register + "\n";
      continue;
    }
    if (arg_type == c4c::backend::bir::TypeKind::I64 &&
        arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
      body += "    mov " + *scratch_register + ", " + std::to_string(arg.immediate) + "\n";
      body += "    mov " + stack_memory_operand + ", " + *scratch_register + "\n";
      continue;
    }
    return std::nullopt;
  }

  if (call->is_variadic) {
    body += "    mov eax, " + std::to_string(float_register_args) + "\n";
  } else {
    body += "    xor eax, eax\n";
  }
  body += "    call ";
  body += function_context.render_asm_symbol_name(call->callee);
  body += "\n";
  if (needs_call_alignment_pad) {
    body += "    add rsp, 8\n";
  }
  if (stack_arg_bytes != 0) {
    body += "    add rsp, " + std::to_string(stack_arg_bytes) + "\n";
  }

  *current_materialized_compare = std::nullopt;
  *previous_i32_name = std::nullopt;
  *current_i8_name = std::nullopt;
  *current_i32_name = std::nullopt;
  *current_ptr_name = std::nullopt;
  *current_f32_value = std::nullopt;
  *current_f64_value = std::nullopt;
  if (!call->result.has_value() || call->result->kind != c4c::backend::bir::Value::Kind::Named) {
    return body;
  }

  const auto* result_home = c4c::backend::prepare::find_prepared_value_home(
      *function_context.prepared_names, *function_context.function_locations, call->result->name);
  if (call->result->type == c4c::backend::bir::TypeKind::Ptr) {
    const auto call_result_selection = select_prepared_call_result_abi_if_supported(
        function_context.function_locations,
        block_context.block_index,
        instruction_index,
        result_home);
    if (!call_result_selection.has_value()) {
      throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
    }
    if (call_result_selection->move != nullptr) {
      if (call_result_selection->move->destination_storage_kind !=
          c4c::backend::prepare::PreparedMoveStorageKind::Register) {
        return std::nullopt;
      }
      if (result_home == nullptr) {
        return std::nullopt;
      }
      if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
          result_home->register_name.has_value()) {
        if (*result_home->register_name != call_result_selection->abi_register) {
          body += "    mov " + *result_home->register_name + ", " +
                  call_result_selection->abi_register + "\n";
        }
        if (*result_home->register_name == "rax") {
          *current_ptr_name = prepared_current_ptr_carrier_name(
              call->result->name,
              function_context.prepared_names,
              function_context.function_locations);
        }
        return body;
      }
      if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
          result_home->offset_bytes.has_value()) {
        body += "    mov " +
                render_prepared_stack_memory_operand(*result_home->offset_bytes, "QWORD") + ", " +
                call_result_selection->abi_register + "\n";
        return body;
      }
      return std::nullopt;
    }
    if (call_result_selection->abi_register == "rax") {
      *current_ptr_name = prepared_current_ptr_carrier_name(
          call->result->name,
          function_context.prepared_names,
          function_context.function_locations);
    }
    return body;
  }
  if (call->result->type != c4c::backend::bir::TypeKind::I32) {
    return body;
  }

  const auto call_result_selection = select_prepared_i32_call_result_abi_if_supported(
      function_context.function_locations,
      block_context.block_index,
      instruction_index,
      result_home);
  if (!call_result_selection.has_value()) {
    throw std::invalid_argument(std::string(kPreparedCallBundleHandoffRequired));
  }
  if (call_result_selection->move != nullptr) {
    if (result_home == nullptr) {
      return std::nullopt;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
        result_home->register_name.has_value()) {
      const auto home_register = narrow_i32_register(*result_home->register_name);
      if (!home_register.has_value()) {
        return std::nullopt;
      }
      if (*home_register != call_result_selection->abi_register) {
        body += "    mov " + *home_register + ", " + call_result_selection->abi_register + "\n";
      }
      if (*home_register == "eax") {
        *current_i32_name = call->result->name;
      }
      return body;
    }
    if (result_home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
        result_home->offset_bytes.has_value()) {
      body += "    mov " +
              render_prepared_stack_memory_operand(*result_home->offset_bytes, "DWORD") + ", " +
              call_result_selection->abi_register + "\n";
      return body;
    }
    return std::nullopt;
  }
  if (call_result_selection->abi_register == "eax") {
    *current_i32_name = call->result->name;
  }
  return body;
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

std::optional<PreparedScalarMemoryAccessRender> render_prepared_scalar_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedAddress& address,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name,
    const std::optional<std::string_view>& current_ptr_name) {
  const auto size_name = prepared_scalar_memory_operand_size_name(type);
  if (!size_name.has_value()) {
    return std::nullopt;
  }
  if (address.base_kind == c4c::backend::prepare::PreparedAddressBaseKind::FrameSlot) {
    const auto memory_operand =
        render_prepared_frame_slot_memory_operand_if_supported(local_layout, address, *size_name);
    if (!memory_operand.has_value()) {
      return std::nullopt;
    }
    return PreparedScalarMemoryAccessRender{
        .setup_asm = {},
        .memory_operand = std::move(*memory_operand),
    };
  }
  if (address.base_kind == c4c::backend::prepare::PreparedAddressBaseKind::PointerValue) {
    return render_prepared_pointer_value_memory_operand_if_supported(
        local_layout,
        prepared_names,
        function_locations,
        function_addressing,
        address,
        *size_name,
        current_ptr_name);
  }
  if (address.base_kind != c4c::backend::prepare::PreparedAddressBaseKind::GlobalSymbol ||
      prepared_names == nullptr || render_asm_symbol_name == nullptr) {
    return std::nullopt;
  }
  const auto memory_operand = render_prepared_symbol_memory_operand_if_supported(
      *prepared_names, address, *size_name, *render_asm_symbol_name);
  if (!memory_operand.has_value()) {
    return std::nullopt;
  }
  return PreparedScalarMemoryAccessRender{
      .setup_asm = {},
      .memory_operand = std::move(*memory_operand),
  };
}

std::optional<PreparedScalarMemoryAccessRender>
render_prepared_scalar_memory_operand_for_inst_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    c4c::BlockLabelId block_label,
    std::size_t inst_index,
    c4c::backend::bir::TypeKind type,
    const std::function<std::string(std::string_view)>* render_asm_symbol_name,
    const std::optional<std::string_view>& current_ptr_name) {
  const auto* prepared_access =
      find_prepared_function_memory_access(function_addressing, block_label, inst_index);
  if (prepared_access == nullptr) {
    return std::nullopt;
  }
  return render_prepared_scalar_memory_operand_if_supported(
      local_layout,
      prepared_names,
      function_locations,
      function_addressing,
      prepared_access->address,
      type,
      render_asm_symbol_name,
      current_ptr_name);
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

std::optional<std::string> render_prepared_local_slot_guard_chain_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& module = *context.module;
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  static const std::unordered_set<std::string_view> kEmptyHelperNames;
  const auto& bounded_same_module_helper_names =
      context.bounded_same_module_helper_names == nullptr
          ? kEmptyHelperNames
          : *context.bounded_same_module_helper_names;
  const auto& bounded_same_module_helper_global_names =
      context.bounded_same_module_helper_global_names == nullptr
          ? kEmptyHelperNames
          : *context.bounded_same_module_helper_global_names;
  const auto& find_block = context.find_block;
  const auto& find_same_module_global = context.find_same_module_global;
  const auto& same_module_global_supports_scalar_load =
      context.same_module_global_supports_scalar_load;
  const auto& render_asm_symbol_name = context.render_asm_symbol_name;
  const auto& emit_same_module_global_data = context.emit_same_module_global_data;
  const auto& prepend_bounded_same_module_helpers =
      context.prepend_bounded_same_module_helpers;
  if (function.blocks.empty() || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  if (context.function_control_flow != nullptr &&
      context.function_control_flow->branch_conditions.size() == 1 &&
      context.function_control_flow->join_transfers.size() == 1) {
    // Keep the single-join countdown family on its dedicated prepared route,
    // even when authoritative transfer metadata is intentionally drifted.
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              stack_layout,
                                              function_addressing,
                                              prepared_names,
                                              context.function_locations,
                                              prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }
  const bool function_has_same_module_call =
      prepared_function_has_same_module_call(module, function);
  const bool function_participates_in_same_module_call =
      function_has_same_module_call ||
      prepared_function_is_same_module_call_callee(module, function.name);
  std::vector<std::string_view> borrowed_same_module_callee_saved_registers;
  if (function_has_same_module_call) {
    const auto same_module_call_preservation_plan =
        select_prepared_i32_param_register_preservation_plan_if_supported(
            function, prepared_names, function_locations);
    if (!same_module_call_preservation_plan.has_value()) {
      return std::nullopt;
    }
    borrowed_same_module_callee_saved_registers =
        same_module_call_preservation_plan->borrowed_wide_registers;
  }
  std::string function_callee_saved_save_asm;
  std::string function_callee_saved_restore_asm;
  if (function_participates_in_same_module_call) {
    const auto function_callee_saved_registers = collect_prepared_function_callee_saved_registers(
        function_locations, borrowed_same_module_callee_saved_registers);
    function_callee_saved_save_asm =
        render_prepared_function_callee_saved_save_asm(function_callee_saved_registers);
    function_callee_saved_restore_asm =
        render_prepared_function_callee_saved_restore_asm(function_callee_saved_registers);
  }
  std::unordered_set<std::string_view> same_module_global_names;
  const c4c::BlockLabelId entry_label_id =
      prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);

  std::unordered_set<c4c::BlockLabelId> authoritative_cyclic_block_labels;
  if (context.function_control_flow != nullptr && entry_label_id != c4c::kInvalidBlockLabel) {
    std::unordered_set<c4c::BlockLabelId> visited_block_labels;
    std::vector<c4c::BlockLabelId> active_block_stack;
    std::function<void(c4c::BlockLabelId)> visit_authoritative_block =
        [&](c4c::BlockLabelId block_label_id) {
          if (block_label_id == c4c::kInvalidBlockLabel) {
            return;
          }
          const auto stack_it =
              std::find(active_block_stack.begin(), active_block_stack.end(), block_label_id);
          if (stack_it != active_block_stack.end()) {
            authoritative_cyclic_block_labels.insert(stack_it, active_block_stack.end());
            return;
          }
          if (!visited_block_labels.insert(block_label_id).second) {
            return;
          }
          const auto* block = c4c::backend::prepare::find_prepared_control_flow_block(
              *context.function_control_flow, block_label_id);
          if (block == nullptr) {
            return;
          }
          active_block_stack.push_back(block_label_id);
          if (block->terminator_kind == c4c::backend::bir::TerminatorKind::Branch) {
            visit_authoritative_block(block->branch_target_label);
          } else if (block->terminator_kind == c4c::backend::bir::TerminatorKind::CondBranch) {
            visit_authoritative_block(block->true_label);
            visit_authoritative_block(block->false_label);
          }
          active_block_stack.pop_back();
        };
    visit_authoritative_block(entry_label_id);
  }

  enum class PreparedRenderedBlockState {
    Rendering,
    Rendered,
  };
  std::unordered_map<std::string_view, PreparedRenderedBlockState> rendered_blocks;
  const auto render_block_reentry_label =
      [&](const c4c::backend::bir::Block& block) -> std::string {
    return ".L" + function.name + "_reentry_" + block.label;
  };
  std::function<std::optional<std::string>(
      const c4c::backend::bir::Block&,
      const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&)>
      render_block =
          [&](const c4c::backend::bir::Block& block,
              const std::optional<c4c::backend::prepare::PreparedShortCircuitContinuationLabels>&
                  continuation) -> std::optional<std::string> {
    const auto block_context = context.make_block_context(block, *layout);
    const bool needs_reentry_label =
        authoritative_cyclic_block_labels.find(block_context.block_label_id) !=
        authoritative_cyclic_block_labels.end();
    const auto rendered_state_it = rendered_blocks.find(block.label);
    if (rendered_state_it != rendered_blocks.end()) {
      if (needs_reentry_label &&
          !has_authoritative_prepared_short_circuit_continuation(continuation)) {
        return "    jmp " + render_block_reentry_label(block) + "\n";
      }
      return std::nullopt;
    }
    if (needs_reentry_label) {
      rendered_blocks.emplace(block.label, PreparedRenderedBlockState::Rendering);
    }
    const c4c::BlockLabelId block_label_id = block_context.block_label_id;
    const auto throw_authoritative_local_slot_handoff =
        [&](std::string_view surface) -> void {
      const bool has_authoritative_multi_block_control_flow =
          context.function_control_flow != nullptr &&
          (!context.function_control_flow->branch_conditions.empty() ||
           !context.function_control_flow->join_transfers.empty()) &&
          has_authoritative_prepared_control_flow_block(context.function_control_flow,
                                                        block_label_id);
      if (continuation.has_value() || has_authoritative_multi_block_control_flow) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared local-slot " +
            std::string(surface) + " handoff through the canonical prepared-module handoff");
      }
    };

    auto rendered_load_or_store =
        [&](const c4c::backend::bir::Inst& inst,
            std::size_t inst_index,
            std::optional<std::string_view>* current_i32_name,
            std::optional<std::string_view>* previous_i32_name,
            std::optional<std::string_view>* current_i8_name,
            std::optional<std::string_view>* current_ptr_name,
            std::optional<MaterializedI32Compare>* current_materialized_compare)
        -> std::optional<std::string> {
      const auto rendered_load = render_prepared_scalar_load_inst_if_supported(
          inst,
          *layout,
          prepared_names,
          function_locations,
          function_addressing,
          block_label_id,
          inst_index,
          render_asm_symbol_name,
          find_same_module_global,
          same_module_global_supports_scalar_load,
          &same_module_global_names,
          current_i32_name,
          previous_i32_name,
          current_i8_name,
          current_ptr_name,
          current_materialized_compare);
      if (rendered_load.has_value()) {
        return rendered_load;
      }
      return render_prepared_scalar_store_inst_if_supported(
          inst,
          *layout,
          block_context.block,
          prepared_names,
          function_locations,
          function_addressing,
          block_label_id,
          inst_index,
          render_asm_symbol_name,
          find_same_module_global,
          same_module_global_supports_scalar_load,
          &same_module_global_names,
          current_i32_name,
          previous_i32_name,
          current_i8_name,
          current_ptr_name,
          current_materialized_compare);
    };

    std::string body;
    std::optional<std::string_view> current_i32_name;
    std::optional<std::string_view> previous_i32_name;
    std::optional<std::string_view> current_i8_name;
    std::optional<std::string_view> current_ptr_name;
    std::optional<PreparedCurrentFloatCarrier> current_f32_value;
    std::optional<PreparedCurrentFloatCarrier> current_f64_value;
    std::optional<MaterializedI32Compare> current_materialized_compare;
    std::unordered_map<std::string_view, std::string_view> i64_i32_aliases;
    const auto compare_index =
        select_prepared_block_terminator_compare_index_if_supported(block, continuation);
    if (!compare_index.has_value()) {
      if (needs_reentry_label) {
        rendered_blocks.erase(block.label);
      }
      return std::nullopt;
    }

    for (std::size_t index = 0; index < *compare_index; ++index) {
      if (const auto rendered_float =
              render_prepared_float_load_or_cast_inst_if_supported(
                  block_context,
                  block.insts[index],
                  index,
                  &same_module_global_names,
                  &current_i32_name,
                  &previous_i32_name,
                  &current_i8_name,
                  &current_ptr_name,
                  &current_f32_value,
                  &current_f64_value,
                  &current_materialized_compare);
          rendered_float.has_value()) {
        body += *rendered_float;
        continue;
      }

      if (const auto rendered_f128 = render_prepared_f128_local_copy_inst_if_supported(
              block_context,
              block.insts[index],
              index,
              &same_module_global_names,
              &current_i32_name,
              &previous_i32_name,
              &current_i8_name,
              &current_ptr_name,
              &current_materialized_compare);
          rendered_f128.has_value()) {
        body += *rendered_f128;
        continue;
      }

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

      if (const auto rendered_call = render_prepared_block_same_module_helper_call_inst_if_supported(
              block_context,
              block.insts[index],
              &current_i32_name,
              &previous_i32_name,
              &current_i8_name,
              &current_ptr_name,
              &current_materialized_compare);
          rendered_call.has_value()) {
        body += *rendered_call;
        continue;
      }

      if (const auto rendered_direct_extern_call =
              render_prepared_block_direct_extern_call_inst_if_supported(
                  block_context,
                  block.insts[index],
                  &current_i32_name,
                  &previous_i32_name,
                  &current_i8_name,
                  &current_ptr_name,
                  &current_f32_value,
                  &current_f64_value,
                  &current_materialized_compare);
          rendered_direct_extern_call.has_value()) {
        body += *rendered_direct_extern_call;
        continue;
      }

      if (const auto rendered_same_module_call =
              render_prepared_block_same_module_call_inst_if_supported(
                  block_context,
                  block.insts[index],
                  &current_i32_name,
                  &previous_i32_name,
                  &current_i8_name,
                  &current_ptr_name,
                  &current_f32_value,
                  &current_f64_value,
                  &current_materialized_compare);
          rendered_same_module_call.has_value()) {
        body += *rendered_same_module_call;
        continue;
      }

      const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&block.insts[index]);
      if (binary != nullptr) {
        const auto rendered_ptr_binary = render_prepared_ptr_binary_inst_if_supported(
            *binary,
            &*layout,
            block_context.block,
            index,
            function_addressing,
            block_label_id,
            prepared_names,
            function_locations,
            i64_i32_aliases,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare);
        if (rendered_ptr_binary.has_value()) {
          body += *rendered_ptr_binary;
          continue;
        }
        const auto rendered_binary = render_prepared_i32_binary_inst_if_supported(
            *binary,
            &*layout,
            block_context.block,
            index,
            function_addressing,
            block_label_id,
            prepared_names,
            function_locations,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare);
        if (rendered_binary.has_value()) {
          body += *rendered_binary;
          continue;
        }
      }

      if (const auto rendered_float_store = render_prepared_float_store_inst_if_supported(
              block_context,
              block.insts[index],
              index,
              &same_module_global_names,
              &current_i32_name,
              &previous_i32_name,
              &current_i8_name,
              &current_ptr_name,
              &current_f32_value,
              &current_f64_value,
              &current_materialized_compare);
          rendered_float_store.has_value()) {
        body += *rendered_float_store;
        continue;
      }

      const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&block.insts[index]);
      if (cast != nullptr) {
        const auto rendered_cast = render_prepared_cast_inst_if_supported(
            *cast,
            prepared_names,
            function_locations,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare,
            &i64_i32_aliases);
        if (rendered_cast.has_value()) {
          body += *rendered_cast;
          continue;
        }
      }

      const auto* select = std::get_if<c4c::backend::bir::SelectInst>(&block.insts[index]);
      if (select != nullptr) {
        const auto rendered_select = render_prepared_select_inst_if_supported(
            block_context,
            index,
            *select,
            prepared_names,
            function_locations,
            i64_i32_aliases,
            &current_i32_name,
            &previous_i32_name,
            &current_i8_name,
            &current_ptr_name,
            &current_materialized_compare);
        if (rendered_select.has_value()) {
          body += *rendered_select;
          continue;
        }
      }
      if (needs_reentry_label) {
        rendered_blocks.erase(block.label);
      }
      throw_authoritative_local_slot_handoff("instruction");
      return std::nullopt;
    }

    if (needs_reentry_label) {
      body = render_block_reentry_label(block) + ":\n" + body;
    }
    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Return) {
      if (*compare_index != block.insts.size()) {
        if (needs_reentry_label) {
          rendered_blocks.erase(block.label);
        }
        return std::nullopt;
      }
      const auto rendered_return =
          block.terminator.value.has_value()
              ? render_prepared_block_return_terminator_if_supported(
                    block_context,
                    std::move(body),
                    *block.terminator.value,
                    current_i32_name,
                    previous_i32_name,
                    function_callee_saved_restore_asm)
              : render_prepared_void_block_return_terminator_if_supported(
                    block_context, std::move(body), function_callee_saved_restore_asm);
      if (!rendered_return.has_value()) {
        if (needs_reentry_label) {
          rendered_blocks.erase(block.label);
        }
        throw_authoritative_local_slot_handoff("return");
        return std::nullopt;
      }
      if (needs_reentry_label) {
        rendered_blocks[block.label] = PreparedRenderedBlockState::Rendered;
      }
      return rendered_return;
    }

    if (block.terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
      const auto rendered_branch =
          render_prepared_block_branch_terminator_if_supported(block_context,
                                                               std::move(body),
                                                               *compare_index,
                                                               current_materialized_compare,
                                                               current_i32_name,
                                                               current_i8_name,
                                                               continuation,
                                                               find_block,
                                                               render_block);
      if (!rendered_branch.has_value()) {
        if (needs_reentry_label) {
          rendered_blocks.erase(block.label);
        }
        throw_authoritative_local_slot_handoff("branch");
        return std::nullopt;
      }
      if (needs_reentry_label) {
        rendered_blocks[block.label] = PreparedRenderedBlockState::Rendered;
      }
      return rendered_branch;
    }

    const auto rendered_cond_branch =
        render_prepared_block_cond_branch_terminator_if_supported(
        block_context,
        std::move(body),
        *compare_index,
        current_materialized_compare,
        current_i32_name,
        current_i8_name,
        find_block,
        render_block);
    if (!rendered_cond_branch.has_value()) {
      if (needs_reentry_label) {
        rendered_blocks.erase(block.label);
      }
      throw_authoritative_local_slot_handoff("cond-branch");
      return std::nullopt;
    }
    if (needs_reentry_label) {
      rendered_blocks[block.label] = PreparedRenderedBlockState::Rendered;
    }
    return rendered_cond_branch;
  };

  auto asm_text = std::string(asm_prefix) + function_callee_saved_save_asm;
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const auto rendered_entry = render_block(entry, std::nullopt);
  if (!rendered_entry.has_value()) {
    return std::nullopt;
  }
  asm_text += *rendered_entry;
  if (context.used_same_module_global_names != nullptr) {
    context.used_same_module_global_names->insert(
        same_module_global_names.begin(), same_module_global_names.end());
  }
  if (context.defer_module_data_emission) {
    return asm_text;
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
  asm_text += rendered_same_module_globals;
  return prepend_bounded_same_module_helpers(std::move(asm_text));
}

std::optional<std::string> render_prepared_local_i32_arithmetic_guard_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  if (!function.params.empty() || function.blocks.size() != 3 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      context.prepared_arch != c4c::TargetArch::X86_64 || entry.insts.empty()) {
    return std::nullopt;
  }

  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              context.stack_layout,
                                              context.function_addressing,
                                              context.prepared_names,
                                              context.function_locations,
                                              context.prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, NamedLocalI32Expr> named_i32_exprs;
  std::vector<std::string_view> candidate_compare_value_names;
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

  auto asm_text = std::string(context.asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  const c4c::BlockLabelId entry_label_id =
      context.prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *context.prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  const auto guard_expression_surface =
      select_prepared_local_i32_guard_expression_surface_if_supported(
          context, *layout, entry_label_id);
  if (!guard_expression_surface.has_value()) {
    return std::nullopt;
  }
  named_i32_exprs = std::move(guard_expression_surface->named_i32_exprs);
  candidate_compare_value_names =
      std::move(guard_expression_surface->candidate_compare_value_names);
  asm_text += guard_expression_surface->setup_asm;

  const auto* prepared_branch_condition =
      find_required_prepared_guard_branch_condition(
          context.function_control_flow, context.prepared_names, entry_label_id);
  if (context.function_control_flow != nullptr && context.prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *context.prepared_names, *context.function_control_flow, entry_label_id)
          .has_value()) {
    return std::nullopt;
  }

  const auto compared_branch_plan =
      select_prepared_or_raw_i32_compared_immediate_branch_plan_if_supported(
          context,
          entry_label_id,
          entry,
          prepared_branch_condition,
          candidate_compare_value_names);
  if (!compared_branch_plan.has_value()) {
    return std::nullopt;
  }

  const auto& compared_value = compared_branch_plan->compared_value;
  const auto compare_immediate = compared_branch_plan->branch_plan.compare_immediate;
  const auto compare_opcode = compared_branch_plan->branch_plan.compare_opcode;
  const auto& true_label = compared_branch_plan->branch_plan.true_label;
  const auto& false_label = compared_branch_plan->branch_plan.false_label;

  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  const auto* true_block = context.find_block(true_label);
  const auto* false_block = context.find_block(false_label);
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
  asm_text += render_prepared_i32_compare_and_branch(
      compare_opcode, compare_immediate, function.name, false_block->label);

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
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  if (!function.params.empty() || function.blocks.size() != 3 ||
      context.prepared_arch != c4c::TargetArch::X86_64 ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      entry.insts.size() != 9) {
    return std::nullopt;
  }
  const auto layout =
      build_prepared_module_local_slot_layout(function,
                                              context.stack_layout,
                                              context.function_addressing,
                                              context.prepared_names,
                                              context.function_locations,
                                              context.prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  const c4c::backend::bir::Block* true_block = context.find_block(entry.terminator.true_label);
  const c4c::backend::bir::Block* false_block = context.find_block(entry.terminator.false_label);
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
      context.prepared_names == nullptr ? c4c::kInvalidBlockLabel
                                : c4c::backend::prepare::resolve_prepared_block_label_id(
                                      *context.prepared_names, entry.label)
                                      .value_or(c4c::kInvalidBlockLabel);
  std::optional<std::string> short_memory;
  if (const auto* prepared_access =
          find_prepared_function_memory_access(context.function_addressing, entry_label_id, 0);
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
          context.function_control_flow, context.prepared_names, entry_label_id);
  if (context.function_control_flow != nullptr && context.prepared_names != nullptr &&
      c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
          *context.prepared_names, *context.function_control_flow, entry_label_id)
          .has_value()) {
    return std::nullopt;
  }

  const auto branch_plan = select_prepared_or_raw_i32_immediate_branch_plan_if_supported(
      context, entry_label_id, entry, prepared_branch_condition, sext_updated->result.name);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }
  auto compare_opcode = branch_plan->compare_opcode;
  std::int64_t compare_immediate = branch_plan->compare_immediate;
  std::string true_label = branch_plan->true_label;
  std::string false_label = branch_plan->false_label;
  if (compare_opcode != c4c::backend::bir::BinaryOpcode::Eq &&
      compare_opcode != c4c::backend::bir::BinaryOpcode::Ne) {
    return std::nullopt;
  }

  true_block = context.find_block(true_label);
  false_block = context.find_block(false_label);
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

  std::string asm_text = std::string(context.asm_prefix);
  if (layout->frame_size != 0) {
    asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    mov " + *short_memory + ", " +
              std::to_string(static_cast<std::int16_t>(store_zero->value.immediate)) + "\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += "    add eax, 1\n";
  asm_text += "    mov " + *short_memory + ", ax\n";
  asm_text += "    movsx eax, " + *short_memory + "\n";
  asm_text += render_prepared_i32_compare_and_branch(
      compare_opcode, compare_immediate, function.name, false_block->label);
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
  return render_prepared_local_i16_arithmetic_guard_if_supported(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_control_flow = function_control_flow,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .find_block = find_block,
      });
}

namespace {

std::optional<std::string> render_prepared_local_i16_i64_sub_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
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
                                              context.function_locations,
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

}  // namespace

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  return render_prepared_local_i16_i64_sub_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
      });
}

std::optional<std::string> render_prepared_local_i16_i64_sub_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_local_i16_i64_sub_return_from_context(context);
}

namespace {

std::optional<std::string> render_prepared_constant_folded_single_block_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr) {
    return std::nullopt;
  }
  const auto* module = context.module;
  const auto& function = *context.function;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  if (!function.params.empty() || function.blocks.size() != 1 ||
      function.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !function.blocks.front().terminator.value.has_value() ||
      prepared_arch != c4c::TargetArch::X86_64 || stack_layout == nullptr ||
      function_addressing == nullptr || prepared_names == nullptr || function_locations == nullptr) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  const auto wrap_i32 = [](std::int64_t value) -> std::int32_t {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
  };
  const auto entry_label_id =
      c4c::backend::prepare::resolve_prepared_block_label_id(*prepared_names, entry.label);
  if (!entry_label_id.has_value()) {
    return std::nullopt;
  }
  const auto layout = build_prepared_module_local_slot_layout(
      function,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      prepared_arch);
  if (!layout.has_value()) {
    return std::nullopt;
  }

  struct ConstantValue {
    c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
    std::int64_t value = 0;
  };

  std::unordered_map<std::string_view, ConstantValue> named_values;
  std::unordered_map<std::size_t, ConstantValue> local_memory;
  std::unordered_map<std::string_view, std::size_t> symbolic_ptr_roots;
  std::size_t next_symbolic_ptr_root = 1;

  const auto intern_symbolic_ptr_root = [&](std::string_view value_name) -> std::size_t {
    const auto [it, inserted] = symbolic_ptr_roots.emplace(value_name, next_symbolic_ptr_root);
    if (inserted) {
      ++next_symbolic_ptr_root;
    }
    return it->second;
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

  const auto resolve_i8_value =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::int8_t> {
        if (value.type != c4c::backend::bir::TypeKind::I8) {
          return std::nullopt;
        }
        if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
          return static_cast<std::int8_t>(value.immediate);
        }
        if (value.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto value_it = named_values.find(value.name);
        if (value_it == named_values.end() ||
            value_it->second.type != c4c::backend::bir::TypeKind::I8) {
          return std::nullopt;
        }
        return static_cast<std::int8_t>(value_it->second.value);
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
        if (const auto frame_offset = find_prepared_value_home_frame_offset(
                *layout, prepared_names, function_locations, value.name);
            frame_offset.has_value()) {
          return frame_offset;
        }
        if (!value.name.empty() && value.name.front() == '@') {
          const std::string_view symbol_name(value.name.data() + 1, value.name.size() - 1);
          if (module != nullptr) {
            for (const auto& string_constant : module->string_constants) {
              if (string_constant.name == symbol_name) {
                return intern_symbolic_ptr_root(value.name);
              }
            }
            for (const auto& global : module->globals) {
              if (global.name == symbol_name) {
                return intern_symbolic_ptr_root(value.name);
              }
            }
          }
        }
        const auto is_param = std::any_of(function.params.begin(),
                                          function.params.end(),
                                          [&](const c4c::backend::bir::Param& param) {
                                            return param.type == c4c::backend::bir::TypeKind::Ptr &&
                                                   param.name == value.name;
                                          });
        if (is_param || block_defines_named_value(entry, value.name)) {
          return std::nullopt;
        }
        return intern_symbolic_ptr_root(value.name);
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

  const auto resolve_string_constant_i8 =
      [&](std::string_view symbol_name,
          std::int64_t byte_offset) -> std::optional<std::int8_t> {
        if (module == nullptr || byte_offset < 0) {
          return std::nullopt;
        }
        for (const auto& string_constant : module->string_constants) {
          if (string_constant.name != symbol_name) {
            continue;
          }
          const auto index = static_cast<std::size_t>(byte_offset);
          if (index >= string_constant.bytes.size()) {
            return std::nullopt;
          }
          return static_cast<std::int8_t>(
              static_cast<unsigned char>(string_constant.bytes[index]));
        }
        return std::nullopt;
      };

  const auto fold_binary_i32 =
      [&](const c4c::backend::bir::BinaryInst& binary) -> std::optional<std::int32_t> {
        if (binary.result.type == c4c::backend::bir::TypeKind::I32 &&
            binary.operand_type == c4c::backend::bir::TypeKind::Ptr) {
          const auto lhs = resolve_ptr_value(binary.lhs);
          const auto rhs = resolve_ptr_value(binary.rhs);
          if (!lhs.has_value() || !rhs.has_value()) {
            return std::nullopt;
          }
          switch (binary.opcode) {
            case c4c::backend::bir::BinaryOpcode::Eq:
              return *lhs == *rhs ? 1 : 0;
            case c4c::backend::bir::BinaryOpcode::Ne:
              return *lhs != *rhs ? 1 : 0;
            default:
              return std::nullopt;
          }
        }
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

    if (const auto* load = std::get_if<c4c::backend::bir::LoadGlobalInst>(&inst)) {
      if (load->result.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      if (load->result.type == c4c::backend::bir::TypeKind::I8) {
        const auto loaded_value = resolve_string_constant_i8(load->global_name, load->byte_offset);
        if (!loaded_value.has_value()) {
          return std::nullopt;
        }
        named_values[load->result.name] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::I8,
            .value = *loaded_value,
        };
        continue;
      }
      return std::nullopt;
    }

    if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
      if (cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
          cast->operand.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      if (cast->opcode == c4c::backend::bir::CastOpcode::SExt &&
          cast->operand.type == c4c::backend::bir::TypeKind::I8 &&
          cast->result.type == c4c::backend::bir::TypeKind::I32) {
        const auto extended = resolve_i8_value(cast->operand);
        if (!extended.has_value()) {
          return std::nullopt;
        }
        named_values[cast->result.name] = ConstantValue{
            .type = c4c::backend::bir::TypeKind::I32,
            .value = static_cast<std::int32_t>(*extended),
        };
        continue;
      }
      return std::nullopt;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst);
    if (binary == nullptr || binary->result.type != c4c::backend::bir::TypeKind::I32) {
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

}  // namespace

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register) {
  return render_prepared_constant_folded_single_block_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
      });
}

std::optional<std::string> render_prepared_constant_folded_single_block_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_constant_folded_single_block_return_from_context(context);
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

std::optional<std::string> render_prepared_param_derived_i64_value_if_supported(
    std::string_view return_register,
    const c4c::backend::bir::Value& value,
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
        named_binaries,
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
        named_i32_binaries,
    const std::unordered_map<std::string_view, const c4c::backend::bir::CastInst*>&
        named_i64_casts,
    const c4c::backend::bir::Param& param,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  if (value.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return "    mov " + std::string(return_register) + ", " +
           std::to_string(static_cast<std::int64_t>(value.immediate)) + "\n";
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
  if (binary.operand_type != c4c::backend::bir::TypeKind::I64 ||
      binary.result.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }

  const auto resolve_immediate_like_i64 =
      [&](const c4c::backend::bir::Value& immediate_like) -> std::optional<std::int64_t> {
        if (immediate_like.kind == c4c::backend::bir::Value::Kind::Immediate &&
            immediate_like.type == c4c::backend::bir::TypeKind::I64) {
          return static_cast<std::int64_t>(immediate_like.immediate);
        }
        if (immediate_like.kind != c4c::backend::bir::Value::Kind::Named ||
            immediate_like.type != c4c::backend::bir::TypeKind::I64) {
          return std::nullopt;
        }
        const auto cast_it = named_i64_casts.find(immediate_like.name);
        if (cast_it == named_i64_casts.end()) {
          return std::nullopt;
        }
        const auto& cast = *cast_it->second;
        if ((cast.opcode != c4c::backend::bir::CastOpcode::SExt &&
             cast.opcode != c4c::backend::bir::CastOpcode::ZExt) ||
            cast.operand.kind != c4c::backend::bir::Value::Kind::Named ||
            cast.operand.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        const auto seed_it = named_i32_binaries.find(cast.operand.name);
        if (seed_it == named_i32_binaries.end()) {
          return std::nullopt;
        }
        const auto& seed = *seed_it->second;
        if (seed.opcode != c4c::backend::bir::BinaryOpcode::Sub ||
            seed.operand_type != c4c::backend::bir::TypeKind::I32 ||
            seed.lhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
            seed.lhs.type != c4c::backend::bir::TypeKind::I32 || seed.lhs.immediate != 0 ||
            seed.rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
            seed.rhs.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        if (cast.opcode == c4c::backend::bir::CastOpcode::SExt) {
          return static_cast<std::int64_t>(static_cast<std::int32_t>(-seed.rhs.immediate));
        }
        return static_cast<std::int64_t>(
            static_cast<std::uint32_t>(static_cast<std::int32_t>(-seed.rhs.immediate)));
      };

  const bool lhs_is_param = binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                            binary.lhs.type == c4c::backend::bir::TypeKind::I64 &&
                            binary.lhs.name == param.name;
  const bool rhs_is_param = binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                            binary.rhs.type == c4c::backend::bir::TypeKind::I64 &&
                            binary.rhs.name == param.name;
  const auto lhs_immediate = resolve_immediate_like_i64(binary.lhs);
  const auto rhs_immediate = resolve_immediate_like_i64(binary.rhs);
  const bool lhs_is_param_rhs_is_imm = lhs_is_param && rhs_immediate.has_value();
  const bool rhs_is_param_lhs_is_imm = rhs_is_param && lhs_immediate.has_value();
  if (!lhs_is_param_rhs_is_imm && !rhs_is_param_lhs_is_imm) {
    return std::nullopt;
  }

  const auto param_register = minimal_param_register(param);
  if (!param_register.has_value()) {
    return std::nullopt;
  }
  const auto immediate =
      lhs_is_param_rhs_is_imm ? *rhs_immediate : *lhs_immediate;
  const auto render_imm32 = [&](std::int64_t value) -> std::optional<std::string> {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) {
      return std::nullopt;
    }
    return std::to_string(static_cast<std::int32_t>(value));
  };
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
    const auto imm = render_imm32(immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    add " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
    const auto imm = render_imm32(*rhs_immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sub " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && rhs_is_param_lhs_is_imm) {
    return "    mov " + std::string(return_register) + ", " +
           std::to_string(*lhs_immediate) + "\n    sub " + std::string(return_register) + ", " +
           *param_register + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
    const auto imm = render_imm32(immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register +
           "\n    imul " + std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
    const auto imm = render_imm32(immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    and " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
    const auto imm = render_imm32(immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    or " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
    const auto imm = render_imm32(immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    xor " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
    const auto imm = render_imm32(*rhs_immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shl " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
    const auto imm = render_imm32(*rhs_immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    shr " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
    const auto imm = render_imm32(*rhs_immediate);
    if (!imm.has_value()) {
      return std::nullopt;
    }
    return "    mov " + std::string(return_register) + ", " + *param_register + "\n    sar " +
           std::string(return_register) + ", " + *imm + "\n";
  }
  return std::nullopt;
}

namespace {

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr || context.entry == nullptr || !context.minimal_param_register) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  const auto& minimal_param_register = context.minimal_param_register;
  if (function.blocks.size() != 1 || entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto& returned = *entry.terminator.value;
  if (returned.type != c4c::backend::bir::TypeKind::I32 &&
      returned.type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }
  if (entry.insts.empty() && function.params.empty() &&
      returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return std::string(asm_prefix) + "    mov " + std::string(return_register) + ", " +
           std::to_string(returned.type == c4c::backend::bir::TypeKind::I32
                              ? static_cast<std::int32_t>(returned.immediate)
                              : static_cast<std::int64_t>(returned.immediate)) +
           "\n    ret\n";
  }
  if (prepared_arch != c4c::TargetArch::X86_64 || function.params.size() != 1) {
    return std::nullopt;
  }

  const auto& param = function.params.front();
  std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
  std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_i32_binaries;
  std::unordered_map<std::string_view, const c4c::backend::bir::CastInst*> named_i64_casts;
  if (entry.insts.empty()) {
    const auto value_render =
        returned.type == c4c::backend::bir::TypeKind::I32
            ? render_prepared_param_derived_i32_value_if_supported(
                  return_register, returned, named_binaries, param, minimal_param_register)
            : render_prepared_param_derived_i64_value_if_supported(
                  return_register,
                  returned,
                  named_binaries,
                  named_i32_binaries,
                  named_i64_casts,
                  param,
                  minimal_param_register);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return std::string(asm_prefix) + render_prepared_return_body(*value_render);
  }

  if (returned.kind != c4c::backend::bir::Value::Kind::Named) {
    return std::nullopt;
  }

  for (const auto& inst : entry.insts) {
    if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
      if (binary->result.kind != c4c::backend::bir::Value::Kind::Named) {
        return std::nullopt;
      }
      if (binary->result.type == c4c::backend::bir::TypeKind::I64) {
        named_binaries.emplace(binary->result.name, binary);
        continue;
      }
      if (binary->result.type == c4c::backend::bir::TypeKind::I32) {
        named_i32_binaries.emplace(binary->result.name, binary);
        continue;
      }
      return std::nullopt;
    }
    const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
    if (cast == nullptr || cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
        cast->result.type != c4c::backend::bir::TypeKind::I64 ||
        (cast->opcode != c4c::backend::bir::CastOpcode::SExt &&
         cast->opcode != c4c::backend::bir::CastOpcode::ZExt)) {
      return std::nullopt;
    }
    named_i64_casts.emplace(cast->result.name, cast);
  }

  const auto value_render =
      returned.type == c4c::backend::bir::TypeKind::I32
          ? render_prepared_param_derived_i32_value_if_supported(
                return_register, returned, named_binaries, param, minimal_param_register)
          : render_prepared_param_derived_i64_value_if_supported(
                return_register,
                returned,
                named_binaries,
                named_i32_binaries,
                named_i64_casts,
                param,
                minimal_param_register);
  if (!value_render.has_value()) {
    return std::nullopt;
  }
  return std::string(asm_prefix) + render_prepared_return_body(*value_render);
}

}  // namespace

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::bir::Block& entry,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix,
    std::string_view return_register,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&)>&
        minimal_param_register) {
  return render_prepared_minimal_immediate_or_param_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .entry = &entry,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
          .minimal_param_register = minimal_param_register,
      });
}

std::optional<std::string> render_prepared_minimal_immediate_or_param_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_immediate_or_param_return_from_context(context);
}

namespace {

std::optional<std::string> render_prepared_minimal_local_slot_return_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.function == nullptr) {
    return std::nullopt;
  }
  const auto& function = *context.function;
  const auto* stack_layout = context.stack_layout;
  const auto* function_addressing = context.function_addressing;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
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
                                              function_locations,
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
      auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
          *layout,
          prepared_names,
          function_locations,
          function_addressing,
          entry_label_id,
          inst_index,
          store->value.type,
          nullptr,
          std::nullopt);
      if (!memory.has_value()) {
        return std::nullopt;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate &&
          store->value.type == c4c::backend::bir::TypeKind::I32) {
        const auto rendered_store = render_prepared_i32_store_to_memory_if_supported(
            store->value,
            std::nullopt,
            memory->memory_operand,
            [](const c4c::backend::bir::Value& value,
               const std::optional<std::string_view>&) -> std::optional<std::string> {
              if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
                return std::nullopt;
              }
              return std::to_string(static_cast<std::int32_t>(value.immediate));
            });
        if (!rendered_store.has_value()) {
          return std::nullopt;
        }
        asm_text += memory->setup_asm + *rendered_store;
        continue;
      }
      if (store->value.kind == c4c::backend::bir::Value::Kind::Named &&
          store->value.type == c4c::backend::bir::TypeKind::Ptr) {
        auto rendered_store = render_prepared_named_ptr_into_register_if_supported(
            store->value.name, "rax", prepared_names, function_locations, std::nullopt);
        if (rendered_store.has_value()) {
          *rendered_store += "    mov " + memory->memory_operand + ", rax\n";
        } else {
          rendered_store = render_prepared_ptr_store_to_memory_if_supported(
              store->value,
              std::nullopt,
              memory->memory_operand,
              [&](std::string_view value_name) -> std::optional<std::string> {
                return render_prepared_value_home_stack_address_if_supported(
                    *layout,
                    context.stack_layout,
                    function_addressing,
                    prepared_names,
                    function_locations,
                    function_name_id,
                    value_name);
              });
        }
        if (!rendered_store.has_value()) {
          return std::nullopt;
        }
        asm_text += memory->setup_asm + *rendered_store;
        continue;
      }
      return std::nullopt;
    }

    const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
    if (load == nullptr || load->byte_offset != 0) {
      return std::nullopt;
    }
    auto memory = render_prepared_scalar_memory_operand_for_inst_if_supported(
        *layout,
        prepared_names,
        function_locations,
        function_addressing,
        entry_label_id,
        inst_index,
        load->result.type,
        nullptr,
        std::nullopt);
    if (!memory.has_value()) {
      return std::nullopt;
    }
    const auto rendered_load =
        render_prepared_scalar_load_from_memory_if_supported(load->result.type, memory->memory_operand);
    if (!rendered_load.has_value()) {
      return std::nullopt;
    }
    asm_text += memory->setup_asm + *rendered_load;
  }

  if (layout->frame_size != 0) {
    asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
  }
  asm_text += "    ret\n";
  return asm_text;
}

}  // namespace

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch,
    std::string_view asm_prefix) {
  return render_prepared_minimal_local_slot_return_from_context(
      PreparedX86FunctionDispatchContext{
          .function = &function,
          .stack_layout = stack_layout,
          .function_addressing = function_addressing,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
      });
}

std::optional<std::string> render_prepared_minimal_local_slot_return_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_local_slot_return_from_context(context);
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

namespace {

struct PreparedDirectExternNamedI32Source {
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
  std::optional<std::int64_t> immediate_i32;
};

struct PreparedDirectExternCurrentI32Carrier {
  std::string_view value_name;
  std::optional<std::string> register_name;
  std::optional<std::string> stack_operand;
};

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

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_from_context(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr ||
      context.bounded_same_module_helper_global_names == nullptr || !context.find_string_constant ||
      !context.find_same_module_global || !context.render_private_data_label ||
      !context.render_asm_symbol_name || !context.emit_string_constant_data ||
      !context.emit_same_module_global_data || !context.prepend_bounded_same_module_helpers) {
    return std::nullopt;
  }
  const auto& module = *context.module;
  const auto& function = *context.function;
  const auto& entry = *context.entry;
  const auto* prepared_names = context.prepared_names;
  const auto* function_locations = context.function_locations;
  const auto prepared_arch = context.prepared_arch;
  const auto asm_prefix = context.asm_prefix;
  const auto return_register = context.return_register;
  const auto& bounded_same_module_helper_global_names =
      *context.bounded_same_module_helper_global_names;
  const auto& find_string_constant = context.find_string_constant;
  const auto& find_same_module_global = context.find_same_module_global;
  const auto& render_private_data_label = context.render_private_data_label;
  const auto& render_asm_symbol_name = context.render_asm_symbol_name;
  const auto& emit_string_constant_data = context.emit_string_constant_data;
  const auto& emit_same_module_global_data = context.emit_same_module_global_data;
  const auto& prepend_bounded_same_module_helpers = context.prepend_bounded_same_module_helpers;
  if (prepared_arch != c4c::TargetArch::X86_64 || !function.params.empty() ||
      !function.local_slots.empty() || function.blocks.size() != 1 || entry.label != "entry" ||
      entry.insts.empty() ||
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, std::int64_t> i32_constants;
  std::unordered_set<std::string_view> emitted_string_names;
  std::vector<const c4c::backend::bir::StringConstant*> used_string_constants;
  std::unordered_set<std::string_view> used_same_module_globals;
  std::string body = "    sub rsp, 8\n";
  bool saw_call = false;
  std::optional<PreparedDirectExternCurrentI32Carrier> current_i32;

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

    for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
      if (!append_prepared_direct_extern_call_argument_if_supported(
              call->args[arg_index],
              call->arg_types[arg_index],
              arg_index,
              instruction_index,
              current_i32,
              i32_constants,
              prepared_names,
              function_locations,
              find_string_constant,
              find_same_module_global,
              render_private_data_label,
              render_asm_symbol_name,
              &emitted_string_names,
              &used_string_constants,
              &used_same_module_globals,
              &body)) {
        return std::nullopt;
      }
    }

    body += "    xor eax, eax\n";
    body += "    call ";
    body += render_asm_symbol_name(call->callee);
    body += "\n";
    if (!finalize_prepared_direct_extern_call_result_if_supported(
            *call, instruction_index, prepared_names, function_locations, &body, &current_i32)) {
      return std::nullopt;
    }
  }

  if (!saw_call) {
    return std::nullopt;
  }

  if (!finalize_prepared_direct_extern_return_if_supported(
          *entry.terminator.value, return_register, current_i32, resolve_i32_constant, &body)) {
    return std::nullopt;
  }

  std::string rendered_data;
  if (context.used_string_names != nullptr) {
    context.used_string_names->insert(emitted_string_names.begin(), emitted_string_names.end());
  }
  if (context.used_same_module_global_names != nullptr) {
    context.used_same_module_global_names->insert(
        used_same_module_globals.begin(), used_same_module_globals.end());
  }
  if (context.defer_module_data_emission) {
    return std::string(asm_prefix) + body;
  }
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

}  // namespace

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
  return render_prepared_minimal_direct_extern_call_sequence_from_context(
      PreparedX86FunctionDispatchContext{
          .module = &module,
          .function = &function,
          .entry = &entry,
          .prepared_names = prepared_names,
          .function_locations = function_locations,
          .prepared_arch = prepared_arch,
          .asm_prefix = asm_prefix,
          .return_register = return_register,
          .bounded_same_module_helper_global_names = &bounded_same_module_helper_global_names,
          .find_string_constant = find_string_constant,
          .find_same_module_global = find_same_module_global,
          .render_private_data_label = render_private_data_label,
          .render_asm_symbol_name = render_asm_symbol_name,
          .emit_string_constant_data = emit_string_constant_data,
          .emit_same_module_global_data = emit_same_module_global_data,
          .prepend_bounded_same_module_helpers = prepend_bounded_same_module_helpers,
      });
}

std::optional<std::string> render_prepared_minimal_direct_extern_call_sequence_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_direct_extern_call_sequence_from_context(context);
}

std::optional<std::string> render_prepared_single_block_return_direct_extern_call_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  return render_prepared_minimal_direct_extern_call_sequence_if_supported(context);
}

std::optional<std::string> render_prepared_single_block_return_dispatch_if_supported(
    const PreparedX86FunctionDispatchContext& context) {
  if (context.module == nullptr || context.function == nullptr || context.entry == nullptr ||
      context.bounded_same_module_helper_global_names == nullptr ||
      !context.find_string_constant || !context.find_same_module_global ||
      !context.render_private_data_label || !context.render_asm_symbol_name ||
      !context.emit_string_constant_data || !context.emit_same_module_global_data ||
      !context.prepend_bounded_same_module_helpers || !context.minimal_param_register) {
    return std::nullopt;
  }
  if (const auto rendered_direct_calls =
          render_prepared_single_block_return_direct_extern_call_if_supported(context);
      rendered_direct_calls.has_value()) {
    return *rendered_direct_calls;
  }
  if (const auto rendered_local_slot =
          render_prepared_minimal_local_slot_return_if_supported(context);
      rendered_local_slot.has_value()) {
    return *rendered_local_slot;
  }
  if (const auto rendered_constant_folded =
          render_prepared_constant_folded_single_block_return_if_supported(context);
      rendered_constant_folded.has_value()) {
    return *rendered_constant_folded;
  }
  if (const auto rendered_local_i16_i64_return =
          render_prepared_local_i16_i64_sub_return_if_supported(context);
      rendered_local_i16_i64_return.has_value()) {
    return *rendered_local_i16_i64_return;
  }
  if (const auto rendered_immediate_or_param =
          render_prepared_minimal_immediate_or_param_return_if_supported(context);
      rendered_immediate_or_param.has_value()) {
    return *rendered_immediate_or_param;
  }
  return render_prepared_local_slot_guard_chain_if_supported(context);
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const c4c::backend::prepare::PreparedBirModule& module,
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::unordered_set<std::string_view>& helper_names,
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
  bool saw_rendered_non_helper = false;

  for (const auto* candidate : defined_functions) {
    if (helper_names.find(candidate->name) != helper_names.end()) {
      continue;
    }
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered.rendered_functions += *rendered_trivial;
      saw_rendered_non_helper = true;
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
    const auto candidate_function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, candidate->name)
            .value_or(c4c::kInvalidFunctionName);
    if (candidate_function_name_id == c4c::kInvalidFunctionName) {
      return std::nullopt;
    }
    const auto* function_locations =
        c4c::backend::prepare::find_prepared_value_location_function(module, candidate->name);
    if (function_locations == nullptr) {
      return std::nullopt;
    }
    const auto* candidate_function_addressing =
        c4c::backend::prepare::find_prepared_addressing(module, candidate_function_name_id);

    const auto candidate_layout = build_prepared_module_local_slot_layout(
        *candidate,
        static_cast<const c4c::backend::prepare::PreparedStackLayout*>(nullptr),
        static_cast<const c4c::backend::prepare::PreparedAddressingFunction*>(nullptr),
        static_cast<const c4c::backend::prepare::PreparedNameTables*>(nullptr),
        static_cast<const c4c::backend::prepare::PreparedValueLocationFunction*>(nullptr),
        prepared_arch);
    if (!candidate_layout.has_value()) {
      return std::nullopt;
    }
    const auto rendered_candidate =
        render_prepared_bounded_multi_defined_call_lane_body_if_supported(
            module, *function_locations, candidate_function_addressing, candidate_function_name_id,
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
    saw_rendered_non_helper = true;
  }

  if (!saw_bounded_entry && !saw_rendered_non_helper) {
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
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || entry_function.name != "main") {
    return std::nullopt;
  }

  struct BoundedSameModuleHelperRender {
    std::string asm_text;
    std::unordered_set<std::string_view> used_string_names;
    std::unordered_set<std::string_view> used_same_module_globals;
  };

  PreparedBoundedSameModuleHelperPrefixRender rendered_helpers;

  const auto render_generic_bounded_same_module_helper_function_if_supported =
      [&](const c4c::backend::bir::Function& candidate)
      -> std::optional<BoundedSameModuleHelperRender> {
    if (prepared_arch != c4c::TargetArch::X86_64 || candidate.is_declaration ||
        candidate.blocks.empty()) {
      return std::nullopt;
    }
    const auto candidate_function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, candidate.name)
            .value_or(c4c::kInvalidFunctionName);
    if (candidate_function_name_id == c4c::kInvalidFunctionName) {
      return std::nullopt;
    }
    const auto* candidate_function_locations =
        c4c::backend::prepare::find_prepared_value_location_function(module.value_locations,
                                                                     candidate_function_name_id);
    if (candidate_function_locations == nullptr) {
      return std::nullopt;
    }
    const auto* candidate_function_control_flow =
        c4c::backend::prepare::find_prepared_control_flow_function(module.control_flow,
                                                                   candidate_function_name_id);
    const auto* candidate_function_addressing =
        c4c::backend::prepare::find_prepared_addressing(module, candidate_function_name_id);
    const auto candidate_asm_prefix = minimal_function_asm_prefix(candidate);
    std::string candidate_return_register;
    if (candidate.return_type == c4c::backend::bir::TypeKind::I32) {
      const auto selected_return_register = minimal_function_return_register(candidate);
      if (!selected_return_register.has_value()) {
        return std::nullopt;
      }
      candidate_return_register = *selected_return_register;
    }
    const auto find_block = [&](std::string_view label) -> const c4c::backend::bir::Block* {
      for (const auto& block : candidate.blocks) {
        if (block.label == label) {
          return &block;
        }
      }
      return nullptr;
    };
    std::unordered_set<std::string_view> used_string_names;
    std::unordered_set<std::string_view> used_same_module_globals;
    const PreparedX86FunctionDispatchContext function_dispatch_context{
        .prepared_module = &module,
        .module = &module.module,
        .function = &candidate,
        .entry = &candidate.blocks.front(),
        .stack_layout = &module.stack_layout,
        .function_addressing = candidate_function_addressing,
        .prepared_names = &module.names,
        .function_locations = candidate_function_locations,
        .function_control_flow = candidate_function_control_flow,
        .prepared_arch = prepared_arch,
        .asm_prefix = candidate_asm_prefix,
        .return_register = candidate_return_register,
        .bounded_same_module_helper_names = &rendered_helpers.helper_names,
        .bounded_same_module_helper_global_names = &rendered_helpers.helper_global_names,
        .find_block = find_block,
        .find_string_constant =
            [&](std::string_view symbol_name) -> const c4c::backend::bir::StringConstant* {
              for (const auto& string_constant : module.module.string_constants) {
                if (string_constant.name == symbol_name) {
                  return &string_constant;
                }
              }
              return nullptr;
            },
        .find_same_module_global = find_same_module_global,
        .same_module_global_supports_scalar_load = same_module_global_supports_scalar_load,
        .render_private_data_label = render_private_data_label,
        .render_asm_symbol_name = render_asm_symbol_name,
        .emit_string_constant_data =
            [](const c4c::backend::bir::StringConstant&) -> std::string { return {}; },
        .emit_same_module_global_data =
            [](const c4c::backend::bir::Global&) -> std::optional<std::string> {
              return std::string{};
            },
        .prepend_bounded_same_module_helpers = [](std::string asm_text) { return asm_text; },
        .minimal_param_register =
            [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
              auto param_it = candidate.params.begin();
              for (std::size_t arg_index = 0; param_it != candidate.params.end();
                   ++param_it, ++arg_index) {
                if (&*param_it == &param) {
                  return minimal_param_register_at(param, arg_index);
                }
              }
              return std::nullopt;
            },
        .used_string_names = &used_string_names,
        .used_same_module_global_names = &used_same_module_globals,
        .defer_module_data_emission = true,
    };
    const auto rendered_void_direct_extern_helper = [&]() -> std::optional<std::string> {
      if (candidate.return_type != c4c::backend::bir::TypeKind::Void ||
          candidate.blocks.size() != 1 || candidate.blocks.front().label != "entry" ||
          candidate.blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
          candidate.blocks.front().terminator.value.has_value()) {
        return std::nullopt;
      }
      const auto layout = build_prepared_module_local_slot_layout(
          candidate,
          &module.stack_layout,
          candidate_function_addressing,
          &module.names,
          candidate_function_locations,
          prepared_arch);
      if (!layout.has_value()) {
        return std::nullopt;
      }
      const auto block_context =
          function_dispatch_context.make_block_context(candidate.blocks.front(), *layout);
      std::string body;
      if (layout->frame_size != 0) {
        body += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
      }
      const auto helper_target_triple =
          module.module.target_triple.empty() ? c4c::default_host_target_triple()
                                              : module.module.target_triple;
      const auto helper_target_profile = c4c::target_profile_from_triple(helper_target_triple);
      std::unordered_map<std::string_view, std::string> pointer_param_abi_registers;
      std::unordered_set<std::string_view> byval_pointer_param_names;
      for (std::size_t param_index = 0; param_index < candidate.params.size(); ++param_index) {
        const auto& param = candidate.params[param_index];
        if (param.type != c4c::backend::bir::TypeKind::Ptr || param.is_varargs ||
            !param.abi.has_value()) {
          continue;
        }
        const auto incoming_register =
            c4c::backend::prepare::call_arg_destination_register_name(
                helper_target_profile, *param.abi, param_index);
        if (incoming_register.has_value()) {
          pointer_param_abi_registers.emplace(param.name, *incoming_register);
          if (param.is_byval) {
            byval_pointer_param_names.insert(param.name);
          }
        }
      }
      const auto render_entry_param_home_materialization =
          [&](const c4c::backend::bir::Param& param,
              std::size_t param_index) -> std::optional<std::string> {
            if (param.is_varargs || !param.abi.has_value()) {
              return std::string{};
            }
            const auto incoming_register =
                c4c::backend::prepare::call_arg_destination_register_name(
                    helper_target_profile, *param.abi, param_index);
            if (!incoming_register.has_value()) {
              return std::string{};
            }
            const auto* home = c4c::backend::prepare::find_prepared_value_home(
                module.names, *candidate_function_locations, param.name);
            if (home == nullptr) {
              return std::string{};
            }

            switch (param.type) {
              case c4c::backend::bir::TypeKind::Ptr:
                if (param.is_byval) {
                  return std::string{};
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                    home->register_name.has_value()) {
                  if (*home->register_name == *incoming_register) {
                    return std::string{};
                  }
                  return "    mov " + *home->register_name + ", " + *incoming_register + "\n";
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                    home->offset_bytes.has_value()) {
                  return "    mov " +
                         render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD") +
                         ", " + *incoming_register + "\n";
                }
                return std::string{};
              case c4c::backend::bir::TypeKind::I64:
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                    home->register_name.has_value()) {
                  if (*home->register_name == *incoming_register) {
                    return std::string{};
                  }
                  return "    mov " + *home->register_name + ", " + *incoming_register + "\n";
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                    home->offset_bytes.has_value()) {
                  return "    mov " +
                         render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD") +
                         ", " + *incoming_register + "\n";
                }
                return std::string{};
              case c4c::backend::bir::TypeKind::I32: {
                const auto narrowed_register = narrow_i32_register(*incoming_register);
                if (!narrowed_register.has_value()) {
                  return std::nullopt;
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                    home->register_name.has_value()) {
                  const auto narrowed_home_register = narrow_i32_register(*home->register_name);
                  if (!narrowed_home_register.has_value()) {
                    return std::nullopt;
                  }
                  if (*narrowed_home_register == *narrowed_register) {
                    return std::string{};
                  }
                  return "    mov " + *narrowed_home_register + ", " + *narrowed_register + "\n";
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                    home->offset_bytes.has_value()) {
                  return "    mov " +
                         render_prepared_stack_memory_operand(*home->offset_bytes, "DWORD") +
                         ", " + *narrowed_register + "\n";
                }
                return std::string{};
              }
              case c4c::backend::bir::TypeKind::I8: {
                const auto narrowed_register = narrow_i8_register(*incoming_register);
                if (!narrowed_register.has_value()) {
                  return std::nullopt;
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                    home->register_name.has_value()) {
                  const auto narrowed_home_register = narrow_i8_register(*home->register_name);
                  if (!narrowed_home_register.has_value()) {
                    return std::nullopt;
                  }
                  if (*narrowed_home_register == *narrowed_register) {
                    return std::string{};
                  }
                  return "    mov " + *narrowed_home_register + ", " + *narrowed_register + "\n";
                }
                if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                    home->offset_bytes.has_value()) {
                  return "    mov " +
                         render_prepared_stack_memory_operand(*home->offset_bytes, "BYTE") +
                         ", " + *narrowed_register + "\n";
                }
                return std::string{};
              }
              default:
                return std::string{};
            }
          };
      for (std::size_t param_index = 0; param_index < candidate.params.size(); ++param_index) {
        const auto rendered_param_home =
            render_entry_param_home_materialization(candidate.params[param_index], param_index);
        if (!rendered_param_home.has_value()) {
          return std::nullopt;
        }
        body += *rendered_param_home;
      }
      std::optional<std::string_view> current_i32_name;
      std::optional<std::string_view> previous_i32_name;
      std::optional<std::string_view> current_i8_name;
      std::optional<std::string_view> current_ptr_name;
      std::optional<PreparedCurrentFloatCarrier> current_f32_value;
      std::optional<PreparedCurrentFloatCarrier> current_f64_value;
      std::optional<MaterializedI32Compare> current_materialized_compare;
      std::unordered_map<std::string_view, std::string_view> i64_i32_aliases;
      const auto render_helper_float_load_or_cast =
          [&](const c4c::backend::bir::Inst& inst,
              std::size_t inst_index) -> std::optional<std::string> {
            return render_prepared_float_load_or_cast_inst_if_supported(
                block_context,
                inst,
                inst_index,
                &used_same_module_globals,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_f32_value,
                &current_f64_value,
                &current_materialized_compare);
          };
      const auto render_helper_float_store =
          [&](const c4c::backend::bir::Inst& inst,
              std::size_t inst_index) -> std::optional<std::string> {
            return render_prepared_float_store_inst_if_supported(
                block_context,
                inst,
                inst_index,
                &used_same_module_globals,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_f32_value,
                &current_f64_value,
                &current_materialized_compare);
          };
      const auto render_helper_f128_local_copy =
          [&](const c4c::backend::bir::Inst& inst,
              std::size_t inst_index) -> std::optional<std::string> {
            return render_prepared_f128_local_copy_inst_if_supported(
                block_context,
                inst,
                inst_index,
                &used_same_module_globals,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_materialized_compare);
          };
      const auto render_helper_local_load_without_prepared_access =
          [&](const c4c::backend::bir::Inst& inst) -> std::optional<std::string> {
            const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
            if (load == nullptr || load->address.has_value() ||
                load->result.kind != c4c::backend::bir::Value::Kind::Named) {
              return std::nullopt;
            }
            const auto size_name = prepared_scalar_memory_operand_size_name(load->result.type);
            if (!size_name.has_value()) {
              return std::nullopt;
            }
            std::optional<std::size_t> base_offset;
            const auto slot_it = layout->offsets.find(load->slot_name);
            if (slot_it != layout->offsets.end()) {
              base_offset = slot_it->second;
            } else {
              base_offset = find_prepared_value_home_frame_offset(
                  *layout, &module.names, candidate_function_locations, load->slot_name);
            }
            if (!base_offset.has_value()) {
              return std::nullopt;
            }
            const auto rendered_load = render_prepared_scalar_load_from_memory_if_supported(
                load->result.type,
                render_prepared_stack_memory_operand(*base_offset + load->byte_offset, *size_name));
            if (!rendered_load.has_value()) {
              return std::nullopt;
            }
            std::string body = *rendered_load;
            current_materialized_compare = std::nullopt;
            if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = prepared_current_ptr_carrier_name(
                  load->result.name, &module.names, candidate_function_locations);
              return body;
            }
            current_ptr_name = std::nullopt;
            if (load->result.type == c4c::backend::bir::TypeKind::I8) {
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = load->result.name;
              return body;
            }
            if (load->result.type != c4c::backend::bir::TypeKind::I32) {
              return std::nullopt;
            }
            if (current_i32_name.has_value()) {
              body = "    mov ecx, eax\n" + body;
              previous_i32_name = current_i32_name;
            } else {
              previous_i32_name = std::nullopt;
            }
            current_i32_name = load->result.name;
            current_i8_name = std::nullopt;
            return body;
          };
      const auto render_helper_local_i64_load_or_store_without_prepared_access =
          [&](const c4c::backend::bir::Inst& inst) -> std::optional<std::string> {
            struct PreparedHelperNamedI64Source {
              std::optional<std::string> register_name;
              std::optional<std::string> stack_operand;
              std::optional<std::int64_t> immediate_i64;
            };
            const auto render_slot_memory = [&](std::string_view slot_name,
                                               std::size_t byte_offset,
                                               std::string_view size_name)
                -> std::optional<std::string> {
              std::optional<std::size_t> base_offset;
              const auto slot_it = layout->offsets.find(slot_name);
              if (slot_it != layout->offsets.end()) {
                base_offset = slot_it->second;
              } else {
                base_offset = find_prepared_value_home_frame_offset(
                    *layout, &module.names, candidate_function_locations, slot_name);
              }
              if (!base_offset.has_value()) {
                return std::nullopt;
              }
              return render_prepared_stack_memory_operand(*base_offset + byte_offset, size_name);
            };
            const auto select_named_i64_source = [&](std::string_view value_name)
                -> std::optional<PreparedHelperNamedI64Source> {
              const auto* home = c4c::backend::prepare::find_prepared_value_home(
                  module.names, *candidate_function_locations, value_name);
              if (home == nullptr) {
                return std::nullopt;
              }
              if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                  home->register_name.has_value()) {
                return PreparedHelperNamedI64Source{
                    .register_name = *home->register_name,
                };
              }
              if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
                  home->offset_bytes.has_value()) {
                return PreparedHelperNamedI64Source{
                    .stack_operand =
                        render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD"),
                };
              }
              return std::nullopt;
            };
            const auto append_named_i64_move_into_register =
                [&](std::string_view destination_register,
                    const PreparedHelperNamedI64Source& source,
                    std::string* rendered_body) -> bool {
                  if (source.immediate_i64.has_value()) {
                    *rendered_body += "    mov " + std::string(destination_register) + ", " +
                                      std::to_string(*source.immediate_i64) + "\n";
                    return true;
                  }
                  if (source.register_name.has_value()) {
                    if (*source.register_name != destination_register) {
                      *rendered_body += "    mov " + std::string(destination_register) + ", " +
                                        *source.register_name + "\n";
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

            if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
              if (store->address.has_value() || store->byte_offset != 0 ||
                  store->value.type != c4c::backend::bir::TypeKind::I64) {
                return std::nullopt;
              }
              const auto memory = render_slot_memory(store->slot_name, 0, "QWORD");
              if (!memory.has_value()) {
                return std::nullopt;
              }
              if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
                current_i32_name = std::nullopt;
                previous_i32_name = std::nullopt;
                current_i8_name = std::nullopt;
                current_ptr_name = std::nullopt;
                return "    mov " + *memory + ", " + std::to_string(store->value.immediate) + "\n";
              }
              if (store->value.kind != c4c::backend::bir::Value::Kind::Named) {
                return std::nullopt;
              }
              const auto source = select_named_i64_source(store->value.name);
              if (!source.has_value()) {
                return std::nullopt;
              }
              std::string body;
              if (!append_named_i64_move_into_register("rax", *source, &body)) {
                return std::nullopt;
              }
              body += "    mov " + *memory + ", rax\n";
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              return body;
            }

            const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
            if (load == nullptr || load->address.has_value() || load->byte_offset != 0 ||
                load->result.kind != c4c::backend::bir::Value::Kind::Named ||
                load->result.type != c4c::backend::bir::TypeKind::I64) {
              return std::nullopt;
            }
            const auto source_memory = render_slot_memory(load->slot_name, 0, "QWORD");
            if (!source_memory.has_value()) {
              return std::nullopt;
            }
            current_i32_name = std::nullopt;
            previous_i32_name = std::nullopt;
            current_i8_name = std::nullopt;
            current_ptr_name = std::nullopt;
            return render_prepared_named_qword_load_from_memory_if_supported(
                *layout,
                load->result.name,
                *source_memory,
                &module.names,
                candidate_function_locations);
          };
      const auto render_helper_local_float_load_or_store_without_prepared_access =
          [&](const c4c::backend::bir::Inst& inst) -> std::optional<std::string> {
            const auto render_slot_memory = [&](std::string_view slot_name,
                                               std::size_t byte_offset,
                                               std::string_view size_name)
                -> std::optional<std::string> {
              std::optional<std::size_t> base_offset;
              const auto slot_it = layout->offsets.find(slot_name);
              if (slot_it != layout->offsets.end()) {
                base_offset = slot_it->second;
              } else {
                base_offset = find_prepared_value_home_frame_offset(
                    *layout, &module.names, candidate_function_locations, slot_name);
              }
              if (!base_offset.has_value()) {
                return std::nullopt;
              }
              return render_prepared_stack_memory_operand(*base_offset + byte_offset, size_name);
            };
            const auto scratch_register =
                choose_prepared_float_scratch_register_if_supported(candidate_function_locations);
            if (!scratch_register.has_value()) {
              return std::nullopt;
            }

            if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
              const auto is_float_type = store->value.type == c4c::backend::bir::TypeKind::F32 ||
                                         store->value.type == c4c::backend::bir::TypeKind::F64;
              if (store->address.has_value() || store->byte_offset != 0 || !is_float_type) {
                return std::nullopt;
              }
              const auto size_name =
                  store->value.type == c4c::backend::bir::TypeKind::F32 ? "DWORD" : "QWORD";
              const auto move_mnemonic =
                  store->value.type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
              const auto memory = render_slot_memory(store->slot_name, 0, size_name);
              if (!memory.has_value()) {
                return std::nullopt;
              }
              std::string body;
              if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
                if (store->value.immediate_bits != 0) {
                  return std::nullopt;
                }
                body += "    pxor " + *scratch_register + ", " + *scratch_register + "\n";
                body += "    " + std::string(move_mnemonic) + " " + *memory + ", " +
                        *scratch_register + "\n";
              } else if (store->value.kind == c4c::backend::bir::Value::Kind::Named) {
                const auto rendered_arg = render_prepared_named_float_source_into_register_if_supported(
                    store->value.type,
                    store->value.name,
                    *scratch_register,
                    &*layout,
                    &module.names,
                    candidate_function_locations);
                if (!rendered_arg.has_value()) {
                  return std::nullopt;
                }
                body += *rendered_arg;
                body += "    " + std::string(move_mnemonic) + " " + *memory + ", " +
                        *scratch_register + "\n";
              } else {
                return std::nullopt;
              }
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              return body;
            }

            const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
            const auto is_float_type =
                load != nullptr && (load->result.type == c4c::backend::bir::TypeKind::F32 ||
                                    load->result.type == c4c::backend::bir::TypeKind::F64);
            if (load == nullptr || load->address.has_value() || load->byte_offset != 0 ||
                load->result.kind != c4c::backend::bir::Value::Kind::Named || !is_float_type) {
              return std::nullopt;
            }
            const auto size_name =
                load->result.type == c4c::backend::bir::TypeKind::F32 ? "DWORD" : "QWORD";
            const auto source_memory = render_slot_memory(load->slot_name, 0, size_name);
            if (!source_memory.has_value()) {
              return std::nullopt;
            }
            const auto rendered_load = render_prepared_named_float_load_from_memory_if_supported(
                *layout,
                load->result.type,
                load->result.name,
                *source_memory,
                &module.names,
                candidate_function_locations);
            if (!rendered_load.has_value()) {
              return std::nullopt;
            }
            std::string body = rendered_load->body;
            current_i32_name = std::nullopt;
            previous_i32_name = std::nullopt;
            current_i8_name = std::nullopt;
            current_ptr_name = std::nullopt;
            return body;
          };
      const auto render_pointer_param_load_if_supported =
          [&](const c4c::backend::bir::Inst& inst,
              std::size_t inst_index) -> std::optional<std::string> {
            const auto* prepared_access = find_prepared_function_memory_access(
                candidate_function_addressing, block_context.block_label_id, inst_index);
            if (prepared_access == nullptr ||
                prepared_access->address.base_kind !=
                    c4c::backend::prepare::PreparedAddressBaseKind::PointerValue ||
                !prepared_access->address.pointer_value_name.has_value() ||
                !prepared_access->address.can_use_base_plus_offset) {
              return std::nullopt;
            }
            const auto pointer_name = c4c::backend::prepare::prepared_value_name(
                module.names, *prepared_access->address.pointer_value_name);
            if (pointer_name.empty()) {
              return std::nullopt;
            }
            const auto register_it = pointer_param_abi_registers.find(pointer_name);
            if (register_it == pointer_param_abi_registers.end()) {
              return std::nullopt;
            }
            const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
            if (load == nullptr || !load->address.has_value() ||
                load->result.kind != c4c::backend::bir::Value::Kind::Named) {
              return std::nullopt;
            }
            const auto size_name = prepared_scalar_memory_operand_size_name(load->result.type);
            if (!size_name.has_value()) {
              return std::nullopt;
            }
            std::string memory_operand =
                std::string(*size_name) + " PTR [" + register_it->second;
            if (prepared_access->address.byte_offset > 0) {
              memory_operand += " + " +
                                std::to_string(prepared_access->address.byte_offset);
            } else if (prepared_access->address.byte_offset < 0) {
              memory_operand += " - " +
                                std::to_string(-prepared_access->address.byte_offset);
            }
            memory_operand += "]";

            const auto finish_scalar_load =
                [&](std::string rendered_load) -> std::optional<std::string> {
                  current_materialized_compare = std::nullopt;
                  current_f32_value = std::nullopt;
                  current_f64_value = std::nullopt;
                  if (load->result.type == c4c::backend::bir::TypeKind::Ptr) {
                    if (prepared_pointer_value_has_authoritative_memory_use(
                            load->result.name, &module.names, candidate_function_addressing)) {
                      const auto home_sync = render_prepared_named_ptr_home_sync_if_supported(
                          *layout, load->result.name, &module.names, candidate_function_locations);
                      if (!home_sync.has_value()) {
                        return std::nullopt;
                      }
                      rendered_load += *home_sync;
                    }
                    current_i32_name = std::nullopt;
                    previous_i32_name = std::nullopt;
                    current_i8_name = std::nullopt;
                    current_ptr_name = prepared_current_ptr_carrier_name(
                        load->result.name, &module.names, candidate_function_locations);
                    return rendered_load;
                  }
                  current_ptr_name = std::nullopt;
                  if (load->result.type == c4c::backend::bir::TypeKind::I8) {
                    current_i32_name = std::nullopt;
                    previous_i32_name = std::nullopt;
                    current_i8_name = load->result.name;
                    return rendered_load;
                  }
                  if (load->result.type == c4c::backend::bir::TypeKind::I32) {
                    const auto home_sync = render_prepared_named_i32_stack_home_sync_if_supported(
                        *layout, load->result.name, &module.names, candidate_function_locations);
                    if (!home_sync.has_value()) {
                      return std::nullopt;
                    }
                    rendered_load += *home_sync;
                    if (current_i32_name.has_value()) {
                      rendered_load = "    mov ecx, eax\n" + rendered_load;
                      previous_i32_name = current_i32_name;
                    } else {
                      previous_i32_name = std::nullopt;
                    }
                    current_i32_name = load->result.name;
                    current_i8_name = std::nullopt;
                    return rendered_load;
                  }
                  return std::nullopt;
                };

            if (load->result.type == c4c::backend::bir::TypeKind::F32 ||
                load->result.type == c4c::backend::bir::TypeKind::F64) {
              const auto* home = find_prepared_named_value_home_if_supported(
                  &module.names, candidate_function_locations, load->result.name);
              std::string destination_register;
              if (home != nullptr &&
                  home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
                  home->register_name.has_value()) {
                destination_register = *home->register_name;
              } else {
                const auto scratch_register = choose_prepared_float_scratch_register_if_supported(
                    candidate_function_locations);
                if (!scratch_register.has_value()) {
                  return std::nullopt;
                }
                destination_register = *scratch_register;
              }
              const auto move_mnemonic =
                  load->result.type == c4c::backend::bir::TypeKind::F32 ? "movss" : "movsd";
              std::string rendered =
                  "    " + std::string(move_mnemonic) + " " + destination_register + ", " +
                  memory_operand + "\n";
              if (!append_prepared_float_home_sync_if_supported(
                      &rendered,
                      load->result.type,
                      destination_register,
                      load->result.name,
                      &*layout,
                      &module.names,
                      candidate_function_locations)) {
                return std::nullopt;
              }
              current_materialized_compare = std::nullopt;
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              if (load->result.type == c4c::backend::bir::TypeKind::F32) {
                current_f32_value = PreparedCurrentFloatCarrier{
                    .value_name = load->result.name,
                    .register_name = destination_register,
                };
                current_f64_value = std::nullopt;
              } else {
                current_f32_value = std::nullopt;
                current_f64_value = PreparedCurrentFloatCarrier{
                    .value_name = load->result.name,
                    .register_name = destination_register,
                };
              }
              return rendered;
            }

            if (load->result.type == c4c::backend::bir::TypeKind::F128) {
              const auto destination_memory_operand =
                  render_prepared_named_f128_source_memory_operand_if_supported(
                      load->result.name,
                      0,
                      &*layout,
                      &candidate,
                      &module.names,
                      candidate_function_locations);
              if (!destination_memory_operand.has_value()) {
                return std::nullopt;
              }
              current_materialized_compare = std::nullopt;
              current_i32_name = std::nullopt;
              previous_i32_name = std::nullopt;
              current_i8_name = std::nullopt;
              current_ptr_name = std::nullopt;
              current_f32_value = std::nullopt;
              current_f64_value = std::nullopt;
              return "    fld " + memory_operand + "\n    fstp " + *destination_memory_operand +
                     "\n";
            }

            const auto rendered_load = render_prepared_scalar_load_from_memory_if_supported(
                load->result.type, memory_operand);
            if (!rendered_load.has_value()) {
              return std::nullopt;
            }
            return finish_scalar_load(*rendered_load);
          };
      const auto render_pointer_param_home_refresh_if_needed =
          [&](std::size_t inst_index) -> std::optional<std::string> {
            const auto* prepared_access = find_prepared_function_memory_access(
                candidate_function_addressing, block_context.block_label_id, inst_index);
            if (prepared_access == nullptr ||
                prepared_access->address.base_kind !=
                    c4c::backend::prepare::PreparedAddressBaseKind::PointerValue ||
                !prepared_access->address.pointer_value_name.has_value()) {
              return std::string{};
            }
            const auto pointer_name = c4c::backend::prepare::prepared_value_name(
                module.names, *prepared_access->address.pointer_value_name);
            if (pointer_name.empty()) {
              return std::string{};
            }
            if (byval_pointer_param_names.find(pointer_name) != byval_pointer_param_names.end()) {
              return std::string{};
            }
            const auto register_it = pointer_param_abi_registers.find(pointer_name);
            if (register_it == pointer_param_abi_registers.end()) {
              return std::string{};
            }
            const auto* home = c4c::backend::prepare::find_prepared_value_home(
                module.names, *candidate_function_locations, pointer_name);
            if (home == nullptr ||
                home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
                !home->offset_bytes.has_value()) {
              return std::string{};
            }
            return "    mov " +
                   render_prepared_stack_memory_operand(*home->offset_bytes, "QWORD") + ", " +
                   register_it->second + "\n";
          };
      for (std::size_t inst_index = 0; inst_index < candidate.blocks.front().insts.size(); ++inst_index) {
        const auto& inst = candidate.blocks.front().insts[inst_index];
        if (const auto rendered_pointer_param_load =
                render_pointer_param_load_if_supported(inst, inst_index);
            rendered_pointer_param_load.has_value()) {
          body += *rendered_pointer_param_load;
          continue;
        }
        const auto rendered_pointer_param_home =
            render_pointer_param_home_refresh_if_needed(inst_index);
        if (!rendered_pointer_param_home.has_value()) {
          return std::nullopt;
        }
        body += *rendered_pointer_param_home;
        if (const auto rendered_float = render_helper_float_load_or_cast(inst, inst_index);
            rendered_float.has_value()) {
          body += *rendered_float;
          continue;
        }
        if (const auto rendered_f128 = render_helper_f128_local_copy(inst, inst_index);
            rendered_f128.has_value()) {
          body += *rendered_f128;
          continue;
        }
        if (const auto rendered_load = render_prepared_scalar_load_inst_if_supported(
                inst,
                *layout,
                &module.names,
                candidate_function_locations,
                candidate_function_addressing,
                block_context.block_label_id,
                inst_index,
                render_asm_symbol_name,
                find_same_module_global,
                [&](const c4c::backend::bir::Global& global,
                    c4c::backend::bir::TypeKind type,
                    std::int64_t byte_offset) {
                  return byte_offset >= 0 &&
                         same_module_global_supports_scalar_load(
                             global, type, static_cast<std::size_t>(byte_offset));
                },
                &used_same_module_globals,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_materialized_compare);
            rendered_load.has_value()) {
          body += *rendered_load;
          continue;
        }
        if (const auto rendered_load_without_access =
                render_helper_local_load_without_prepared_access(inst);
            rendered_load_without_access.has_value()) {
          body += *rendered_load_without_access;
          continue;
        }
        if (const auto rendered_i64_local =
                render_helper_local_i64_load_or_store_without_prepared_access(inst);
            rendered_i64_local.has_value()) {
          body += *rendered_i64_local;
          continue;
        }
        if (const auto rendered_float_local =
                render_helper_local_float_load_or_store_without_prepared_access(inst);
            rendered_float_local.has_value()) {
          body += *rendered_float_local;
          continue;
        }
        if (const auto rendered_float_store = render_helper_float_store(inst, inst_index);
            rendered_float_store.has_value()) {
          body += *rendered_float_store;
          continue;
        }
        if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst)) {
          const auto rendered_cast = render_prepared_cast_inst_if_supported(
              *cast,
              &module.names,
              candidate_function_locations,
              &current_i32_name,
              &previous_i32_name,
              &current_i8_name,
              &current_ptr_name,
              &current_materialized_compare,
              &i64_i32_aliases);
          if (rendered_cast.has_value()) {
            body += *rendered_cast;
            continue;
          }
        }
        if (const auto rendered_store = render_prepared_scalar_store_inst_if_supported(
                inst,
                *layout,
                &candidate.blocks.front(),
                &module.names,
                candidate_function_locations,
                candidate_function_addressing,
                block_context.block_label_id,
                inst_index,
                render_asm_symbol_name,
                find_same_module_global,
                [&](const c4c::backend::bir::Global& global,
                    c4c::backend::bir::TypeKind type,
                    std::int64_t byte_offset) {
                  return byte_offset >= 0 &&
                         same_module_global_supports_scalar_load(
                             global, type, static_cast<std::size_t>(byte_offset));
                },
                &used_same_module_globals,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_materialized_compare);
            rendered_store.has_value()) {
          body += *rendered_store;
          continue;
        }
        if (const auto rendered_call = render_prepared_block_direct_extern_call_inst_if_supported(
                block_context,
                inst,
                &current_i32_name,
                &previous_i32_name,
                &current_i8_name,
                &current_ptr_name,
                &current_f32_value,
                &current_f64_value,
                &current_materialized_compare);
            rendered_call.has_value()) {
          body += *rendered_call;
          continue;
        }
        if (const auto rendered_helper_call =
                render_prepared_block_same_module_helper_call_inst_if_supported(
                    block_context,
                    inst,
                    &current_i32_name,
                    &previous_i32_name,
                    &current_i8_name,
                    &current_ptr_name,
                    &current_materialized_compare);
            rendered_helper_call.has_value()) {
          body += *rendered_helper_call;
          current_f32_value = std::nullopt;
          current_f64_value = std::nullopt;
          continue;
        }
        if (const auto rendered_same_module_call =
                render_prepared_block_same_module_call_inst_if_supported(
                    block_context,
                    inst,
                    &current_i32_name,
                    &previous_i32_name,
                    &current_i8_name,
                    &current_ptr_name,
                    &current_f32_value,
                    &current_f64_value,
                    &current_materialized_compare);
            rendered_same_module_call.has_value()) {
          body += *rendered_same_module_call;
          current_f32_value = std::nullopt;
          current_f64_value = std::nullopt;
          continue;
        }
        return std::nullopt;
      }
      const auto rendered_return = render_prepared_void_block_return_terminator_if_supported(
          block_context, std::move(body), {});
      if (!rendered_return.has_value()) {
        return std::nullopt;
      }
      return candidate_asm_prefix + *rendered_return;
    };
    const auto rendered_void_helper = rendered_void_direct_extern_helper();
    const auto rendered_asm =
        rendered_void_helper.has_value()
            ? rendered_void_helper
            : render_prepared_local_slot_guard_chain_if_supported(function_dispatch_context);
    if (!rendered_asm.has_value()) {
      return std::nullopt;
    }
    return BoundedSameModuleHelperRender{
        .asm_text = std::move(*rendered_asm),
        .used_string_names = std::move(used_string_names),
        .used_same_module_globals = std::move(used_same_module_globals),
    };
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

    const auto candidate_return_register = minimal_function_return_register(candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }
    if (const auto rendered_minimal =
            render_prepared_minimal_immediate_or_param_return_if_supported(
                candidate,
                candidate.blocks.front(),
                prepared_arch,
                minimal_function_asm_prefix(candidate),
                *candidate_return_register,
                [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
                  auto param_it = candidate.params.begin();
                  for (std::size_t param_index = 0; param_it != candidate.params.end();
                       ++param_it, ++param_index) {
                    if (&*param_it == &param) {
                      return minimal_param_register_at(param, param_index);
                    }
                  }
                  return std::nullopt;
                });
        rendered_minimal.has_value()) {
      return BoundedSameModuleHelperRender{
          .asm_text = std::move(*rendered_minimal),
          .used_same_module_globals = {},
      };
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
          return render_prepared_i32_move_to_register_if_supported(
              value,
              current_i32_name,
              "eax",
              [&](std::string_view value_name) -> std::optional<std::string> {
                const auto param_it = param_registers.find(value_name);
                if (param_it == param_registers.end()) {
                  return std::nullopt;
                }
                return param_it->second;
              });
        };
    const auto render_i32_operand =
        [&](const c4c::backend::bir::Value& value,
            const std::optional<std::string_view>& current_i32_name) -> std::optional<std::string> {
          return render_prepared_i32_operand_if_supported(
              value,
              current_i32_name,
              [&](std::string_view value_name) -> std::optional<std::string> {
                const auto param_it = param_registers.find(value_name);
                if (param_it == param_registers.end()) {
                  return std::nullopt;
                }
                return param_it->second;
              });
        };
    const auto render_supported_helper_inst =
        [&](const c4c::backend::bir::Inst& inst,
            std::size_t inst_index,
            std::optional<std::string_view>* current_i32_name)
        -> std::optional<BoundedSameModuleHelperRender> {
      if (current_i32_name == nullptr) {
        return std::nullopt;
      }
      if (const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&inst)) {
        if (binary->result.kind != c4c::backend::bir::Value::Kind::Named) {
          return std::nullopt;
        }
        const auto rendered_binary = render_prepared_i32_binary_in_eax_if_supported(
            *binary, *current_i32_name, render_value_to_eax, render_i32_operand);
        if (!rendered_binary.has_value()) {
          return std::nullopt;
        }
        *current_i32_name = binary->result.name;
        return BoundedSameModuleHelperRender{
            .asm_text = std::move(*rendered_binary),
            .used_same_module_globals = {},
        };
      }

      const auto* store = std::get_if<c4c::backend::bir::StoreGlobalInst>(&inst);
      if (store == nullptr || store->address.has_value() || store->byte_offset != 0 ||
          store->value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      const auto selected_global_memory =
          select_prepared_same_module_scalar_memory_for_inst_if_supported(
              module.names,
              candidate_function_addressing,
              entry_block_label_id,
              inst_index,
              c4c::backend::bir::TypeKind::I32,
              render_asm_symbol_name,
              find_same_module_global,
              [](const c4c::backend::bir::Global& global, std::int64_t byte_offset) {
                return global.type == c4c::backend::bir::TypeKind::I32 && byte_offset == 0;
              });
      if (!selected_global_memory.has_value()) {
        return std::nullopt;
      }

      const auto rendered_store = render_prepared_i32_store_to_memory_if_supported(
          store->value,
          *current_i32_name,
          selected_global_memory->memory_operand,
          [&](const c4c::backend::bir::Value& value,
              const std::optional<std::string_view>& current_name) -> std::optional<std::string> {
            return render_prepared_i32_operand_if_supported(
                value,
                current_name,
                [&](std::string_view value_name) -> std::optional<std::string> {
                  const auto param_it = param_registers.find(value_name);
                  if (param_it == param_registers.end()) {
                    return std::nullopt;
                  }
                  return param_it->second;
                });
          });
      if (!rendered_store.has_value()) {
        return std::nullopt;
      }
      BoundedSameModuleHelperRender rendered{
          .asm_text = std::move(*rendered_store),
          .used_same_module_globals = {},
      };
      rendered.used_same_module_globals.insert(selected_global_memory->global->name);
      return rendered;
    };
    std::unordered_set<std::string_view> used_same_module_globals;
    std::string body;
    std::optional<std::string_view> current_i32_name;
    for (const auto& inst : candidate.blocks.front().insts) {
      const auto inst_index = static_cast<std::size_t>(&inst - candidate.blocks.front().insts.data());
      const auto rendered_inst =
          render_supported_helper_inst(inst, inst_index, &current_i32_name);
      if (!rendered_inst.has_value()) {
        return std::nullopt;
      }
      body += rendered_inst->asm_text;
      used_same_module_globals.insert(
          rendered_inst->used_same_module_globals.begin(),
          rendered_inst->used_same_module_globals.end());
    }

    const auto& returned = *candidate.blocks.front().terminator.value;
    const auto render_return_to_register = render_prepared_i32_move_to_register_if_supported(
        returned,
        current_i32_name,
        *candidate_return_register,
        [&](std::string_view value_name) -> std::optional<std::string> {
          const auto param_it = param_registers.find(value_name);
          if (param_it == param_registers.end()) {
            return std::nullopt;
          }
          return param_it->second;
        });
    if (!render_return_to_register.has_value()) {
      return std::nullopt;
    }
    body += *render_return_to_register;
    body += "    ret\n";
    return BoundedSameModuleHelperRender{
        .asm_text = minimal_function_asm_prefix(candidate) + body,
        .used_same_module_globals = std::move(used_same_module_globals),
    };
  };

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
    if (const auto rendered_generic =
            render_generic_bounded_same_module_helper_function_if_supported(*candidate);
        rendered_generic.has_value()) {
      for (const auto string_name : rendered_generic->used_string_names) {
        rendered_helpers.helper_string_names.insert(string_name);
      }
      for (const auto global_name : rendered_generic->used_same_module_globals) {
        rendered_helpers.helper_global_names.insert(global_name);
      }
      rendered_helpers.helper_names.insert(candidate->name);
      rendered_helpers.helper_prefix += rendered_generic->asm_text;
      continue;
    }
    const auto rendered_helper =
        render_bounded_same_module_helper_function_if_supported(*candidate);
    if (!rendered_helper.has_value()) {
      return std::nullopt;
    }
    for (const auto string_name : rendered_helper->used_string_names) {
      rendered_helpers.helper_string_names.insert(string_name);
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
    const std::unordered_set<std::string_view>& helper_string_names,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  std::string rendered_data;
  std::unordered_set<std::string_view> emitted_string_names;
  std::unordered_set<std::string_view> used_same_module_globals(
      rendered_module.used_same_module_global_names.begin(),
      rendered_module.used_same_module_global_names.end());
  for (const auto& string_name : helper_string_names) {
    const auto* string_constant = find_string_constant(string_name);
    if (string_constant == nullptr || !emitted_string_names.insert(string_name).second) {
      continue;
    }
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& string_name : rendered_module.used_string_names) {
    const auto* string_constant = find_string_constant(string_name);
    if (string_constant == nullptr || !emitted_string_names.insert(string_name).second) {
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
    const std::unordered_set<std::string_view>& helper_names,
    const std::unordered_set<std::string_view>& helper_string_names,
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
      module, defined_functions, prepared_arch, helper_names, render_trivial_defined_function,
      minimal_function_return_register, minimal_function_asm_prefix, has_string_constant,
      has_same_module_global, render_private_data_label, render_asm_symbol_name);
  if (!rendered_module.has_value()) {
    return std::nullopt;
  }

  const auto rendered_data = render_prepared_bounded_multi_defined_call_lane_data_if_supported(
      *rendered_module, module.module, helper_string_names, helper_global_names, find_string_constant,
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
    const std::function<bool(const c4c::backend::bir::Global&,
                             c4c::backend::bir::TypeKind,
                             std::size_t)>& same_module_global_supports_scalar_load,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Param&, std::size_t)>&
        minimal_param_register_at,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
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
            minimal_function_asm_prefix, find_same_module_global,
            same_module_global_supports_scalar_load, minimal_param_register_at,
            render_private_data_label, render_asm_symbol_name);
        helpers.has_value()) {
      state.helper_prefix = helpers->helper_prefix;
      state.helper_names = std::move(helpers->helper_names);
      state.helper_string_names = std::move(helpers->helper_string_names);
      state.helper_global_names = std::move(helpers->helper_global_names);
      state.has_bounded_same_module_helpers = true;
    }
  }
  state.rendered_module = render_prepared_bounded_multi_defined_call_lane_if_supported(
      module, defined_functions, prepared_arch, state.helper_names, state.helper_string_names,
      state.helper_global_names,
      render_trivial_defined_function, minimal_function_return_register, minimal_function_asm_prefix,
      has_string_constant, has_same_module_global, render_private_data_label, render_asm_symbol_name,
      find_string_constant, emit_string_constant_data, emit_same_module_global_data);
  return state;
}

}  // namespace c4c::backend::x86
