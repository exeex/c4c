#pragma once

#include "../bir.hpp"
#include "../../codegen/lir/ir.hpp"
#include "../target.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

struct PrepareOptions {
  bool run_legalize = true;
  bool run_stack_layout = true;
  bool run_liveness = true;
  bool run_regalloc = true;
};

struct PrepareNote {
  std::string phase;
  std::string message;
};

struct PreparedStackObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
};

struct PreparedStackLayout {
  std::vector<PreparedStackObject> objects;
};

struct PreparedLivenessObject {
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  std::string contract_kind;
};

struct PreparedLiveness {
  std::vector<PreparedLivenessObject> objects;
};

enum class PrepareRoute {
  SemanticBirShared,
  BootstrapLirFallback,
};

[[nodiscard]] constexpr std::string_view prepare_route_name(PrepareRoute route) {
  switch (route) {
    case PrepareRoute::SemanticBirShared:
      return "semantic_bir_shared";
    case PrepareRoute::BootstrapLirFallback:
      return "bootstrap_lir_fallback";
  }
  return "unknown";
}

enum class PreparedBirInvariant {
  NoTargetFacingI1,
  NoPhiNodes,
};

[[nodiscard]] constexpr std::string_view prepared_bir_invariant_name(PreparedBirInvariant invariant) {
  switch (invariant) {
    case PreparedBirInvariant::NoTargetFacingI1:
      return "no_target_facing_i1";
    case PreparedBirInvariant::NoPhiNodes:
      return "no_phi_nodes";
  }
  return "unknown";
}

struct PreparedLirModule {
  c4c::codegen::lir::LirModule module;
  Target target = Target::X86_64;
  PrepareRoute route = PrepareRoute::BootstrapLirFallback;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

struct PreparedBirModule {
  c4c::backend::bir::Module module;
  Target target = Target::X86_64;
  PrepareRoute route = PrepareRoute::SemanticBirShared;
  std::vector<PreparedBirInvariant> invariants;
  PreparedStackLayout stack_layout;
  PreparedLiveness liveness;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

PreparedLirModule prepare_bootstrap_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options = {});

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

PreparedLirModule prepare_lir_module_with_options(
    const c4c::codegen::lir::LirModule& module,
    Target target,
    const PrepareOptions& options = {});

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare
