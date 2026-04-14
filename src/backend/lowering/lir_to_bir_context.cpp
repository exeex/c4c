#include "lir_to_bir.hpp"

#include <utility>

namespace c4c::backend {

void BirLoweringContext::note(std::string phase, std::string message) {
  notes.push_back(BirLoweringNote{
      .phase = std::move(phase),
      .message = std::move(message),
  });
}

BirLoweringContext make_lowering_context(const c4c::codegen::lir::LirModule& module,
                                         const BirLoweringOptions& options) {
  return BirLoweringContext{
      .lir_module = module,
      .options = options,
      .notes = {},
  };
}

}  // namespace c4c::backend
