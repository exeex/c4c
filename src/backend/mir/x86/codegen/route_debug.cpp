#include "x86_codegen.hpp"

#include <algorithm>
#include <cstdlib>
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
  std::optional<std::string> facts;
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
  std::optional<std::string> facts;
  std::string next_surface;
};

struct ModuleLaneRejectionReport {
  std::string summary;
  std::string next_surface;
};

struct RouteDebugFocus {
  std::optional<std::string_view> function;
  std::optional<std::string_view> block;
};

constexpr std::string_view kMirFocusValueEnv = "C4C_MIR_FOCUS_VALUE";

struct SingleBlockVoidCallSequenceFacts {
  std::size_t same_module_call_count = 0;
  std::size_t direct_variadic_extern_call_count = 0;
  std::size_t direct_fixed_arity_extern_call_count = 0;
  std::size_t indirect_call_count = 0;
  std::size_t other_call_effect_count = 0;
};

struct BoundedSameModuleVariadicHelperFacts {
  std::size_t explicit_variadic_runtime_call_count = 0;
  std::size_t same_module_helper_call_count = 0;
  std::size_t direct_extern_call_count = 0;
  std::size_t indirect_call_count = 0;
  std::size_t other_call_effect_count = 0;
};

struct SingleBlockAggregateForwardingWrapperFacts {
  std::size_t direct_extern_call_count = 0;
  std::size_t same_module_aggregate_call_wrapper_count = 0;
  std::size_t forwarded_aggregate_arg_count = 0;
};

struct SingleBlockFloatingAggregateCallHelperFacts {
  std::size_t aggregate_pointer_param_count = 0;
  std::size_t byval_aggregate_param_count = 0;
  std::size_t floating_lane_load_count = 0;
  std::size_t floating_widening_cast_count = 0;
  std::size_t direct_variadic_extern_call_count = 0;
  std::size_t floating_variadic_arg_count = 0;
};

struct SingleBlockFloatingAggregateSretCopyoutHelperFacts {
  std::size_t same_module_global_load_count = 0;
  std::size_t scratch_slot_store_count = 0;
  std::size_t scratch_slot_reload_count = 0;
  std::size_t sret_floating_store_count = 0;
};

struct SingleBlockSameModuleScalarCallWrapperFacts {
  std::size_t local_slot_reload_count = 0;
  std::size_t scalar_same_module_helper_call_count = 0;
  std::size_t width_adjusting_cast_count = 0;
  std::size_t same_module_sink_wrapper_count = 0;
};

struct SingleBlockI64ImmediateReturnHelperFacts {
  std::string_view opcode = "binary";
  std::string_view param_operand_side = "unknown";
  std::string_view immediate_operand_side = "unknown";
  std::string_view immediate_source = "direct i64";
};

struct SingleBlockI64AshrReturnHelperFacts {
  std::string_view param_operand_side = "unknown";
  std::string_view shift_operand_side = "unknown";
  std::string_view shift_source = "direct i64";
};

struct CompareDrivenEntryFacts {
  std::size_t total_param_count = 0;
  std::size_t non_variadic_i32_param_count = 0;
  std::size_t non_i32_or_varargs_param_count = 0;
  bool function_is_variadic = false;
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

std::string counted_label(std::size_t count,
                          std::string_view singular,
                          std::string_view plural) {
  return std::to_string(count) + " " + std::string(count == 1 ? singular : plural);
}

std::string join_fact_labels(const std::vector<std::string>& labels) {
  if (labels.empty()) {
    return {};
  }
  if (labels.size() == 1) {
    return labels.front();
  }
  if (labels.size() == 2) {
    return labels.front() + " and " + labels.back();
  }

  std::ostringstream out;
  for (std::size_t index = 0; index < labels.size(); ++index) {
    if (index != 0) {
      out << (index + 1 == labels.size() ? ", and " : ", ");
    }
    out << labels[index];
  }
  return out.str();
}

std::string render_single_block_void_call_sequence_facts(
    const SingleBlockVoidCallSequenceFacts& facts) {
  std::ostringstream out;
  out << "prepared call-wrapper facts: same-module calls=" << facts.same_module_call_count
      << ", direct variadic extern calls=" << facts.direct_variadic_extern_call_count
      << ", direct fixed-arity extern calls=" << facts.direct_fixed_arity_extern_call_count
      << ", indirect calls=" << facts.indirect_call_count
      << ", other call side effects=" << facts.other_call_effect_count;
  return out.str();
}

std::string summarize_single_block_void_call_sequence_facts(
    const SingleBlockVoidCallSequenceFacts& facts) {
  std::vector<std::string> labels;
  if (facts.same_module_call_count != 0) {
    labels.push_back(counted_label(facts.same_module_call_count,
                                   "same-module call",
                                   "same-module calls"));
  }
  if (facts.direct_variadic_extern_call_count != 0) {
    labels.push_back(counted_label(facts.direct_variadic_extern_call_count,
                                   "direct variadic extern call",
                                   "direct variadic extern calls"));
  }
  if (facts.direct_fixed_arity_extern_call_count != 0) {
    labels.push_back(counted_label(facts.direct_fixed_arity_extern_call_count,
                                   "direct fixed-arity extern call",
                                   "direct fixed-arity extern calls"));
  }
  if (facts.indirect_call_count != 0) {
    labels.push_back(counted_label(facts.indirect_call_count, "indirect call", "indirect calls"));
  }
  if (facts.other_call_effect_count != 0) {
    labels.push_back(counted_label(facts.other_call_effect_count,
                                   "other call side effect",
                                   "other call side effects"));
  }
  return join_fact_labels(labels);
}

std::string render_bounded_same_module_variadic_helper_facts(
    const BoundedSameModuleVariadicHelperFacts& facts) {
  std::ostringstream out;
  out << "prepared variadic-helper facts: explicit variadic runtime calls="
      << facts.explicit_variadic_runtime_call_count
      << ", same-module helper calls=" << facts.same_module_helper_call_count
      << ", direct extern calls=" << facts.direct_extern_call_count
      << ", indirect calls=" << facts.indirect_call_count
      << ", other call side effects=" << facts.other_call_effect_count;
  return out.str();
}

std::string summarize_bounded_same_module_variadic_helper_facts(
    const BoundedSameModuleVariadicHelperFacts& facts) {
  std::vector<std::string> labels;
  if (facts.explicit_variadic_runtime_call_count != 0) {
    labels.push_back(counted_label(facts.explicit_variadic_runtime_call_count,
                                   "explicit variadic runtime call",
                                   "explicit variadic runtime calls"));
  }
  if (facts.same_module_helper_call_count != 0) {
    labels.push_back(counted_label(facts.same_module_helper_call_count,
                                   "same-module helper call",
                                   "same-module helper calls"));
  }
  if (facts.direct_extern_call_count != 0) {
    labels.push_back(counted_label(facts.direct_extern_call_count,
                                   "direct extern call",
                                   "direct extern calls"));
  }
  if (facts.indirect_call_count != 0) {
    labels.push_back(counted_label(facts.indirect_call_count, "indirect call", "indirect calls"));
  }
  if (facts.other_call_effect_count != 0) {
    labels.push_back(counted_label(facts.other_call_effect_count,
                                   "other call side effect",
                                   "other call side effects"));
  }
  return join_fact_labels(labels);
}

std::string render_single_block_aggregate_forwarding_wrapper_facts(
    const SingleBlockAggregateForwardingWrapperFacts& facts) {
  std::ostringstream out;
  out << "prepared aggregate-wrapper facts: direct extern calls="
      << facts.direct_extern_call_count
      << ", same-module aggregate call wrappers="
      << facts.same_module_aggregate_call_wrapper_count
      << ", forwarded aggregate arguments=" << facts.forwarded_aggregate_arg_count;
  return out.str();
}

std::string summarize_single_block_aggregate_forwarding_wrapper_facts(
    const SingleBlockAggregateForwardingWrapperFacts& facts) {
  std::vector<std::string> labels;
  if (facts.direct_extern_call_count != 0) {
    labels.push_back(counted_label(facts.direct_extern_call_count,
                                   "direct extern call",
                                   "direct extern calls"));
  }
  if (facts.same_module_aggregate_call_wrapper_count != 0) {
    labels.push_back(counted_label(facts.same_module_aggregate_call_wrapper_count,
                                   "same-module aggregate call wrapper",
                                   "same-module aggregate call wrappers"));
  }
  if (facts.forwarded_aggregate_arg_count != 0) {
    labels.push_back(counted_label(facts.forwarded_aggregate_arg_count,
                                   "forwarded aggregate argument",
                                   "forwarded aggregate arguments"));
  }
  return join_fact_labels(labels);
}

std::string render_single_block_floating_aggregate_call_helper_facts(
    const SingleBlockFloatingAggregateCallHelperFacts& facts) {
  std::ostringstream out;
  out << "prepared floating-aggregate helper facts: aggregate pointer params="
      << facts.aggregate_pointer_param_count
      << ", byval aggregate params=" << facts.byval_aggregate_param_count
      << ", floating lane loads=" << facts.floating_lane_load_count
      << ", floating widening casts=" << facts.floating_widening_cast_count
      << ", direct variadic extern calls=" << facts.direct_variadic_extern_call_count
      << ", floating variadic call args=" << facts.floating_variadic_arg_count;
  return out.str();
}

std::string render_single_block_floating_aggregate_sret_copyout_helper_facts(
    const SingleBlockFloatingAggregateSretCopyoutHelperFacts& facts) {
  std::ostringstream out;
  out << "prepared floating-aggregate return-helper facts: same-module global loads="
      << facts.same_module_global_load_count
      << ", scratch slot stores=" << facts.scratch_slot_store_count
      << ", scratch slot reloads=" << facts.scratch_slot_reload_count
      << ", sret floating stores=" << facts.sret_floating_store_count;
  return out.str();
}

std::string render_single_block_same_module_scalar_call_wrapper_facts(
    const SingleBlockSameModuleScalarCallWrapperFacts& facts) {
  std::ostringstream out;
  out << "prepared helper-family facts: local-slot reloads=" << facts.local_slot_reload_count
      << ", scalar same-module helper calls=" << facts.scalar_same_module_helper_call_count
      << ", width-adjusting casts=" << facts.width_adjusting_cast_count
      << ", same-module sink wrappers=" << facts.same_module_sink_wrapper_count;
  return out.str();
}

std::string render_single_block_i64_immediate_return_helper_facts(
    const SingleBlockI64ImmediateReturnHelperFacts& facts) {
  std::ostringstream out;
  out << "prepared i64 immediate return-helper facts: opcode=" << facts.opcode
      << ", param operand side=" << facts.param_operand_side
      << ", immediate operand side=" << facts.immediate_operand_side
      << ", immediate source=" << facts.immediate_source;
  return out.str();
}

std::string render_single_block_i64_ashr_return_helper_facts(
    const SingleBlockI64AshrReturnHelperFacts& facts) {
  std::ostringstream out;
  out << "prepared i64 arithmetic-right-shift return-helper facts: param operand side="
      << facts.param_operand_side << ", shift operand side=" << facts.shift_operand_side
      << ", shift source=" << facts.shift_source;
  return out.str();
}

CompareDrivenEntryFacts build_compare_driven_entry_facts(const c4c::backend::bir::Function& function) {
  CompareDrivenEntryFacts facts;
  facts.total_param_count = function.params.size();
  facts.function_is_variadic = function.is_variadic;
  for (const auto& param : function.params) {
    if (!param.is_varargs && param.type == c4c::backend::bir::TypeKind::I32) {
      ++facts.non_variadic_i32_param_count;
    } else {
      ++facts.non_i32_or_varargs_param_count;
    }
  }
  return facts;
}

std::string render_compare_driven_entry_facts(const CompareDrivenEntryFacts& facts) {
  std::ostringstream out;
  out << "prepared compare-driven-entry facts: params=" << facts.total_param_count
      << ", non-variadic i32 params=" << facts.non_variadic_i32_param_count
      << ", non-i32 or varargs params=" << facts.non_i32_or_varargs_param_count
      << ", function variadic=" << (facts.function_is_variadic ? "yes" : "no");
  return out.str();
}

std::string summarize_single_block_same_module_scalar_call_wrapper_facts(
    const SingleBlockSameModuleScalarCallWrapperFacts& facts) {
  std::vector<std::string> labels;
  if (facts.local_slot_reload_count != 0) {
    labels.push_back(counted_label(facts.local_slot_reload_count,
                                   "local-slot reload",
                                   "local-slot reloads"));
  }
  if (facts.scalar_same_module_helper_call_count != 0) {
    labels.push_back(counted_label(facts.scalar_same_module_helper_call_count,
                                   "scalar same-module helper call",
                                   "scalar same-module helper calls"));
  }
  if (facts.width_adjusting_cast_count != 0) {
    labels.push_back(counted_label(facts.width_adjusting_cast_count,
                                   "width-adjusting cast",
                                   "width-adjusting casts"));
  }
  if (facts.same_module_sink_wrapper_count != 0) {
    labels.push_back(counted_label(facts.same_module_sink_wrapper_count,
                                   "same-module sink wrapper",
                                   "same-module sink wrappers"));
  }
  return join_fact_labels(labels);
}

std::optional<std::string_view> current_route_debug_focus_value() {
  const char* value = std::getenv(kMirFocusValueEnv.data());
  if (value == nullptr || *value == '\0') {
    return std::nullopt;
  }
  return std::string_view(value);
}

std::unordered_set<c4c::backend::prepare::PreparedValueId> collect_function_prepared_value_ids(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::ValueNameId focus_value_id) {
  std::unordered_set<c4c::backend::prepare::PreparedValueId> value_ids;
  if (function_locations == nullptr) {
    return value_ids;
  }
  for (const auto& home : function_locations->value_homes) {
    if (home.value_name == focus_value_id) {
      value_ids.insert(home.value_id);
    }
  }
  return value_ids;
}

constexpr std::string_view kCompareDrivenEntryParamShapeError =
    "x86 backend emitter only supports multi-block compare-driven entry routes through the canonical prepared-module handoff when the function exposes exactly one non-variadic i32 parameter";
constexpr std::string_view kScalarPreparedControlFlowShapeError =
    "x86 backend emitter only supports a minimal single-block i32 return terminator, a bounded equality-against-immediate guard family with immediate return leaves including fixed-offset same-module global i32 loads and pointer-backed same-module global roots, or one bounded compare-against-zero branch family through the canonical prepared-module handoff";
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
      lane_name == "single-block-aggregate-forwarding-wrapper" ||
      lane_name == "single-block-same-module-scalar-call-wrapper" ||
      lane_name == "single-block-floating-aggregate-sret-copyout-helper" ||
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
    if (attempt->lane_name == "single-block-aggregate-forwarding-wrapper") {
      return "inspect the current x86 same-module aggregate-call support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-same-module-scalar-call-wrapper") {
      return "inspect the current x86 same-module scalar helper-family support in " + lane_surface;
    }
    if (attempt->lane_name == "single-block-floating-aggregate-sret-copyout-helper") {
      return "inspect the current x86 floating aggregate return-helper support in " + lane_surface;
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
        } else if (it->lane_name == "single-block-aggregate-forwarding-wrapper") {
          summary = "single-block aggregate-forwarding wrapper recognized the function, but the "
                    "prepared same-module aggregate-call shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-same-module-scalar-call-wrapper") {
          summary =
              "single-block same-module scalar call-wrapper family recognized the function, but "
              "the prepared helper-family shape is outside the current x86 support";
        } else if (it->lane_name == "single-block-floating-aggregate-sret-copyout-helper") {
          summary = "single-block floating aggregate sret copyout helper recognized the "
                    "function, but the prepared return-helper shape is outside the current x86 "
                    "support";
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
        .facts = it->facts,
        .next_surface = next_surface_hint(&*it, kind),
    };
  }

  return FinalRejectionReport{
      .kind = FinalRejectionKind::OrdinaryRouteMiss,
      .attempt = nullptr,
      .summary = "current x86 lanes did not recognize this prepared function shape",
      .facts = std::nullopt,
      .next_surface =
          "inspect src/backend/mir/x86/codegen/prepared_module_emit.cpp for the next top-level lane",
  };
}

std::optional<FunctionRouteAttempt> build_bounded_variadic_helper_lane_attempt(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function* entry_function,
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow) {
  const auto find_defined_function = [&](std::string_view callee_name)
      -> const c4c::backend::bir::Function* {
    for (const auto* candidate : defined_functions) {
      if (candidate != nullptr && candidate->name == callee_name) {
        return candidate;
      }
    }
    return nullptr;
  };

  if (defined_functions.size() <= 1 || entry_function == nullptr || entry_function->name != "main" ||
      &function == entry_function || !function.is_variadic ||
      function.return_type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }
  if (function_control_flow != nullptr && !function_control_flow->join_transfers.empty()) {
    return std::nullopt;
  }

  BoundedSameModuleVariadicHelperFacts facts;
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
      if (call == nullptr) {
        continue;
      }
      if (string_contains(call->callee, "llvm.va_start.") ||
          string_contains(call->callee, "llvm.va_end.") ||
          string_contains(call->callee, "llvm.va_arg.")) {
        ++facts.explicit_variadic_runtime_call_count;
        continue;
      }
      if (call->is_indirect || call->callee_value.has_value()) {
        ++facts.indirect_call_count;
        continue;
      }
      if (call->callee.empty()) {
        ++facts.other_call_effect_count;
        continue;
      }
      if (call->callee.rfind("llvm.", 0) == 0) {
        ++facts.other_call_effect_count;
        continue;
      }
      if (const auto* callee = find_defined_function(call->callee);
          callee != nullptr && !callee->is_declaration) {
        ++facts.same_module_helper_call_count;
        continue;
      }
      ++facts.direct_extern_call_count;
    }
  }

  const bool carries_explicit_variadic_runtime = facts.explicit_variadic_runtime_call_count != 0;
  if (!carries_explicit_variadic_runtime && function.blocks.size() <= 1) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports non-entry bounded same-module variadic helpers when "
      "they already reduce to the current direct-extern or local-slot-guard helper surfaces";
  if (carries_explicit_variadic_runtime) {
    detail += "; this helper still carries explicit variadic-runtime state";
  } else {
    detail += "; this helper remains a multi-block variadic body carrying " +
              summarize_bounded_same_module_variadic_helper_facts(facts);
  }
  return FunctionRouteAttempt{
      .lane_name = "bounded-same-module-variadic-helper",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_bounded_same_module_variadic_helper_facts(facts),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_void_call_sequence_lane_attempt(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || function.blocks.size() != 1 ||
      function.return_type != c4c::backend::bir::TypeKind::Void) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value() || entry.insts.empty()) {
    return std::nullopt;
  }

  const auto find_defined_function = [&](std::string_view callee_name)
      -> const c4c::backend::bir::Function* {
    for (const auto* candidate : defined_functions) {
      if (candidate != nullptr && candidate->name == callee_name) {
        return candidate;
      }
    }
    return nullptr;
  };

  SingleBlockVoidCallSequenceFacts facts;

  for (const auto& inst : entry.insts) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr) {
      continue;
    }
    if (call->is_indirect || call->callee_value.has_value()) {
      ++facts.indirect_call_count;
      continue;
    }
    if (call->callee.empty()) {
      ++facts.other_call_effect_count;
      continue;
    }
    if (call->callee.rfind("llvm.", 0) == 0) {
      ++facts.other_call_effect_count;
      continue;
    }
    if (const auto* callee = find_defined_function(call->callee);
        callee != nullptr && !callee->is_declaration) {
      ++facts.same_module_call_count;
      continue;
    }
    if (call->is_variadic) {
      ++facts.direct_variadic_extern_call_count;
      continue;
    }
    ++facts.direct_fixed_arity_extern_call_count;
  }

  if (facts.same_module_call_count == 0 &&
      facts.direct_variadic_extern_call_count == 0 &&
      facts.direct_fixed_arity_extern_call_count == 0 &&
      facts.indirect_call_count == 0 &&
      facts.other_call_effect_count == 0) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports trivial single-block void helpers through the canonical "
      "prepared-module handoff; this prepared call-wrapper still carries " +
      summarize_single_block_void_call_sequence_facts(facts);
  return FunctionRouteAttempt{
      .lane_name = "single-block-void-call-sequence",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_single_block_void_call_sequence_facts(facts),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_i64_ashr_return_helper_lane_attempt(
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

  return FunctionRouteAttempt{
      .lane_name = "single-block-i64-ashr-return-helper",
      .matched = false,
      .detail =
          "x86 backend emitter only supports single-block i64 return helpers when they already "
          "reduce to the current direct passthrough or local i16/i64-sub helper surfaces; this "
          "helper still carries an i64 arithmetic-right-shift immediate return",
      .facts = render_single_block_i64_ashr_return_helper_facts(
          SingleBlockI64AshrReturnHelperFacts{
              .param_operand_side = "lhs",
              .shift_operand_side = "rhs",
              .shift_source = "direct i64",
          }),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_i64_immediate_return_helper_lane_attempt(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || !function.local_slots.empty() || function.blocks.size() != 1 ||
      function.return_type != c4c::backend::bir::TypeKind::I64 || function.params.size() != 1 ||
      function.params.front().type != c4c::backend::bir::TypeKind::I64) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      !entry.terminator.value.has_value() ||
      (entry.insts.size() != 1 && entry.insts.size() != 3)) {
    return std::nullopt;
  }

  const auto* binary = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts.back());
  if (binary == nullptr || binary->operand_type != c4c::backend::bir::TypeKind::I64 ||
      binary->opcode == c4c::backend::bir::BinaryOpcode::AShr ||
      binary->result.kind != c4c::backend::bir::Value::Kind::Named ||
      binary->result.type != c4c::backend::bir::TypeKind::I64 ||
      entry.terminator.value->kind != c4c::backend::bir::Value::Kind::Named ||
      entry.terminator.value->name != binary->result.name) {
    return std::nullopt;
  }

  const auto describe_i64_extended_i32_immediate =
      [&](const c4c::backend::bir::Value& value) -> std::optional<std::string_view> {
    if (value.kind != c4c::backend::bir::Value::Kind::Named ||
        value.type != c4c::backend::bir::TypeKind::I64 || entry.insts.size() != 3) {
      return std::nullopt;
    }

    const auto* seed = std::get_if<c4c::backend::bir::BinaryInst>(&entry.insts[0]);
    const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[1]);
    if (seed == nullptr || cast == nullptr ||
        (cast->opcode != c4c::backend::bir::CastOpcode::SExt &&
         cast->opcode != c4c::backend::bir::CastOpcode::ZExt) ||
        cast->result.kind != c4c::backend::bir::Value::Kind::Named ||
        cast->result.type != c4c::backend::bir::TypeKind::I64 ||
        cast->result.name != value.name || cast->operand.kind != c4c::backend::bir::Value::Kind::Named ||
        cast->operand.type != c4c::backend::bir::TypeKind::I32 ||
        seed->opcode != c4c::backend::bir::BinaryOpcode::Sub ||
        seed->operand_type != c4c::backend::bir::TypeKind::I32 ||
        seed->result.kind != c4c::backend::bir::Value::Kind::Named ||
        seed->result.type != c4c::backend::bir::TypeKind::I32 ||
        seed->result.name != cast->operand.name ||
        seed->lhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
        seed->lhs.type != c4c::backend::bir::TypeKind::I32 || seed->lhs.immediate != 0 ||
        seed->rhs.kind != c4c::backend::bir::Value::Kind::Immediate ||
        seed->rhs.type != c4c::backend::bir::TypeKind::I32) {
      return std::nullopt;
    }

    if (cast->opcode == c4c::backend::bir::CastOpcode::SExt) {
      return "sign-extended i32";
    }
    return "zero-extended i32";
  };

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
  const auto lhs_extended_immediate = describe_i64_extended_i32_immediate(binary->lhs);
  const auto rhs_extended_immediate = describe_i64_extended_i32_immediate(binary->rhs);
  const auto lhs_is_immediate_like = lhs_is_immediate || lhs_extended_immediate.has_value();
  const auto rhs_is_immediate_like = rhs_is_immediate || rhs_extended_immediate.has_value();
  if (!((lhs_is_param && rhs_is_immediate_like) || (lhs_is_immediate_like && rhs_is_param))) {
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

  const auto param_operand_side = lhs_is_param ? "lhs" : "rhs";
  const auto immediate_operand_side = lhs_is_immediate_like ? "lhs" : "rhs";
  const auto immediate_source =
      lhs_is_immediate ? "direct i64"
      : rhs_is_immediate ? "direct i64"
                         : (lhs_extended_immediate.has_value() ? *lhs_extended_immediate
                                                               : *rhs_extended_immediate);

  return FunctionRouteAttempt{
      .lane_name = "single-block-i64-immediate-return-helper",
      .matched = false,
      .detail =
          "x86 backend emitter only supports single-block i64 return helpers when they already "
          "reduce to the current direct passthrough or established scalar helper surfaces; this "
          "helper still carries an i64 " +
          std::string(operation) + " immediate return",
      .facts = render_single_block_i64_immediate_return_helper_facts(
          SingleBlockI64ImmediateReturnHelperFacts{
              .opcode = operation,
              .param_operand_side = param_operand_side,
              .immediate_operand_side = immediate_operand_side,
              .immediate_source = immediate_source,
          }),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_floating_aggregate_call_helper_lane_attempt(
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

  const auto is_floating_lane = [](c4c::backend::bir::TypeKind type) {
    return type == c4c::backend::bir::TypeKind::F32 ||
           type == c4c::backend::bir::TypeKind::F64 ||
           type == c4c::backend::bir::TypeKind::F128;
  };

  SingleBlockFloatingAggregateCallHelperFacts facts;
  bool saw_floating_aggregate_param = false;
  std::unordered_set<std::string_view> pointer_param_names;

  for (const auto& param : function.params) {
    if (param.type != c4c::backend::bir::TypeKind::Ptr) {
      continue;
    }
    ++facts.aggregate_pointer_param_count;
    pointer_param_names.insert(param.name);
    if (param.is_byval) {
      ++facts.byval_aggregate_param_count;
    }
    if (param.abi.has_value() &&
        (param.abi->primary_class == c4c::backend::bir::AbiValueClass::Sse ||
         param.abi->secondary_class == c4c::backend::bir::AbiValueClass::Sse ||
         param.abi->primary_class == c4c::backend::bir::AbiValueClass::X87 ||
         param.abi->secondary_class == c4c::backend::bir::AbiValueClass::X87)) {
      saw_floating_aggregate_param = true;
    }
  }

  for (const auto& inst : entry.insts) {
    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst);
        load != nullptr && load->address.has_value() &&
        pointer_param_names.find(load->address->base_name) != pointer_param_names.end() &&
        is_floating_lane(load->result.type)) {
      ++facts.floating_lane_load_count;
      saw_floating_aggregate_param = true;
    }
    if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&inst);
        cast != nullptr &&
        cast->opcode == c4c::backend::bir::CastOpcode::FPExt &&
        is_floating_lane(cast->operand.type) && is_floating_lane(cast->result.type) &&
        cast->operand.type != cast->result.type) {
      ++facts.floating_widening_cast_count;
    }
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr || call->is_indirect || call->callee_value.has_value()) {
      continue;
    }
    if (call->is_variadic && call->callee.rfind("llvm.", 0) != 0) {
      ++facts.direct_variadic_extern_call_count;
      for (const auto arg_type : call->arg_types) {
        if (arg_type == c4c::backend::bir::TypeKind::F64 ||
            arg_type == c4c::backend::bir::TypeKind::F128) {
          ++facts.floating_variadic_arg_count;
        }
      }
    }
  }

  if (!saw_floating_aggregate_param && facts.aggregate_pointer_param_count != 0 &&
      facts.floating_variadic_arg_count != 0) {
    saw_floating_aggregate_param = true;
  }

  if (facts.direct_variadic_extern_call_count == 0 || facts.aggregate_pointer_param_count == 0 ||
      !saw_floating_aggregate_param || facts.floating_variadic_arg_count == 0) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports single-block floating aggregate helpers when those "
      "aggregate arguments already reduce to the current local-slot or scalar helper surfaces; "
      "this helper still forwards floating aggregate lanes through byval/pointer wrappers into a "
      "direct variadic extern call";
  return FunctionRouteAttempt{
      .lane_name = "single-block-floating-aggregate-call-helper",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_single_block_floating_aggregate_call_helper_facts(facts),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_floating_aggregate_sret_copyout_helper_lane_attempt(
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || function.is_variadic ||
      function.return_type != c4c::backend::bir::TypeKind::Void || function.blocks.size() != 1 ||
      function.params.size() != 1 || function.params.front().type != c4c::backend::bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value()) {
    return std::nullopt;
  }

  const auto is_floating_lane = [](c4c::backend::bir::TypeKind type) {
    return type == c4c::backend::bir::TypeKind::F32 ||
           type == c4c::backend::bir::TypeKind::F64 ||
           type == c4c::backend::bir::TypeKind::F128;
  };

  const auto is_lowering_scratch_slot = [&](std::string_view slot_name) {
    const auto slot_it = std::find_if(function.local_slots.begin(),
                                      function.local_slots.end(),
                                      [&](const c4c::backend::bir::LocalSlot& slot) {
                                        return slot.name == slot_name;
                                      });
    return slot_it != function.local_slots.end() &&
           slot_it->storage_kind == c4c::backend::bir::LocalSlotStorageKind::LoweringScratch;
  };

  const std::string_view sret_param_name = function.params.front().name;
  SingleBlockFloatingAggregateSretCopyoutHelperFacts facts;
  bool saw_call = false;
  for (const auto& inst : entry.insts) {
    if (std::holds_alternative<c4c::backend::bir::CallInst>(inst)) {
      saw_call = true;
      continue;
    }
    if (const auto* load = std::get_if<c4c::backend::bir::LoadLocalInst>(&inst); load != nullptr) {
      if (load->address.has_value() &&
          load->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::GlobalSymbol &&
          is_floating_lane(load->result.type)) {
        ++facts.same_module_global_load_count;
      }
      if (!load->address.has_value() && is_floating_lane(load->result.type) &&
          is_lowering_scratch_slot(load->slot_name)) {
        ++facts.scratch_slot_reload_count;
      }
      continue;
    }
    if (const auto* store = std::get_if<c4c::backend::bir::StoreLocalInst>(&inst);
        store != nullptr && is_floating_lane(store->value.type)) {
      if (!store->address.has_value() && is_lowering_scratch_slot(store->slot_name)) {
        ++facts.scratch_slot_store_count;
      }
      if (store->address.has_value() &&
          store->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::PointerValue &&
          (store->address->base_name == sret_param_name ||
           (store->address->base_value.kind == c4c::backend::bir::Value::Kind::Named &&
            store->address->base_value.name == sret_param_name))) {
        ++facts.sret_floating_store_count;
      }
    }
  }

  if (saw_call || facts.same_module_global_load_count == 0 || facts.sret_floating_store_count == 0) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports single-block floating aggregate sret copyout helpers "
      "when those same-module aggregate returns already reduce to the current return-helper "
      "surfaces; this helper still copies floating aggregate lanes from same-module globals "
      "through scratch slots into an sret destination";
  return FunctionRouteAttempt{
      .lane_name = "single-block-floating-aggregate-sret-copyout-helper",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_single_block_floating_aggregate_sret_copyout_helper_facts(facts),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_aggregate_forwarding_wrapper_lane_attempt(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || function.is_variadic ||
      function.return_type != c4c::backend::bir::TypeKind::Void || !function.params.empty() ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value() || entry.insts.empty()) {
    return std::nullopt;
  }

  auto find_defined_function = [&](std::string_view callee_name)
      -> const c4c::backend::bir::Function* {
    for (const auto* candidate : defined_functions) {
      if (candidate != nullptr && candidate->name == callee_name) {
        return candidate;
      }
    }
    return nullptr;
  };

  SingleBlockAggregateForwardingWrapperFacts facts;

  for (const auto& inst : entry.insts) {
    const auto* call = std::get_if<c4c::backend::bir::CallInst>(&inst);
    if (call == nullptr) {
      continue;
    }
    if (call->is_indirect || call->callee.empty() || call->callee_value.has_value() ||
        call->args.size() != call->arg_types.size()) {
      return std::nullopt;
    }

    const auto* callee = find_defined_function(call->callee);
    if (callee == nullptr || callee->is_declaration) {
      if (call->callee.rfind("llvm.", 0) == 0) {
        return std::nullopt;
      }
      ++facts.direct_extern_call_count;
      continue;
    }

    if (callee->is_variadic ||
        callee->return_type != c4c::backend::bir::TypeKind::Void ||
        callee->params.size() != call->args.size()) {
      return std::nullopt;
    }

    std::size_t forwarded_aggregate_args = 0;
    for (std::size_t arg_index = 0; arg_index < call->args.size(); ++arg_index) {
      const auto& param = callee->params[arg_index];
      const auto call_arg_abi =
          arg_index < call->arg_abi.size() ? std::optional{call->arg_abi[arg_index]} : std::nullopt;
      const bool call_site_carries_aggregate_abi =
          call_arg_abi.has_value() &&
          (call_arg_abi->byval_copy || call_arg_abi->primary_class == c4c::backend::bir::AbiValueClass::Memory ||
           call_arg_abi->secondary_class == c4c::backend::bir::AbiValueClass::Memory ||
           call_arg_abi->size_bytes > sizeof(void*));
      if (param.type == c4c::backend::bir::TypeKind::Ptr &&
          call->arg_types[arg_index] == c4c::backend::bir::TypeKind::Ptr &&
          (param.size_bytes > 0 || call_site_carries_aggregate_abi)) {
        ++forwarded_aggregate_args;
      }
    }

    if (forwarded_aggregate_args == 0) {
      return std::nullopt;
    }

    ++facts.same_module_aggregate_call_wrapper_count;
    facts.forwarded_aggregate_arg_count += forwarded_aggregate_args;
  }

  if (facts.same_module_aggregate_call_wrapper_count == 0 || facts.direct_extern_call_count == 0) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports single-block aggregate-forwarding wrappers when their "
      "direct extern preamble and same-module aggregate calls already reduce to the current "
      "helper surfaces; this wrapper family still carries " +
      summarize_single_block_aggregate_forwarding_wrapper_facts(facts);
  return FunctionRouteAttempt{
      .lane_name = "single-block-aggregate-forwarding-wrapper",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_single_block_aggregate_forwarding_wrapper_facts(facts),
  };
}

std::optional<FunctionRouteAttempt> build_single_block_same_module_scalar_call_wrapper_lane_attempt(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    const c4c::backend::bir::Function& function) {
  if (function.is_declaration || function.is_variadic ||
      function.return_type != c4c::backend::bir::TypeKind::Void || !function.params.empty() ||
      function.blocks.size() != 1 || function.local_slots.empty()) {
    return std::nullopt;
  }

  const auto& entry = function.blocks.front();
  if (entry.terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
      entry.terminator.value.has_value() || entry.insts.empty()) {
    return std::nullopt;
  }

  auto find_defined_function = [&](std::string_view callee_name)
      -> const c4c::backend::bir::Function* {
    for (const auto* candidate : defined_functions) {
      if (candidate != nullptr && candidate->name == callee_name) {
        return candidate;
      }
    }
    return nullptr;
  };
  const auto is_direct_same_module_call =
      [&](const c4c::backend::bir::CallInst& call) -> const c4c::backend::bir::Function* {
    if (call.is_indirect || call.callee.empty() || call.callee_value.has_value() ||
        call.callee.rfind("llvm.", 0) == 0) {
      return nullptr;
    }
    const auto* callee = find_defined_function(call.callee);
    if (callee == nullptr || callee->is_declaration) {
      return nullptr;
    }
    return callee;
  };
  const auto is_named_value =
      [](const c4c::backend::bir::Value& value,
         std::string_view name,
         c4c::backend::bir::TypeKind type) {
        return value.kind == c4c::backend::bir::Value::Kind::Named && value.name == name &&
               value.type == type;
      };
  const auto is_integer_scalar = [](c4c::backend::bir::TypeKind type) {
    return type == c4c::backend::bir::TypeKind::I32 || type == c4c::backend::bir::TypeKind::I64;
  };
  const auto is_width_adjusting_integer_cast = [&](const c4c::backend::bir::CastInst& cast) {
    return (cast.opcode == c4c::backend::bir::CastOpcode::SExt ||
            cast.opcode == c4c::backend::bir::CastOpcode::ZExt) &&
           is_integer_scalar(cast.operand.type) && is_integer_scalar(cast.result.type) &&
           cast.operand.type != cast.result.type;
  };
  const auto feeds_local_slot_reload =
      [&](const c4c::backend::bir::Value& value, std::size_t before_index) -> bool {
    if (value.kind != c4c::backend::bir::Value::Kind::Named) {
      return false;
    }
    if (before_index == 0) {
      return false;
    }
    const auto* load =
        std::get_if<c4c::backend::bir::LoadLocalInst>(&entry.insts[before_index - 1]);
    return load != nullptr && load->result.kind == c4c::backend::bir::Value::Kind::Named &&
           load->result.name == value.name && load->result.type == value.type &&
           load->address.has_value() &&
           load->address->base_kind == c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot &&
           is_integer_scalar(load->result.type);
  };

  std::size_t wrapper_pair_count = 0;
  SingleBlockSameModuleScalarCallWrapperFacts facts;
  std::optional<std::string_view> sink_name;

  for (std::size_t inst_index = 0; inst_index < entry.insts.size(); ++inst_index) {
    const auto* helper_call = std::get_if<c4c::backend::bir::CallInst>(&entry.insts[inst_index]);
    if (helper_call == nullptr) {
      continue;
    }

    const auto* helper = is_direct_same_module_call(*helper_call);
    if (helper == nullptr || helper->is_variadic || !is_integer_scalar(helper_call->return_type) ||
        !helper_call->result.has_value() ||
        helper_call->result->kind != c4c::backend::bir::Value::Kind::Named ||
        helper_call->result->type != helper_call->return_type || helper_call->args.empty()) {
      continue;
    }

    std::size_t sink_index = inst_index + 1;
    std::string_view forwarded_name = helper_call->result->name;
    auto forwarded_type = helper_call->result->type;
    bool pair_has_local_slot_reload = false;

    if (helper_call->args.size() == 1 && helper_call->arg_types.size() == 1) {
      const auto& helper_arg = helper_call->args.front();
      if (feeds_local_slot_reload(helper_arg, inst_index)) {
        pair_has_local_slot_reload = true;
      } else if (inst_index != 0) {
        const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[inst_index - 1]);
        if (cast != nullptr && is_width_adjusting_integer_cast(*cast) &&
            cast->result.kind == c4c::backend::bir::Value::Kind::Named &&
            cast->result.name == helper_arg.name && cast->result.type == helper_arg.type &&
            is_named_value(helper_arg, cast->result.name, cast->result.type)) {
          ++facts.width_adjusting_cast_count;
          if (feeds_local_slot_reload(cast->operand, inst_index - 1)) {
            pair_has_local_slot_reload = true;
          }
        }
      }
    }

    if (sink_index < entry.insts.size()) {
      if (const auto* cast = std::get_if<c4c::backend::bir::CastInst>(&entry.insts[sink_index]);
          cast != nullptr && is_width_adjusting_integer_cast(*cast) &&
          is_named_value(cast->operand, helper_call->result->name, helper_call->result->type) &&
          helper_call->result->type == c4c::backend::bir::TypeKind::I32 &&
          cast->result.kind == c4c::backend::bir::Value::Kind::Named &&
          cast->result.type == c4c::backend::bir::TypeKind::I64) {
        ++facts.width_adjusting_cast_count;
        forwarded_name = cast->result.name;
        forwarded_type = cast->result.type;
        ++sink_index;
      }
    }

    if (sink_index >= entry.insts.size()) {
      continue;
    }

    const auto* sink_call = std::get_if<c4c::backend::bir::CallInst>(&entry.insts[sink_index]);
    if (sink_call == nullptr || sink_call->args.size() != 1 ||
        sink_call->arg_types.size() != sink_call->args.size() ||
        !is_named_value(sink_call->args.front(), forwarded_name, forwarded_type) ||
        sink_call->arg_types.front() != forwarded_type) {
      continue;
    }

    const auto* sink = is_direct_same_module_call(*sink_call);
    if (sink == nullptr || sink->is_variadic ||
        sink_call->return_type != c4c::backend::bir::TypeKind::Void || sink->params.size() != 1 ||
        sink->params.front().type != forwarded_type) {
      continue;
    }

    if (!sink_name.has_value()) {
      sink_name = sink_call->callee;
    } else if (*sink_name != sink_call->callee) {
      return std::nullopt;
    }

    ++wrapper_pair_count;
    if (pair_has_local_slot_reload) {
      ++facts.local_slot_reload_count;
    }
    ++facts.scalar_same_module_helper_call_count;
    ++facts.same_module_sink_wrapper_count;
    inst_index = sink_index;
  }

  if (wrapper_pair_count < 2 || !sink_name.has_value()) {
    return std::nullopt;
  }

  std::string detail =
      "x86 backend emitter only supports single-block same-module scalar call-wrapper families "
      "when they already reduce to the current local-slot or established scalar helper surfaces; "
      "this wrapper family still carries " +
      summarize_single_block_same_module_scalar_call_wrapper_facts(facts);
  return FunctionRouteAttempt{
      .lane_name = "single-block-same-module-scalar-call-wrapper",
      .matched = false,
      .detail = std::move(detail),
      .facts = render_single_block_same_module_scalar_call_wrapper_facts(facts),
  };
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

std::vector<std::string_view> collect_focused_bir_block_labels(
    const c4c::backend::bir::Function& function,
    std::optional<std::string_view> focus_block) {
  std::vector<std::string_view> labels;
  for (const auto& block : function.blocks) {
    if (focus_block.has_value() && block.label != *focus_block) {
      continue;
    }
    labels.push_back(block.label);
  }
  return labels;
}

std::vector<std::string_view> collect_focused_prepared_block_labels(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedControlFlowFunction* function_control_flow,
    std::optional<std::string_view> focus_block) {
  std::vector<std::string_view> labels;
  if (function_control_flow == nullptr) {
    return labels;
  }
  for (const auto& block : function_control_flow->blocks) {
    const auto label = c4c::backend::prepare::prepared_block_label(names, block.block_label);
    if (focus_block.has_value() && label != *focus_block) {
      continue;
    }
    labels.push_back(label);
  }
  return labels;
}

std::string render_route_report(const c4c::backend::prepare::PreparedBirModule& module,
                                RouteDebugVerbosity verbosity,
                                RouteDebugFocus focus) {
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
  const auto focus_value = current_route_debug_focus_value();
  if (focus.function.has_value()) {
    out << "focus function: " << *focus.function << "\n";
  }
  if (focus.block.has_value()) {
    out << "focus block: " << *focus.block << "\n";
  }
  if (focus_value.has_value()) {
    out << "focus value: " << *focus_value << "\n";
  }

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
  const auto same_module_global_supports_scalar_load =
      [&](const c4c::backend::bir::Global& global,
          c4c::backend::bir::TypeKind type,
          std::size_t byte_offset) -> bool {
    const auto scalar_size = same_module_global_scalar_size(type);
    if (!scalar_size.has_value()) {
      return false;
    }
    if (global.initializer_symbol_name.has_value()) {
      return type == c4c::backend::bir::TypeKind::Ptr && byte_offset == 0;
    }
    if (same_module_global_is_zero_initialized(global) &&
        byte_offset <= global.size_bytes &&
        *scalar_size <= global.size_bytes - byte_offset) {
      return true;
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
  std::size_t focused_function_count = 0;
  std::size_t focused_bir_block_count = 0;
  std::size_t focused_prepared_block_count = 0;
  std::size_t focused_prepared_value_count = 0;
  std::size_t focused_prepared_move_bundle_count = 0;
  for (const auto* function : defined_functions) {
    if (focus.function.has_value() && function->name != *focus.function) {
      continue;
    }
    ++focused_function_count;
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
    const auto focused_bir_blocks =
        focus.block.has_value() ? collect_focused_bir_block_labels(*function, focus.block)
                                : std::vector<std::string_view>{};
    const auto focused_prepared_blocks =
        focus.block.has_value()
            ? collect_focused_prepared_block_labels(module.names, function_control_flow, focus.block)
            : std::vector<std::string_view>{};
    const auto focus_value_id =
        focus_value.has_value()
            ? c4c::backend::prepare::resolve_prepared_value_name_id(module.names, *focus_value)
            : std::nullopt;
    const auto focused_prepared_value_ids =
        focus_value_id.has_value()
            ? collect_function_prepared_value_ids(function_locations, *focus_value_id)
            : std::unordered_set<c4c::backend::prepare::PreparedValueId>{};
    const auto focused_prepared_values =
        function_locations == nullptr || !focus_value_id.has_value()
            ? std::size_t{0}
            : static_cast<std::size_t>(std::count_if(
                  function_locations->value_homes.begin(),
                  function_locations->value_homes.end(),
                  [&](const c4c::backend::prepare::PreparedValueHome& home) {
                    return home.value_name == *focus_value_id;
                  }));
    const auto focused_prepared_move_bundles =
        function_locations == nullptr || focused_prepared_value_ids.empty()
            ? std::size_t{0}
            : static_cast<std::size_t>(std::count_if(
                  function_locations->move_bundles.begin(),
                  function_locations->move_bundles.end(),
                  [&](const c4c::backend::prepare::PreparedMoveBundle& bundle) {
                    return std::any_of(
                        bundle.moves.begin(),
                        bundle.moves.end(),
                        [&](const c4c::backend::prepare::PreparedMoveResolution& move) {
                          return focused_prepared_value_ids.count(move.from_value_id) != 0 ||
                                 focused_prepared_value_ids.count(move.to_value_id) != 0;
                        });
                  }));
    if (focus.block.has_value()) {
      focused_bir_block_count += focused_bir_blocks.size();
      focused_prepared_block_count += focused_prepared_blocks.size();
    }
    if (focus_value.has_value()) {
      focused_prepared_value_count += focused_prepared_values;
      focused_prepared_move_bundle_count += focused_prepared_move_bundles;
    }

    if (verbosity == RouteDebugVerbosity::Summary) {
      out << "\nfunction " << report.function_name << "\n";
      out << "- bir blocks: " << report.bir_block_count << "\n";
      out << "- prepared blocks: " << report.prepared_block_count << "\n";
      out << "- branch conditions: " << report.branch_condition_count << "\n";
      out << "- join transfers: " << report.join_transfer_count << "\n";
      if (focus_value.has_value()) {
        out << "- focused prepared values: " << focused_prepared_values << "\n";
        out << "- focused prepared move bundles: " << focused_prepared_move_bundles << "\n";
      }
      if (focus.block.has_value()) {
        out << "- focused bir blocks: " << focused_bir_blocks.size() << "\n";
        for (const auto label : focused_bir_blocks) {
          append_indented_line(out, 1, std::string("- ") + std::string(label));
        }
        out << "- focused prepared blocks: " << focused_prepared_blocks.size() << "\n";
        for (const auto label : focused_prepared_blocks) {
          append_indented_line(out, 1, std::string("- ") + std::string(label));
        }
      }
    } else {
      out << "\nfunction " << report.function_name << "\n";
      out << "  facts: bir_blocks=" << report.bir_block_count
          << ", prepared_blocks=" << report.prepared_block_count
          << ", branch_conditions=" << report.branch_condition_count
          << ", join_transfers=" << report.join_transfer_count << "\n";
      if (focus_value.has_value()) {
        out << "  focused prepared values: " << focused_prepared_values << "\n";
        out << "  focused prepared move bundles: " << focused_prepared_move_bundles << "\n";
      }
      if (focus.block.has_value()) {
        out << "  focused bir blocks: " << focused_bir_blocks.size() << "\n";
        for (const auto label : focused_bir_blocks) {
          append_indented_line(out, 2, std::string("- ") + std::string(label));
        }
        out << "  focused prepared blocks: " << focused_prepared_blocks.size() << "\n";
        for (const auto label : focused_prepared_blocks) {
          append_indented_line(out, 2, std::string("- ") + std::string(label));
        }
      }
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
                              const std::function<std::optional<std::string>()>& try_render,
                              const std::function<std::optional<std::string>()>& render_facts =
                                  std::function<std::optional<std::string>()>{}) {
      bool matched = false;
      std::optional<std::string> detail;
      std::optional<std::string> facts;
      try {
        matched = try_render().has_value();
      } catch (const std::exception& ex) {
        detail = ex.what();
      } catch (...) {
        detail = "unknown backend exception";
      }
      if (detail.has_value() && static_cast<bool>(render_facts)) {
        facts = render_facts();
      }
      report.attempts.push_back(FunctionRouteAttempt{
          .lane_name = std::move(lane_name),
          .matched = matched,
          .detail = std::move(detail),
          .facts = std::move(facts),
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
        throw std::invalid_argument(std::string(kScalarPreparedControlFlowShapeError));
      }
      return std::nullopt;
    }, [&]() -> std::optional<std::string> {
      return render_compare_driven_entry_facts(build_compare_driven_entry_facts(*function));
    });
    if (const auto bounded_variadic_helper_attempt = build_bounded_variadic_helper_lane_attempt(
            defined_functions, entry_function, *function, function_control_flow);
        bounded_variadic_helper_attempt.has_value()) {
      report.attempts.push_back(std::move(*bounded_variadic_helper_attempt));
    }
    if (const auto single_block_void_call_sequence_attempt =
            build_single_block_void_call_sequence_lane_attempt(defined_functions, *function);
        single_block_void_call_sequence_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_void_call_sequence_attempt));
    }
    if (const auto single_block_i64_ashr_return_helper_attempt =
            build_single_block_i64_ashr_return_helper_lane_attempt(*function);
        single_block_i64_ashr_return_helper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_i64_ashr_return_helper_attempt));
    }
    if (const auto single_block_i64_immediate_return_helper_attempt =
            build_single_block_i64_immediate_return_helper_lane_attempt(*function);
        single_block_i64_immediate_return_helper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_i64_immediate_return_helper_attempt));
    }
    if (const auto single_block_floating_aggregate_sret_copyout_helper_attempt =
            build_single_block_floating_aggregate_sret_copyout_helper_lane_attempt(*function);
        single_block_floating_aggregate_sret_copyout_helper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_floating_aggregate_sret_copyout_helper_attempt));
    }
    if (const auto single_block_floating_aggregate_call_helper_attempt =
            build_single_block_floating_aggregate_call_helper_lane_attempt(*function);
        single_block_floating_aggregate_call_helper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_floating_aggregate_call_helper_attempt));
    }
    if (const auto single_block_aggregate_forwarding_wrapper_attempt =
            build_single_block_aggregate_forwarding_wrapper_lane_attempt(defined_functions,
                                                                         *function);
        single_block_aggregate_forwarding_wrapper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_aggregate_forwarding_wrapper_attempt));
    }
    if (const auto single_block_same_module_scalar_call_wrapper_attempt =
            build_single_block_same_module_scalar_call_wrapper_lane_attempt(defined_functions,
                                                                            *function);
        single_block_same_module_scalar_call_wrapper_attempt.has_value()) {
      report.attempts.push_back(std::move(*single_block_same_module_scalar_call_wrapper_attempt));
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
        if (rejection.facts.has_value()) {
          out << "- final facts: " << *rejection.facts << "\n";
        }
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
        if (rejection.facts.has_value()) {
          out << "  final facts: " << *rejection.facts << "\n";
        }
        out << "  next inspect: " << rejection.next_surface << "\n";
      } else {
        out << "  final: matched " << matched_it->lane_name << "\n";
      }
    }
  }

  if (focus.function.has_value()) {
    out << "\nfocused functions matched: " << focused_function_count << "\n";
    if (focused_function_count == 0) {
      out << "no defined function matched the requested MIR focus\n";
    }
  }
  if (focus.block.has_value()) {
    out << "focused bir blocks matched: " << focused_bir_block_count << "\n";
    out << "focused prepared blocks matched: " << focused_prepared_block_count << "\n";
    if (focused_function_count != 0 &&
        focused_bir_block_count == 0 &&
        focused_prepared_block_count == 0) {
      out << "no block in the focused function matched the requested MIR block focus\n";
    }
  }
  if (focus_value.has_value()) {
    out << "focused prepared values matched: " << focused_prepared_value_count << "\n";
    out << "focused prepared move bundles matched: " << focused_prepared_move_bundle_count << "\n";
    if (focused_function_count != 0 && focused_prepared_value_count == 0) {
      out << "no prepared value in the focused function matched the requested MIR value focus\n";
    }
  }

  return out.str();
}

}  // namespace

std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return render_route_report(module,
                             RouteDebugVerbosity::Summary,
                             RouteDebugFocus{
                                 .function = focus_function,
                                 .block = focus_block,
                             });
}

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return render_route_report(module,
                             RouteDebugVerbosity::Trace,
                             RouteDebugFocus{
                                 .function = focus_function,
                                 .block = focus_block,
                             });
}

}  // namespace c4c::backend::x86
