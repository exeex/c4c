#include "x86_codegen.hpp"

#include <algorithm>
#include <exception>
#include <sstream>

namespace c4c::backend::x86 {

namespace {

enum class RouteDebugVerbosity {
  Summary,
  Trace,
};

struct FunctionRouteAttempt {
  std::string lane_name;
  bool matched = false;
  std::optional<std::string> detail;
};

struct FunctionRouteReport {
  std::string function_name;
  std::size_t bir_block_count = 0;
  std::size_t prepared_block_count = 0;
  std::size_t branch_condition_count = 0;
  std::size_t join_transfer_count = 0;
  std::vector<FunctionRouteAttempt> attempts;
};

enum class FinalRejectionKind {
  OrdinaryRouteMiss,
  UnsupportedPreparedShape,
  MissingPreparedContract,
  BackendException,
};

struct FinalRejectionReport {
  FinalRejectionKind kind = FinalRejectionKind::OrdinaryRouteMiss;
  const FunctionRouteAttempt* attempt = nullptr;
  std::string summary;
  std::string next_surface;
};

struct ModuleLaneRejectionReport {
  std::string summary;
  std::string next_surface;
};

void append_indented_line(std::ostringstream& out,
                          std::size_t indent,
                          std::string_view text) {
  for (std::size_t i = 0; i < indent; ++i) {
    out << "  ";
  }
  out << text << "\n";
}

std::string_view route_result_text(bool matched) {
  return matched ? "matched" : "rejected";
}

bool string_contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

constexpr std::string_view kCompareDrivenEntryParamShapeError =
    "x86 backend emitter only supports multi-block compare-driven entry routes through the canonical prepared-module handoff when the function exposes exactly one non-variadic i32 parameter";
constexpr std::string_view kBoundedMultiFunctionLaneShapeError =
    "x86 backend emitter only supports a single-function prepared module or one bounded multi-defined-function main-entry lane with same-module symbol calls and direct variadic runtime calls through the canonical prepared-module handoff";

std::string lane_next_surface(std::string_view lane_name) {
  if (lane_name == "countdown-entry-routes") {
    return "src/backend/mir/x86/codegen/prepared_countdown_render.cpp";
  }
  if (lane_name == "compare-driven-entry") {
    return "src/backend/mir/x86/codegen/prepared_module_emit.cpp";
  }
  if (lane_name == "local-i32-arithmetic-guard" ||
      lane_name == "local-i16-arithmetic-guard" ||
      lane_name == "local-slot-guard-chain" ||
      lane_name == "bounded-same-module-variadic-helper" ||
      lane_name == "single-block-floating-aggregate-call-helper" ||
      lane_name == "single-block-i64-immediate-return-helper" ||
      lane_name == "single-block-i64-ashr-return-helper" ||
      lane_name == "single-block-void-call-sequence" ||
      lane_name == "single-block-return-dispatch" ||
      lane_name == "trivial-defined-function") {
    return "src/backend/mir/x86/codegen/prepared_local_slot_render.cpp";
  }
  return "src/backend/mir/x86/codegen/prepared_module_emit.cpp";
}

std::string next_surface_hint(const FunctionRouteAttempt* attempt,
                              FinalRejectionKind kind) {
  if (attempt == nullptr) {
    return "inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane";
  }

  const auto lane_surface = lane_next_surface(attempt->lane_name);
  if (kind == FinalRejectionKind::MissingPreparedContract && attempt->detail.has_value()) {
    const auto detail = std::string_view(*attempt->detail);
    if (string_contains(detail, "value-home") ||
        string_contains(detail, "return-bundle") ||
        string_contains(detail, "call-bundle") ||
        string_contains(detail, "ABI metadata")) {
      return "inspect the prepared value-location handoff consumed in " + lane_surface;
    }
    if (string_contains(detail, "short-circuit") ||
        string_contains(detail, "guard-chain") ||
        string_contains(detail, "compare-join") ||
        string_contains(detail, "loop-countdown")) {
      return "inspect the prepared control-flow handoff consumed in " + lane_surface;
    }
    return "inspect the prepared-module handoff contract consumed in " + lane_surface;
  }

  if (kind == FinalRejectionKind::UnsupportedPreparedShape) {
    if (attempt->lane_name == "bounded-same-module-variadic-helper") {
      return "inspect the current x86 same-module helper shape support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-floating-aggregate-call-helper") {
      return "inspect the current x86 floating aggregate helper support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-i64-immediate-return-helper") {
      return "inspect the current x86 single-block i64 return-helper support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-i64-ashr-return-helper") {
      return "inspect the current x86 single-block i64 return-helper support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-void-call-sequence") {
      return "inspect the current x86 single-block call-sequence support in " + lane_surface;
    }
    return "inspect the current x86 shape support in " + lane_surface;
  }

  if (kind == FinalRejectionKind::BackendException) {
    return "inspect the backend exception path in " + lane_surface;
  }

  return "inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane";
}

ModuleLaneRejectionReport build_module_lane_rejection_report(
    const std::optional<std::string>& detail) {
  if (!detail.has_value()) {
    return ModuleLaneRejectionReport{
        .summary =
            "bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
        .next_surface =
            "inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp",
    };
  }

  return ModuleLaneRejectionReport{
      .summary =
          "bounded multi-function handoff recognized the module, but the prepared shape is outside the current x86 support",
      .next_surface =
          "inspect the current x86 bounded multi-function shape support in src/backend/mir/x86/codegen/prepared_module_emit.cpp",
  };
}

FinalRejectionKind classify_rejection_kind(std::string_view detail) {
  if (string_contains(detail, "only supports") || string_contains(detail, "unsupported")) {
    return FinalRejectionKind::UnsupportedPreparedShape;
  }
  if (string_contains(detail, "requires") ||
      string_contains(detail, "authoritative") ||
      string_contains(detail, "metadata") ||
      string_contains(detail, "handoff")) {
    return FinalRejectionKind::MissingPreparedContract;
  }
  return FinalRejectionKind::BackendException;
}

FinalRejectionReport build_final_rejection_report(const FunctionRouteReport& report) {
  for (auto it = report.attempts.rbegin(); it != report.attempts.rend(); ++it) {
    if (!it->detail.has_value()) {
      continue;
    }

    const auto kind = classify_rejection_kind(*it->detail);
    std::string summary;
    switch (kind) {
      case FinalRejectionKind::UnsupportedPreparedShape:
        if (it->lane_name == "bounded-same-module-variadic-helper") {
          summary = "bounded same-module variadic helper lane recognized the function, but the "
                    "prepared helper shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-floating-aggregate-call-helper") {
          summary =
              "single-block floating aggregate call helper recognized the function, but the "
              "prepared aggregate-helper shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-i64-immediate-return-helper") {
          summary = "single-block i64 immediate return helper recognized the function, but the "
                    "prepared return-helper shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-i64-ashr-return-helper") {
          summary =
              "single-block i64 arithmetic-right-shift return helper recognized the function, "
              "but the prepared return-helper shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-void-call-sequence") {
          summary = "single-block void call-sequence helper recognized the function, but the "
                    "prepared call-wrapper shape is outside the current x86 support";
        } else {
          summary = it->lane_name +
                    " recognized the function, but the prepared shape is outside the current x86 "
                    "support";
        }
        break;
      case FinalRejectionKind::MissingPreparedContract:
        summary = it->lane_name +
                  " is missing prepared handoff data required by the current x86 route";
        break;
      case FinalRejectionKind::BackendException:
        summary = it->lane_name + " rejected the function with a backend exception";
        break;
      case FinalRejectionKind::OrdinaryRouteMiss:
        break;
    }
    return FinalRejectionReport{
        .kind = kind,
        .attempt = &*it,
        .summary = std::move(summary),
        .next_surface = next_surface_hint(&*it, kind),
    };
  }

  return FinalRejectionReport{
      .kind = FinalRejectionKind::OrdinaryRouteMiss,
      .attempt = nullptr,
      .summary = "current x86 lanes did not recognize this prepared function shape",
      .next_surface =
          "inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
  };
}

std::optional<std::string> build_bounded_variadic_helper_lane_detail(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function* entry_function,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow) {
  const auto carries_explicit_variadic_runtime = [&]() {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
        if (call == nullptr) {
          continue;
        }
        if (string_contains(call->callee, "llvm.va_start.") ||
            string_contains(call->callee, "llvm.va_end.") ||
            string_contains(call->callee, "llvm.va_arg.")) {
          return true;
        }
      }
    }
    return false;
  };

  if (defined_functions.size() <= 1 || entry_function == nullptr || entry_function->name != "main" ||
      &function == entry_function || !function.is_variadic ||
      function.return_type != c4c::backend::bir::TypeKind::Void ||
      (!carries_explicit_variadic_runtime() && function.blocks.size() <= 1)) {
    return std::nullopt;
  }
  if (function_control_flow != nullptr && !function_control_flow->join_transfers.empty()) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports non-entry bounded same-module variadic helpers when "
      "they already reduce to the current direct-extern or local-slot-guard helper surfaces";
  if (carries_explicit_variadic_runtime()) {
    detail += "; this helper still carries explicit variadic-runtime state";
  } else {
    detail += "; this helper remains a multi-block variadic body";
  }
  return detail;
}

std::optional<std::string> build_single_block_void_call_sequence_lane_detail(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || !function.local_slots.empty() || function.blocks.size() != 1 ||
      function.return_type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value() || entry.insts.empty()) {
    return std::nullopt;
  }

  bool saw_same_module_call = false;
  bool saw_direct_variadic_extern_call = false;
  bool saw_indirect_call = false;
  bool saw_other_call_effect = false;

  for (const auto& inst : entry.insts) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr) {
      continue;
    }
    if (call->is_indirect || call->callee_value.has_value()) {
      saw_indirect_call = true;
      continue;
    }
    if (call->is_variadic) {
      saw_direct_variadic_extern_call = true;
      continue;
    }
    if (!call->callee.empty() && call->callee.rfind("llvm.", 0) != 0) {
      saw_same_module_call = true;
      continue;
    }
    saw_other_call_effect = true;
  }

  if (!saw_same_module_call && !saw_direct_variadic_extern_call && !saw_indirect_call &&
      !saw_other_call_effect) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports trivial single-block void helpers through the canonical "
      "prepared-module handoff";
  if (saw_same_module_call && saw_direct_variadic_extern_call) {
    detail += "; this helper still carries same-module call wrappers and direct variadic extern "
              "calls";
  } else if (saw_same_module_call) {
    detail += "; this helper still carries same-module call wrappers";
  } else if (saw_direct_variadic_extern_call) {
    detail += "; this helper still carries direct variadic extern calls";
  } else if (saw_indirect_call) {
    detail += "; this helper still carries indirect calls";
  } else {
    detail += "; this helper still carries call side effects";
  }
  return detail;
}

std::optional<std::string> build_single_block_i64_ashr_return_helper_lane_detail(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || !function.local_slots.empty() || function.blocks.size() != 1 ||
      function.return_type != c4c::backend::bir::TypeKind::I64 || function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value() || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
  if (binary == nullptr || binary->opcode != c4c::backend::bir::BinaryOpcode::AShr ||
      binary->operand_type != c4c::backend::bir::TypeKind::I64 ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I64 ||
      binary->lhs.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->lhs.type != c4c::backend::bir::TypeKind::I64 ||
      binary->lhs.name != function.params.front().name ||
      binary->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
      binary->rhs.type != c4c::backend::bir::TypeKind::I64 ||
      entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      entry.terminator.value->name != binary->result.name) {
    return std::nullopt;
  }

  return "x86 backend emitter only supports single-block i64 return helpers when they already "
         "reduce to the current direct passthrough or local i16/i64-sub helper surfaces; this "
         "helper still carries an i64 arithmetic-right-shift immediate return";
}

std::optional<std::string> build_single_block_i64_immediate_return_helper_lane_detail(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || !function.local_slots.empty() || function.blocks.size() != 1 ||
      function.return_type != c4c::backend::bir::TypeKind::I64 || function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value() || entry.insts.size() != 1) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.front());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I64 ||
      binary->opcode == c4c::backend::bir::BinaryOpcode::AShr ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I64 ||
      entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      entry.terminator.value->name != binary->result.name) {
    return std::nullopt;
  }

  const auto lhs_is_param = binary->lhs.kind == c4c::backend::bir::Value::Kind::Named &&
                            binary->lhs.type == c4c::backend::bir::TypeKind::I64 &&
                            binary->lhs.name == function.params.front().name;
  const auto rhs_is_param = binary->rhs.kind == c4c::backend::bir::Value::Kind::Named &&
                            binary->rhs.type == c4c::backend::bir::TypeKind::I64 &&
                            binary->rhs.name == function.params.front().name;
  const auto lhs_is_immediate = binary->lhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                                binary->lhs.type == c4c::backend::bir::TypeKind::I64;
  const auto rhs_is_immediate = binary->rhs.kind == c4c::backend::bir::Value::Kind::Immediate &&
                                binary->rhs.type == c4c::backend::bir::TypeKind::I64;
  if (!((lhs_is_param && rhs_is_immediate) || (lhs_is_immediate && rhs_is_param))) {
    return std::nullopt;
  }

  std::string_view operation = "binary";
  switch (binary->opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      operation = "add";
      break;
    case c4c::backend::bir::BinaryOpcode::Sub:
      operation = lhs_is_immediate ? "reverse-subtract" : "subtract";
      break;
    case c4c::backend::bir::BinaryOpcode::And:
      operation = "and";
      break;
    case c4c::backend::bir::BinaryOpcode::Or:
      operation = "or";
      break;
    case c4c::backend::bir::BinaryOpcode::Xor:
      operation = "xor";
      break;
    case c4c::backend::bir::BinaryOpcode::Shl:
      operation = "logical-left-shift";
      break;
    case c4c::backend::bir::BinaryOpcode::LShr:
      operation = "logical-right-shift";
      break;
    default:
      return std::nullopt;
  }

  return "x86 backend emitter only supports single-block i64 return helpers when they already "
         "reduce to the current direct passthrough or established scalar helper surfaces; this "
         "helper still carries an i64 " +
         std::string(operation) + " immediate return";
}

std::optional<std::string> build_single_block_floating_aggregate_call_helper_lane_detail(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || function.return_type != c4c::backend::bir::TypeKind::Void ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  bool saw_direct_variadic_extern_call = false;
  bool saw_floating_aggregate_param = false;
  bool saw_floating_aggregate_call_arg = false;
  bool saw_pointer_aggregate_param = false;

  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::Ptr || param.size_bytes == 0) {
      continue;
    }
    saw_pointer_aggregate_param = true;
    if (param.abi.has_value() &&
        (param.abi->primary_class == c4c::backend::bir::AbiValueClass::Sse ||
         param.abi->secondary_class == c4c::backend::bir::AbiValueClass::Sse ||
         param.abi->primary_class == c4c::backend::bir::AbiValueClass::X87 ||
         param.abi->secondary_class == c4c::backend::bir::AbiValueClass::X87)) {
      saw_floating_aggregate_param = true;
    }
  }

  for (const auto& inst : entry.insts) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr || call->is_indirect || call->callee_value.has_value()) {
      continue;
    }
    if (call->is_variadic && call->callee.rfind("llvm.", 0) != 0) {
      saw_direct_variadic_extern_call = true;
      for (const auto arg_type : call->arg_types) {
        if (arg_type == c4c::backend::bir::TypeKind::F64 ||
            arg_type == c4c::backend::bir::TypeKind::F128) {
          saw_floating_aggregate_call_arg = true;
        }
      }
    }
  }

  if (!saw_floating_aggregate_param && saw_pointer_aggregate_param &&
      saw_floating_aggregate_call_arg) {
    saw_floating_aggregate_param = true;
  }

  if (!saw_direct_variadic_extern_call || !saw_pointer_aggregate_param ||
      !saw_floating_aggregate_param || !saw_floating_aggregate_call_arg) {
    return std::nullopt;
  }

  return "x86 backend emitter only supports single-block floating aggregate helpers when those "
         "aggregate arguments already reduce to the current local-slot or scalar helper surfaces; "
         "this helper still forwards floating aggregate lanes through byval/pointer wrappers into "
         "a direct variadic extern call";
}

std::string_view prepared_function_name_or_none(
    const c4c::backend::prepare::PreparedNameTables& names,
    c4c::FunctionNameId function_name_id) {
  if (function_name_id == c4c::kInvalidFunctionName) {
    return "<none>";
  }
  return c4c::backend::prepare::prepared_function_name(names, function_name_id);
}

const c4c::backend::prepare::PreparedControlFlowFunction* find_control_flow_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name_id) {
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_control_flow_function(
      module.control_flow, function_name_id);
}

const c4c::backend::prepare::PreparedAddressingFunction* find_addressing_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name_id) {
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_addressing(module, function_name_id);
}

const c4c::backend::prepare::PreparedValueLocationFunction* find_value_location_function(
    const c4c::backend::prepare::PreparedBirModule& module,
    c4c::FunctionNameId function_name_id) {
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_value_location_function(
      module.value_locations, function_name_id);
}

std::string resolve_target_triple(const c4c::backend::prepare::PreparedBirModule& module) {
  return module.module.target_triple.empty() ? c4c::default_host_target_triple()
                                             : module.module.target_triple;
}

std::optional<c4c::TargetArch> resolve_prepared_arch(
    const c4c::backend::prepare::PreparedBirModule& module) {
  const auto target_profile = module.target_profile.arch != c4c::TargetArch::Unknown
                                  ? module.target_profile
                                  : c4c::target_profile_from_triple(resolve_target_triple(module));
  if (target_profile.arch != c4c::TargetArch::X86_64 &&
      target_profile.arch != c4c::TargetArch::I686) {
    return std::nullopt;
  }
  return target_profile.arch;
}

std::string render_route_report(const c4c::backend::prepare::PreparedBirModule& module,
                                RouteDebugVerbosity verbosity) {
  std::ostringstream out;

  const auto prepared_arch = resolve_prepared_arch(module);
  if (!prepared_arch.has_value()) {
    out << "x86 handoff debug is only available for x86 targets\n";
    return out.str();
  }

  std::vector<const c4c::backend::bir::Function*> defined_functions;
  const c4c::backend::bir::Function* entry_function = nullptr;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    defined_functions.push_back(&function);
    if (function.name == "main") {
      entry_function = &function;
    } else if (entry_function == nullptr) {
      entry_function = &function;
    }
  }

  const auto target_triple = resolve_target_triple(module);
  out << (verbosity == RouteDebugVerbosity::Summary ? "x86 handoff summary\n"
                                                    : "x86 handoff trace\n");
  out << "target: " << target_triple << "\n";
  out << "defined functions: " << defined_functions.size() << "\n";
  out << "entry function: " << (entry_function == nullptr ? "<none>" : entry_function->name)
      << "\n";

  if (defined_functions.empty()) {
    return out.str();
  }

  const auto narrow_register = [](const std::optional<std::string>& wide_register)
      -> std::optional<std::string> {
    if (!wide_register.has_value()) {
      return std::nullopt;
    }
    return narrow_i32_register(*wide_register);
  };
  const auto render_asm_symbol_name = [&](std::string_view logical_name) -> std::string {
    if (target_triple.find("apple-darwin") != std::string::npos) {
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
    if (target_triple.find("apple-darwin") != std::string::npos) {
      return "L" + label;
    }
    return ".L." + label;
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
  const auto emit_string_constant_data =
      [](const c4c::backend::bir::StringConstant&) -> std::string { return {}; };
  const auto emit_same_module_global_data =
      [](const c4c::backend::bir::Global&) -> std::optional<std::string> {
    return std::string{};
  };
  const auto minimal_function_return_register =
      [&](const c4c::backend::bir::Function& candidate) -> std::optional<std::string> {
    if (!candidate.return_abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_result_destination_register_name(
        module.target_profile.arch != c4c::TargetArch::Unknown
            ? module.target_profile
            : c4c::target_profile_from_triple(target_triple),
        *candidate.return_abi));
  };
  const auto minimal_function_asm_prefix =
      [&](const c4c::backend::bir::Function& candidate) -> std::string {
    return ".intel_syntax noprefix\n.text\n.globl " + candidate.name + "\n.type " +
           candidate.name + ", @function\n" + candidate.name + ":\n";
  };
  const auto minimal_param_register_at =
      [&](const c4c::backend::bir::Param& param,
          std::size_t arg_index) -> std::optional<std::string> {
    if (param.is_varargs || param.is_sret || param.is_byval || !param.abi.has_value()) {
      return std::nullopt;
    }
    return narrow_register(c4c::backend::prepare::call_arg_destination_register_name(
        module.target_profile.arch != c4c::TargetArch::Unknown
            ? module.target_profile
            : c4c::target_profile_from_triple(target_triple),
        *param.abi, arg_index));
  };
  const auto render_trivial_defined_function_if_supported =
      [&](const c4c::backend::bir::Function& candidate) -> std::optional<std::string> {
    return c4c::backend::x86::render_prepared_trivial_defined_function_if_supported(
        candidate, *prepared_arch, minimal_function_return_register, minimal_function_asm_prefix);
  };

  std::optional<std::string> multi_defined_dispatch_detail;
  auto multi_defined_dispatch =
      c4c::backend::x86::PreparedModuleMultiDefinedDispatchState{};
  try {
    multi_defined_dispatch =
        c4c::backend::x86::build_prepared_module_multi_defined_dispatch_state(
            module, defined_functions, entry_function, *prepared_arch,
            render_trivial_defined_function_if_supported, minimal_function_return_register,
            minimal_function_asm_prefix, find_same_module_global,
            same_module_global_supports_scalar_load, minimal_param_register_at,
            [&](std::string_view symbol_name) {
              return render_private_data_label("@" + std::string(symbol_name));
            },
            [&](std::string_view symbol_name) {
              return find_string_constant(symbol_name) != nullptr;
            },
            [&](std::string_view symbol_name) {
              return find_same_module_global(symbol_name) != nullptr;
            },
            render_asm_symbol_name, find_string_constant, emit_string_constant_data,
            emit_same_module_global_data);
  } catch (const std::exception& ex) {
    multi_defined_dispatch_detail = ex.what();
  } catch (...) {
    multi_defined_dispatch_detail = "unknown backend exception";
  }

  if (verbosity == RouteDebugVerbosity::Summary) {
    out << "module-level bounded multi-function lane: "
        << (multi_defined_dispatch.rendered_module.has_value() ? "matched" : "rejected")
        << "\n";
    if (!multi_defined_dispatch.rendered_module.has_value()) {
      const auto rejection = build_module_lane_rejection_report(multi_defined_dispatch_detail);
      const auto detail =
          multi_defined_dispatch_detail.value_or(std::string(kBoundedMultiFunctionLaneShapeError));
      out << "- module-level final rejection: " << rejection.summary << "\n";
      out << "- module-level final detail: " << detail << "\n";
      out << "- module-level next inspect: " << rejection.next_surface << "\n";
    }
  } else {
    out << "module-level bounded multi-function lane\n";
    out << "  result: "
        << (multi_defined_dispatch.rendered_module.has_value() ? "matched" : "rejected")
        << "\n";
    if (!multi_defined_dispatch.rendered_module.has_value()) {
      const auto rejection = build_module_lane_rejection_report(multi_defined_dispatch_detail);
      const auto detail =
          multi_defined_dispatch_detail.value_or(std::string(kBoundedMultiFunctionLaneShapeError));
      out << "  final: rejected: " << rejection.summary << "\n";
      out << "  final detail: " << detail << "\n";
      out << "  next inspect: " << rejection.next_surface << "\n";
    } else if (multi_defined_dispatch_detail.has_value()) {
      out << "  detail: " << *multi_defined_dispatch_detail << "\n";
    }
  }

  const std::unordered_set<std::string_view> kNoHelperNames;
  for (const auto* function : defined_functions) {
    const auto function_name_id =
        c4c::backend::prepare::resolve_prepared_function_name_id(module.names, function->name)
            .value_or(c4c::kInvalidFunctionName);
    const auto* function_control_flow = find_control_flow_function(module, function_name_id);
    const auto* function_addressing = find_addressing_function(module, function_name_id);
    const auto* function_locations = find_value_location_function(module, function_name_id);

    const auto find_block = [&](std::string_view label) -> const c4c::backend::bir::Block* {
      for (const auto& block : function->blocks) {
        if (block.label == label) {
          return &block;
        }
      }
      return nullptr;
    };
    const auto minimal_param_register =
        [&](const c4c::backend::bir::Param& param) -> std::optional<std::string> {
      for (std::size_t arg_index = 0; arg_index < function->params.size(); ++arg_index) {
        if (&function->params[arg_index] != &param) {
          continue;
        }
        return minimal_param_register_at(param, arg_index);
      }
      return std::nullopt;
    };

    FunctionRouteReport report{
        .function_name = function->name,
        .bir_block_count = function->blocks.size(),
        .prepared_block_count = function_control_flow == nullptr ? 0 : function_control_flow->blocks.size(),
        .branch_condition_count =
            function_control_flow == nullptr ? 0 : function_control_flow->branch_conditions.size(),
        .join_transfer_count =
            function_control_flow == nullptr ? 0 : function_control_flow->join_transfers.size(),
    };

    if (verbosity == RouteDebugVerbosity::Summary) {
      out << "\nfunction " << report.function_name << "\n";
      out << "- bir blocks: " << report.bir_block_count << "\n";
      out << "- prepared blocks: " << report.prepared_block_count << "\n";
      out << "- branch conditions: " << report.branch_condition_count << "\n";
      out << "- join transfers: " << report.join_transfer_count << "\n";
    } else {
      out << "\nfunction " << report.function_name << "\n";
      out << "  facts: bir_blocks=" << report.bir_block_count
          << ", prepared_blocks=" << report.prepared_block_count
          << ", branch_conditions=" << report.branch_condition_count
          << ", join_transfers=" << report.join_transfer_count << "\n";
    }

    if (function->blocks.empty()) {
      if (verbosity == RouteDebugVerbosity::Summary) {
        out << "- final: empty function body\n";
      } else {
        out << "  final: empty function body\n";
      }
      continue;
    }

    const auto asm_prefix = minimal_function_asm_prefix(*function);
    const auto return_register =
        function->return_type == c4c::backend::bir::TypeKind::Void
            ? std::optional<std::string>{}
            : minimal_function_return_register(*function);
    std::unordered_set<std::string_view> used_string_names;
    std::unordered_set<std::string_view> used_same_module_global_names;
    const PreparedX86FunctionDispatchContext context{
        .prepared_module = &module,
        .module = &module.module,
        .function = function,
        .entry = &function->blocks.front(),
        .stack_layout = &module.stack_layout,
        .function_addressing = function_addressing,
        .prepared_names = &module.names,
        .function_locations = function_locations,
        .function_control_flow = function_control_flow,
        .prepared_arch = *prepared_arch,
        .asm_prefix = asm_prefix,
        .return_register = return_register.value_or(std::string{}),
        .bounded_same_module_helper_names =
            multi_defined_dispatch.has_bounded_same_module_helpers
                ? &multi_defined_dispatch.helper_names
                : &kNoHelperNames,
        .bounded_same_module_helper_global_names =
            multi_defined_dispatch.has_bounded_same_module_helpers
                ? &multi_defined_dispatch.helper_global_names
                : &kNoHelperNames,
        .find_block = find_block,
        .find_string_constant = find_string_constant,
        .find_same_module_global = find_same_module_global,
        .same_module_global_supports_scalar_load = same_module_global_supports_scalar_load,
        .render_private_data_label = render_private_data_label,
        .render_asm_symbol_name = render_asm_symbol_name,
        .emit_string_constant_data = emit_string_constant_data,
        .emit_same_module_global_data = emit_same_module_global_data,
        .prepend_bounded_same_module_helpers = [](std::string asm_text) { return asm_text; },
        .minimal_param_register = minimal_param_register,
        .used_string_names = &used_string_names,
        .used_same_module_global_names = &used_same_module_global_names,
        .defer_module_data_emission = true,
    };

    const auto try_lane = [&](std::string lane_name,
                              const std::function<std::optional<std::string>()>& try_render) {
      bool matched = false;
      std::optional<std::string> detail;
      try {
        matched = try_render().has_value();
      } catch (const std::exception& ex) {
        detail = ex.what();
      } catch (...) {
        detail = "unknown backend exception";
      }
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = std::move(lane_name),
          .matched = matched,
          .detail = std::move(detail),
      });
    };

    try_lane("trivial-defined-function", [&]() {
      return render_trivial_defined_function_if_supported(*function);
    });
    try_lane("local-i32-arithmetic-guard", [&]() {
      return render_prepared_local_i32_arithmetic_guard_if_supported(context);
    });
    try_lane("countdown-entry-routes", [&]() {
      return render_prepared_countdown_entry_routes_if_supported(context);
    });
    try_lane("local-i16-arithmetic-guard", [&]() {
      return render_prepared_local_i16_arithmetic_guard_if_supported(context);
    });
    try_lane("local-slot-guard-chain", [&]() {
      return render_prepared_local_slot_guard_chain_if_supported(context);
    });
    try_lane("single-block-return-dispatch", [&]() {
      return render_prepared_single_block_return_dispatch_if_supported(context);
    });
    try_lane("compare-driven-entry", [&]() -> std::optional<std::string> {
      if (function_control_flow == nullptr ||
          function->blocks.size() <= 1 || function_control_flow->branch_conditions.empty() ||
          function_control_flow->join_transfers.empty()) {
        return std::nullopt;
      }
      if (function->params.size() > 1) {
        throw std::invalid_argument(std::string(kCompareDrivenEntryParamShapeError));
      }
      return std::nullopt;
    });
    if (const auto bounded_variadic_helper_detail = build_bounded_variadic_helper_lane_detail(
            defined_functions, entry_function, *function, function_control_flow);
        bounded_variadic_helper_detail.has_value()) {
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = "bounded-same-module-variadic-helper",
          .matched = false,
          .detail = std::move(bounded_variadic_helper_detail),
      });
    }
    if (const auto single_block_void_call_sequence_detail =
            build_single_block_void_call_sequence_lane_detail(*function);
        single_block_void_call_sequence_detail.has_value()) {
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = "single-block-void-call-sequence",
          .matched = false,
          .detail = std::move(single_block_void_call_sequence_detail),
      });
    }
    if (const auto single_block_i64_ashr_return_helper_detail =
            build_single_block_i64_ashr_return_helper_lane_detail(*function);
        single_block_i64_ashr_return_helper_detail.has_value()) {
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = "single-block-i64-ashr-return-helper",
          .matched = false,
          .detail = std::move(single_block_i64_ashr_return_helper_detail),
      });
    }
    if (const auto single_block_i64_immediate_return_helper_detail =
            build_single_block_i64_immediate_return_helper_lane_detail(*function);
        single_block_i64_immediate_return_helper_detail.has_value()) {
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = "single-block-i64-immediate-return-helper",
          .matched = false,
          .detail = std::move(single_block_i64_immediate_return_helper_detail),
      });
    }
    if (const auto single_block_floating_aggregate_call_helper_detail =
            build_single_block_floating_aggregate_call_helper_lane_detail(*function);
        single_block_floating_aggregate_call_helper_detail.has_value()) {
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = "single-block-floating-aggregate-call-helper",
          .matched = false,
          .detail = std::move(single_block_floating_aggregate_call_helper_detail),
      });
    }

    const auto matched_it = std::find_if(report.attempts.begin(),
                                         report.attempts.end(),
                                         [](const FunctionRouteAttempt& attempt) {
                                           return attempt.matched;
                                         });

    if (verbosity == RouteDebugVerbosity::Summary) {
      out << "- top-level lane: "
          << (matched_it == report.attempts.end() ? "no current lane matched"
                                                  : matched_it->lane_name)
          << "\n";
      if (matched_it == report.attempts.end()) {
        const auto rejection = build_final_rejection_report(report);
        out << "- final rejection: " << rejection.summary << "\n";
        out << "- next inspect: " << rejection.next_surface << "\n";
      }
    } else {
      for (const auto& attempt : report.attempts) {
        out << "  try lane " << attempt.lane_name << "\n";
        out << "    result: " << route_result_text(attempt.matched) << "\n";
        if (attempt.detail.has_value()) {
          out << "    detail: " << *attempt.detail << "\n";
        }
      }
      if (matched_it == report.attempts.end()) {
        const auto rejection = build_final_rejection_report(report);
        out << "  final: rejected: " << rejection.summary << "\n";
        if (rejection.attempt != nullptr && rejection.attempt->detail.has_value()) {
          out << "  final detail: " << *rejection.attempt->detail << "\n";
        }
        out << "  next inspect: " << rejection.next_surface << "\n";
      } else {
        out << "  final: matched " << matched_it->lane_name << "\n";
      }
    }
  }

  return out.str();
}

}  // namespace

std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return render_route_report(module, RouteDebugVerbosity::Summary);
}

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module) {
  return render_route_report(module, RouteDebugVerbosity::Trace);
}

}  // namespace c4c::backend::x86
