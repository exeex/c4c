#include "module_emit.hpp"
#include "../abi/x86_target_abi.hpp"
#include "module_data_emit.hpp"

#include <algorithm>
#include <functional>
#include <stdexcept>

namespace c4c::backend::x86::module {

namespace {

constexpr std::string_view kScalarPreparedControlFlowShapeError =
    "x86 backend emitter only supports a minimal single-block i32 return terminator, a bounded equality-against-immediate guard family with immediate return leaves including fixed-offset same-module global i32 loads and pointer-backed same-module global roots, or one bounded compare-against-zero branch family through the canonical prepared-module handoff";

}  // namespace

std::string emit_prepared_module_text(
    const c4c::backend::prepare::PreparedBirModule& module) {
  constexpr std::string_view kCompareDrivenEntryParamShapeError =
      "x86 backend emitter only supports multi-block compare-driven entry routes through the canonical prepared-module handoff when the function exposes exactly one non-variadic i32 parameter";
  const auto resolved_target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  const auto prepared_arch = resolved_target_profile.arch;
  if (prepared_arch != c4c::TargetArch::X86_64 && prepared_arch != c4c::TargetArch::I686) {
    throw std::invalid_argument(
        "x86 backend emitter requires an x86 target for the canonical prepared-module handoff");
  }

  std::vector<const c4c::backend::bir::Function*> defined_functions;
  const c4c::backend::bir::Function* entry_function_ptr = nullptr;
  for (const auto& candidate : module.module.functions) {
    if (candidate.is_declaration) {
      continue;
    }
    defined_functions.push_back(&candidate);
    if (candidate.name == "main") {
      entry_function_ptr = &candidate;
    } else if (entry_function_ptr == nullptr) {
      entry_function_ptr = &candidate;
    }
  }
  if (entry_function_ptr == nullptr) {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  }
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
  const auto minimal_param_register_at =
      [&](const c4c::backend::bir::Param& param,
          std::size_t arg_index) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_arg_destination_register_name(
        resolved_target_profile, *param.abi, arg_index));
  };
  const auto resolved_target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto module_data_support =
      c4c::backend::x86::module::make_module_data_support(module, resolved_target_triple);
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
  const auto throw_multi_defined_contract = [&]() -> void {
    throw std::invalid_argument(
        "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff");
  };
  const auto multi_defined_dispatch = [&]() {
    return c4c::backend::x86::build_prepared_module_multi_defined_dispatch_state(
        module, defined_functions, entry_function_ptr, prepared_arch,
        render_trivial_defined_function_if_supported, minimal_function_return_register,
        minimal_function_asm_prefix,
        [&](std::string_view name) { return module_data_support.find_same_module_global(name); },
        [&](const c4c::backend::bir::Global& global,
            c4c::backend::bir::TypeKind type,
            std::size_t byte_offset) {
          return module_data_support.same_module_global_supports_scalar_load(global, type,
                                                                             byte_offset);
        },
        minimal_param_register_at,
        [&](std::string_view symbol_name) {
          const std::string prefixed_name = "@" + std::string(symbol_name);
          return module_data_support.render_private_data_label(prefixed_name);
        },
        [&](std::string_view symbol_name) { return module_data_support.has_string_constant(symbol_name); },
        [&](std::string_view symbol_name) {
          return module_data_support.has_same_module_global(symbol_name);
        },
        [&](std::string_view symbol_name) {
          return module_data_support.render_asm_symbol_name(symbol_name);
        },
        [&](std::string_view symbol_name) {
          return module_data_support.find_string_constant(symbol_name);
        },
        [&](const c4c::backend::bir::StringConstant& string_constant) {
          return module_data_support.emit_string_constant_data(string_constant);
        },
        [&](const c4c::backend::bir::Global& global) {
          return module_data_support.emit_same_module_global_data(global);
        });
  }();
  const auto& bounded_same_module_helper_names = multi_defined_dispatch.helper_names;
  const auto& bounded_same_module_helper_global_names =
      multi_defined_dispatch.helper_global_names;
  const auto prepend_bounded_same_module_helpers = [&](std::string asm_text) {
    if (!multi_defined_dispatch.helper_prefix.empty()) {
      return multi_defined_dispatch.helper_prefix + asm_text;
    }
    return asm_text;
  };
  const auto throw_multi_defined_contract_if_active =
      [&](std::optional<std::string_view> detailed_error = std::nullopt) -> void {
    if (defined_functions.size() > 1 && multi_defined_dispatch.has_bounded_same_module_helpers) {
      if (detailed_error.has_value()) {
        throw std::invalid_argument(std::string(*detailed_error));
      }
      throw_multi_defined_contract();
    }
  };
  const auto annotate_multi_defined_local_slot_detail =
      [&](std::string_view function_name, std::string_view lane_name) -> std::string {
    return "x86 backend emitter requires the authoritative prepared local-slot instruction handoff through the canonical prepared-module handoff [bounded function: " +
           std::string(function_name) + ", lane: " + std::string(lane_name) + "]";
  };
    if (multi_defined_dispatch.rendered_module.has_value()) {
    return module_data_support.finalize_multi_defined_rendered_module_text(
        multi_defined_dispatch.helper_prefix + *multi_defined_dispatch.rendered_module);
  }
  const auto render_defined_function =
      [&](const c4c::backend::bir::Function& function,
          bool defer_module_data_emission,
          std::unordered_set<std::string_view>* used_string_names,
          std::unordered_set<std::string_view>* used_same_module_global_names) -> std::string {
    if (function.is_declaration || function.blocks.empty()) {
      constexpr std::string_view kMinimalReturnFunctionError =
          "x86 backend emitter only supports a minimal i32 return function through the canonical prepared-module handoff";
      throw_multi_defined_contract_if_active(kMinimalReturnFunctionError);
      throw std::invalid_argument(std::string(kMinimalReturnFunctionError));
    }

    const c4c::FunctionNameId function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function.name)
            .value_or(c4c::kInvalidFunctionName);
    const auto asm_prefix = minimal_function_asm_prefix(function);
    const auto return_register = function.return_type == c4c::backend::bir::TypeKind::Void
                                     ? std::optional<std::string>{}
                                     : minimal_function_return_register(function);
    if (function.return_type != c4c::backend::bir::TypeKind::Void &&
        !return_register.has_value()) {
      throw std::invalid_argument(
          "x86 backend emitter requires prepared return ABI metadata for the canonical prepared-module handoff");
    }
    const std::string return_register_text = return_register.value_or(std::string{});
    const auto minimal_param_register =
        [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
      auto param_it = function.params.begin();
      for (std::size_t arg_index = 0; param_it != function.params.end(); ++param_it, ++arg_index) {
        if (&*param_it != &param) {
          continue;
        }
        return minimal_param_register_at(param, arg_index);
      }
      return std::nullopt;
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

    const auto& entry = function.blocks.front();
    static const std::unordered_set<std::string_view> kNoHelperNames;
    const auto identity_prepend = [](std::string asm_text) { return asm_text; };
    const std::function<std::string(std::string)> prepend_helpers =
        defer_module_data_emission
            ? std::function<std::string(std::string)>(identity_prepend)
            : std::function<std::string(std::string)>(prepend_bounded_same_module_helpers);
    const PreparedX86FunctionDispatchContext function_dispatch_context{
        .prepared_module = &module,
        .module = &module.module,
        .function = &function,
        .entry = &entry,
        .stack_layout = &module.stack_layout,
        .function_addressing = find_addressing_function(),
        .prepared_names = &module.names,
        .function_locations = find_value_location_function(),
        .function_control_flow = find_control_flow_function(),
        .prepared_arch = prepared_arch,
        .asm_prefix = asm_prefix,
        .return_register = return_register_text,
        .bounded_same_module_helper_names =
            defer_module_data_emission ? &kNoHelperNames : &bounded_same_module_helper_names,
        .bounded_same_module_helper_global_names =
            defer_module_data_emission ? &kNoHelperNames : &bounded_same_module_helper_global_names,
        .find_block = find_block,
        .find_string_constant =
            [&](std::string_view name) { return module_data_support.find_string_constant(name); },
        .find_same_module_global = [&](std::string_view name) {
          return module_data_support.find_same_module_global(name);
        },
        .same_module_global_supports_scalar_load =
            [&](const c4c::backend::bir::Global& global,
                c4c::backend::bir::TypeKind type,
                std::size_t byte_offset) {
              return module_data_support.same_module_global_supports_scalar_load(global, type,
                                                                                 byte_offset);
            },
        .render_private_data_label = [&](std::string_view pool_name) {
          return module_data_support.render_private_data_label(pool_name);
        },
        .render_asm_symbol_name = [&](std::string_view logical_name) {
          return module_data_support.render_asm_symbol_name(logical_name);
        },
        .emit_string_constant_data = [&](const c4c::backend::bir::StringConstant& string_constant) {
          return module_data_support.emit_string_constant_data(string_constant);
        },
        .emit_same_module_global_data = [&](const c4c::backend::bir::Global& global) {
          return module_data_support.emit_same_module_global_data(global);
        },
        .prepend_bounded_same_module_helpers = prepend_helpers,
        .minimal_param_register = minimal_param_register,
        .used_string_names = used_string_names,
        .used_same_module_global_names = used_same_module_global_names,
        .defer_module_data_emission = defer_module_data_emission,
    };
    const auto render_local_structural_dispatch_if_supported =
        [&]() -> std::optional<std::string> {
      if (const auto rendered_local_i32_guard =
              c4c::backend::x86::render_prepared_local_i32_arithmetic_guard_if_supported(
                  function_dispatch_context);
          rendered_local_i32_guard.has_value()) {
        return rendered_local_i32_guard;
      }
      if (const auto rendered_countdown_entry =
              c4c::backend::x86::render_prepared_countdown_entry_routes_if_supported(
                  function_dispatch_context);
          rendered_countdown_entry.has_value()) {
        return rendered_countdown_entry;
      }
      if (const auto rendered_local_i16_guard =
              c4c::backend::x86::render_prepared_local_i16_arithmetic_guard_if_supported(
                  function_dispatch_context);
          rendered_local_i16_guard.has_value()) {
        return rendered_local_i16_guard;
      }
      return c4c::backend::x86::render_prepared_local_slot_guard_chain_if_supported(
          function_dispatch_context);
    };
    const auto required_function_frame_size = [&]() -> std::size_t {
      const auto* function_addressing = find_addressing_function();
      if (function_addressing == nullptr) {
        return module.stack_layout.frame_size_bytes;
      }
      const auto required_frame_alignment =
          std::max<std::size_t>(16, function_addressing->frame_alignment_bytes);
      if (function_addressing->frame_size_bytes == 0) {
        return module.stack_layout.frame_size_bytes;
      }
      const auto addressed_frame_size =
          ((function_addressing->frame_size_bytes + required_frame_alignment - 1) /
           required_frame_alignment) *
          required_frame_alignment;
      return std::max(module.stack_layout.frame_size_bytes, addressed_frame_size);
    };
    const auto required_frame_size_for_home =
        [&](const c4c::backend::prepare::PreparedValueHome& home) -> std::size_t {
      return home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot
                 ? required_function_frame_size()
                 : 0;
    };
    const auto narrow_home_register =
        [&](const c4c::backend::prepare::PreparedValueHome& home) -> std::optional<std::string> {
      if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
          !home.register_name.has_value()) {
        return std::nullopt;
      }
      return narrow_i32_register(*home.register_name);
    };
    const auto append_i32_home_move =
        [&](std::string& body,
            const c4c::backend::prepare::PreparedValueHome& home,
            std::string_view destination_register) -> bool {
      if (const auto source_register = narrow_home_register(home); source_register.has_value()) {
        if (*source_register != destination_register) {
          body += "    mov " + std::string(destination_register) + ", " + *source_register + "\n";
        }
        return true;
      }
      if (home.kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot &&
          home.offset_bytes.has_value()) {
        body += "    mov " + std::string(destination_register) + ", " +
                c4c::backend::x86::render_prepared_stack_memory_operand(*home.offset_bytes, "DWORD") +
                "\n";
        return true;
      }
      if (home.kind ==
              c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate &&
          home.immediate_i32.has_value()) {
        body += "    mov " + std::string(destination_register) + ", " +
                std::to_string(static_cast<std::int32_t>(*home.immediate_i32)) + "\n";
        return true;
      }
      return false;
    };
    const auto render_frame_wrapped_return =
        [&](std::string body, std::size_t frame_size, bool include_prefix) -> std::string {
      std::string rendered = include_prefix ? std::string(asm_prefix) : std::string{};
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
    const auto find_function_block_index =
        [&](std::string_view block_label) -> std::optional<std::size_t> {
      for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
        if (function.blocks[block_index].label == block_label) {
          return block_index;
        }
      }
      return std::nullopt;
    };
    const auto render_named_before_return_body_if_supported =
        [&](const c4c::backend::bir::Block& return_block,
            std::string_view value_name) -> std::optional<std::string> {
      if (prepared_arch != c4c::TargetArch::X86_64) {
        return std::nullopt;
      }
      const auto* function_locations = find_value_location_function();
      if (function_locations == nullptr) {
        return std::nullopt;
      }
      const auto block_index = find_function_block_index(return_block.label);
      if (!block_index.has_value()) {
        return std::nullopt;
      }
      const auto* source_home = c4c::backend::prepare::find_prepared_value_home(
          module.names, *function_locations, value_name);
      if (source_home == nullptr) {
        return std::nullopt;
      }
      const auto* before_return_bundle = c4c::backend::prepare::find_prepared_move_bundle(
          *function_locations,
          c4c::backend::prepare::PreparedMovePhase::BeforeReturn,
          *block_index,
          return_block.insts.size());
      if (before_return_bundle == nullptr || before_return_bundle->moves.size() != 1) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared return-bundle handoff through the canonical prepared-module handoff");
      }
      const auto& return_move = before_return_bundle->moves.front();
      if (return_move.destination_kind !=
              c4c::backend::prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
          return_move.destination_storage_kind !=
              c4c::backend::prepare::PreparedMoveStorageKind::Register ||
          !return_move.destination_register_name.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared return-bundle handoff through the canonical prepared-module handoff");
      }
      const auto destination_register = narrow_i32_register(*return_move.destination_register_name);
      if (!destination_register.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared return-bundle handoff through the canonical prepared-module handoff");
      }
      std::string body;
      if (!append_i32_home_move(body, *source_home, *destination_register)) {
        return std::nullopt;
      }
      return render_frame_wrapped_return(body, required_frame_size_for_home(*source_home), false);
    };
    const auto render_materialized_compare_join_return_if_supported =
        [&](const c4c::backend::prepare::PreparedResolvedMaterializedCompareJoinReturnArm&
                prepared_return_arm,
            const c4c::backend::bir::Param& param) -> std::optional<std::string> {
      return c4c::backend::x86::render_prepared_materialized_compare_join_return_if_supported(
          *return_register,
          module,
          prepared_return_arm,
          param,
          minimal_param_register,
          [&](std::string_view logical_name) {
            return module_data_support.render_asm_symbol_name(logical_name);
          },
          [&](const c4c::backend::bir::Global& global,
              c4c::backend::bir::TypeKind type,
              std::size_t byte_offset) {
            return module_data_support.same_module_global_supports_scalar_load(global, type,
                                                                               byte_offset);
          });
    };
    const auto render_minimal_scalar_move_bundle_return_if_supported =
        [&]() -> std::optional<std::string> {
      constexpr std::string_view kScalarReturnContractError =
          "x86 backend emitter requires authoritative prepared value-home and move-bundle data for minimal scalar return routes through the canonical prepared-module handoff";
      if (prepared_arch != c4c::TargetArch::X86_64 || function.blocks.size() != 1 ||
          entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
          !entry.terminator.value.has_value()) {
        return std::nullopt;
      }

      const auto* function_locations = find_value_location_function();
      if (function_locations == nullptr) {
        return std::nullopt;
      }

      const auto& returned = *entry.terminator.value;
      if (returned.type != c4c::backend::bir::TypeKind::I32 ||
          returned.kind != c4c::backend::bir::Value::Kind::Named ||
          function.params.size() != 1) {
        return std::nullopt;
      }

      const auto find_named_source_home =
          [&](std::string_view value_name) -> const c4c::backend::prepare::PreparedValueHome* {
        return c4c::backend::prepare::find_prepared_value_home(
            module.names, *function_locations, value_name);
      };
      const auto narrow_destination_register =
          [&](const c4c::backend::prepare::PreparedMoveResolution& move)
          -> std::optional<std::string> {
        if (move.destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::Register) {
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
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared return-bundle handoff through the canonical prepared-module handoff");
      }
      const auto return_destination_register =
          narrow_destination_register(before_return_bundle->moves.front());
      if (!return_destination_register.has_value()) {
        throw std::invalid_argument(
            "x86 backend emitter requires the authoritative prepared return-bundle handoff through the canonical prepared-module handoff");
      }

      const auto& param = function.params.front();
      if (param.type != c4c::backend::bir::TypeKind::I32 || param.is_varargs || param.is_sret ||
          param.is_byval) {
        return std::nullopt;
      }
      const auto* source_param_home = find_named_source_home(param.name);
      if (source_param_home == nullptr) {
        throw std::invalid_argument(std::string(kScalarReturnContractError));
      }

      if (entry.insts.empty()) {
        if (returned.name != param.name) {
          return std::nullopt;
        }
        std::string body;
        if (!append_i32_home_move(body, *source_param_home, *return_destination_register)) {
          throw std::invalid_argument(std::string(kScalarReturnContractError));
        }
        return render_frame_wrapped_return(
            body, required_frame_size_for_home(*source_param_home), true);
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
        throw std::invalid_argument(std::string(kScalarReturnContractError));
      }
      const auto& entry_move = before_instruction_bundle->moves.front();
      if (entry_move.destination_kind !=
              c4c::backend::prepare::PreparedMoveDestinationKind::Value ||
          entry_move.to_value_id == 0 ||
          entry_move.to_value_id != before_return_bundle->moves.front().from_value_id) {
        throw std::invalid_argument(std::string(kScalarReturnContractError));
      }
      const auto instruction_destination_register = narrow_destination_register(entry_move);
      if (!instruction_destination_register.has_value()) {
        throw std::invalid_argument(std::string(kScalarReturnContractError));
      }

      const auto immediate =
          lhs_is_param_rhs_is_imm ? binary->rhs.immediate : binary->lhs.immediate;
      std::string body;
      if (!append_i32_home_move(body, *source_param_home, *instruction_destination_register)) {
        throw std::invalid_argument(std::string(kScalarReturnContractError));
      }
      if (binary->opcode == c4c::backend::bir::BinaryOpcode::Add) {
        body += "    add " + *instruction_destination_register + ", " +
                std::to_string(static_cast<std::int32_t>(immediate)) + "\n";
      } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Sub &&
                 lhs_is_param_rhs_is_imm) {
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
      } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::Shl &&
                 lhs_is_param_rhs_is_imm) {
        body += "    shl " + *instruction_destination_register + ", " +
                std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
      } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::LShr &&
                 lhs_is_param_rhs_is_imm) {
        body += "    shr " + *instruction_destination_register + ", " +
                std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
      } else if (binary->opcode == c4c::backend::bir::BinaryOpcode::AShr &&
                 lhs_is_param_rhs_is_imm) {
        body += "    sar " + *instruction_destination_register + ", " +
                std::to_string(static_cast<std::int32_t>(binary->rhs.immediate)) + "\n";
      } else {
        return std::nullopt;
      }
      if (*return_destination_register != *instruction_destination_register) {
        body += "    mov " + *return_destination_register + ", " +
                *instruction_destination_register + "\n";
      }
      return render_frame_wrapped_return(
          body, required_frame_size_for_home(*source_param_home), true);
    };

    if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !entry.terminator.value.has_value()) {
      if (function.params.size() == 1) {
        const std::unordered_map<std::string_view, const c4c::backend::bir::BinaryInst*>
            named_binaries;
        const auto rendered_compare_driven_entry =
            c4c::backend::x86::render_prepared_compare_driven_entry_if_supported(
                function_dispatch_context,
                [&](const c4c::backend::bir::Block& return_block,
                    const c4c::backend::bir::Value& value) -> std::optional<std::string> {
                  if (value.kind == c4c::backend::bir::Value::Kind::Named) {
                    const auto prepared_return =
                        render_named_before_return_body_if_supported(return_block, value.name);
                    if (!prepared_return.has_value()) {
                      throw std::invalid_argument(
                          "x86 backend emitter requires authoritative prepared value-home and return-bundle data for named return values through the canonical prepared-module handoff");
                    }
                    return prepared_return;
                  }
                  const auto value_render =
                      c4c::backend::x86::render_prepared_param_derived_i32_value_if_supported(
                          *return_register,
                          value,
                          named_binaries,
                          function.params.front(),
                          minimal_param_register);
                  if (!value_render.has_value()) {
                    return std::nullopt;
                  }
                  return c4c::backend::x86::render_prepared_return_body(*value_render);
                },
                render_materialized_compare_join_return_if_supported);
        if (rendered_compare_driven_entry.has_value()) {
          return *rendered_compare_driven_entry;
        }
      }
      try {
        if (const auto rendered_local_structural_dispatch =
                render_local_structural_dispatch_if_supported();
            rendered_local_structural_dispatch.has_value()) {
          return *rendered_local_structural_dispatch;
        }
      } catch (const std::invalid_argument& error) {
        if (std::string_view(error.what()).find(
                "authoritative prepared local-slot ") != std::string_view::npos) {
          if (defined_functions.size() > 1 && multi_defined_dispatch.has_bounded_same_module_helpers) {
            throw std::invalid_argument(
                annotate_multi_defined_local_slot_detail(function.name,
                                                         "local-structural-dispatch"));
          }
          throw_multi_defined_contract_if_active();
        }
        throw;
      }
      if (const auto* function_control_flow = find_control_flow_function();
          function_control_flow != nullptr &&
          function.blocks.size() > 1 && !function_control_flow->branch_conditions.empty() &&
          !function_control_flow->join_transfers.empty()) {
        if (function.params.size() > 1) {
          throw_multi_defined_contract_if_active(kScalarPreparedControlFlowShapeError);
          throw std::invalid_argument(std::string(kScalarPreparedControlFlowShapeError));
        }
      }
      throw_multi_defined_contract_if_active(kScalarPreparedControlFlowShapeError);
      throw std::invalid_argument(std::string(kScalarPreparedControlFlowShapeError));
    }

    const auto& returned = *entry.terminator.value;
    if (returned.type != c4c::backend::bir::TypeKind::I32) {
      constexpr std::string_view kI32ReturnValueError =
          "x86 backend emitter only supports i32 return values through the canonical prepared-module handoff";
      throw_multi_defined_contract_if_active(kI32ReturnValueError);
      throw std::invalid_argument(std::string(kI32ReturnValueError));
    }
    if (const auto rendered_scalar_move_bundle_return =
            render_minimal_scalar_move_bundle_return_if_supported();
        rendered_scalar_move_bundle_return.has_value()) {
      return *rendered_scalar_move_bundle_return;
    }
    if (const auto rendered_single_block_return =
            c4c::backend::x86::render_prepared_single_block_return_dispatch_if_supported(
                function_dispatch_context);
        rendered_single_block_return.has_value()) {
      return *rendered_single_block_return;
    }

    constexpr std::string_view kDirectI32ReturnShapeError =
        "x86 backend emitter only supports direct immediate i32 returns, constant-evaluable straight-line no-parameter i32 return expressions, direct single-parameter i32 passthrough returns, single-parameter i32 add-immediate/sub-immediate/mul-immediate/and-immediate/or-immediate/xor-immediate/shl-immediate/lshr-immediate/ashr-immediate returns, a bounded equality-against-immediate guard family with immediate return leaves, or one bounded compare-against-zero branch family through the canonical prepared-module handoff";
    throw_multi_defined_contract_if_active(kDirectI32ReturnShapeError);
    throw std::invalid_argument(std::string(kDirectI32ReturnShapeError));
  };

  if (defined_functions.size() > 1) {
    std::string rendered_functions;
    std::unordered_set<std::string_view> used_string_names;
    std::unordered_set<std::string_view> used_same_module_global_names;
    for (const auto* function : defined_functions) {
      rendered_functions += render_defined_function(
          *function, true, &used_string_names, &used_same_module_global_names);
    }
    return module_data_support.finalize_selected_module_text(
        multi_defined_dispatch.helper_prefix + rendered_functions, used_string_names,
        &used_same_module_global_names);
  }

  return render_defined_function(*entry_function_ptr, false, nullptr, nullptr);
}


}  // namespace c4c::backend::x86::module
