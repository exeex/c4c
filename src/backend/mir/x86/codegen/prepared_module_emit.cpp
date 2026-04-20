#include "x86_codegen.hpp"

#include <algorithm>
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
  const c4c::FunctionNameId function_name_id =
      c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name)
          .value_or(c4c::kInvalidFunctionName);
  const auto asm_prefix = ".intel_syntax noprefix\n.text\n.globl " + function.name +
                          "\n.type " + function.name + ", @function\n" + function.name + ":\n";
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
    if (wide_register == "eax" || wide_register == "ebx" || wide_register == "ecx" ||
        wide_register == "edx" || wide_register == "edi" || wide_register == "esi" ||
        wide_register == "ebp" || wide_register == "esp" || wide_register == "r8d" ||
        wide_register == "r9d" || wide_register == "r10d" || wide_register == "r11d" ||
        wide_register == "r12d" || wide_register == "r13d" || wide_register == "r14d" ||
        wide_register == "r15d") {
      return std::string(wide_register);
    }
    return std::string(wide_register);
  };
  const auto narrow_register = [&](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return narrow_i32_register(*wide_register);
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
    if (function_name_id == c4c::kInvalidFunctionName) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_control_flow_function(
        module.control_flow, function_name_id);
  };
  const auto find_addressing_function =
      [&]() -> const c4c::backend::prepare::PreparedAddressingFunction* {
    if (function_name_id == c4c::kInvalidFunctionName) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_addressing(module, function_name_id);
  };
  const auto find_value_location_function =
      [&]() -> const c4c::backend::prepare::PreparedValueLocationFunction* {
    if (function_name_id == c4c::kInvalidFunctionName) {
      return nullptr;
    }
    return c4c::backend::prepare::find_prepared_value_location_function(
        module.value_locations, function_name_id);
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
    return c4c::backend::x86::render_prepared_trivial_defined_function_if_supported(
        candidate, prepared_arch, minimal_function_return_register, minimal_function_asm_prefix);
  };
  const auto multi_defined_dispatch = c4c::backend::x86::build_prepared_module_multi_defined_dispatch_state(
      module.module, defined_functions, function_ptr, prepared_arch,
      render_trivial_defined_function_if_supported, minimal_function_return_register,
      minimal_function_asm_prefix, find_same_module_global, minimal_param_register_at,
      [&](std::string_view symbol_name) { return find_string_constant(symbol_name) != nullptr; },
      [&](std::string_view symbol_name) { return find_same_module_global(symbol_name) != nullptr; },
      [&](std::string_view symbol_name) {
        const std::string prefixed_name = "@" + std::string(symbol_name);
        return render_private_data_label(prefixed_name);
      },
      [&](std::string_view symbol_name) { return render_asm_symbol_name(symbol_name); },
      [&](std::string_view symbol_name) { return find_string_constant(symbol_name); },
      emit_string_constant_data, emit_same_module_global_data);
  const auto& bounded_same_module_helper_names = multi_defined_dispatch.helper_names;
  const auto& bounded_same_module_helper_global_names =
      multi_defined_dispatch.helper_global_names;
  const auto prepend_bounded_same_module_helpers = [&](std::string asm_text) {
    if (!multi_defined_dispatch.helper_prefix.empty()) {
      return multi_defined_dispatch.helper_prefix + asm_text;
    }
    return asm_text;
  };
  const auto throw_multi_defined_contract = [&]() -> void {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  };
  const auto throw_multi_defined_contract_if_active = [&]() -> void {
    if (defined_functions.size() > 1 && multi_defined_dispatch.has_bounded_same_module_helpers) {
      throw_multi_defined_contract();
    }
  };
  if (multi_defined_dispatch.rendered_module.has_value()) {
    return *multi_defined_dispatch.rendered_module;
  }
  if (defined_functions.size() > 1) {
    if (!multi_defined_dispatch.has_bounded_same_module_helpers) {
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
    return c4c::backend::x86::render_prepared_local_slot_guard_chain_if_supported(
        module.module,
        function,
        entry,
        &module.stack_layout,
        find_addressing_function(),
        &module.names,
        find_control_flow_function(),
        prepared_arch,
        asm_prefix,
        bounded_same_module_helper_names,
        bounded_same_module_helper_global_names,
        find_block,
        find_same_module_global,
        same_module_global_supports_scalar_load,
        render_asm_symbol_name,
        emit_same_module_global_data,
        prepend_bounded_same_module_helpers);
  };
  const auto render_local_i32_arithmetic_guard_if_supported =
      [&]() -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_local_i32_arithmetic_guard_if_supported(
        function,
        entry,
        &module.stack_layout,
        find_addressing_function(),
        &module.names,
        find_control_flow_function(),
        prepared_arch,
        asm_prefix,
        find_block);
  };
  const auto render_local_i16_arithmetic_guard_if_supported =
      [&]() -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_local_i16_arithmetic_guard_if_supported(
        function,
        entry,
        &module.stack_layout,
        find_addressing_function(),
        &module.names,
        find_control_flow_function(),
        prepared_arch,
        asm_prefix,
        find_block);
  };
  const auto render_param_derived_return_if_supported =
      [&](const c4c::backend::bir::Value& value,
          const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>&
              named_binaries,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    const auto value_render = c4c::backend::x86::render_prepared_param_derived_i32_value_if_supported(
        *return_register, value, named_binaries, param, minimal_param_register);
    if (!value_render.has_value()) {
      return std::nullopt;
    }
    return c4c::backend::x86::render_prepared_return_body(*value_render);
  };
  const auto render_materialized_compare_join_return_if_supported =
      [&](const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
              prepared_return_arm,
          const c4c::backend::bir::Param& param) -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_materialized_compare_join_return_if_supported(
        *return_register,
        module.names,
        prepared_return_arm,
        param,
        minimal_param_register,
        render_asm_symbol_name,
        same_module_global_supports_scalar_load);
  };
  const auto render_minimal_scalar_move_bundle_return_if_supported =
      [&]() -> std::optional<std::string> {
    if (prepared_arch != c4c::TargetArch::X86_64 || function.params.size() != 1 ||
        function.blocks.size() != 1 || entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto* function_locations = find_value_location_function();
    if (function_locations == nullptr) {
      return std::nullopt;
    }

    const auto& param = function.params.front();
    if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
        param.is_byval) {
      return std::nullopt;
    }

    const auto narrow_home_register =
        [&](const c4c::backend::prepare::PreparedValueHome& home) -> std::optional<std::string> {
      if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !home.register_name.has_value()) {
        return std::nullopt;
      }
      return narrow_i32_register(*home.register_name);
    };
    struct MinimalScalarSourceHome {
      std::optional<std::string> register_name;
      std::optional<std::string> stack_operand;
      std::size_t frame_size = 0;
    };
    const auto find_named_source_home =
        [&](std::string_view value_name) -> std::optional<MinimalScalarSourceHome> {
      const auto* home =
          c4c::backend::prepare::find_prepared_value_home(module.names, *function_locations, value_name);
      if (home == nullptr) {
        return std::nullopt;
      }
      if (const auto register_name = narrow_home_register(*home); register_name.has_value()) {
        return MinimalScalarSourceHome{
            .register_name = *register_name,
            .stack_operand = std::nullopt,
            .frame_size = 0,
        };
      }
      if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
          home->offset_bytes.has_value()) {
        return MinimalScalarSourceHome{
            .register_name = std::nullopt,
            .stack_operand = c4c::backend::x86::render_prepared_stack_memory_operand(
                *home->offset_bytes, "DWORD"),
            .frame_size =
                std::max(module.stack_layout.frame_size_bytes,
                         *home->offset_bytes + sizeof(std::int32_t)),
        };
      }
      return std::nullopt;
    };
    const auto render_frame_wrapped_return =
        [&](std::string body, std::size_t frame_size) -> std::string {
      std::string rendered = asm_prefix;
      if (frame_size != 0) {
        rendered += "    sub rsp, " + std::to_string(frame_size) + "\n";
      }
      rendered += body;
      if (frame_size != 0) {
        rendered += "    add rsp, " + std::to_string(frame_size) + "\n";
      }
      rendered += "    ret\n";
      return rendered;
    };
    const auto source_param_home = find_named_source_home(param.name);
    if (!source_param_home.has_value()) {
      return std::nullopt;
    }

    const auto narrow_destination_register =
        [&](const c4c::backend::prepare::PreparedMoveResolution& move) -> std::optional<std::string> {
      if (move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::Register) {
        return std::nullopt;
      }
      if (move.destination_register_name.has_value()) {
        return narrow_i32_register(*move.destination_register_name);
      }
      if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::Value) {
        return std::nullopt;
      }
      const auto* destination_home =
          c4c::backend::prepare::find_prepared_value_home(*function_locations, move.to_value_id);
      if (destination_home == nullptr || destination_home->kind !=
                                         c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !destination_home->register_name.has_value()) {
        return std::nullopt;
      }
      return narrow_i32_register(*destination_home->register_name);
    };

    const auto* before_return_bundle = c4c::backend::prepare::find_prepared_move_bundle(
        *function_locations,
        c4c::backend::prepare::PreparedMovePhase::BeforeReturn,
        0,
        entry.insts.size());
    if (before_return_bundle == nullptr || before_return_bundle->moves.size() != 1) {
      return std::nullopt;
    }
    const auto return_destination_register =
        narrow_destination_register(before_return_bundle->moves.front());
    if (!return_destination_register.has_value()) {
      return std::nullopt;
    }

    const auto& returned = *entry.terminator.value;
    if (returned.type != c4c::backend::bir::TypeKind::I32 ||
        returned.kind != c4c::backend::bir::Value::Kind::Named) {
      return std::nullopt;
    }

    if (entry.insts.empty()) {
      if (returned.name != param.name) {
        return std::nullopt;
      }
      std::string body;
      if (source_param_home->register_name.has_value()) {
        if (*return_destination_register != *source_param_home->register_name) {
          body += "    mov " + *return_destination_register + ", " +
                  *source_param_home->register_name + "\n";
        }
      } else if (source_param_home->stack_operand.has_value()) {
        body += "    mov " + *return_destination_register + ", " +
                *source_param_home->stack_operand + "\n";
      } else {
        return std::nullopt;
      }
      return render_frame_wrapped_return(body, source_param_home->frame_size);
    }

    if (entry.insts.size() != 1) {
      return std::nullopt;
    }

    const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
    if (binary == nullptr || returned.name != binary->result.name ||
        binary->operand_type != c4c::backend::bir::TypeKind::I32 ||
        binary->result.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

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
    if (!lhs_is_param_rhs_is_imm && !rhs_is_param_lhs_is_imm) {
      return std::nullopt;
    }
    const auto* before_instruction_bundle = c4c::backend::prepare::find_prepared_move_bundle(
        *function_locations,
        c4c::backend::prepare::PreparedMovePhase::BeforeInstruction,
        0,
        0);
    if (before_instruction_bundle == nullptr || before_instruction_bundle->moves.size() != 1) {
      return std::nullopt;
    }
    const auto& entry_move = before_instruction_bundle->moves.front();
    if (entry_move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::Value ||
        entry_move.to_value_id == 0 || entry_move.to_value_id != before_return_bundle->moves.front().from_value_id) {
      return std::nullopt;
    }
    const auto instruction_destination_register = narrow_destination_register(entry_move);
    if (!instruction_destination_register.has_value()) {
      return std::nullopt;
    }

    const auto immediate = lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
    std::string body;
    if (source_param_home->register_name.has_value() &&
        *instruction_destination_register != *source_param_home->register_name) {
      body += "    mov " + *instruction_destination_register + ", " +
              *source_param_home->register_name + "\n";
    } else if (source_param_home->stack_operand.has_value()) {
      body += "    mov " + *instruction_destination_register + ", " +
              *source_param_home->stack_operand + "\n";
    } else {
      return std::nullopt;
    }
    if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add) {
      body += "    add " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Sub && lhs_is_param_rhs_is_imm) {
      body += "    sub " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Mul) {
      body += "    imul " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::And) {
      body += "    and " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Or) {
      body += "    or " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Xor) {
      body += "    xor " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Shl && lhs_is_param_rhs_is_imm) {
      body += "    shl " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::LShr && lhs_is_param_rhs_is_imm) {
      body += "    shr " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
    } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::AShr && lhs_is_param_rhs_is_imm) {
      body += "    sar " + *instruction_destination_register + ", " +
              std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
    } else {
      return std::nullopt;
    }
    if (*return_destination_register != *instruction_destination_register) {
      body += "    mov " + *return_destination_register + ", " + *instruction_destination_register + "\n";
    }
    return render_frame_wrapped_return(body, source_param_home->frame_size);
  };
  if (const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>
          named_binaries;
      entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value()) {
    if (const auto rendered_compare_driven_entry =
            c4c::backend::x86::render_prepared_compare_driven_entry_if_supported(
        module,
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
                function,
                entry,
                &module.names,
                find_control_flow_function(),
                prepared_arch,
                asm_prefix);
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
      if (const auto rendered_scalar_move_bundle_return =
              render_minimal_scalar_move_bundle_return_if_supported();
          rendered_scalar_move_bundle_return.has_value()) {
        return *rendered_scalar_move_bundle_return;
      }
      if (const auto rendered_single_block_return =
          c4c::backend::x86::render_prepared_single_block_return_dispatch_if_supported(
              module.module, function, entry, &module.stack_layout, find_addressing_function(), &module.names,
              prepared_arch, asm_prefix, *return_register,
              bounded_same_module_helper_global_names, find_string_constant, find_same_module_global,
              render_private_data_label, render_asm_symbol_name, emit_string_constant_data,
              emit_same_module_global_data, prepend_bounded_same_module_helpers,
              minimal_param_register);
      rendered_single_block_return.has_value()) {
    return *rendered_single_block_return;
  }

  throw_multi_defined_contract_if_active();
  throw std::invalid_argument(
      "x86 backend emitter only supports direct immediate i32 returns, constant-evaluable straight-line no-parameter i32 return expressions, direct single-parameter i32 passthrough returns, single-parameter i32 add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff");
}


}  // namespace c4c::backend::x86
