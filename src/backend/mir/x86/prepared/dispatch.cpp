#include "prepared.hpp"

namespace c4c::backend::x86::prepared {

FastPath classify_module_fast_path(const c4c::backend::prepare::PreparedBirModule& module,
                                   std::optional<std::string_view> focus_function) {
  FastPath decision{};
  std::size_t defined_functions = 0;
  for (const auto& function : module.module.functions) {
    if (function.is_declaration) {
      continue;
    }
    if (focus_function.has_value() && function.name != *focus_function) {
      continue;
    }
    ++defined_functions;
  }

  decision.accepted = defined_functions <= 1;
  decision.lane = defined_functions <= 1 ? "single-defined-function" : "multi-defined-module";
  decision.reason = decision.accepted
                        ? "bounded contract-first fast path available"
                        : "behavior-recovery packet must reintroduce multi-function dispatch";
  return decision;
}

}  // namespace c4c::backend::x86::prepared
