#include "lir_to_bir.hpp"

#include <stdexcept>

namespace c4c::backend {

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options) {
  (void)module;
  (void)options;
  return BirLoweringResult{};
}

std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  (void)module;
  return std::nullopt;
}

bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module) {
  (void)module;
  throw std::invalid_argument("bir lowering disabled after try_lower_* purge");
}

}  // namespace c4c::backend
