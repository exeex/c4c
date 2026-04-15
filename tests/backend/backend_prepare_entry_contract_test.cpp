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

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& module,
                                                      std::string_view source_kind,
                                                      std::string_view source_name) {
  for (const auto& object : module.stack_layout.objects) {
    if (object.source_kind == source_kind && object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

bir::Module make_prepare_contract_bir_module() {
  bir::Module module;
  bir::Function function;
  function.name = "id_pair";
  function.return_type = bir::TypeKind::Void;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%ret.sret",
      .size_bytes = 8,
      .align_bytes = 4,
      .is_sret = true,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "%p.p",
      .size_bytes = 8,
      .align_bytes = 4,
      .is_byval = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "flag.slot",
      .type = bir::TypeKind::I1,
      .size_bytes = 1,
      .align_bytes = 1,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "param.copy.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%call.result.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%call.result.4",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .callee = "make_pair",
      .args = {bir::Value::named(bir::TypeKind::Ptr, "%call.result")},
      .arg_types = {bir::TypeKind::Ptr},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = 8,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
      .sret_storage_name = "%call.result",
  });
  entry.terminator = bir::ReturnTerminator{};

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
      make_prepare_contract_bir_module(), c4c::backend::Target::Riscv64);
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
  if (!contains_note(prepared_bir.notes, "stack_layout", "local-slot stack objects")) {
    return fail("semantic-BIR prepare stack-layout note should mention local-slot stack objects");
  }
  if (!contains_note(prepared_bir.notes, "stack_layout", "byval/sret memory-route")) {
    return fail("semantic-BIR prepare stack-layout note should mention byval/sret memory-route objects");
  }
  if (!contains_note(prepared_bir.notes, "stack_layout", "aggregate call-result sret storage")) {
    return fail("semantic-BIR prepare stack-layout note should mention aggregate call-result storage");
  }
  constexpr std::string_view kExpectedBirPhases[] = {
      "legalize",
      "stack_layout",
  };
  if (prepared_bir.completed_phases.size() != std::size(kExpectedBirPhases)) {
    return fail("semantic-BIR prepare entry should complete legalize and stack_layout in this slice");
  }
  for (std::size_t index = 0; index < std::size(kExpectedBirPhases); ++index) {
    if (prepared_bir.completed_phases[index] != kExpectedBirPhases[index]) {
      return fail("unexpected semantic-BIR prepare phase order");
    }
  }
  if (prepared_bir.stack_layout.objects.size() != 7) {
    return fail("semantic-BIR stack layout should publish local-slot, byval/sret, and call-result frame objects");
  }
  const auto* local_slot = find_stack_object(prepared_bir, "local_slot", "flag.slot");
  if (local_slot == nullptr || local_slot->function_name != "id_pair" ||
      local_slot->type != bir::TypeKind::I32 || local_slot->size_bytes != 4 ||
      local_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout artifact drifted from the legalized local-slot contract");
  }
  const auto* byval_copy_slot = find_stack_object(prepared_bir, "byval_copy_slot", "param.copy.0");
  if (byval_copy_slot == nullptr || byval_copy_slot->function_name != "id_pair" ||
      byval_copy_slot->type != bir::TypeKind::I32 || byval_copy_slot->size_bytes != 4 ||
      byval_copy_slot->align_bytes != 4) {
    return fail("semantic-BIR stack layout should publish byval-copy local slots as prepared frame objects");
  }
  const auto* sret_param = find_stack_object(prepared_bir, "sret_param", "%ret.sret");
  if (sret_param == nullptr || sret_param->function_name != "id_pair" ||
      sret_param->type != bir::TypeKind::Ptr || sret_param->size_bytes != 8 ||
      sret_param->align_bytes != 4) {
    return fail("semantic-BIR stack layout should publish the sret memory route as a prepared frame object");
  }
  const auto* byval_param = find_stack_object(prepared_bir, "byval_param", "%p.p");
  if (byval_param == nullptr || byval_param->function_name != "id_pair" ||
      byval_param->type != bir::TypeKind::Ptr || byval_param->size_bytes != 8 ||
      byval_param->align_bytes != 4) {
    return fail("semantic-BIR stack layout should publish the byval memory route as a prepared frame object");
  }
  const auto* call_result = find_stack_object(prepared_bir, "call_result_sret", "%call.result");
  if (call_result == nullptr || call_result->function_name != "id_pair" ||
      call_result->type != bir::TypeKind::Ptr || call_result->size_bytes != 8 ||
      call_result->align_bytes != 4) {
    return fail("semantic-BIR stack layout should publish aggregate call-result sret storage as a prepared frame object");
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
