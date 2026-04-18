#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

std::string emit_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& module) {
  const auto resolved_target_profile = module.target_profile.arch != c4c::TargetArch::Unknown
                                           ? module.target_profile
                                           : c4c::target_profile_from_triple(
                                                 module.module.target_triple.empty()
                                                     ? c4c::default_host_target_triple()
                                                     : module.module.target_triple);
  const auto prepared_arch = resolved_target_profile.arch;
  if (prepared_arch != c4c::TargetArch::X86_64 && prepared_arch != c4c::TargetArch::I686) {
    throw std::invalid_argument(
        "x86 backend emitter requires an x86 target for the canonical prepared-module handoff");
  }

  std::vector<const c4c::backend::bir::Function*> defined_functions;
  const c4c::backend::bir::Function* function_ptr = nullptr;
  for (const auto& candidate : module.module.functions) {
    if (candidate.is_declaration) {
      continue;
    }
    defined_functions.push_back(&candidate);
    if (candidate.name == "main") {
      function_ptr = &candidate;
    } else if (function_ptr == nullptr) {
      function_ptr = &candidate;
    }
  }
  if (function_ptr == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  }

  const auto& function = *function_ptr;
  const auto asm_prefix = ".intel_syntax noprefix\n.text\n.globl " + function.name +
                          "\n.type " + function.name + ", @function\n" + function.name + ":\n";
  const auto narrow_abi_register = [](std::string_view wide_register) -> std::optional<std::string> {
    if (wide_register == "rax") return std::string("eax");
    if (wide_register == "rdi") return std::string("edi");
    if (wide_register == "rsi") return std::string("esi");
    if (wide_register == "rdx") return std::string("edx");
    if (wide_register == "rcx") return std::string("ecx");
    if (wide_register == "r8") return std::string("r8d");
    if (wide_register == "r9") return std::string("r9d");
    return std::string(wide_register);
  };
  const auto narrow_register = [&](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return narrow_abi_register(*wide_register);
  };
  const auto return_register = [&]() -> std::optional<std::string> {
    if (!function.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_result_destination_register_name(
        resolved_target_profile, *function.return_abi));
  }();
  if (!return_register.has_value()) {
    throw std::invalid_argument(
        "x86 backend emitter requires prepared return ABI metadata for the canonical prepared-module handoff");
  }
  const auto minimal_param_register_at =
      [&](const c4c::backend::bir::Param& param,
          std::size_t arg_index) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_arg_destination_register_name(
        resolved_target_profile, *param.abi, arg_index));
  };
  const auto minimal_param_register =
      [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    return minimal_param_register_at(param, 0);
  };
  const auto find_block = [&](std::string_view label) -> const c4c::backend::bir::Block* {
    for (const auto& block : function.blocks) {
      if (block.label == label) {
        return &block;
      }
    }
    return nullptr;
  };
  const auto find_control_flow_function =
      [&]() -> const c4c::backend::prepare::PreparedControlFlowFunction* {
    for (const auto& control_flow_function : module.control_flow.functions) {
      if (control_flow_function.function_name == function.name) {
        return &control_flow_function;
      }
    }
    return nullptr;
  };
  struct LocalSlotLayout {
    std::unordered_map<std::string_view, std::size_t> offsets;
    std::size_t frame_size = 0;
  };
  const auto align_up = [](std::size_t value, std::size_t align) -> std::size_t {
    if (align <= 1) {
      return value;
    }
    const auto remainder = value % align;
    return remainder == 0 ? value : value + (align - remainder);
  };
  const auto build_local_slot_layout = [&]() -> std::optional<LocalSlotLayout> {
    if (prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }
    LocalSlotLayout layout;
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
  };
  const auto render_stack_memory_operand =
      [](std::size_t byte_offset, std::string_view size_name) -> std::string {
    if (byte_offset == 0) {
      return std::string(size_name) + " PTR [rsp]";
    }
    return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
  };
  const auto render_stack_address_expr = [](std::size_t byte_offset) -> std::string {
    if (byte_offset == 0) {
      return "[rsp]";
    }
    return "[rsp + " + std::to_string(byte_offset) + "]";
  };
  const auto wrap_i32 = [](std::int64_t value) -> std::int32_t {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(value));
  };
  const auto render_local_address_operand_if_supported =
      [&](const LocalSlotLayout& local_layout,
          const std::optional<c4c::backend::bir::MemoryAddress>& address,
          std::string_view size_name) -> std::optional<std::string> {
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
    return render_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset), size_name);
  };
  const auto resolved_target_triple =
      module.module.target_triple.empty() ? c4c::default_host_target_triple()
                                          : module.module.target_triple;
  const auto render_asm_symbol_name = [&](std::string_view logical_name) -> std::string {
    if (resolved_target_triple.find("apple-darwin") != std::string::npos) {
      return "_" + std::string(logical_name);
    }
    return std::string(logical_name);
  };
  const auto render_private_data_label = [&](std::string_view pool_name) -> std::string {
    std::string label(pool_name);
    if (!label.empty() && label.front() == '@') {
      label.erase(label.begin());
    }
    while (!label.empty() && label.front() == '.') {
      label.erase(label.begin());
    }
    if (resolved_target_triple.find("apple-darwin") != std::string::npos) {
      return "L" + label;
    }
    return ".L." + label;
  };
  const auto escape_asm_bytes = [](std::string_view raw_bytes) -> std::string {
    std::string escaped;
    escaped.reserve(raw_bytes.size());
    for (const unsigned char ch : raw_bytes) {
      switch (ch) {
        case '\\':
          escaped += "\\\\";
          break;
        case '"':
          escaped += "\\\"";
          break;
        case '\n':
          escaped += "\\n";
          break;
        case '\t':
          escaped += "\\t";
          break;
        case '\r':
          escaped += "\\r";
          break;
        case '\0':
          break;
        default:
          escaped.push_back(static_cast<char>(ch));
          break;
      }
    }
    return escaped;
  };
  const auto find_string_constant =
      [&](std::string_view name) -> const c4c::backend::bir::StringConstant* {
    for (const auto& string_constant : module.module.string_constants) {
      if (string_constant.name == name) {
        return &string_constant;
      }
    }
    return nullptr;
  };
  const auto emit_string_constant_data =
      [&](const c4c::backend::bir::StringConstant& string_constant) -> std::string {
    const auto label = render_private_data_label("@" + string_constant.name);
    return ".section .rodata\n" + label + ":\n    .asciz \"" +
           escape_asm_bytes(string_constant.bytes) + "\"\n";
  };
  const auto emit_defined_global_prelude =
      [&](std::string_view symbol_name, std::size_t align_bytes, bool is_zero_init) -> std::string {
    std::string prelude = is_zero_init ? ".bss\n" : ".data\n";
    prelude += ".globl " + std::string(symbol_name) + "\n";
    if (resolved_target_triple.find("apple-darwin") == std::string::npos) {
      prelude += ".type " + std::string(symbol_name) + ", @object\n";
    }
    if (align_bytes > 1) {
      prelude += ".p2align " + std::to_string(align_bytes == 2 ? 1 : 2) + "\n";
    }
    prelude += std::string(symbol_name) + ":\n";
    return prelude;
  };
  const auto find_same_module_global =
      [&](std::string_view name) -> const c4c::backend::bir::Global* {
    for (const auto& global : module.module.globals) {
      if (global.name == name) {
        if (global.is_extern || global.is_thread_local) {
          return nullptr;
        }
        return &global;
      }
    }
    return nullptr;
  };
  const auto same_module_global_scalar_size =
      [&](c4c::backend::bir::TypeKind type) -> std::optional<std::size_t> {
    switch (type) {
      case c4c::backend::bir::TypeKind::I8:
        return 1;
      case c4c::backend::bir::TypeKind::I32:
        return 4;
      case c4c::backend::bir::TypeKind::Ptr:
        return 8;
      default:
        return std::nullopt;
    }
  };
  const auto same_module_global_supports_scalar_load =
      [&](const c4c::backend::bir::Global& global,
          c4c::backend::bir::TypeKind type,
          std::size_t byte_offset) -> bool {
    if (!same_module_global_scalar_size(type).has_value()) {
      return false;
    }
    if (global.initializer_symbol_name.has_value()) {
      return type == c4c::backend::bir::TypeKind::Ptr && byte_offset == 0;
    }
    if (global.initializer.has_value()) {
      const auto init_size = same_module_global_scalar_size(global.initializer->type);
      return global.initializer->kind == c4c::backend::bir::Value::Kind::Immediate &&
             init_size.has_value() && global.initializer->type == type && byte_offset == 0;
    }
    std::size_t current_offset = 0;
    for (const auto& element : global.initializer_elements) {
      const auto element_size = same_module_global_scalar_size(element.type);
      if (!element_size.has_value()) {
        return false;
      }
      if (current_offset == byte_offset && element.type == type) {
        return true;
      }
      current_offset += *element_size;
    }
    return false;
  };
  const auto same_module_global_is_zero_initialized =
      [&](const c4c::backend::bir::Global& global) -> bool {
    if (global.initializer_symbol_name.has_value()) {
      return false;
    }
    if (global.initializer.has_value()) {
      if (global.initializer->kind != c4c::backend::bir::Value::Kind::Immediate) {
        return false;
      }
      return global.initializer->immediate == 0 && global.initializer->immediate_bits == 0;
    }
    if (global.initializer_elements.empty()) {
      return false;
    }
    for (const auto& element : global.initializer_elements) {
      if (element.kind == c4c::backend::bir::Value::Kind::Named) {
        return false;
      }
      if (element.immediate != 0 || element.immediate_bits != 0) {
        return false;
      }
    }
    return true;
  };
  const auto emit_same_module_global_data =
      [&](const c4c::backend::bir::Global& global) -> std::optional<std::string> {
    const auto symbol_name = render_asm_symbol_name(global.name);
    const auto append_initializer_value = [&](std::string* data,
                                              const c4c::backend::bir::Value& value) -> bool {
      if (value.kind == c4c::backend::bir::Value::Kind::Named) {
        if (value.type != c4c::backend::bir::TypeKind::Ptr || value.name.empty() ||
            value.name.front() != '@') {
          return false;
        }
        *data += "    .quad " + render_asm_symbol_name(value.name.substr(1)) + "\n";
        return true;
      }
      switch (value.type) {
        case c4c::backend::bir::TypeKind::I8:
          *data += "    .byte " + std::to_string(static_cast<std::int32_t>(
                                      static_cast<std::int8_t>(value.immediate))) +
                   "\n";
          return true;
        case c4c::backend::bir::TypeKind::I32:
          *data += "    .long " +
                   std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
          return true;
        case c4c::backend::bir::TypeKind::Ptr:
          if (value.immediate != 0 || value.immediate_bits != 0) {
            return false;
          }
          *data += "    .quad 0\n";
          return true;
        default:
          return false;
      }
    };
    std::string data = emit_defined_global_prelude(symbol_name,
                                                   global.align_bytes > 0 ? global.align_bytes : 4,
                                                   same_module_global_is_zero_initialized(global));
    if (global.initializer_symbol_name.has_value()) {
      data += "    .quad " + render_asm_symbol_name(*global.initializer_symbol_name) + "\n";
      return data;
    }
    if (global.initializer.has_value()) {
      if (!append_initializer_value(&data, *global.initializer)) {
        return std::nullopt;
      }
      return data;
    }
    if (global.initializer_elements.empty()) {
      return std::nullopt;
    }
    for (const auto& element : global.initializer_elements) {
      if (!append_initializer_value(&data, element)) {
        return std::nullopt;
      }
    }
    return data;
  };
  const auto minimal_function_return_register =
      [&](const c4c::backend::bir::Function& candidate) -> std::optional<std::string> {
    if (!candidate.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_result_destination_register_name(
        resolved_target_profile, *candidate.return_abi));
  };
  const auto minimal_function_asm_prefix =
      [&](const c4c::backend::bir::Function& candidate) -> std::string {
    return ".intel_syntax noprefix\n.text\n.globl " + candidate.name + "\n.type " + candidate.name +
           ", @function\n" + candidate.name + ":\n";
  };
  const auto render_trivial_defined_function_if_supported =
      [&](const c4c::backend::bir::Function& candidate) -> std::optional<std::string> {
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
           std::to_string(
               static_cast<std::int32_t>(candidate_entry.terminator.value->immediate)) +
           "\n    ret\n";
  };
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

          const auto render_rhs = [&](const c4c::backend::bir::Value& rhs)
              -> std::optional<std::string> {
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
  std::optional<std::string> bounded_same_module_helper_prefix;
  std::unordered_set<std::string_view> bounded_same_module_helper_names;
  std::unordered_set<std::string_view> bounded_same_module_helper_global_names;
  if (defined_functions.size() > 1 && function_ptr != nullptr && function_ptr->name == "main") {
    std::string rendered_helpers;
    bool helpers_rendered = true;
    for (const auto* candidate : defined_functions) {
      if (candidate == function_ptr) {
        continue;
      }
      if (const auto rendered_trivial = render_trivial_defined_function_if_supported(*candidate);
          rendered_trivial.has_value()) {
        bounded_same_module_helper_names.insert(candidate->name);
        rendered_helpers += *rendered_trivial;
        continue;
      }
      const auto rendered_helper =
          render_bounded_same_module_helper_function_if_supported(*candidate);
      if (!rendered_helper.has_value()) {
        helpers_rendered = false;
        bounded_same_module_helper_names.clear();
        bounded_same_module_helper_global_names.clear();
        break;
      }
      for (const auto global_name : rendered_helper->used_same_module_globals) {
        bounded_same_module_helper_global_names.insert(global_name);
      }
      bounded_same_module_helper_names.insert(candidate->name);
      rendered_helpers += rendered_helper->asm_text;
    }
    if (helpers_rendered) {
      bounded_same_module_helper_prefix = std::move(rendered_helpers);
    }
  }
  const auto prepend_bounded_same_module_helpers = [&](std::string asm_text) {
    if (bounded_same_module_helper_prefix.has_value()) {
      return *bounded_same_module_helper_prefix + asm_text;
    }
    return asm_text;
  };
  const auto throw_multi_defined_contract = [&]() -> void {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  };
  const auto throw_multi_defined_contract_if_active = [&]() -> void {
    if (defined_functions.size() > 1 && bounded_same_module_helper_prefix.has_value()) {
      throw_multi_defined_contract();
    }
  };
  const auto render_bounded_multi_defined_call_lane_if_supported =
      [&]() -> std::optional<std::string> {
    if (defined_functions.size() <= 1 || prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }

    static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    std::unordered_set<std::string_view> emitted_string_names;
    std::vector<const c4c::backend::bir::StringConstant*> used_string_constants;
    std::unordered_set<std::string_view> used_same_module_globals;
    std::string rendered_functions;
    bool saw_bounded_entry = false;

    for (const auto* candidate : defined_functions) {
      if (const auto rendered_trivial = render_trivial_defined_function_if_supported(*candidate);
          rendered_trivial.has_value()) {
        rendered_functions += *rendered_trivial;
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

      struct CandidateLocalSlotLayout {
        std::unordered_map<std::string_view, std::size_t> offsets;
        std::size_t frame_size = 0;
      };
      const auto build_candidate_local_slot_layout = [&]() -> std::optional<CandidateLocalSlotLayout> {
        CandidateLocalSlotLayout layout;
        std::size_t next_offset = 0;
        std::size_t max_align = 16;
        for (const auto& slot : candidate->local_slots) {
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
      };
      const auto candidate_layout = build_candidate_local_slot_layout();
      if (!candidate_layout.has_value()) {
        return std::nullopt;
      }
      const auto candidate_stack_adjust = candidate_layout->frame_size + 8;
      const auto render_candidate_local_i32 = [&](std::string_view slot_name) -> std::optional<std::string> {
        const auto slot_it = candidate_layout->offsets.find(slot_name);
        if (slot_it == candidate_layout->offsets.end()) {
          return std::nullopt;
        }
        return render_stack_memory_operand(slot_it->second + 8, "DWORD");
      };

      std::string body = "    sub rsp, " + std::to_string(candidate_stack_adjust) + "\n";
      std::optional<std::string_view> current_i32_name;
      for (const auto& inst : candidate->blocks.front().insts) {
        if (const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst)) {
          if (call->args.size() != call->arg_types.size() || call->args.size() > 6 ||
              (call->result.has_value() &&
               call->result->type != c4c::backend::bir::TypeKind::I32 &&
               call->result->type != c4c::backend::bir::TypeKind::Void)) {
            return std::nullopt;
          }
          std::string callee_name;
          if (!call->is_indirect) {
            if (call->callee.empty() || call->callee_value.has_value()) {
              return std::nullopt;
            }
            callee_name = call->callee;
          } else {
            if (!call->callee.empty() || !call->callee_value.has_value() ||
                call->callee_value->kind != c4c::backend::bir::Value::Kind::Named ||
                call->callee_value->type != c4c::backend::bir::TypeKind::Ptr ||
                call->callee_value->name.empty() || call->callee_value->name.front() != '@') {
              return std::nullopt;
            }
            callee_name = std::string(call->callee_value->name.substr(1));
            bool found_same_module_function = false;
            for (const auto* available_function : defined_functions) {
              if (available_function->name == callee_name) {
                found_same_module_function = true;
                break;
              }
            }
            if (!found_same_module_function) {
              return std::nullopt;
            }
          }

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
            if (arg.kind == c4c::backend::bir::Value::Kind::Immediate) {
              body += "    mov ";
              body += kArgRegs32[arg_index];
              body += ", ";
              body += std::to_string(static_cast<std::int32_t>(arg.immediate));
              body += "\n";
              continue;
            }
            if (arg.kind != c4c::backend::bir::Value::Kind::Named || !current_i32_name.has_value() ||
                arg.name != *current_i32_name) {
              return std::nullopt;
            }
            body += "    mov ";
            body += kArgRegs32[arg_index];
            body += ", eax\n";
          }

          body += "    xor eax, eax\n";
          body += "    call ";
          body += render_asm_symbol_name(callee_name);
          body += "\n";
          current_i32_name =
              call->result.has_value() && call->result->type == c4c::backend::bir::TypeKind::I32
                  ? std::optional<std::string_view>(call->result->name)
                  : std::nullopt;
          continue;
        }

        if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
          if (store->byte_offset != 0 || store->address.has_value() ||
              store->value.type != c4c::backend::bir::TypeKind::I32) {
            return std::nullopt;
          }
          const auto memory = render_candidate_local_i32(store->slot_name);
          if (!memory.has_value()) {
            return std::nullopt;
          }
          if (store->value.kind == c4c::backend::bir::Value::Kind::Immediate) {
            body += "    mov " + *memory + ", " +
                    std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
            current_i32_name = std::nullopt;
            continue;
          }
          if (store->value.kind != c4c::backend::bir::Value::Kind::Named ||
              !current_i32_name.has_value() || store->value.name != *current_i32_name) {
            return std::nullopt;
          }
          body += "    mov " + *memory + ", eax\n";
          current_i32_name = std::nullopt;
          continue;
        }

        const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
        if (load == nullptr || load->byte_offset != 0 || load->address.has_value() ||
            load->result.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        const auto memory = render_candidate_local_i32(load->slot_name);
        if (!memory.has_value()) {
          return std::nullopt;
        }
        body += "    mov eax, " + *memory + "\n";
        current_i32_name = load->result.name;
      }

      const auto& returned = *candidate->blocks.front().terminator.value;
      if (returned.kind != c4c::backend::bir::Value::Kind::Immediate ||
          returned.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      body += "    mov " + *candidate_return_register + ", " +
              std::to_string(static_cast<std::int32_t>(returned.immediate)) +
              "\n    add rsp, " + std::to_string(candidate_stack_adjust) + "\n    ret\n";
      rendered_functions += minimal_function_asm_prefix(*candidate) + body;
      saw_bounded_entry = true;
    }

    if (!saw_bounded_entry) {
      return std::nullopt;
    }

    std::string rendered_data;
    for (const auto* string_constant : used_string_constants) {
      rendered_data += emit_string_constant_data(*string_constant);
    }
    for (const auto& global : module.module.globals) {
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
    return rendered_functions + rendered_data;
  };
  if (const auto rendered_multi_defined = render_bounded_multi_defined_call_lane_if_supported();
      rendered_multi_defined.has_value()) {
    return *rendered_multi_defined;
  }
  if (defined_functions.size() > 1) {
    if (!bounded_same_module_helper_prefix.has_value()) {
      throw_multi_defined_contract();
    }
  }
  if (function.is_declaration || function.params.size() > 1 ||
      function.return_type != c4c::backend::bir::TypeKind::I32 || function.blocks.empty()) {
    throw_multi_defined_contract_if_active();
    throw std::invalid_argument(
        "x86 backend emitter only supports a minimal i32 return function through the canonical prepared-module handoff");
  }

  const auto& entry = function.blocks.front();
  const auto render_constant_folded_single_block_return_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value()) {
      return std::nullopt;
    }

    LocalSlotLayout constant_layout;
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
              note_numeric_suffix_root(store->slot_name, stored_size, std::max<std::size_t>(1, stored_size));
            }
          }
          if (store->address.has_value() &&
              store->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot &&
              store->address->size_bytes != 0) {
            note_numeric_suffix_root(store->address->base_name,
                                     store->address->size_bytes,
                                     std::max<std::size_t>(1, store->address->align_bytes));
          }
          continue;
        }
        if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
            load != nullptr && load->address.has_value() &&
            load->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot &&
            load->address->size_bytes != 0) {
          note_numeric_suffix_root(load->address->base_name,
                                   load->address->size_bytes,
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
        const bool scalar_like_slot =
            slot.type == c4c::backend::bir::TypeKind::I8 ||
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
            root_slot.size_bytes == 0 ||
            root_slot.align_bytes > 16) {
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
              return static_cast<std::int32_t>(
                  static_cast<std::uint32_t>(*lhs) << (static_cast<std::uint32_t>(*rhs) & 31u));
            case c4c::backend::bir::BinaryOpcode::LShr:
              return static_cast<std::int32_t>(
                  static_cast<std::uint32_t>(*lhs) >> (static_cast<std::uint32_t>(*rhs) & 31u));
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
        const auto address = resolve_local_address(store->slot_name, store->byte_offset, store->address);
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
    return asm_prefix + "    mov " + *return_register + ", " +
           std::to_string(static_cast<std::int32_t>(*folded_return)) + "\n    ret\n";
  };
  const auto render_minimal_local_slot_return_if_supported =
      [&]() -> std::optional<std::string> {
    if (function.params.empty() == false || function.blocks.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value() || entry.insts.empty()) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }
    const auto& returned = *entry.terminator.value;
    const auto* final_load =
        std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts.back());
    if (final_load == nullptr || returned.kind != c4c::backend::bir::Value::Kind::Named ||
        returned.type != c4c::backend::bir::TypeKind::I32 ||
        final_load->result.name != returned.name ||
        final_load->result.type != c4c::backend::bir::TypeKind::I32 ||
        final_load->byte_offset != 0) {
      return std::nullopt;
    }

    auto asm_text = asm_prefix;
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
          memory = render_local_address_operand_if_supported(
              *layout,
              store->address,
              store->value.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD"
                                                                     : "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(store->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(
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
                      render_stack_memory_operand(pointee_slot_it->second, "QWORD").substr(10) +
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
        memory = render_local_address_operand_if_supported(
            *layout,
            load->address,
            load->result.type == c4c::backend::bir::TypeKind::Ptr ? "QWORD" : "DWORD");
      } else {
        const auto slot_it = layout->offsets.find(load->slot_name);
        if (slot_it == layout->offsets.end()) {
          return std::nullopt;
        }
        memory = render_stack_memory_operand(
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
  };
  const auto find_authoritative_join_edge_transfers =
      [&](const c4c::backend::prepare::PreparedJoinTransfer& join_transfer)
      -> std::optional<std::pair<
          const c4c::backend::prepare::PreparedEdgeValueTransfer*,
          const c4c::backend::prepare::PreparedEdgeValueTransfer*>> {
    if (!join_transfer.source_true_transfer_index.has_value() ||
        !join_transfer.source_false_transfer_index.has_value()) {
      return std::nullopt;
    }

    const auto true_index = *join_transfer.source_true_transfer_index;
    const auto false_index = *join_transfer.source_false_transfer_index;
    if (true_index >= join_transfer.edge_transfers.size() ||
        false_index >= join_transfer.edge_transfers.size() ||
        true_index == false_index) {
      return std::nullopt;
    }

    return std::pair{
        &join_transfer.edge_transfers[true_index],
        &join_transfer.edge_transfers[false_index],
    };
  };
  struct AuthoritativeJoinTransfers {
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
  };
  const auto classify_authoritative_join_transfers =
      [&](const c4c::backend::prepare::PreparedJoinTransfer& join_transfer)
      -> std::optional<AuthoritativeJoinTransfers> {
    const auto authoritative_transfers = find_authoritative_join_edge_transfers(join_transfer);
    if (!authoritative_transfers.has_value()) {
      return std::nullopt;
    }

    const auto* true_transfer = authoritative_transfers->first;
    const auto* false_transfer = authoritative_transfers->second;
    if (true_transfer == nullptr || false_transfer == nullptr ||
        true_transfer->successor_label != join_transfer.join_block_label ||
        false_transfer->successor_label != join_transfer.join_block_label) {
      return std::nullopt;
    }

    return AuthoritativeJoinTransfers{
        .true_transfer = true_transfer,
        .false_transfer = false_transfer,
    };
  };
  struct AuthoritativeBranchJoinTransfer {
    const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
  };
  const auto build_authoritative_branch_join_transfer =
      [&](const c4c::backend::prepare::PreparedJoinTransfer& join_transfer)
      -> std::optional<AuthoritativeBranchJoinTransfer> {
    if ((join_transfer.kind !=
             c4c::backend::prepare::PreparedJoinTransferKind::SelectMaterialization &&
         join_transfer.kind != c4c::backend::prepare::PreparedJoinTransferKind::EdgeStoreSlot) ||
        join_transfer.result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const auto authoritative_transfers = classify_authoritative_join_transfers(join_transfer);
    if (!authoritative_transfers.has_value()) {
      return std::nullopt;
    }

    return AuthoritativeBranchJoinTransfer{
        .join_transfer = &join_transfer,
        .true_transfer = authoritative_transfers->true_transfer,
        .false_transfer = authoritative_transfers->false_transfer,
    };
  };
  const auto find_authoritative_branch_owned_join_transfer =
      [&](const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
          std::string_view source_block_label)
      -> std::optional<AuthoritativeBranchJoinTransfer> {
    const auto* join_transfer =
        c4c::backend::prepare::find_branch_owned_join_transfer(control_flow, source_block_label);
    if (join_transfer == nullptr) {
      return std::nullopt;
    }
    return build_authoritative_branch_join_transfer(*join_transfer);
  };
  const auto find_authoritative_select_materialization_join_transfer =
      [&](const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
          std::string_view source_block_label)
      -> std::optional<AuthoritativeBranchJoinTransfer> {
    const auto* join_transfer =
        c4c::backend::prepare::find_select_materialization_join_transfer(control_flow,
                                                                         source_block_label);
    if (join_transfer == nullptr) {
      return std::nullopt;
    }
    return build_authoritative_branch_join_transfer(*join_transfer);
  };
  struct AuthoritativeJoinTransferSources {
    const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
  };
  struct ClassifiedShortCircuitIncoming {
    bool short_circuit_on_compare_true = false;
    bool short_circuit_value = false;
  };
  struct AuthoritativeShortCircuitJoinSources {
    const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
    ClassifiedShortCircuitIncoming classified_incoming;
  };
  struct AuthoritativeJoinBranchSources {
    const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
    const c4c::backend::bir::Block* true_predecessor = nullptr;
    const c4c::backend::bir::Block* false_predecessor = nullptr;
  };
  struct MaterializedCompareJoinContext {
    const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
    const c4c::backend::bir::Block* true_predecessor = nullptr;
    const c4c::backend::bir::Block* false_predecessor = nullptr;
    const c4c::backend::bir::Block* join_block = nullptr;
    const c4c::backend::bir::BinaryInst* trailing_binary = nullptr;
    std::size_t carrier_index = 0;
    std::string_view joined_value_name;
  };
  const auto find_authoritative_join_transfer_sources =
      [&](const AuthoritativeBranchJoinTransfer& authoritative_join_transfer)
      -> std::optional<AuthoritativeJoinTransferSources> {
    const auto* join_transfer = authoritative_join_transfer.join_transfer;
    const auto* true_transfer = authoritative_join_transfer.true_transfer;
    const auto* false_transfer = authoritative_join_transfer.false_transfer;
    if (join_transfer == nullptr || true_transfer == nullptr || false_transfer == nullptr) {
      return std::nullopt;
    }

    return AuthoritativeJoinTransferSources{
        .join_transfer = join_transfer,
        .true_transfer = true_transfer,
        .false_transfer = false_transfer,
    };
  };
  const auto find_authoritative_join_branch_sources =
      [&](const AuthoritativeBranchJoinTransfer& authoritative_join_transfer,
          std::string_view true_block_label,
          std::string_view false_block_label)
      -> std::optional<AuthoritativeJoinBranchSources> {
    const auto authoritative_sources =
        find_authoritative_join_transfer_sources(authoritative_join_transfer);
    if (!authoritative_sources.has_value() ||
        authoritative_sources->join_transfer == nullptr ||
        authoritative_sources->true_transfer == nullptr ||
        authoritative_sources->false_transfer == nullptr) {
      return std::nullopt;
    }
    const auto* join_transfer = authoritative_sources->join_transfer;
    const auto* true_transfer = authoritative_sources->true_transfer;
    const auto* false_transfer = authoritative_sources->false_transfer;
    if (join_transfer->source_true_incoming_label.has_value() &&
        true_transfer->predecessor_label != *join_transfer->source_true_incoming_label) {
      return std::nullopt;
    }
    if (join_transfer->source_false_incoming_label.has_value() &&
        false_transfer->predecessor_label != *join_transfer->source_false_incoming_label) {
      return std::nullopt;
    }

    const auto* true_predecessor = find_block(true_block_label);
    const auto* false_predecessor = find_block(false_block_label);
    if (true_predecessor == nullptr || false_predecessor == nullptr ||
        true_predecessor == &entry || false_predecessor == &entry ||
        true_predecessor == false_predecessor || !true_predecessor->insts.empty() ||
        !false_predecessor->insts.empty()) {
      return std::nullopt;
    }
    if (true_predecessor->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        false_predecessor->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        true_predecessor->terminator.target_label != join_transfer->join_block_label ||
        false_predecessor->terminator.target_label != join_transfer->join_block_label) {
      return std::nullopt;
    }

    return AuthoritativeJoinBranchSources{
        .join_transfer = join_transfer,
        .true_transfer = true_transfer,
        .false_transfer = false_transfer,
        .true_predecessor = true_predecessor,
        .false_predecessor = false_predecessor,
    };
  };
  const auto find_materialized_compare_join_context =
      [&](const AuthoritativeBranchJoinTransfer& authoritative_join_transfer,
          std::string_view true_block_label,
          std::string_view false_block_label)
      -> std::optional<MaterializedCompareJoinContext> {
    const auto join_sources = find_authoritative_join_branch_sources(authoritative_join_transfer,
                                                                     true_block_label,
                                                                     false_block_label);
    if (!join_sources.has_value() || join_sources->join_transfer == nullptr ||
        join_sources->true_transfer == nullptr || join_sources->false_transfer == nullptr ||
        join_sources->true_predecessor == nullptr || join_sources->false_predecessor == nullptr ||
        join_sources->true_predecessor == join_sources->false_predecessor) {
      return std::nullopt;
    }

    const auto* join_transfer = join_sources->join_transfer;
    const auto* join_block = find_block(join_transfer->join_block_label);
    if (join_block == nullptr || join_block == &entry ||
        join_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !join_block->terminator.value.has_value() || join_block->insts.empty() ||
        join_sources->true_predecessor == join_block ||
        join_sources->false_predecessor == join_block ||
        join_transfer->join_block_label != join_block->label ||
        join_transfer->result.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }

    std::size_t carrier_index = join_block->insts.size() - 1;
    const c4c::backend::bir::BinaryInst* trailing_binary = nullptr;
    if (const auto* trailing =
            std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts.back());
        trailing != nullptr &&
        join_block->terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
        join_block->terminator.value->name == trailing->result.name) {
      if (join_block->insts.size() < 2) {
        return std::nullopt;
      }
      trailing_binary = trailing;
      carrier_index = join_block->insts.size() - 2;
    }

    const std::string_view joined_value_name = join_transfer->result.name;
    const auto join_result_matches_prepared_value =
        [&](const auto& result) {
          return result.type == c4c::backend::bir::TypeKind::I32 &&
                 result.name == joined_value_name;
        };
    if (const auto* select =
            std::get_if<c4c::backend::bir::SelectInst>(&join_block->insts[carrier_index]);
        select != nullptr) {
      if (!join_result_matches_prepared_value(select->result)) {
        return std::nullopt;
      }
    } else if (const auto* load_local =
                   std::get_if<c4c::backend::bir::LoadLocalInst>(&join_block->insts[carrier_index]);
               load_local != nullptr) {
      if (!join_result_matches_prepared_value(load_local->result)) {
        return std::nullopt;
      }
    } else {
      return std::nullopt;
    }
    if (join_block->terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
        (trailing_binary == nullptr &&
         join_block->terminator.value->name != joined_value_name)) {
      return std::nullopt;
    }

    return MaterializedCompareJoinContext{
        .join_transfer = join_transfer,
        .true_transfer = join_sources->true_transfer,
        .false_transfer = join_sources->false_transfer,
        .true_predecessor = join_sources->true_predecessor,
        .false_predecessor = join_sources->false_predecessor,
        .join_block = join_block,
        .trailing_binary = trailing_binary,
        .carrier_index = carrier_index,
        .joined_value_name = joined_value_name,
    };
  };
  const auto render_local_slot_guard_chain_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.empty() ||
        prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }
    static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    const auto is_supported_guard_compare_opcode =
        [](c4c::backend::bir::BinaryOpcode opcode) {
          return opcode == c4c::backend::bir::BinaryOpcode::Eq ||
                 opcode == c4c::backend::bir::BinaryOpcode::Ne ||
                 opcode == c4c::backend::bir::BinaryOpcode::Sgt ||
                 opcode == c4c::backend::bir::BinaryOpcode::Sge ||
                 opcode == c4c::backend::bir::BinaryOpcode::Slt ||
                 opcode == c4c::backend::bir::BinaryOpcode::Sle;
        };
    struct GuardJoinContinuation {
      std::string_view incoming_label;
      const c4c::backend::bir::Block* true_block = nullptr;
      const c4c::backend::bir::Block* false_block = nullptr;
    };
    struct ShortCircuitTarget {
      const c4c::backend::bir::Block* block = nullptr;
      std::optional<GuardJoinContinuation> continuation;
    };
    struct ShortCircuitPlan {
      ShortCircuitTarget on_compare_true;
      ShortCircuitTarget on_compare_false;
    };
    struct CompareDrivenBranchRenderPlan {
      ShortCircuitPlan branch_plan;
      std::string compare_setup;
      std::string false_branch_opcode;
    };
    struct ShortCircuitJoinContext {
      const c4c::backend::prepare::PreparedJoinTransfer* join_transfer = nullptr;
      const c4c::backend::bir::Block* join_block = nullptr;
      const c4c::backend::prepare::PreparedEdgeValueTransfer* true_transfer = nullptr;
      const c4c::backend::prepare::PreparedEdgeValueTransfer* false_transfer = nullptr;
      ClassifiedShortCircuitIncoming classified_incoming;
      GuardJoinContinuation continuation_plan;
    };
    struct ShortCircuitRenderContext {
      bool omit_true_continuation = false;
      bool omit_false_lane = false;
    };
    struct RenderedShortCircuitFalseLane {
      std::string label;
      std::string rendered;
    };
    struct RenderedShortCircuitLanes {
      std::string rendered_true;
      RenderedShortCircuitFalseLane rendered_false_lane;
    };
    struct DirectBranchTargets {
      const c4c::backend::bir::Block* true_block = nullptr;
      const c4c::backend::bir::Block* false_block = nullptr;
    };
    struct ShortCircuitEntryCompareContext {
      const c4c::backend::prepare::PreparedBranchCondition* branch_condition = nullptr;
      std::string compare_setup;
      std::string false_branch_opcode;
    };
    struct MaterializedI32Compare {
      std::string_view i1_name;
      std::optional<std::string_view> i32_name;
      c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
      std::string compare_setup;
    };
    const auto* function_control_flow = find_control_flow_function();
    const auto resolve_direct_branch_targets =
        [&](const c4c::backend::bir::Block& source_block,
            std::string_view true_label,
            std::string_view false_label) -> std::optional<DirectBranchTargets> {
      DirectBranchTargets targets{
          .true_block = find_block(true_label),
          .false_block = find_block(false_label),
      };
      if (targets.true_block == nullptr || targets.false_block == nullptr) {
        return std::nullopt;
      }
      return targets;
    };
    const auto build_direct_branch_plan_from_targets =
        [&](const c4c::backend::bir::Block& source_block,
            const DirectBranchTargets& direct_targets) -> std::optional<ShortCircuitPlan> {
      if (direct_targets.true_block == nullptr || direct_targets.false_block == nullptr ||
          direct_targets.true_block == &source_block ||
          direct_targets.false_block == &source_block) {
        return std::nullopt;
      }

      return ShortCircuitPlan{
          .on_compare_true =
              ShortCircuitTarget{
                  .block = direct_targets.true_block,
                  .continuation = std::nullopt,
              },
          .on_compare_false =
              ShortCircuitTarget{
                  .block = direct_targets.false_block,
                  .continuation = std::nullopt,
              },
      };
    };
    const auto build_plain_cond_direct_branch_targets =
        [&](const c4c::backend::bir::Block& source_block) -> std::optional<DirectBranchTargets> {
      return resolve_direct_branch_targets(source_block,
                                           source_block.terminator.true_label,
                                           source_block.terminator.false_label);
    };
    const auto build_direct_branch_targets_from_continuation =
        [&](const GuardJoinContinuation& continuation_plan) -> std::optional<DirectBranchTargets> {
      if (continuation_plan.true_block == nullptr || continuation_plan.false_block == nullptr) {
        return std::nullopt;
      }
      return DirectBranchTargets{
          .true_block = continuation_plan.true_block,
          .false_block = continuation_plan.false_block,
      };
    };
    const auto build_compare_join_continuation =
        [&](const c4c::backend::prepare::PreparedJoinTransfer& join_transfer,
            const c4c::backend::bir::Block& join_block,
            const c4c::backend::prepare::PreparedBranchCondition& join_branch_condition)
        -> std::optional<GuardJoinContinuation> {
      if (!join_branch_condition.predicate.has_value() ||
          !join_branch_condition.compare_type.has_value() ||
          !join_branch_condition.lhs.has_value() || !join_branch_condition.rhs.has_value() ||
          *join_branch_condition.compare_type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }

      const bool lhs_is_join_result_rhs_is_zero =
          join_branch_condition.lhs->kind == c4c::backend::bir::Value::Kind::Named &&
          join_branch_condition.lhs->name == join_transfer.result.name &&
          join_branch_condition.rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          join_branch_condition.rhs->type == c4c::backend::bir::TypeKind::I32 &&
          join_branch_condition.rhs->immediate == 0;
      const bool rhs_is_join_result_lhs_is_zero =
          join_branch_condition.rhs->kind == c4c::backend::bir::Value::Kind::Named &&
          join_branch_condition.rhs->name == join_transfer.result.name &&
          join_branch_condition.lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          join_branch_condition.lhs->type == c4c::backend::bir::TypeKind::I32 &&
          join_branch_condition.lhs->immediate == 0;
      if (!lhs_is_join_result_rhs_is_zero && !rhs_is_join_result_lhs_is_zero) {
        return std::nullopt;
      }

      const auto direct_targets = resolve_direct_branch_targets(join_block,
                                                                join_branch_condition.true_label,
                                                                join_branch_condition.false_label);
      if (!direct_targets.has_value()) {
        return std::nullopt;
      }

      GuardJoinContinuation continuation{
          .incoming_label = {},
          .true_block = nullptr,
          .false_block = nullptr,
      };
      if (*join_branch_condition.predicate == c4c::backend::bir::BinaryOpcode::Ne) {
        continuation.true_block = direct_targets->true_block;
        continuation.false_block = direct_targets->false_block;
      } else if (*join_branch_condition.predicate == c4c::backend::bir::BinaryOpcode::Eq) {
        continuation.true_block = direct_targets->false_block;
        continuation.false_block = direct_targets->true_block;
      } else {
        return std::nullopt;
      }
      return continuation;
    };
    const auto classify_short_circuit_join_incoming =
        [&](const c4c::backend::prepare::PreparedEdgeValueTransfer& true_transfer,
            const c4c::backend::prepare::PreparedEdgeValueTransfer& false_transfer)
        -> std::optional<ClassifiedShortCircuitIncoming> {
      const auto classify_join_incoming =
          [](const c4c::backend::prepare::PreparedEdgeValueTransfer& incoming)
          -> std::optional<bool> {
        if (incoming.incoming_value.kind == c4c::backend::bir::Value::Kind::Immediate &&
            incoming.incoming_value.type == c4c::backend::bir::TypeKind::I32 &&
            (incoming.incoming_value.immediate == 0 || incoming.incoming_value.immediate == 1)) {
          return incoming.incoming_value.immediate != 0;
        }
        return std::nullopt;
      };

      const auto true_lane_short_circuit_value =
          classify_join_incoming(true_transfer);
      const auto false_lane_short_circuit_value =
          classify_join_incoming(false_transfer);
      if (true_lane_short_circuit_value.has_value() ==
          false_lane_short_circuit_value.has_value()) {
        return std::nullopt;
      }

      return ClassifiedShortCircuitIncoming{
          .short_circuit_on_compare_true = true_lane_short_circuit_value.has_value(),
          .short_circuit_value = true_lane_short_circuit_value.value_or(
              *false_lane_short_circuit_value),
      };
    };
    const auto find_authoritative_short_circuit_join_sources =
        [&](const AuthoritativeBranchJoinTransfer& authoritative_join_transfer)
        -> std::optional<AuthoritativeShortCircuitJoinSources> {
      if (authoritative_join_transfer.join_transfer == nullptr ||
          authoritative_join_transfer.join_transfer->edge_transfers.size() != 2) {
        return std::nullopt;
      }

      const auto join_sources =
          find_authoritative_join_transfer_sources(authoritative_join_transfer);
      if (!join_sources.has_value() || join_sources->join_transfer == nullptr ||
          join_sources->true_transfer == nullptr || join_sources->false_transfer == nullptr) {
        return std::nullopt;
      }

      const auto classified_incoming =
          classify_short_circuit_join_incoming(*join_sources->true_transfer,
                                               *join_sources->false_transfer);
      if (!classified_incoming.has_value()) {
        return std::nullopt;
      }

      return AuthoritativeShortCircuitJoinSources{
          .join_transfer = join_sources->join_transfer,
          .true_transfer = join_sources->true_transfer,
          .false_transfer = join_sources->false_transfer,
          .classified_incoming = *classified_incoming,
      };
    };
    const auto build_short_circuit_target_from_transfer =
        [&](const c4c::backend::prepare::PreparedEdgeValueTransfer& transfer,
            bool is_short_circuit_lane,
            bool short_circuit_value,
            const c4c::backend::bir::Block& rhs_entry,
            const GuardJoinContinuation& continuation_plan)
        -> std::optional<ShortCircuitTarget> {
      if (is_short_circuit_lane) {
        const auto* target =
            short_circuit_value ? continuation_plan.true_block
                                : continuation_plan.false_block;
        return ShortCircuitTarget{
            .block = target,
            .continuation = std::nullopt,
        };
      }

      GuardJoinContinuation continuation = continuation_plan;
      continuation.incoming_label = transfer.predecessor_label;
      return ShortCircuitTarget{
          .block = &rhs_entry,
          .continuation = continuation,
      };
    };
    const auto build_short_circuit_join_context_from_sources =
        [&](const AuthoritativeShortCircuitJoinSources& join_sources,
            const auto& find_branch_condition_fn)
        -> std::optional<ShortCircuitJoinContext> {
      const auto* join_transfer = join_sources.join_transfer;
      const auto* true_transfer = join_sources.true_transfer;
      const auto* false_transfer = join_sources.false_transfer;
      if (join_transfer == nullptr || true_transfer == nullptr || false_transfer == nullptr) {
        return std::nullopt;
      }

      const auto* join_block = find_block(join_transfer->join_block_label);
      if (join_block == nullptr) {
        return std::nullopt;
      }

      const auto* join_branch_condition = find_branch_condition_fn(join_block->label);
      if (join_branch_condition == nullptr) {
        return std::nullopt;
      }

      const auto continuation_plan = build_compare_join_continuation(
          *join_transfer, *join_block, *join_branch_condition);
      if (!continuation_plan.has_value()) {
        return std::nullopt;
      }

      return ShortCircuitJoinContext{
          .join_transfer = join_transfer,
          .join_block = join_block,
          .true_transfer = true_transfer,
          .false_transfer = false_transfer,
          .classified_incoming = join_sources.classified_incoming,
          .continuation_plan = *continuation_plan,
      };
    };
    const auto find_short_circuit_join_context =
        [&](const c4c::backend::prepare::PreparedControlFlowFunction& control_flow,
            const auto& find_branch_condition_fn,
            std::string_view source_block_label)
        -> std::optional<ShortCircuitJoinContext> {
      const auto authoritative_join_transfer =
          find_authoritative_select_materialization_join_transfer(control_flow,
                                                                 source_block_label);
      if (!authoritative_join_transfer.has_value()) {
        return std::nullopt;
      }
      const auto join_sources =
          find_authoritative_short_circuit_join_sources(*authoritative_join_transfer);
      if (!join_sources.has_value()) {
        return std::nullopt;
      }
      return build_short_circuit_join_context_from_sources(*join_sources,
                                                           find_branch_condition_fn);
    };
    const auto build_short_circuit_entry_direct_branch_targets =
        [&](const c4c::backend::prepare::PreparedBranchCondition* branch_condition,
            const c4c::backend::bir::Block& source_block)
        -> std::optional<DirectBranchTargets> {
      if (branch_condition == nullptr || !branch_condition->predicate.has_value() ||
          !branch_condition->compare_type.has_value() ||
          !branch_condition->lhs.has_value() || !branch_condition->rhs.has_value() ||
          branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
          branch_condition->condition_value.kind !=
              c4c::backend::bir::Value::Kind::Named ||
          branch_condition->condition_value.name != source_block.terminator.condition.name) {
        return std::nullopt;
      }

      return resolve_direct_branch_targets(source_block,
                                           branch_condition->true_label,
                                           branch_condition->false_label);
    };
    const auto build_short_circuit_plan =
        [&](const ShortCircuitJoinContext& join_context,
            const ShortCircuitPlan& direct_branch_plan)
        -> std::optional<ShortCircuitPlan> {
      if (join_context.true_transfer == nullptr ||
          join_context.false_transfer == nullptr) {
        return std::nullopt;
      }

      const bool short_circuit_on_compare_true =
          join_context.classified_incoming.short_circuit_on_compare_true;
      const bool short_circuit_on_compare_false = !short_circuit_on_compare_true;

      const auto* rhs_entry = short_circuit_on_compare_true
                                  ? direct_branch_plan.on_compare_false.block
                                  : direct_branch_plan.on_compare_true.block;
      if (rhs_entry == nullptr) {
        return std::nullopt;
      }

      const auto on_compare_true =
          build_short_circuit_target_from_transfer(
              *join_context.true_transfer,
              short_circuit_on_compare_true,
              join_context.classified_incoming.short_circuit_value,
              *rhs_entry,
              join_context.continuation_plan);
      const auto on_compare_false =
          build_short_circuit_target_from_transfer(
              *join_context.false_transfer,
              short_circuit_on_compare_false,
              join_context.classified_incoming.short_circuit_value,
              *rhs_entry,
              join_context.continuation_plan);
      if (!on_compare_true.has_value() || !on_compare_false.has_value()) {
        return std::nullopt;
      }

      ShortCircuitPlan plan{
          .on_compare_true = *on_compare_true,
          .on_compare_false = *on_compare_false,
      };
      if (plan.on_compare_true.block == nullptr ||
          plan.on_compare_false.block == nullptr) {
        return std::nullopt;
      }
      return plan;
    };
    const auto render_short_circuit_target =
        [&](const auto& render_block_fn,
            const ShortCircuitTarget& target,
            bool omit_continuation) -> std::optional<std::string> {
      if (target.block == nullptr) {
        return std::nullopt;
      }
      return render_block_fn(*target.block,
                             omit_continuation ? std::nullopt : target.continuation);
    };
    const auto render_short_circuit_false_lane =
        [&](const auto& render_block_fn,
            const ShortCircuitPlan& plan,
            const ShortCircuitRenderContext& render_context)
        -> std::optional<RenderedShortCircuitFalseLane> {
      RenderedShortCircuitFalseLane rendered_false_lane{
          .label = ".L" + function.name + "_" + std::string(plan.on_compare_false.block->label),
          .rendered = {},
      };
      if (render_context.omit_false_lane) {
        return rendered_false_lane;
      }

      const auto rendered_false =
          render_short_circuit_target(render_block_fn, plan.on_compare_false, false);
      if (!rendered_false.has_value()) {
        return std::nullopt;
      }
      rendered_false_lane.rendered =
          rendered_false_lane.label + ":\n" + *rendered_false;
      return rendered_false_lane;
    };
    const auto render_short_circuit_lanes =
        [&](const auto& render_block_fn,
            const ShortCircuitPlan& plan,
            const ShortCircuitRenderContext& render_context)
        -> std::optional<RenderedShortCircuitLanes> {
      const auto rendered_true =
          render_short_circuit_target(render_block_fn,
                                      plan.on_compare_true,
                                      render_context.omit_true_continuation);
      if (!rendered_true.has_value()) {
        return std::nullopt;
      }

      const auto rendered_false_lane =
          render_short_circuit_false_lane(render_block_fn, plan, render_context);
      if (!rendered_false_lane.has_value()) {
        return std::nullopt;
      }
      return RenderedShortCircuitLanes{
          .rendered_true = *rendered_true,
          .rendered_false_lane = *rendered_false_lane,
      };
    };
    const auto build_short_circuit_render_context =
        [&](const ShortCircuitPlan& plan) -> std::optional<ShortCircuitRenderContext> {
      const bool true_target_renders_false_lane =
          plan.on_compare_true.continuation.has_value() &&
          plan.on_compare_false.block ==
              plan.on_compare_true.continuation->false_block;
      const bool false_target_renders_true_lane =
          plan.on_compare_false.continuation.has_value() &&
          plan.on_compare_true.block ==
              plan.on_compare_false.continuation->true_block;
      if (true_target_renders_false_lane && false_target_renders_true_lane) {
        return std::nullopt;
      }
      return ShortCircuitRenderContext{
          .omit_true_continuation = false_target_renders_true_lane,
          .omit_false_lane = true_target_renders_false_lane,
      };
    };
    const auto assemble_short_circuit_rendered_plan =
        [&](std::string_view rendered_body,
            std::string_view compare_prefix,
            std::string_view compare_branch,
            std::string_view rendered_true,
            const RenderedShortCircuitFalseLane& rendered_false_lane) {
      std::string rendered = std::string(rendered_body) + std::string(compare_prefix) +
                             "    " + std::string(compare_branch) + " " +
                             rendered_false_lane.label + "\n" + std::string(rendered_true);
      if (!rendered_false_lane.rendered.empty()) {
        rendered += rendered_false_lane.rendered;
      }
      return rendered;
    };
    const auto render_compare_driven_branch_plan =
        [&](const auto& render_block_fn,
            std::string_view rendered_body,
            const CompareDrivenBranchRenderPlan& render_plan) -> std::optional<std::string> {
      const auto render_context = build_short_circuit_render_context(render_plan.branch_plan);
      if (!render_context.has_value()) {
        return std::nullopt;
      }

      const auto rendered_lanes =
          render_short_circuit_lanes(render_block_fn, render_plan.branch_plan, *render_context);
      if (!rendered_lanes.has_value()) {
        return std::nullopt;
      }

      return assemble_short_circuit_rendered_plan(rendered_body,
                                                  render_plan.compare_setup,
                                                  render_plan.false_branch_opcode,
                                                  rendered_lanes->rendered_true,
                                                  rendered_lanes->rendered_false_lane);
    };

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
    std::function<std::optional<std::string>(const c4c::backend::bir::Block&,
                                             const std::optional<GuardJoinContinuation>&)>
        render_block =
            [&](const c4c::backend::bir::Block& block,
                const std::optional<GuardJoinContinuation>& continuation)
        -> std::optional<std::string> {
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
                memory = render_local_address_operand_if_supported(
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
                memory = render_stack_memory_operand(
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
                  !same_module_global_supports_scalar_load(
                      *global, load->result.type, load->byte_offset)) {
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
                return "    mov DWORD PTR [rip + " + render_asm_symbol_name(store->global_name) +
                       "], " +
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
              memory = render_local_address_operand_if_supported(
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
              memory = render_stack_memory_operand(
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
              return "    lea rax, " + render_stack_address_expr(pointee_slot_it->second) +
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
            const auto* branch_compare =
                std::get_if<c4c::backend::bir::BinaryInst>(&block.insts.back());
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
                  call->is_variadic || call->args.size() != call->arg_types.size() ||
                  call->args.size() > 6 ||
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
                const auto immediate =
                    lhs_is_current_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
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

          const auto render_false_branch_compare =
              [&](const c4c::backend::bir::BinaryInst& compare)
              -> std::optional<std::pair<std::string, std::string>> {
            if (current_materialized_compare.has_value() &&
                current_materialized_compare->i32_name.has_value()) {
              const bool lhs_is_materialized_rhs_is_zero =
                  compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                  compare.lhs.name == *current_materialized_compare->i32_name &&
                  compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                  compare.rhs.type == c4c::backend::bir::TypeKind::I32 &&
                  compare.rhs.immediate == 0;
              const bool rhs_is_materialized_lhs_is_zero =
                  compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                  compare.rhs.name == *current_materialized_compare->i32_name &&
                  compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                  compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
                  compare.lhs.immediate == 0;
              if (lhs_is_materialized_rhs_is_zero || rhs_is_materialized_lhs_is_zero) {
                const bool branch_when_original_false =
                    compare.opcode == c4c::backend::bir::BinaryOpcode::Ne;
                const char* branch_opcode = nullptr;
                if (current_materialized_compare->opcode == c4c::backend::bir::BinaryOpcode::Eq) {
                  branch_opcode = branch_when_original_false ? "jne" : "je";
                } else {
                  branch_opcode = branch_when_original_false ? "je" : "jne";
                }
                return std::pair<std::string, std::string>{current_materialized_compare->compare_setup,
                                                           branch_opcode};
              }
            }

            if (compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.lhs.type == c4c::backend::bir::TypeKind::I32 &&
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.rhs.type == c4c::backend::bir::TypeKind::I32) {
              const auto compare_setup =
                  "    mov eax, " +
                  std::to_string(static_cast<std::int32_t>(compare.lhs.immediate)) +
                  "\n    cmp eax, " +
                  std::to_string(static_cast<std::int32_t>(compare.rhs.immediate)) + "\n";
              const char* branch_opcode = nullptr;
              switch (compare.opcode) {
                case c4c::backend::bir::BinaryOpcode::Eq:
                  branch_opcode = "jne";
                  break;
                case c4c::backend::bir::BinaryOpcode::Ne:
                  branch_opcode = "je";
                  break;
                case c4c::backend::bir::BinaryOpcode::Sgt:
                  branch_opcode = "jle";
                  break;
                case c4c::backend::bir::BinaryOpcode::Sge:
                  branch_opcode = "jl";
                  break;
                case c4c::backend::bir::BinaryOpcode::Slt:
                  branch_opcode = "jge";
                  break;
                case c4c::backend::bir::BinaryOpcode::Sle:
                  branch_opcode = "jg";
                  break;
                default:
                  return std::nullopt;
              }
              return std::pair<std::string, std::string>{compare_setup, branch_opcode};
            }

            if (!current_i32_name.has_value()) {
              return std::nullopt;
            }
            const bool lhs_is_current_rhs_is_imm =
                compare.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                compare.lhs.name == *current_i32_name &&
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.rhs.type == c4c::backend::bir::TypeKind::I32;
            const bool rhs_is_current_lhs_is_imm =
                compare.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                compare.rhs.name == *current_i32_name &&
                compare.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                compare.lhs.type == c4c::backend::bir::TypeKind::I32;
            if (!lhs_is_current_rhs_is_imm && !rhs_is_current_lhs_is_imm) {
              return std::nullopt;
            }
            const auto compare_immediate =
                lhs_is_current_rhs_is_imm ? compare.rhs.immediate : compare.lhs.immediate;
            const auto compare_setup =
                compare_immediate == 0
                    ? std::string("    test eax, eax\n")
                    : "    cmp eax, " +
                          std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
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
                branch_opcode_for_current_immediate(lhs_is_current_rhs_is_imm);
            if (branch_opcode == nullptr) {
              return std::nullopt;
            }
            return std::pair<std::string, std::string>{compare_setup, branch_opcode};
          };
          const auto render_false_branch_compare_from_condition =
              [&](const c4c::backend::prepare::PreparedBranchCondition& condition)
              -> std::optional<std::pair<std::string, std::string>> {
            if (!condition.predicate.has_value() || !condition.lhs.has_value() ||
                !condition.rhs.has_value()) {
              return std::nullopt;
            }

            c4c::backend::bir::BinaryInst compare{
                .opcode = *condition.predicate,
                .result = condition.condition_value,
                .operand_type = condition.compare_type.value_or(c4c::backend::bir::TypeKind::I32),
                .lhs = *condition.lhs,
                .rhs = *condition.rhs,
            };
            return render_false_branch_compare(compare);
          };
          const auto find_trailing_guard_compare =
              [&](const c4c::backend::bir::Block& source_block,
                  std::size_t current_compare_index,
                  std::optional<std::string_view> expected_condition_name)
              -> const c4c::backend::bir::BinaryInst* {
            if (current_compare_index + 1 != source_block.insts.size()) {
              return nullptr;
            }

            const auto* compare =
                std::get_if<c4c::backend::bir::BinaryInst>(&source_block.insts.back());
            if (compare == nullptr || !is_supported_guard_compare_opcode(compare->opcode) ||
                compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
                (compare->result.type != c4c::backend::bir::TypeKind::I1 &&
                 compare->result.type != c4c::backend::bir::TypeKind::I32)) {
              return nullptr;
            }
            if (expected_condition_name.has_value() &&
                compare->result.name != *expected_condition_name) {
              return nullptr;
            }
            return compare;
          };
          const auto build_short_circuit_entry_compare_context =
              [&](const auto& find_branch_condition_fn,
                  const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare)
              -> std::optional<ShortCircuitEntryCompareContext> {
            const auto* branch_condition = find_branch_condition_fn(source_block.label);
            auto false_branch_compare =
                branch_condition != nullptr
                    ? render_false_branch_compare_from_condition(*branch_condition)
                    : std::optional<std::pair<std::string, std::string>>{};
            if (!false_branch_compare.has_value()) {
              false_branch_compare = render_false_branch_compare(compare);
            }
            if (!false_branch_compare.has_value()) {
              return std::nullopt;
            }
            return ShortCircuitEntryCompareContext{
                .branch_condition = branch_condition,
                .compare_setup = std::move(false_branch_compare->first),
                .false_branch_opcode = std::move(false_branch_compare->second),
            };
          };
          const auto build_compare_driven_render_plan =
              [&](ShortCircuitPlan branch_plan,
                  const ShortCircuitEntryCompareContext& compare_context)
              -> CompareDrivenBranchRenderPlan {
            return CompareDrivenBranchRenderPlan{
                .branch_plan = std::move(branch_plan),
                .compare_setup = compare_context.compare_setup,
                .false_branch_opcode = compare_context.false_branch_opcode,
            };
          };
          const auto find_branch_condition =
              [&](std::string_view block_label)
              -> const c4c::backend::prepare::PreparedBranchCondition* {
            if (function_control_flow == nullptr) {
              return nullptr;
            }
            return c4c::backend::prepare::find_prepared_branch_condition(
                *function_control_flow, block_label);
          };
          const auto build_compare_driven_entry_render_plan =
              [&](const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare,
                  const auto& branch_plan_builder)
              -> std::optional<CompareDrivenBranchRenderPlan> {
            const auto compare_context =
                build_short_circuit_entry_compare_context(find_branch_condition,
                                                          source_block,
                                                          compare);
            if (!compare_context.has_value()) {
              return std::nullopt;
            }

            const auto branch_plan = branch_plan_builder(*compare_context);
            if (!branch_plan.has_value()) {
              return std::nullopt;
            }

            return build_compare_driven_render_plan(*branch_plan, *compare_context);
          };
          const auto build_compare_driven_direct_entry_render_plan =
              [&](const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare,
                  const auto& direct_target_builder)
              -> std::optional<CompareDrivenBranchRenderPlan> {
            return build_compare_driven_entry_render_plan(
                source_block,
                compare,
                [&](const ShortCircuitEntryCompareContext& compare_context)
                    -> std::optional<ShortCircuitPlan> {
                  const auto direct_targets = direct_target_builder(compare_context);
                  if (!direct_targets.has_value()) {
                    return std::nullopt;
                  }
                  return build_direct_branch_plan_from_targets(source_block, *direct_targets);
                });
          };
          const auto build_short_circuit_entry_render_plan =
              [&](const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare,
                  const ShortCircuitJoinContext& join_context)
              -> std::optional<CompareDrivenBranchRenderPlan> {
            auto direct_entry_render_plan = build_compare_driven_direct_entry_render_plan(
                source_block,
                compare,
                [&](const ShortCircuitEntryCompareContext& compare_context)
                    -> std::optional<DirectBranchTargets> {
                  return build_short_circuit_entry_direct_branch_targets(
                      compare_context.branch_condition, source_block);
                });
            if (!direct_entry_render_plan.has_value()) {
              return std::nullopt;
            }

            const auto short_circuit_branch_plan =
                build_short_circuit_plan(join_context,
                                         direct_entry_render_plan->branch_plan);
            if (!short_circuit_branch_plan.has_value()) {
              return std::nullopt;
            }

            direct_entry_render_plan->branch_plan = *short_circuit_branch_plan;
            return direct_entry_render_plan;
          };
          const auto build_plain_cond_entry_render_plan =
              [&](const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare)
              -> std::optional<CompareDrivenBranchRenderPlan> {
            return build_compare_driven_direct_entry_render_plan(
                source_block,
                compare,
                [&](const ShortCircuitEntryCompareContext&)
                    -> std::optional<DirectBranchTargets> {
                  return build_plain_cond_direct_branch_targets(source_block);
                });
          };
          const auto build_compare_join_entry_render_plan =
              [&](const c4c::backend::bir::Block& source_block,
                  const c4c::backend::bir::BinaryInst& compare,
                  const GuardJoinContinuation& continuation_plan)
              -> std::optional<CompareDrivenBranchRenderPlan> {
            return build_compare_driven_direct_entry_render_plan(
                source_block,
                compare,
                [&](const ShortCircuitEntryCompareContext&)
                    -> std::optional<DirectBranchTargets> {
                  return build_direct_branch_targets_from_continuation(continuation_plan);
                });
          };

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
              const bool reaches_continuation_join =
                  block.label == continuation->incoming_label ||
                  block.terminator.target_label == continuation->incoming_label;
              if (reaches_continuation_join) {
                const auto* compare =
                    find_trailing_guard_compare(block, compare_index, std::nullopt);
                if (compare == nullptr) {
                  return std::nullopt;
                }
                const auto compare_join_render_plan =
                    build_compare_join_entry_render_plan(block, *compare, *continuation);
                if (!compare_join_render_plan.has_value()) {
                  return std::nullopt;
                }
                return render_compare_driven_branch_plan(render_block,
                                                         body,
                                                         *compare_join_render_plan);
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

          if (block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
            return std::nullopt;
          }
          const auto* compare = find_trailing_guard_compare(
              block, compare_index, block.terminator.condition.name);
          if (compare == nullptr) {
            return std::nullopt;
          }

          const auto try_render_short_circuit_plan =
              [&]() -> std::optional<std::string> {
            if (function_control_flow == nullptr) {
              return std::nullopt;
            }

            const auto join_context = find_short_circuit_join_context(*function_control_flow,
                                                                      find_branch_condition,
                                                                      block.label);
            if (!join_context.has_value()) {
              return std::nullopt;
            }

            const auto short_circuit_render_plan =
                build_short_circuit_entry_render_plan(block, *compare, *join_context);
            if (!short_circuit_render_plan.has_value()) {
              return std::nullopt;
            }
            return render_compare_driven_branch_plan(render_block,
                                                     body,
                                                     *short_circuit_render_plan);
          };
          if (const auto rendered_short_circuit = try_render_short_circuit_plan();
              rendered_short_circuit.has_value()) {
            return rendered_short_circuit;
          }

          const auto plain_cond_render_plan =
              build_plain_cond_entry_render_plan(block, *compare);
          if (!plain_cond_render_plan.has_value()) {
            return std::nullopt;
          }
          return render_compare_driven_branch_plan(render_block,
                                                   body,
                                                   *plain_cond_render_plan);
        };

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    const auto rendered_entry = render_block(entry, std::nullopt);
    if (!rendered_entry.has_value()) {
      return std::nullopt;
    }
    std::string rendered_same_module_globals;
    for (const auto& global : module.module.globals) {
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
  };
  const auto render_local_i32_arithmetic_guard_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 3 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        prepared_arch != c4c::TargetArch::X86_64 || entry.insts.empty()) {
      return std::nullopt;
    }

    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
    if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
        false_block == &entry ||
        true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
        !true_block->insts.empty() || !false_block->insts.empty()) {
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
    const auto render_guard_return =
        [&](std::int32_t returned_imm) -> std::string {
      std::string rendered = "    mov eax, " + std::to_string(returned_imm) + "\n";
      if (layout->frame_size != 0) {
        rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
      }
      rendered += "    ret\n";
      return rendered;
    };

    std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_operand;
    std::function<std::optional<std::string>(const c4c::backend::bir::Value&)> render_i32_value;
    render_i32_operand =
        [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
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
    render_i32_value =
        [&](const c4c::backend::bir::Value& value) -> std::optional<std::string> {
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

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }

    for (std::size_t index = 0; index + 1 < entry.insts.size(); ++index) {
      const auto& inst = entry.insts[index];
      if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst)) {
        if (store->byte_offset != 0 || store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
            store->value.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        std::optional<std::string> memory;
        if (store->address.has_value()) {
          memory = render_local_address_operand_if_supported(*layout, store->address, "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(store->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(slot_it->second, "DWORD");
        }
        if (!memory.has_value()) {
          return std::nullopt;
        }
        asm_text += "    mov " + *memory + ", " +
                    std::to_string(static_cast<std::int32_t>(store->value.immediate)) + "\n";
        continue;
      }

      if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst)) {
        if (load->byte_offset != 0 || load->result.type != c4c::backend::bir::TypeKind::I32) {
          return std::nullopt;
        }
        std::optional<std::string> memory;
        if (load->address.has_value()) {
          memory = render_local_address_operand_if_supported(*layout, load->address, "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(load->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_stack_memory_operand(slot_it->second, "DWORD");
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

    const bool lhs_is_value_rhs_is_imm =
        compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->rhs.type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_value_lhs_is_imm =
        compare->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        compare->lhs.type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_value_rhs_is_imm && !rhs_is_value_lhs_is_imm) {
      return std::nullopt;
    }

    const auto compared_value = lhs_is_value_rhs_is_imm ? compare->lhs : compare->rhs;
    const auto compared_render = render_i32_value(compared_value);
    if (!compared_render.has_value()) {
      return std::nullopt;
    }
    const auto compare_immediate =
        lhs_is_value_rhs_is_imm ? compare->rhs.immediate : compare->lhs.immediate;
    asm_text += *compared_render;
    asm_text += compare_immediate == 0
                    ? "    test eax, eax\n"
                    : "    cmp eax, " +
                          std::to_string(static_cast<std::int32_t>(compare_immediate)) + "\n";
    asm_text += std::string("    ") +
                (compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je") + " .L" +
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
  };
  const auto render_loop_join_countdown_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 4 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }

    const auto* function_control_flow = find_control_flow_function();
    const auto* loop_block = find_block(entry.terminator.target_label);
    if (function_control_flow == nullptr || loop_block == nullptr || loop_block == &entry ||
        loop_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto* branch_condition = c4c::backend::prepare::find_prepared_branch_condition(
        *function_control_flow, loop_block->label);
    if (branch_condition == nullptr || !branch_condition->predicate.has_value() ||
        !branch_condition->compare_type.has_value() || !branch_condition->lhs.has_value() ||
        !branch_condition->rhs.has_value() ||
        *branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
        branch_condition->condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
        branch_condition->condition_value.name != loop_block->terminator.condition.name) {
      return std::nullopt;
    }

    const auto* carried_counter = [&]() -> const c4c::backend::bir::Value* {
      if (branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
          branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32 &&
          branch_condition->rhs->immediate == 0) {
        return &*branch_condition->lhs;
      }
      if (branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
          branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
          branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32 &&
          branch_condition->lhs->immediate == 0) {
        return &*branch_condition->rhs;
      }
      return static_cast<const c4c::backend::bir::Value*>(nullptr);
    }();
    if (carried_counter == nullptr) {
      return std::nullopt;
    }

    const auto* join_transfer = c4c::backend::prepare::find_prepared_join_transfer(
        *function_control_flow, loop_block->label, carried_counter->name);
    if (join_transfer == nullptr ||
        join_transfer->kind != c4c::backend::prepare::PreparedJoinTransferKind::LoopCarry ||
        !join_transfer->storage_name.has_value() ||
        join_transfer->result.type != c4c::backend::bir::TypeKind::I32 ||
        join_transfer->edge_transfers.size() != 2) {
      return std::nullopt;
    }
    const auto* join_edges = incoming_transfers_for_join(
        *function_control_flow, join_transfer->join_block_label, join_transfer->result.name);
    if (join_edges == nullptr || join_edges->size() != 2) {
      return std::nullopt;
    }

    const bool lhs_is_counter_rhs_is_zero =
        branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
        branch_condition->lhs->name == join_transfer->result.name &&
        branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32 &&
        branch_condition->rhs->immediate == 0;
    const bool rhs_is_counter_lhs_is_zero =
        branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
        branch_condition->rhs->name == join_transfer->result.name &&
        branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32 &&
        branch_condition->lhs->immediate == 0;
    if (!lhs_is_counter_rhs_is_zero && !rhs_is_counter_lhs_is_zero) {
      return std::nullopt;
    }

    std::string body_label;
    std::string exit_label;
    if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Ne) {
      body_label = branch_condition->true_label;
      exit_label = branch_condition->false_label;
    } else if (*branch_condition->predicate == c4c::backend::bir::BinaryOpcode::Eq) {
      body_label = branch_condition->false_label;
      exit_label = branch_condition->true_label;
    } else {
      return std::nullopt;
    }

    const auto* body_block = find_block(body_label);
    const auto* exit_block = find_block(exit_label);
    if (loop_block == nullptr || body_block == nullptr || exit_block == nullptr ||
        entry.terminator.target_label != loop_block->label || loop_block == &entry ||
        body_block == &entry || exit_block == &entry || body_block == loop_block ||
        exit_block == loop_block || exit_block == body_block ||
        loop_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        loop_block->terminator.true_label != branch_condition->true_label ||
        loop_block->terminator.false_label != branch_condition->false_label ||
        body_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        body_block->terminator.target_label != loop_block->label ||
        exit_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !exit_block->terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto& exit_value = *exit_block->terminator.value;
    const bool exit_returns_counter =
        exit_value.kind == c4c::backend::bir::Value::Kind::Named &&
        exit_value.type == c4c::backend::bir::TypeKind::I32 &&
        exit_value.name == join_transfer->result.name;
    const bool exit_returns_zero =
        exit_value.kind == c4c::backend::bir::Value::Kind::Immediate &&
        exit_value.type == c4c::backend::bir::TypeKind::I32 && exit_value.immediate == 0;
    if (!exit_returns_counter && !exit_returns_zero) {
      return std::nullopt;
    }

    if (body_block->insts.size() < 1 || loop_block->insts.size() < 1) {
      return std::nullopt;
    }
    const auto* body_update =
        std::get_if<c4c::backend::bir::BinaryInst>(&body_block->insts.front());
    if (body_update == nullptr || body_update->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
        body_update->operand_type != c4c::backend::bir::TypeKind::I32 ||
        body_update->result.type != c4c::backend::bir::TypeKind::I32 ||
        body_update->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
        body_update->lhs.name != join_transfer->result.name ||
        body_update->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
        body_update->rhs.type != c4c::backend::bir::TypeKind::I32 ||
        body_update->rhs.immediate != 1) {
      return std::nullopt;
    }

    const c4c::backend::prepare::PreparedEdgeValueTransfer* entry_incoming = nullptr;
    const c4c::backend::prepare::PreparedEdgeValueTransfer* body_incoming = nullptr;
    for (const auto& incoming : *join_edges) {
      if (incoming.predecessor_label == entry.label) {
        entry_incoming = &incoming;
      } else if (incoming.predecessor_label == body_block->label) {
        body_incoming = &incoming;
      }
    }
    if (entry_incoming == nullptr || body_incoming == nullptr ||
        entry_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Immediate ||
        entry_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
        entry_incoming->incoming_value.immediate < 0 ||
        body_incoming->incoming_value.kind != c4c::backend::bir::Value::Kind::Named ||
        body_incoming->incoming_value.type != c4c::backend::bir::TypeKind::I32 ||
        body_incoming->incoming_value.name != body_update->result.name) {
      return std::nullopt;
    }

    const auto loop_label = ".L" + function.name + "_" + loop_block->label;
    const auto body_label_asm = ".L" + function.name + "_" + body_block->label;
    const auto exit_label_asm = ".L" + function.name + "_" + exit_block->label;

    auto asm_text = asm_prefix;
    asm_text += "    mov eax, " +
                std::to_string(static_cast<std::int32_t>(entry_incoming->incoming_value.immediate)) +
                "\n";
    asm_text += loop_label + ":\n";
    asm_text += "    test eax, eax\n";
    asm_text += "    je " + exit_label_asm + "\n";
    asm_text += body_label_asm + ":\n";
    asm_text += "    sub eax, 1\n";
    asm_text += "    jmp " + loop_label + "\n";
    asm_text += exit_label_asm + ":\n";
    if (exit_returns_zero) {
      asm_text += "    mov eax, 0\n";
    }
    asm_text += "    ret\n";
    return asm_text;
  };
  const auto render_local_i32_countdown_loop_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() < 4 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
        prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }

    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }

    const auto resolve_slot_return_block =
        [&](std::string_view label,
            std::string_view slot_name) -> const c4c::backend::bir::Block* {
          const auto* block = find_block(label);
          if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
              !block->terminator.value.has_value() || block->insts.size() != 1) {
            return nullptr;
          }
          const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&block->insts.front());
          if (load == nullptr || load->address.has_value() || load->byte_offset != 0 ||
              load->result.type != c4c::backend::bir::TypeKind::I32 ||
              load->slot_name != slot_name ||
              block->terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
              block->terminator.value->type != c4c::backend::bir::TypeKind::I32 ||
              block->terminator.value->name != load->result.name) {
            return nullptr;
          }
          return block;
        };

    const auto resolve_immediate_return_block =
        [&](std::string_view label) -> const c4c::backend::bir::Block* {
          const auto* block = find_block(label);
          if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
              !block->terminator.value.has_value() || !block->insts.empty() ||
              block->terminator.value->kind != c4c::backend::bir::Value::Kind::Immediate ||
              block->terminator.value->type != c4c::backend::bir::TypeKind::I32) {
            return nullptr;
          }
          return block;
        };

    const auto match_counter_init_block =
        [&](const c4c::backend::bir::Block* block,
            std::optional<std::string_view> expected_slot_name)
        -> std::optional<std::pair<const c4c::backend::bir::StoreLocalInst*,
                                   const c4c::backend::bir::Block*>> {
          if (block == nullptr || block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch ||
              block->insts.empty()) {
            return std::nullopt;
          }

          const c4c::backend::bir::StoreLocalInst* last_store = nullptr;
          for (const auto& inst : block->insts) {
            const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
            if (store == nullptr || store->address.has_value() || store->byte_offset != 0 ||
                store->value.kind != c4c::backend::bir::Value::Kind::Immediate ||
                store->value.type != c4c::backend::bir::TypeKind::I32) {
              return std::nullopt;
            }
            if (expected_slot_name.has_value() && store->slot_name != *expected_slot_name) {
              return std::nullopt;
            }
            if (last_store != nullptr && store->slot_name != last_store->slot_name) {
              return std::nullopt;
            }
            last_store = store;
          }

          const auto* next_block = find_block(block->terminator.target_label);
          if (last_store == nullptr || next_block == nullptr) {
            return std::nullopt;
          }
          return std::pair{last_store, next_block};
        };

    struct CountdownSegment {
      const c4c::backend::bir::Block* init_block = nullptr;
      const c4c::backend::bir::StoreLocalInst* init_store = nullptr;
      const c4c::backend::bir::Block* entry_target = nullptr;
      const c4c::backend::bir::Block* cond_block = nullptr;
      const c4c::backend::bir::Block* body_block = nullptr;
      const c4c::backend::bir::Block* return_block = nullptr;
      const c4c::backend::bir::Block* guard_block = nullptr;
      const c4c::backend::bir::Block* guard_true_return_block = nullptr;
      const c4c::backend::bir::Block* continuation_init_block = nullptr;
    };

    const auto match_countdown_segment =
        [&](const c4c::backend::bir::Block* init_block,
            std::optional<std::string_view> expected_slot_name,
            bool allow_guard_exit) -> std::optional<CountdownSegment> {
          const auto init_info = match_counter_init_block(init_block, expected_slot_name);
          if (!init_info.has_value()) {
            return std::nullopt;
          }

          CountdownSegment segment;
          segment.init_block = init_block;
          segment.init_store = init_info->first;
          segment.entry_target = init_info->second;
          if (layout->offsets.find(segment.init_store->slot_name) == layout->offsets.end() ||
              segment.entry_target == nullptr || segment.entry_target == init_block) {
            return std::nullopt;
          }

          const auto matches_cond_block =
              [&](const c4c::backend::bir::Block* cond_block) -> bool {
            if (cond_block == nullptr || cond_block == init_block || cond_block->insts.size() != 2 ||
                cond_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
              return false;
            }
            const auto* cond_load =
                std::get_if<c4c::backend::bir::LoadLocalInst>(&cond_block->insts.front());
            const auto* cond_compare =
                std::get_if<c4c::backend::bir::BinaryInst>(&cond_block->insts.back());
            return cond_load != nullptr && cond_compare != nullptr &&
                   !cond_load->address.has_value() && cond_load->byte_offset == 0 &&
                   cond_load->result.type == c4c::backend::bir::TypeKind::I32 &&
                   cond_load->slot_name == segment.init_store->slot_name &&
                   cond_compare->opcode == c4c::backend::bir::BinaryOpcode::Ne &&
                   cond_compare->operand_type == c4c::backend::bir::TypeKind::I32 &&
                   (cond_compare->result.type == c4c::backend::bir::TypeKind::I1 ||
                    cond_compare->result.type == c4c::backend::bir::TypeKind::I32) &&
                   cond_compare->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                   cond_compare->lhs.name == cond_load->result.name &&
                   cond_compare->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                   cond_compare->rhs.type == c4c::backend::bir::TypeKind::I32 &&
                   cond_compare->rhs.immediate == 0 &&
                   cond_block->terminator.condition.kind ==
                       c4c::backend::bir::Value::Kind::Named &&
                   cond_block->terminator.condition.name == cond_compare->result.name;
          };

          const auto matches_body_block =
              [&](const c4c::backend::bir::Block* body_block,
                  const c4c::backend::bir::Block* cond_block) -> bool {
            if (body_block == nullptr || body_block == init_block || body_block == cond_block ||
                body_block->insts.size() != 3 ||
                body_block->terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
              return false;
            }
            const auto* body_load =
                std::get_if<c4c::backend::bir::LoadLocalInst>(&body_block->insts[0]);
            const auto* body_sub =
                std::get_if<c4c::backend::bir::BinaryInst>(&body_block->insts[1]);
            const auto* body_store =
                std::get_if<c4c::backend::bir::StoreLocalInst>(&body_block->insts[2]);
            return body_load != nullptr && body_sub != nullptr && body_store != nullptr &&
                   !body_load->address.has_value() && body_load->byte_offset == 0 &&
                   body_load->result.type == c4c::backend::bir::TypeKind::I32 &&
                   body_load->slot_name == segment.init_store->slot_name &&
                   body_sub->opcode == c4c::backend::bir::BinaryOpcode::Sub &&
                   body_sub->operand_type == c4c::backend::bir::TypeKind::I32 &&
                   body_sub->result.type == c4c::backend::bir::TypeKind::I32 &&
                   body_sub->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                   body_sub->lhs.name == body_load->result.name &&
                   body_sub->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                   body_sub->rhs.type == c4c::backend::bir::TypeKind::I32 &&
                   body_sub->rhs.immediate == 1 &&
                   !body_store->address.has_value() && body_store->byte_offset == 0 &&
                   body_store->slot_name == segment.init_store->slot_name &&
                   body_store->value.kind == c4c::backend::bir::Value::Kind::Named &&
                   body_store->value.name == body_sub->result.name &&
                   body_store->value.type == c4c::backend::bir::TypeKind::I32 &&
                   find_block(body_block->terminator.target_label) == cond_block;
          };

          if (matches_cond_block(segment.entry_target)) {
            const auto* candidate_body = find_block(segment.entry_target->terminator.true_label);
            if (matches_body_block(candidate_body, segment.entry_target)) {
              segment.cond_block = segment.entry_target;
              segment.body_block = candidate_body;
            }
          }

          if (segment.cond_block == nullptr &&
              segment.entry_target->terminator.kind == c4c::backend::bir::TerminatorKind::Branch) {
            const auto* candidate_cond = find_block(segment.entry_target->terminator.target_label);
            if (matches_cond_block(candidate_cond) &&
                find_block(candidate_cond->terminator.true_label) == segment.entry_target &&
                matches_body_block(segment.entry_target, candidate_cond)) {
              segment.cond_block = candidate_cond;
              segment.body_block = segment.entry_target;
            }
          }

          if (segment.cond_block == nullptr || segment.body_block == nullptr) {
            return std::nullopt;
          }

          if (const auto* return_block =
                  resolve_slot_return_block(segment.cond_block->terminator.false_label,
                                            segment.init_store->slot_name);
              return_block != nullptr) {
            segment.return_block = return_block;
            return segment;
          }

          if (!allow_guard_exit) {
            return std::nullopt;
          }

          const auto* guard_block = find_block(segment.cond_block->terminator.false_label);
          if (guard_block == nullptr || guard_block->insts.size() != 2 ||
              guard_block->terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
            return std::nullopt;
          }

          const auto* guard_load =
              std::get_if<c4c::backend::bir::LoadLocalInst>(&guard_block->insts.front());
          const auto* guard_compare =
              std::get_if<c4c::backend::bir::BinaryInst>(&guard_block->insts.back());
          if (guard_load == nullptr || guard_compare == nullptr || guard_load->address.has_value() ||
              guard_load->byte_offset != 0 ||
              guard_load->result.type != c4c::backend::bir::TypeKind::I32 ||
              guard_load->slot_name != segment.init_store->slot_name ||
              guard_compare->opcode != c4c::backend::bir::BinaryOpcode::Ne ||
              guard_compare->operand_type != c4c::backend::bir::TypeKind::I32 ||
              (guard_compare->result.type != c4c::backend::bir::TypeKind::I1 &&
               guard_compare->result.type != c4c::backend::bir::TypeKind::I32) ||
              guard_compare->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
              guard_compare->lhs.name != guard_load->result.name ||
              guard_compare->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
              guard_compare->rhs.type != c4c::backend::bir::TypeKind::I32 ||
              guard_compare->rhs.immediate != 0 ||
              guard_block->terminator.condition.kind != c4c::backend::bir::Value::Kind::Named ||
              guard_block->terminator.condition.name != guard_compare->result.name) {
            return std::nullopt;
          }

          const auto* guard_true_return =
              resolve_immediate_return_block(guard_block->terminator.true_label);
          const auto* continuation_init = find_block(guard_block->terminator.false_label);
          if (guard_true_return == nullptr || continuation_init == nullptr ||
              continuation_init == guard_block) {
            return std::nullopt;
          }

          segment.guard_block = guard_block;
          segment.guard_true_return_block = guard_true_return;
          segment.continuation_init_block = continuation_init;
          return segment;
        };

    const auto first_segment = match_countdown_segment(&entry, std::nullopt, true);
    if (!first_segment.has_value()) {
      return std::nullopt;
    }

    std::optional<CountdownSegment> second_segment;
    if (first_segment->continuation_init_block != nullptr) {
      second_segment =
          match_countdown_segment(first_segment->continuation_init_block,
                                  std::string_view(first_segment->init_store->slot_name),
                                  false);
      if (!second_segment.has_value() || second_segment->continuation_init_block != nullptr) {
        return std::nullopt;
      }
    } else if (first_segment->return_block == nullptr) {
      return std::nullopt;
    }

    const auto slot_it = layout->offsets.find(first_segment->init_store->slot_name);
    if (slot_it == layout->offsets.end()) {
      return std::nullopt;
    }

    std::unordered_set<const c4c::backend::bir::Block*> used_blocks = {
        first_segment->init_block,
        first_segment->cond_block,
        first_segment->body_block,
    };
    if (first_segment->return_block != nullptr) {
      used_blocks.insert(first_segment->return_block);
    }
    if (first_segment->guard_block != nullptr) {
      used_blocks.insert(first_segment->guard_block);
    }
    if (first_segment->guard_true_return_block != nullptr) {
      used_blocks.insert(first_segment->guard_true_return_block);
    }
    if (first_segment->continuation_init_block != nullptr) {
      used_blocks.insert(first_segment->continuation_init_block);
    }
    if (second_segment.has_value()) {
      used_blocks.insert(second_segment->init_block);
      used_blocks.insert(second_segment->cond_block);
      used_blocks.insert(second_segment->body_block);
      if (second_segment->return_block != nullptr) {
        used_blocks.insert(second_segment->return_block);
      }
    }

    for (const auto& block : function.blocks) {
      if (used_blocks.find(&block) == used_blocks.end()) {
        return std::nullopt;
      }
    }

    const auto counter_memory = render_stack_memory_operand(slot_it->second, "DWORD");
    const auto render_segment =
        [&](const CountdownSegment& segment,
            const CountdownSegment* next_segment,
            bool emit_return,
            bool emit_init) -> std::string {
          const auto cond_label = ".L" + function.name + "_" + std::string(segment.cond_block->label);
          const auto body_label = ".L" + function.name + "_" + std::string(segment.body_block->label);
          std::string asm_text;
          if (emit_init) {
            asm_text += "    mov " + counter_memory + ", " +
                        std::to_string(static_cast<std::int32_t>(segment.init_store->value.immediate)) +
                        "\n";
            asm_text += "    jmp " +
                        (segment.entry_target == segment.body_block ? body_label : cond_label) + "\n";
          }
          asm_text += cond_label + ":\n";
          asm_text += "    mov eax, " + counter_memory + "\n";
          asm_text += "    test eax, eax\n";
          asm_text += "    jne " + body_label + "\n";
          if (next_segment != nullptr) {
            const auto next_cond_label =
                ".L" + function.name + "_" + std::string(next_segment->cond_block->label);
            const auto next_body_label =
                ".L" + function.name + "_" + std::string(next_segment->body_block->label);
            asm_text += "    mov " + counter_memory + ", " +
                        std::to_string(static_cast<std::int32_t>(next_segment->init_store->value.immediate)) +
                        "\n";
            asm_text += "    jmp " +
                        (next_segment->entry_target == next_segment->body_block ? next_body_label
                                                                                : next_cond_label) +
                        "\n";
          } else if (emit_return) {
            asm_text += "    mov eax, " + counter_memory + "\n";
            if (layout->frame_size != 0) {
              asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
            }
            asm_text += "    ret\n";
          } else {
            return std::string();
          }
          asm_text += body_label + ":\n";
          asm_text += "    mov eax, " + counter_memory + "\n";
          asm_text += "    sub eax, 1\n";
          asm_text += "    mov " + counter_memory + ", eax\n";
          asm_text += "    jmp " + cond_label + "\n";
          return asm_text;
        };

    auto asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    asm_text += render_segment(*first_segment,
                               second_segment.has_value() ? &*second_segment : nullptr,
                               second_segment.has_value() ? false : true,
                               true);
    if (second_segment.has_value()) {
      asm_text += render_segment(*second_segment, nullptr, true, false);
    }
    return asm_text;
  };
  const auto render_local_i16_arithmetic_guard_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 3 ||
        prepared_arch != c4c::TargetArch::X86_64 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        entry.insts.size() != 9) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
    if (!layout.has_value()) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
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

    const auto slot_it = layout->offsets.find(store_zero->slot_name);
    if (slot_it == layout->offsets.end()) {
      return std::nullopt;
    }
    const auto short_memory = render_stack_memory_operand(slot_it->second, "WORD");
    const auto render_return_block =
        [&](const c4c::backend::bir::Block& block) -> std::optional<std::string> {
      const auto& value = *block.terminator.value;
      if (value.kind != c4c::backend::bir::Value::Kind::Immediate ||
          value.type != c4c::backend::bir::TypeKind::I32) {
        return std::nullopt;
      }
      std::string rendered = "    mov eax, " +
                             std::to_string(static_cast<std::int32_t>(value.immediate)) + "\n";
      if (layout->frame_size != 0) {
        rendered += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
      }
      rendered += "    ret\n";
      return rendered;
    };
    const auto rendered_true = render_return_block(*true_block);
    const auto rendered_false = render_return_block(*false_block);
    if (!rendered_true.has_value() || !rendered_false.has_value()) {
      return std::nullopt;
    }

    std::string asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    asm_text += "    mov " + short_memory + ", " +
                std::to_string(static_cast<std::int16_t>(store_zero->value.immediate)) + "\n";
    asm_text += "    movsx eax, " + short_memory + "\n";
    asm_text += "    add eax, 1\n";
    asm_text += "    mov " + short_memory + ", ax\n";
    asm_text += "    movsx eax, " + short_memory + "\n";
    asm_text += compare->rhs.immediate == 0
                    ? "    test eax, eax\n"
                    : "    cmp eax, " +
                          std::to_string(static_cast<std::int32_t>(compare->rhs.immediate)) + "\n";
    asm_text += "    " +
                std::string(compare->opcode == c4c::backend::bir::BinaryOpcode::Eq ? "jne" : "je") +
                " .L" + function.name + "_" + false_block->label + "\n";
    asm_text += *rendered_true;
    asm_text += ".L" + function.name + "_" + false_block->label + ":\n";
    asm_text += *rendered_false;
    return asm_text;
  };
  const auto render_local_i16_i64_sub_return_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value() || entry.insts.size() != 10) {
      return std::nullopt;
    }
    const auto layout = build_local_slot_layout();
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
    if (store_short->slot_name != load_short->slot_name ||
        store_short->slot_name != store_result->slot_name ||
        store_short->slot_name != load_result->slot_name ||
        store_long->slot_name != load_long->slot_name) {
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

    const auto short_slot_it = layout->offsets.find(store_short->slot_name);
    const auto long_slot_it = layout->offsets.find(store_long->slot_name);
    if (short_slot_it == layout->offsets.end() || long_slot_it == layout->offsets.end()) {
      return std::nullopt;
    }
    const auto short_memory = render_stack_memory_operand(short_slot_it->second, "WORD");
    const auto long_memory = render_stack_memory_operand(long_slot_it->second, "QWORD");

    std::string asm_text = asm_prefix;
    if (layout->frame_size != 0) {
      asm_text += "    sub rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    asm_text += "    mov " + short_memory + ", " +
                std::to_string(static_cast<std::int16_t>(store_short->value.immediate)) + "\n";
    asm_text += "    mov " + long_memory + ", " +
                std::to_string(static_cast<std::int64_t>(store_long->value.immediate)) + "\n";
    asm_text += "    movsx rax, " + short_memory + "\n";
    asm_text += "    sub rax, " + long_memory + "\n";
    asm_text += "    mov " + short_memory + ", ax\n";
    asm_text += "    movsx eax, " + short_memory + "\n";
    if (layout->frame_size != 0) {
      asm_text += "    add rsp, " + std::to_string(layout->frame_size) + "\n";
    }
    asm_text += "    ret\n";
    return asm_text;
  };
  const auto render_param_derived_value_if_supported =
      [&](const c4c::backend::bir::Value& value,
          const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
              named_binaries,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    if (value.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }
    if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
      return "    mov " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(value.immediate)) +
             "\n";
    }
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }
    if (value.name == param.name) {
      if (const auto param_register = minimal_param_register(param); param_register.has_value()) {
        return "    mov " + *return_register + ", " + *param_register + "\n";
      }
      return std::nullopt;
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

    const auto immediate =
        lhs_is_param_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
      return "    mov " + *return_register + ", " + *param_register + "\n    add " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    sub " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      return "    mov " + *return_register + ", " + *param_register + "\n    imul " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
      return "    mov " + *return_register + ", " + *param_register + "\n    and " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
      return "    mov " + *return_register + ", " + *param_register + "\n    or " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
      return "    mov " + *return_register + ", " + *param_register + "\n    xor " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    shl " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    shr " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
      return "    mov " + *return_register + ", " + *param_register + "\n    sar " +
             *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    return std::nullopt;
  };
  const auto apply_supported_immediate_binary =
      [&](const c4c::backend::bir::BinaryInst& binary,
          std::string_view source_name) -> std::optional<std::string> {
    if (binary.operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary.result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    const bool lhs_is_source_rhs_is_imm =
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.lhs.name == source_name &&
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.rhs.type == c4c::backend::bir::TypeKind::I32;
    const bool rhs_is_source_lhs_is_imm =
        binary.rhs.kind == c4c::backend::bir::Value::Kind::Named &&
        binary.rhs.name == source_name &&
        binary.lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
        binary.lhs.type == c4c::backend::bir::TypeKind::I32;
    if (!lhs_is_source_rhs_is_imm && !rhs_is_source_lhs_is_imm) {
      return std::nullopt;
    }

    const auto immediate =
        lhs_is_source_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Add) {
      return "    add " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_source_rhs_is_imm) {
      return "    sub " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      return "    imul " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::And) {
      return "    and " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Or) {
      return "    or " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Xor) {
      return "    xor " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_source_rhs_is_imm) {
      return "    shl " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_source_rhs_is_imm) {
      return "    shr " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    if (binary.opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_source_rhs_is_imm) {
      return "    sar " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(binary.rhs.immediate)) + "\n";
    }
    return std::nullopt;
  };
  const auto find_prepared_eq_i32_branch_condition =
      [&](const c4c::backend::bir::Block& source_block)
      -> const c4c::backend::prepare::PreparedBranchCondition* {
    if (source_block.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        source_block.terminator.condition.kind != c4c::backend::bir::Value::Kind::Named) {
      return nullptr;
    }

    const auto* function_control_flow = find_control_flow_function();
    if (function_control_flow == nullptr) {
      return nullptr;
    }

    const auto* branch_condition = c4c::backend::prepare::find_prepared_branch_condition(
        *function_control_flow, source_block.label);
    if (branch_condition == nullptr || !branch_condition->predicate.has_value() ||
        !branch_condition->compare_type.has_value() || !branch_condition->lhs.has_value() ||
        !branch_condition->rhs.has_value() ||
        *branch_condition->predicate != c4c::backend::bir::BinaryOpcode::Eq ||
        *branch_condition->compare_type != c4c::backend::bir::TypeKind::I32 ||
        branch_condition->condition_value.kind != c4c::backend::bir::Value::Kind::Named ||
        branch_condition->condition_value.name != source_block.terminator.condition.name ||
        branch_condition->true_label != source_block.terminator.true_label ||
        branch_condition->false_label != source_block.terminator.false_label) {
      return nullptr;
    }

    return branch_condition;
  };
  const auto find_prepared_eq_i32_param_zero_branch_condition =
      [&](const c4c::backend::bir::Block& source_block,
          const c4c::backend::bir::Param& param)
      -> const c4c::backend::prepare::PreparedBranchCondition* {
    const auto* branch_condition = find_prepared_eq_i32_branch_condition(source_block);
    if (branch_condition == nullptr) {
      return nullptr;
    }

    const bool lhs_is_param_rhs_is_zero =
        branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Named &&
        branch_condition->lhs->name == param.name &&
        branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        branch_condition->rhs->type == c4c::backend::bir::TypeKind::I32 &&
        branch_condition->rhs->immediate == 0;
    const bool rhs_is_param_lhs_is_zero =
        branch_condition->rhs->kind == c4c::backend::bir::Value::Kind::Named &&
        branch_condition->rhs->name == param.name &&
        branch_condition->lhs->kind == c4c::backend::bir::Value::Kind::Immediate &&
        branch_condition->lhs->type == c4c::backend::bir::TypeKind::I32 &&
        branch_condition->lhs->immediate == 0;
    if (!lhs_is_param_rhs_is_zero && !rhs_is_param_lhs_is_zero) {
      return nullptr;
    }

    return branch_condition;
  };
  const auto render_param_derived_return_if_supported =
      [&](const c4c::backend::bir::Value& value,
          const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
              named_binaries,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    const auto value_render = render_param_derived_value_if_supported(value, named_binaries, param);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return *value_render + "    ret\n";
  };
  const auto render_materialized_compare_join_return_if_supported =
      [&](const MaterializedCompareJoinContext& compare_join_context,
          const c4c::backend::bir::Value& selected_value,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    const auto* join_block = compare_join_context.join_block;
    if (join_block == nullptr || compare_join_context.carrier_index > join_block->insts.size()) {
      return std::nullopt;
    }

    std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
    for (std::size_t inst_index = 0; inst_index < compare_join_context.carrier_index;
         ++inst_index) {
      const auto* binary =
          std::get_if<c4c::backend::bir::BinaryInst>(&join_block->insts[inst_index]);
      if (binary == nullptr) {
        return std::nullopt;
      }
      named_binaries.emplace(binary->result.name, binary);
    }

    const auto value_render =
        render_param_derived_value_if_supported(selected_value, named_binaries, param);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    if (compare_join_context.trailing_binary == nullptr) {
      return *value_render + "    ret\n";
    }

    const auto trailing_render = apply_supported_immediate_binary(
        *compare_join_context.trailing_binary, compare_join_context.joined_value_name);
    if (!trailing_render.has_value()) {
      return std::nullopt;
    }
    return *value_render + *trailing_render + "    ret\n";
  };
  const auto render_minimal_compare_branch_if_supported =
      [&]() -> std::optional<std::string> {
    if (function.blocks.size() != 3 || function.params.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 || entry.insts.size() != 1 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto& param = function.params.front();
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
        param.is_byval) {
      return std::nullopt;
    }
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }

    const auto* branch_condition = find_prepared_eq_i32_param_zero_branch_condition(entry, param);
    if (branch_condition == nullptr) {
      return std::nullopt;
    }

    const auto* true_block = find_block(entry.terminator.true_label);
    const auto* false_block = find_block(entry.terminator.false_label);
    if (true_block == nullptr || false_block == nullptr || true_block == &entry ||
        false_block == &entry || true_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        false_block->terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
        !true_block->insts.empty() || !false_block->insts.empty()) {
      return std::nullopt;
    }

    const auto& true_value = *true_block->terminator.value;
    const auto& false_value = *false_block->terminator.value;
    const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*> named_binaries;
    const auto true_return =
        render_param_derived_return_if_supported(true_value, named_binaries, param);
    const auto false_return =
        render_param_derived_return_if_supported(false_value, named_binaries, param);
    if (!true_return.has_value() || !false_return.has_value()) {
      return std::nullopt;
    }

    const std::string false_label = ".L" + function.name + "_" + false_block->label;
    return asm_prefix + "    test " + *param_register + ", " + *param_register + "\n    jne " +
           false_label + "\n" + *true_return +
           false_label + ":\n" + *false_return;
  };
  const auto render_materialized_compare_join_if_supported =
      [&]() -> std::optional<std::string> {
    if (function.blocks.size() != 4 || function.params.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
      return std::nullopt;
    }

    const auto& param = function.params.front();
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
        param.is_byval) {
      return std::nullopt;
    }
    const auto param_register = minimal_param_register(param);
    if (!param_register.has_value()) {
      return std::nullopt;
    }

    const auto* function_control_flow = find_control_flow_function();
    const auto* branch_condition = find_prepared_eq_i32_param_zero_branch_condition(entry, param);
    if (function_control_flow == nullptr || branch_condition == nullptr) {
      return std::nullopt;
    }

    const auto authoritative_join_transfer =
        find_authoritative_branch_owned_join_transfer(*function_control_flow, entry.label);
    if (!authoritative_join_transfer.has_value()) {
      return std::nullopt;
    }
    const auto compare_join_context =
        find_materialized_compare_join_context(*authoritative_join_transfer,
                                              branch_condition->true_label,
                                              branch_condition->false_label);
    if (!compare_join_context.has_value()) {
      return std::nullopt;
    }

    const auto* true_transfer = compare_join_context->true_transfer;
    const auto* false_transfer = compare_join_context->false_transfer;
    const auto* true_join_predecessor = compare_join_context->true_predecessor;
    const auto* false_join_predecessor = compare_join_context->false_predecessor;
    const auto true_return = render_materialized_compare_join_return_if_supported(
        *compare_join_context, true_transfer->incoming_value, param);
    const auto false_return = render_materialized_compare_join_return_if_supported(
        *compare_join_context, false_transfer->incoming_value, param);
    if (!true_return.has_value() || !false_return.has_value()) {
      return std::nullopt;
    }

    const std::string false_label = ".L" + function.name + "_" + false_join_predecessor->label;
    return asm_prefix + "    test " + *param_register + ", " + *param_register + "\n    jne " +
           false_label + "\n" + *true_return +
           false_label + ":\n" + *false_return;
  };
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    if (const auto rendered_branch = render_minimal_compare_branch_if_supported();
        rendered_branch.has_value()) {
      return *rendered_branch;
    }
    if (const auto rendered_join = render_materialized_compare_join_if_supported();
        rendered_join.has_value()) {
      return *rendered_join;
    }
    if (const auto rendered_local_i32_guard = render_local_i32_arithmetic_guard_if_supported();
        rendered_local_i32_guard.has_value()) {
      return *rendered_local_i32_guard;
    }
    if (const auto rendered_loop_join_countdown = render_loop_join_countdown_if_supported();
        rendered_loop_join_countdown.has_value()) {
      return *rendered_loop_join_countdown;
    }
    if (const auto rendered_countdown_loop = render_local_i32_countdown_loop_if_supported();
        rendered_countdown_loop.has_value()) {
      return *rendered_countdown_loop;
    }
    if (const auto rendered_local_i16_guard = render_local_i16_arithmetic_guard_if_supported();
        rendered_local_i16_guard.has_value()) {
      return *rendered_local_i16_guard;
    }
    if (const auto rendered_local_slot_guard_chain = render_local_slot_guard_chain_if_supported();
        rendered_local_slot_guard_chain.has_value()) {
      return *rendered_local_slot_guard_chain;
    }
    throw_multi_defined_contract_if_active();
    throw std::invalid_argument(
        "x86 backend emitter only supports a minimal single-block i32 return terminator, a bounded equality-against-immediate guard family with immediate return leaves including fixed-offset same-module global i32 loads and pointer-backed same-module global roots, or one bounded compare-against-zero branch family through the canonical prepared-module handoff");
  }

  const auto& returned = *entry.terminator.value;
  if (returned.type != c4c::backend::bir::TypeKind::I32) {
    throw_multi_defined_contract_if_active();
    throw std::invalid_argument(
        "x86 backend emitter only supports i32 return values through the canonical prepared-module handoff");
  }
  const auto render_minimal_direct_extern_call_sequence_if_supported =
      [&]() -> std::optional<std::string> {
    if (prepared_arch != c4c::TargetArch::X86_64 || !function.params.empty() ||
        !function.local_slots.empty() || function.blocks.size() != 1 ||
        entry.label != "entry" || entry.insts.empty() ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value()) {
      return std::nullopt;
    }

    static constexpr const char* kArgRegs64[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    std::unordered_map<std::string_view, std::int64_t> i32_constants;
    std::unordered_set<std::string_view> emitted_string_names;
    std::vector<const c4c::backend::bir::StringConstant*> used_string_constants;
    std::unordered_set<std::string_view> used_same_module_globals;
    std::string body = "    sub rsp, 8\n";
    bool saw_call = false;
    std::optional<std::string_view> current_i32_name;

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
        current_i32_name = std::nullopt;
        continue;
      }

      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr || call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
          call->args.size() != call->arg_types.size() || call->args.size() > 6) {
        return std::nullopt;
      }
      saw_call = true;

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
        const auto arg_value = resolve_i32_constant(arg);
        if (!arg_value.has_value()) {
          return std::nullopt;
        }
        body += "    mov ";
        body += kArgRegs32[arg_index];
        body += ", ";
        body += std::to_string(static_cast<std::int32_t>(*arg_value));
        body += "\n";
      }

      body += "    xor eax, eax\n";
      body += "    call ";
      body += render_asm_symbol_name(call->callee);
      body += "\n";
      current_i32_name =
          call->result.has_value() && call->result->type == c4c::backend::bir::TypeKind::I32
              ? std::optional<std::string_view>(call->result->name)
              : std::nullopt;
    }

    if (!saw_call) {
      return std::nullopt;
    }

    if (entry.terminator.value->kind == c4c::backend::bir::Value::Kind::Named &&
        current_i32_name.has_value() && entry.terminator.value->name == *current_i32_name) {
      body += "    add rsp, 8\n    ret\n";
    } else {
      const auto returned_value = resolve_i32_constant(*entry.terminator.value);
      if (!returned_value.has_value()) {
        return std::nullopt;
      }
      body += "    mov ";
      body += *return_register;
      body += ", ";
      body += std::to_string(static_cast<std::int32_t>(*returned_value));
      body += "\n    add rsp, 8\n    ret\n";
    }

    std::string rendered_data;
    for (const auto* string_constant : used_string_constants) {
      rendered_data += emit_string_constant_data(*string_constant);
    }
    for (const auto& global : module.module.globals) {
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
    return prepend_bounded_same_module_helpers(asm_prefix + body + rendered_data);
  };
  if (const auto rendered_direct_calls = render_minimal_direct_extern_call_sequence_if_supported();
      rendered_direct_calls.has_value()) {
    return *rendered_direct_calls;
  }
  if (const auto rendered_constant_folded =
          render_constant_folded_single_block_return_if_supported();
      rendered_constant_folded.has_value()) {
    return *rendered_constant_folded;
  }
  if (const auto rendered_local_slot = render_minimal_local_slot_return_if_supported();
      rendered_local_slot.has_value()) {
    return *rendered_local_slot;
  }
  if (const auto rendered_local_i16_i64_return =
          render_local_i16_i64_sub_return_if_supported();
      rendered_local_i16_i64_return.has_value()) {
    return *rendered_local_i16_i64_return;
  }
  if (entry.insts.empty()) {
    if (returned.kind == c4c::backend::bir::Value::Kind::Immediate) {
      if (!function.params.empty()) {
        throw_multi_defined_contract_if_active();
        throw std::invalid_argument(
            "x86 backend emitter only supports parameter-free immediate i32 returns through the canonical prepared-module handoff");
      }
      return asm_prefix + "    mov " + *return_register + ", " +
             std::to_string(static_cast<std::int32_t>(returned.immediate)) + "\n    ret\n";
    }

    if (function.params.size() == 1 && prepared_arch == c4c::TargetArch::X86_64) {
      const auto& param = function.params.front();
      if (param.type == c4c::backend::bir::TypeKind::I32 && !param.is_varargs &&
          !param.is_sret && !param.is_byval && returned.name == param.name) {
        if (const auto param_register = minimal_param_register(param); param_register.has_value()) {
          return asm_prefix + "    mov " + *return_register + ", " + *param_register + "\n    ret\n";
        }
      }
    }
  }

  if (entry.insts.size() == 1 && function.params.size() == 1 &&
      returned.kind == c4c::backend::bir::Value::Kind::Named &&
      prepared_arch == c4c::TargetArch::X86_64) {
    const auto& param = function.params.front();
    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
    if (binary != nullptr && param.type == c4c::backend::bir::TypeKind::I32 && !param.is_varargs &&
        !param.is_sret && !param.is_byval &&
        binary->operand_type == c4c::backend::bir::TypeKind::I32 &&
        binary->result.type == c4c::backend::bir::TypeKind::I32 &&
        returned.name == binary->result.name) {
      const bool lhs_is_param_rhs_is_imm =
          binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
          binary->lhs.name == param.name &&
          binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          binary->rhs.type == c4c::backend::bir::TypeKind::I32;
      const bool rhs_is_param_lhs_is_imm =
          binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
          binary->rhs.name == param.name &&
          binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
          binary->lhs.type == c4c::backend::bir::TypeKind::I32;
      const auto param_register = minimal_param_register(param);
      if (!param_register.has_value()) {
        throw_multi_defined_contract_if_active();
        throw std::invalid_argument(
            "x86 backend emitter requires prepared parameter ABI metadata for the canonical prepared-module handoff");
      }
      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    add " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    sub " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Mul &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    imul " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::And &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    and " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Or &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    or " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Xor &&
          (lhs_is_param_rhs_is_imm || rhs_is_param_lhs_is_imm)) {
        const auto immediate =
            lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    xor " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(immediate)) + "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Shl &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    shl " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::LShr &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    shr " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }

      if (binary->opcode == c4c::backend::bir::BinaryOpcode::AShr &&
          lhs_is_param_rhs_is_imm) {
        return asm_prefix + "    mov " + *return_register + ", " + *param_register +
               "\n    sar " + *return_register + ", " +
               std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) +
               "\n    ret\n";
      }
    }
  }

  throw_multi_defined_contract_if_active();
  throw std::invalid_argument(
      "x86 backend emitter only supports direct immediate i32 returns, constant-evaluable straight-line no-parameter i32 return expressions, direct single-parameter i32 passthrough returns, single-parameter i32 add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff");
}


}  // namespace c4c::backend::x86
