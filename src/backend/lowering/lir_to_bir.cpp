#include "lir_to_bir.hpp"

#include <stdexcept>
#include <utility>

namespace c4c::backend {

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options) {
  auto context = make_lowering_context(module, options);
  context.note("pipeline", "begin lir_to_bir lowering pipeline");

  auto analysis = analyze_module(context);
  auto lowered = lower_module(context, analysis);

  context.note("pipeline", "finish lir_to_bir lowering pipeline");
  return BirLoweringResult{
      .module = std::move(lowered),
      .analysis = std::move(analysis),
      .notes = std::move(context.notes),
  };
}

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  return try_lower_to_bir_with_options(module, BirLoweringOptions{}).module;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  auto result = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (result.module.has_value()) {
    return *result.module;
  }
  throw std::invalid_argument(
      "lir_to_bir pipeline skeleton is wired, but semantic instruction lowering is still disabled");
}

}  // namespace c4c::backend
