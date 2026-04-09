#pragma once

#include "../bir.hpp"
#include "../../codegen/lir/ir.hpp"

#include <optional>
#include <string>
#include <vector>

namespace c4c::backend {

struct BirLoweringOptions {
  bool normalize_cfg = true;
  bool lower_phi = true;
  bool lower_memory = true;
  bool lower_calls = true;
  bool legalize_types = true;
  bool lower_aggregates = true;
  bool allow_bounded_pattern_folds = true;
};

struct BirLoweringNote {
  std::string phase;
  std::string message;
};

struct BirLoweringResult {
  std::optional<bir::Module> module;
  std::vector<BirLoweringNote> notes;
};

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options);
std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
