#include "src/backend/prepare/prepare.hpp"

#include <iostream>
#include <string_view>

namespace {

namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool contains_note(const std::vector<prepare::PrepareNote>& notes,
                   std::string_view phase,
                   std::string_view needle) {
  for (const auto& note : notes) {
    if (note.phase == phase && note.message.find(needle) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool contains_invariant(const prepare::PreparedBirModule& module,
                        prepare::PreparedBirInvariant invariant) {
  for (const auto& entry : module.invariants) {
    if (entry == invariant) {
      return true;
    }
  }
  return false;
}

bir::Module make_minimal_bir_module() {
  bir::Module module;
  bir::Function function;
  function.name = "main";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

lir::LirModule make_minimal_lir_module() {
  lir::LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  lir::LirFunction function;
  function.name = "main";
  function.signature_text = "define i32 @main()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.terminator = lir::LirRet{
      .value_str = "i32 0",
      .type_str = "i32",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));
  return module;
}

}  // namespace

int main() {
  const auto prepared_bir = prepare::prepare_semantic_bir_module_with_options(
      make_minimal_bir_module(), c4c::backend::Target::Riscv64);
  if (prepared_bir.route != prepare::PrepareRoute::SemanticBirShared) {
    return fail("semantic-BIR prepare entry should advertise the shared prepare route");
  }
  if (prepare::prepare_route_name(prepared_bir.route) != "semantic_bir_shared") {
    return fail("semantic-BIR prepare route name drifted");
  }
  if (!contains_invariant(prepared_bir, prepare::PreparedBirInvariant::NoTargetFacingI1)) {
    return fail("semantic-BIR prepare entry should advertise the no-target-facing-i1 invariant");
  }
  if (!contains_invariant(prepared_bir, prepare::PreparedBirInvariant::NoPhiNodes)) {
    return fail("semantic-BIR prepare entry should advertise the no-phi-nodes invariant");
  }
  if (prepared_bir.invariants.size() != 2) {
    return fail("semantic-BIR prepare entry should advertise exactly the active legality invariants");
  }
  if (prepare::prepared_bir_invariant_name(prepared_bir.invariants[0]) != "no_phi_nodes") {
    return fail("semantic-BIR prepare phi invariant name drifted");
  }
  if (prepare::prepared_bir_invariant_name(prepared_bir.invariants[1]) != "no_target_facing_i1") {
    return fail("semantic-BIR prepare legality invariant name drifted");
  }
  if (!contains_note(prepared_bir.notes,
                     "prepare",
                     "shared semantic-BIR to prepared-BIR route")) {
    return fail("missing semantic-BIR prepare entry contract note");
  }
  if (!contains_note(prepared_bir.notes, "legalize", "signature/storage bookkeeping")) {
    return fail("semantic-BIR prepare legalize note should mention signature/storage bookkeeping");
  }
  if (!contains_note(prepared_bir.notes, "legalize", "memory-address/load-store bookkeeping")) {
    return fail("semantic-BIR prepare legalize note should mention memory-address/load-store bookkeeping");
  }
  if (!contains_note(prepared_bir.notes, "legalize", "call return type text")) {
    return fail("semantic-BIR prepare legalize note should mention call return type text");
  }
  if (prepared_bir.completed_phases.size() != 1 || prepared_bir.completed_phases[0] != "legalize") {
    return fail("semantic-BIR prepare entry should only complete legalize in the current slice");
  }

  const auto prepared_lir = prepare::prepare_bootstrap_lir_module_with_options(
      make_minimal_lir_module(), c4c::backend::Target::X86_64);
  if (prepared_lir.route != prepare::PrepareRoute::BootstrapLirFallback) {
    return fail("bootstrap-LIR prepare entry should advertise the fallback prepare route");
  }
  if (prepare::prepare_route_name(prepared_lir.route) != "bootstrap_lir_fallback") {
    return fail("bootstrap-LIR prepare route name drifted");
  }
  if (!contains_note(prepared_lir.notes, "prepare", "bootstrap LIR fallback route")) {
    return fail("missing bootstrap-LIR prepare fallback note");
  }
  constexpr std::string_view kExpectedPhases[] = {
      "legalize",
      "stack_layout",
      "liveness",
      "regalloc",
  };
  if (prepared_lir.completed_phases.size() != std::size(kExpectedPhases)) {
    return fail("unexpected bootstrap-LIR prepare phase count");
  }
  for (std::size_t index = 0; index < std::size(kExpectedPhases); ++index) {
    if (prepared_lir.completed_phases[index] != kExpectedPhases[index]) {
      return fail("unexpected bootstrap-LIR prepare phase order");
    }
  }

  return 0;
}
