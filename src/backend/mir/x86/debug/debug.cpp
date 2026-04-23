#include "debug.hpp"

#include "../abi/abi.hpp"

#include "../../../prealloc/prealloc.hpp"

#include <sstream>

namespace c4c::backend::x86::debug {

namespace {

std::size_t count_defined_functions(const c4c::backend::prepare::PreparedBirModule& module) {
  std::size_t count = 0;
  for (const auto& function : module.module.functions) {
    if (!function.is_declaration) {
      ++count;
    }
  }
  return count;
}

std::string render_report(const c4c::backend::prepare::PreparedBirModule& module,
                          bool trace,
                          std::optional<std::string_view> focus_function,
                          std::optional<std::string_view> focus_block) {
  std::ostringstream out;
  const auto target_profile = c4c::backend::x86::abi::resolve_target_profile(module);
  const auto target_triple = c4c::backend::x86::abi::resolve_target_triple(module);
  const auto defined_functions = count_defined_functions(module);
  out << "x86 route " << (trace ? "trace" : "summary") << "\n";
  out << "target triple: " << target_triple << "\n";
  out << "target arch: " << static_cast<int>(target_profile.arch) << "\n";
  out << "defined functions: " << defined_functions << "\n";
  if (focus_function.has_value()) {
    out << "focus function: " << *focus_function << "\n";
  }
  if (focus_block.has_value()) {
    out << "focus block: " << *focus_block << "\n";
  }
  out << "route owner: x86/debug\n";
  out << "module emitter: x86/module\n";
  if (trace) {
    for (const auto& function : module.module.functions) {
      if (function.is_declaration) {
        continue;
      }
      if (focus_function.has_value() && function.name != *focus_function) {
        continue;
      }
      out << "- function " << function.name << ": "
          << function.blocks.size() << " blocks, return type "
          << c4c::backend::bir::render_type(function.return_type) << "\n";
    }
  }
  out << "status: contract-first debug surface; structured lane diagnostics still deferred\n";
  return out.str();
}

}  // namespace

std::string summarize_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return render_report(module, false, focus_function, focus_block);
}

std::string trace_prepared_module_routes(
    const c4c::backend::prepare::PreparedBirModule& module,
    std::optional<std::string_view> focus_function,
    std::optional<std::string_view> focus_block) {
  return render_report(module, true, focus_function, focus_block);
}

}  // namespace c4c::backend::x86::debug
