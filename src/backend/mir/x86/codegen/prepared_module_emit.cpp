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
  const auto bounded_same_module_helpers =
      function_ptr != nullptr
          ? render_prepared_bounded_same_module_helper_prefix_if_supported(
                defined_functions, *function_ptr, prepared_arch,
                render_trivial_defined_function_if_supported, minimal_function_return_register,
                minimal_function_asm_prefix, find_same_module_global, minimal_param_register_at,
                render_asm_symbol_name)
          : std::nullopt;
  const auto& bounded_same_module_helper_names =
      bounded_same_module_helpers
          ? bounded_same_module_helpers->helper_names
          : std::unordered_set<std::string_view>{};
  const auto& bounded_same_module_helper_global_names =
      bounded_same_module_helpers
          ? bounded_same_module_helpers->helper_global_names
          : std::unordered_set<std::string_view>{};
  const auto prepend_bounded_same_module_helpers = [&](std::string asm_text) {
    if (bounded_same_module_helpers.has_value()) {
      return bounded_same_module_helpers->helper_prefix + asm_text;
    }
    return asm_text;
  };
  const auto throw_multi_defined_contract = [&]() -> void {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  };
  const auto throw_multi_defined_contract_if_active = [&]() -> void {
    if (defined_functions.size() > 1 && bounded_same_module_helpers.has_value()) {
      throw_multi_defined_contract();
    }
  };
  if (const auto rendered_multi_defined = render_prepared_bounded_multi_defined_call_lane_if_supported(
          module.module, defined_functions, prepared_arch, bounded_same_module_helper_global_names,
          render_trivial_defined_function_if_supported, minimal_function_return_register,
          minimal_function_asm_prefix,
          [&](std::string_view symbol_name) { return find_string_constant(symbol_name) != nullptr; },
          [&](std::string_view symbol_name) { return find_same_module_global(symbol_name) != nullptr; },
          [&](std::string_view symbol_name) {
            const std::string prefixed_name = "@" + std::string(symbol_name);
            return render_private_data_label(prefixed_name);
          },
          [&](std::string_view symbol_name) { return render_asm_symbol_name(symbol_name); },
          [&](std::string_view symbol_name) { return find_string_constant(symbol_name); },
          emit_string_constant_data, emit_same_module_global_data);
      rendered_multi_defined.has_value()) {
    return *rendered_multi_defined;
  }
  if (defined_functions.size() > 1) {
    if (!bounded_same_module_helpers.has_value()) {
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
  const auto render_local_slot_guard_chain_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.empty() ||
        prepared_arch != c4c::TargetArch::X86_64) {
      return std::nullopt;
    }
    const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
    if (!layout.has_value()) {
      return std::nullopt;
    }
    static constexpr const char* kArgRegs32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    const auto* function_control_flow = find_control_flow_function();
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
                    continuation)
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
              return "    lea rax, " +
                     render_prepared_stack_address_expr(pointee_slot_it->second) +
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
                      [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan&
                              prepared_plan) -> std::optional<ShortCircuitPlan> {
                        return c4c::backend::x86::build_prepared_short_circuit_plan(
                            prepared_plan, find_block);
                      });
              if (compare_join_render_plan.has_value()) {
                return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
                    function.name,
                    body,
                    *compare_join_render_plan,
                    find_block,
                    render_block);
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

            const auto join_context = c4c::backend::x86::
                find_prepared_short_circuit_join_context_if_supported(
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
                    [&](const c4c::backend::prepare::PreparedShortCircuitBranchPlan&
                            prepared_plan) -> std::optional<ShortCircuitPlan> {
                      return c4c::backend::x86::build_prepared_short_circuit_plan(
                          prepared_plan, find_block);
                    });
            if (!short_circuit_render_plan.has_value()) {
              return std::nullopt;
            }
            return c4c::backend::x86::render_compare_driven_branch_plan_with_block_renderer(
                function.name,
                body,
                *short_circuit_render_plan,
                find_block,
                render_block);
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
              function.name,
              body,
              *plain_cond_render_plan,
              find_block,
              render_block);
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

    const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
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
          memory = render_prepared_local_address_operand_if_supported(*layout,
                                                                      store->address,
                                                                      "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(store->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_prepared_stack_memory_operand(slot_it->second, "DWORD");
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
          memory = render_prepared_local_address_operand_if_supported(*layout,
                                                                      load->address,
                                                                      "DWORD");
        } else {
          const auto slot_it = layout->offsets.find(load->slot_name);
          if (slot_it == layout->offsets.end()) {
            return std::nullopt;
          }
          memory = render_prepared_stack_memory_operand(slot_it->second, "DWORD");
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

    const auto* function_control_flow = find_control_flow_function();
    const auto* prepared_branch_condition =
        function_control_flow == nullptr
            ? nullptr
            : c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                                    entry.label);
    if (function_control_flow != nullptr &&
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            *function_control_flow, entry.label)
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
        const auto* candidate =
            c4c::backend::prepare::find_prepared_i32_immediate_branch_condition(
                *function_control_flow, entry.label, value_name);
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
      true_label = prepared_immediate_branch->true_label;
      false_label = prepared_immediate_branch->false_label;
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
  };
  const auto render_local_i16_arithmetic_guard_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 3 ||
        prepared_arch != c4c::TargetArch::X86_64 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch ||
        entry.insts.size() != 9) {
      return std::nullopt;
    }
    const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
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

    const auto slot_it = layout->offsets.find(store_zero->slot_name);
    if (slot_it == layout->offsets.end()) {
      return std::nullopt;
    }
    const auto short_memory = render_prepared_stack_memory_operand(slot_it->second, "WORD");
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
    const auto* function_control_flow = find_control_flow_function();
    const auto* prepared_branch_condition =
        function_control_flow == nullptr
            ? nullptr
            : c4c::backend::prepare::find_prepared_branch_condition(*function_control_flow,
                                                                    entry.label);
    if (function_control_flow != nullptr &&
        c4c::backend::prepare::find_authoritative_branch_owned_join_transfer(
            *function_control_flow, entry.label)
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
          prepared_branch_condition->condition_value.kind !=
              c4c::backend::bir::Value::Kind::Named) {
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
      true_label = prepared_branch_condition->true_label;
      false_label = prepared_branch_condition->false_label;
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
  };
  const auto render_local_i16_i64_sub_return_if_supported =
      [&]() -> std::optional<std::string> {
    if (!function.params.empty() || function.blocks.size() != 1 ||
        prepared_arch != c4c::TargetArch::X86_64 ||
        entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value() || entry.insts.size() != 10) {
      return std::nullopt;
    }
    const auto layout = build_prepared_module_local_slot_layout(function, prepared_arch);
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
    const auto short_memory =
        render_prepared_stack_memory_operand(short_slot_it->second, "WORD");
    const auto long_memory = render_prepared_stack_memory_operand(long_slot_it->second, "QWORD");

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
  const auto render_supported_immediate_binary =
      [&](const c4c::backend::prepare::PreparedSupportedImmediateBinary& binary)
      -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_supported_immediate_binary(*return_register,
                                                                         binary);
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
      [&](const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
              prepared_return_arm,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_materialized_compare_join_return_if_supported(
        *return_register,
        prepared_return_arm,
        param,
        minimal_param_register,
        render_asm_symbol_name,
        same_module_global_supports_scalar_load);
  };
  if (const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>
          named_binaries;
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    if (const auto rendered_compare_driven_entry =
            c4c::backend::x86::render_prepared_compare_driven_entry_if_supported(
        module.module,
        find_control_flow_function(),
        function,
        entry,
        prepared_arch,
        asm_prefix,
        minimal_param_register,
        [&](const c4c::backend::bir::Value& value) {
          const auto& param = function.params.front();
          return render_param_derived_return_if_supported(value, named_binaries, param);
        },
        render_materialized_compare_join_return_if_supported,
        emit_same_module_global_data);
        rendered_compare_driven_entry.has_value()) {
      return *rendered_compare_driven_entry;
    }
    if (const auto rendered_local_i32_guard = render_local_i32_arithmetic_guard_if_supported();
        rendered_local_i32_guard.has_value()) {
      return *rendered_local_i32_guard;
    }
    if (const auto rendered_countdown_entry =
            c4c::backend::x86::render_prepared_countdown_entry_routes_if_supported(
                function, entry, find_control_flow_function(), prepared_arch, asm_prefix);
        rendered_countdown_entry.has_value()) {
      return *rendered_countdown_entry;
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
          render_prepared_constant_folded_single_block_return_if_supported(
              function, prepared_arch, asm_prefix, *return_register);
      rendered_constant_folded.has_value()) {
    return *rendered_constant_folded;
  }
  if (const auto rendered_local_slot = render_prepared_minimal_local_slot_return_if_supported(
          function, prepared_arch, asm_prefix);
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
