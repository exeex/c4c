#pragma once

#include "bir.hpp"
#include "../../codegen/lir/ir.hpp"

#include <cstddef>
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
  bool preserve_dynamic_alloca = false;
  bool allow_bounded_pattern_folds = false;
};

struct BirLoweringNote {
  std::string phase;
  std::string message;
};

struct BirFunctionPreScan {
  std::string function_name;
  std::size_t block_count = 0;
  std::size_t instruction_count = 0;
  bool has_calls = false;
  bool has_memory_ops = false;
  bool has_control_flow = false;
};

struct BirModuleAnalysis {
  std::size_t function_count = 0;
  std::size_t global_count = 0;
  std::size_t string_constant_count = 0;
  std::size_t extern_decl_count = 0;
  bool has_calls = false;
  bool has_memory_ops = false;
  bool has_control_flow = false;
  std::vector<BirFunctionPreScan> functions;
};

struct BirLoweringResult {
  std::optional<bir::Module> module;
  BirModuleAnalysis analysis;
  std::vector<BirLoweringNote> notes;
};

BirLoweringResult try_lower_to_bir_with_options(
    const c4c::codegen::lir::LirModule& module,
    const BirLoweringOptions& options);
std::optional<bir::Module> try_lower_to_bir(const c4c::codegen::lir::LirModule& module);
bir::Module lower_to_bir(const c4c::codegen::lir::LirModule& module);

}  // namespace c4c::backend
