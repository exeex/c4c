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

const prepare::PreparedLivenessObject* find_liveness_object(const prepare::PreparedBirModule& module,
                                                            std::string_view source_kind,
                                                            std::string_view source_name) {
  for (const auto& object : module.liveness.objects) {
    if (object.source_kind == source_kind && object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocFunction* find_regalloc_function(
    const prepare::PreparedBirModule& module,
    std::string_view function_name) {
  for (const auto& function : module.regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocObject* find_regalloc_object(const prepare::PreparedRegallocFunction& function,
                                                            std::string_view source_kind,
                                                            std::string_view source_name) {
  for (const auto& object : function.objects) {
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
      .name = "carry.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "param.copy.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_byval_copy = true,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "addressed.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "scratch.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .storage_kind = bir::LocalSlotStorageKind::LoweringScratch,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "writeonly.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "window.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "readonly.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "callread.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "multiwrite.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "phi.slot",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .phi_observation = bir::PhiObservation{
          .result = bir::Value::named(bir::TypeKind::I32, "phi.result"),
          .incomings =
              {
                  bir::PhiIncoming{
                      .label = "then",
                      .value = bir::Value::immediate_i32(1),
                  },
                  bir::PhiIncoming{
                      .label = "else",
                      .value = bir::Value::immediate_i32(0),
                  },
              },
      },
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
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%va.arg.result.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%va.arg.result.4",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "carry.slot",
      .value = bir::Value::immediate_i32(7),
      .align_bytes = 4,
  });
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
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "callread.load.0"),
      .slot_name = "callread.slot",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "llvm.va_arg.aggregate",
      .args = {
          bir::Value::named(bir::TypeKind::Ptr, "%va.arg.result"),
          bir::Value::named(bir::TypeKind::Ptr, "%ap.cursor"),
      },
      .arg_types = {bir::TypeKind::Ptr, bir::TypeKind::Ptr},
      .arg_abi =
          {bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
               .size_bytes = 8,
               .align_bytes = 4,
               .primary_class = bir::AbiValueClass::Memory,
               .sret_pointer = true,
           },
           bir::CallArgAbiInfo{
               .type = bir::TypeKind::Ptr,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "addressed.load"),
      .slot_name = "flag.slot",
      .address =
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = "addressed.slot",
              .size_bytes = 4,
              .align_bytes = 4,
          },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "carry.load"),
      .slot_name = "carry.slot",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "writeonly.slot",
      .value = bir::Value::immediate_i32(11),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "window.slot",
      .value = bir::Value::immediate_i32(13),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "window.load"),
      .slot_name = "window.slot",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "readonly.load.0"),
      .slot_name = "readonly.slot",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "readonly.load.1"),
      .slot_name = "readonly.slot",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "multiwrite.slot",
      .value = bir::Value::immediate_i32(17),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "multiwrite.slot",
      .value = bir::Value::immediate_i32(19),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "callread.load.1"),
      .slot_name = "callread.slot",
      .align_bytes = 4,
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
  if (!contains_note(prepared_bir.notes,
                     "stack_layout",
                     "local-slot, lowering scratch, address-taken local-slot, and phi-materialize stack objects")) {
    return fail("semantic-BIR prepare stack-layout note should mention local-slot stack objects");
  }
  if (!contains_note(prepared_bir.notes, "stack_layout", "byval/sret memory-route")) {
    return fail("semantic-BIR prepare stack-layout note should mention byval/sret memory-route objects");
  }
  if (!contains_note(prepared_bir.notes, "stack_layout", "aggregate call-result sret storage")) {
    return fail("semantic-BIR prepare stack-layout note should mention aggregate call-result storage");
  }
  if (!contains_note(prepared_bir.notes, "stack_layout", "aggregate va_arg output storage")) {
    return fail("semantic-BIR prepare stack-layout note should mention aggregate va_arg output storage");
  }
  if (!contains_note(prepared_bir.notes, "liveness", "value-storage or address-exposed storage contracts")) {
    return fail("semantic-BIR prepare liveness note should mention the prepared frame-object contracts");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "register_candidate or fixed_stack_storage")) {
    return fail("semantic-BIR prepare regalloc note should mention the prepared allocation contracts");
  }
  if (!contains_note(prepared_bir.notes,
                     "regalloc",
                     "direct read/write, addressed-access, and call-argument exposure counts")) {
    return fail("semantic-BIR prepare regalloc note should mention the prepared access/exposure summary");
  }
  if (!contains_note(prepared_bir.notes,
                     "regalloc",
                     "target-neutral priority buckets for single-point, multi-point, and call-spanning value-storage objects")) {
    return fail("semantic-BIR prepare regalloc note should mention the target-neutral priority buckets");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "compact access-shape summaries")) {
    return fail("semantic-BIR prepare regalloc note should mention compact access-shape summaries");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "assignment-readiness cues")) {
    return fail("semantic-BIR prepare regalloc note should mention assignment-readiness cues");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "first and last access-kind cues")) {
    return fail("semantic-BIR prepare regalloc note should mention first and last access-kind cues");
  }
  if (!contains_note(prepared_bir.notes,
                     "regalloc",
                     "instruction-order access windows and call-crossing cues")) {
    return fail("semantic-BIR prepare regalloc note should mention instruction-order access windows");
  }
  constexpr std::string_view kExpectedBirPhases[] = {
      "legalize",
      "stack_layout",
      "liveness",
      "regalloc",
  };
  if (prepared_bir.completed_phases.size() != std::size(kExpectedBirPhases)) {
    return fail("semantic-BIR prepare entry should complete legalize, stack_layout, liveness, and regalloc in this slice");
  }
  for (std::size_t index = 0; index < std::size(kExpectedBirPhases); ++index) {
    if (prepared_bir.completed_phases[index] != kExpectedBirPhases[index]) {
      return fail("unexpected semantic-BIR prepare phase order");
    }
  }
  if (prepared_bir.stack_layout.objects.size() != 19) {
    return fail(
        "semantic-BIR stack layout should publish local-slot, lowering scratch, address-taken local-slot, phi-materialize, byval/sret, call-result, and va_arg frame objects");
  }
  const auto* local_slot = find_stack_object(prepared_bir, "local_slot", "flag.slot");
  if (local_slot == nullptr || local_slot->function_name != "id_pair" ||
      local_slot->type != bir::TypeKind::I32 || local_slot->size_bytes != 4 ||
      local_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout artifact drifted from the legalized local-slot contract");
  }
  const auto* scratch_slot = find_stack_object(prepared_bir, "lowering_scratch_slot", "scratch.slot");
  if (scratch_slot == nullptr || scratch_slot->function_name != "id_pair" ||
      scratch_slot->type != bir::TypeKind::I32 || scratch_slot->size_bytes != 4 ||
      scratch_slot->align_bytes != 4) {
    return fail(
        "semantic-BIR stack layout should publish lowering-created scratch slots as prepared frame objects");
  }
  const auto* carry_slot = find_stack_object(prepared_bir, "local_slot", "carry.slot");
  if (carry_slot == nullptr || carry_slot->function_name != "id_pair" ||
      carry_slot->type != bir::TypeKind::I32 || carry_slot->size_bytes != 4 ||
      carry_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve call-crossing local slots as prepared frame objects");
  }
  const auto* window_slot = find_stack_object(prepared_bir, "local_slot", "window.slot");
  if (window_slot == nullptr || window_slot->function_name != "id_pair" ||
      window_slot->type != bir::TypeKind::I32 || window_slot->size_bytes != 4 ||
      window_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve non-call-spanning local slots as prepared frame objects");
  }
  const auto* readonly_slot = find_stack_object(prepared_bir, "local_slot", "readonly.slot");
  if (readonly_slot == nullptr || readonly_slot->function_name != "id_pair" ||
      readonly_slot->type != bir::TypeKind::I32 || readonly_slot->size_bytes != 4 ||
      readonly_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve non-call-spanning read-only local slots as prepared frame objects");
  }
  const auto* callread_slot = find_stack_object(prepared_bir, "local_slot", "callread.slot");
  if (callread_slot == nullptr || callread_slot->function_name != "id_pair" ||
      callread_slot->type != bir::TypeKind::I32 || callread_slot->size_bytes != 4 ||
      callread_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve call-spanning read-only local slots as prepared frame objects");
  }
  const auto* multiwrite_slot = find_stack_object(prepared_bir, "local_slot", "multiwrite.slot");
  if (multiwrite_slot == nullptr || multiwrite_slot->function_name != "id_pair" ||
      multiwrite_slot->type != bir::TypeKind::I32 || multiwrite_slot->size_bytes != 4 ||
      multiwrite_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve non-call-spanning multi-write local slots as prepared frame objects");
  }
  const auto* byval_copy_slot = find_stack_object(prepared_bir, "byval_copy_slot", "param.copy.0");
  if (byval_copy_slot == nullptr || byval_copy_slot->function_name != "id_pair" ||
      byval_copy_slot->type != bir::TypeKind::I32 || byval_copy_slot->size_bytes != 4 ||
      byval_copy_slot->align_bytes != 4) {
    return fail("semantic-BIR stack layout should publish byval-copy local slots as prepared frame objects");
  }
  const auto* address_taken_slot =
      find_stack_object(prepared_bir, "address_taken_local_slot", "addressed.slot");
  if (address_taken_slot == nullptr || address_taken_slot->function_name != "id_pair" ||
      address_taken_slot->type != bir::TypeKind::I32 || address_taken_slot->size_bytes != 4 ||
      address_taken_slot->align_bytes != 4) {
    return fail(
        "semantic-BIR stack layout should publish address-taken local slots as prepared frame objects");
  }
  const auto* phi_slot = find_stack_object(prepared_bir, "phi_materialize_slot", "phi.slot");
  if (phi_slot == nullptr || phi_slot->function_name != "id_pair" ||
      phi_slot->type != bir::TypeKind::I32 || phi_slot->size_bytes != 4 ||
      phi_slot->align_bytes != 4) {
    return fail(
        "semantic-BIR stack layout should publish phi-materialized local slots as prepared frame objects");
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
  const auto* va_arg_result =
      find_stack_object(prepared_bir, "va_arg_aggregate_result", "%va.arg.result");
  if (va_arg_result == nullptr || va_arg_result->function_name != "id_pair" ||
      va_arg_result->type != bir::TypeKind::Ptr || va_arg_result->size_bytes != 8 ||
      va_arg_result->align_bytes != 4) {
    return fail(
        "semantic-BIR stack layout should publish aggregate va_arg output storage as a prepared frame object");
  }
  if (prepared_bir.liveness.objects.size() != prepared_bir.stack_layout.objects.size()) {
    return fail("semantic-BIR liveness should classify every prepared stack-layout object");
  }
  const auto* local_slot_liveness = find_liveness_object(prepared_bir, "local_slot", "flag.slot");
  if (local_slot_liveness == nullptr || local_slot_liveness->function_name != "id_pair" ||
      local_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should classify local stack objects as value storage");
  }
  const auto* scratch_slot_liveness =
      find_liveness_object(prepared_bir, "lowering_scratch_slot", "scratch.slot");
  if (scratch_slot_liveness == nullptr || scratch_slot_liveness->function_name != "id_pair" ||
      scratch_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should classify lowering scratch stack objects as value storage");
  }
  const auto* carry_slot_liveness = find_liveness_object(prepared_bir, "local_slot", "carry.slot");
  if (carry_slot_liveness == nullptr || carry_slot_liveness->function_name != "id_pair" ||
      carry_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep call-crossing local slots in the value-storage contract");
  }
  const auto* window_slot_liveness = find_liveness_object(prepared_bir, "local_slot", "window.slot");
  if (window_slot_liveness == nullptr || window_slot_liveness->function_name != "id_pair" ||
      window_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep non-call-spanning local slots in the value-storage contract");
  }
  const auto* readonly_slot_liveness =
      find_liveness_object(prepared_bir, "local_slot", "readonly.slot");
  if (readonly_slot_liveness == nullptr || readonly_slot_liveness->function_name != "id_pair" ||
      readonly_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep non-call-spanning read-only local slots in the value-storage contract");
  }
  const auto* callread_slot_liveness =
      find_liveness_object(prepared_bir, "local_slot", "callread.slot");
  if (callread_slot_liveness == nullptr || callread_slot_liveness->function_name != "id_pair" ||
      callread_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep call-spanning read-only local slots in the value-storage contract");
  }
  const auto* multiwrite_slot_liveness =
      find_liveness_object(prepared_bir, "local_slot", "multiwrite.slot");
  if (multiwrite_slot_liveness == nullptr || multiwrite_slot_liveness->function_name != "id_pair" ||
      multiwrite_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep non-call-spanning multi-write local slots in the value-storage contract");
  }
  const auto* address_taken_liveness =
      find_liveness_object(prepared_bir, "address_taken_local_slot", "addressed.slot");
  if (address_taken_liveness == nullptr || address_taken_liveness->function_name != "id_pair" ||
      address_taken_liveness->contract_kind != "address_exposed_storage") {
    return fail(
        "semantic-BIR liveness should classify address-taken stack objects as address-exposed storage");
  }
  const auto* sret_param_liveness = find_liveness_object(prepared_bir, "sret_param", "%ret.sret");
  if (sret_param_liveness == nullptr || sret_param_liveness->function_name != "id_pair" ||
      sret_param_liveness->contract_kind != "address_exposed_storage") {
    return fail("semantic-BIR liveness should classify sret memory routes as address-exposed storage");
  }
  const auto* call_result_liveness =
      find_liveness_object(prepared_bir, "call_result_sret", "%call.result");
  if (call_result_liveness == nullptr || call_result_liveness->function_name != "id_pair" ||
      call_result_liveness->contract_kind != "address_exposed_storage") {
    return fail(
        "semantic-BIR liveness should classify aggregate call-result storage as address-exposed storage");
  }
  if (prepared_bir.regalloc.functions.size() != 1) {
    return fail("semantic-BIR regalloc should publish one per-function grouping for this module");
  }
  const auto* regalloc_function = find_regalloc_function(prepared_bir, "id_pair");
  if (regalloc_function == nullptr) {
    return fail("semantic-BIR regalloc should publish the function grouping by semantic function name");
  }
  if (regalloc_function->objects.size() != prepared_bir.liveness.objects.size()) {
    return fail("semantic-BIR regalloc should classify every prepared liveness object");
  }
  if (regalloc_function->register_candidate_count != 14 ||
      regalloc_function->fixed_stack_storage_count != 5) {
    return fail("semantic-BIR regalloc should summarize register-candidate vs fixed-stack prepared objects");
  }
  const auto* local_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "flag.slot");
  if (local_slot_regalloc == nullptr || local_slot_regalloc->contract_kind != "value_storage" ||
      local_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should treat value-storage objects as register candidates");
  }
  if (local_slot_regalloc->priority_bucket != "single_point_value") {
    return fail("semantic-BIR regalloc should classify single-touch value storage into the single-point priority bucket");
  }
  if (local_slot_regalloc->assignment_readiness != "single_point_read_candidate") {
    return fail("semantic-BIR regalloc should expose a single-point read readiness cue for single-read value storage");
  }
  if (local_slot_regalloc->access_shape != "direct_read_only") {
    return fail("semantic-BIR regalloc should summarize single-read value storage with a direct-read-only access shape");
  }
  if (local_slot_regalloc->direct_read_count != 1 || local_slot_regalloc->direct_write_count != 0 ||
      local_slot_regalloc->addressed_access_count != 0 ||
      local_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct-access counts for local value-storage objects");
  }
  if (local_slot_regalloc->last_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish direct-read last-access cues");
  }
  if (local_slot_regalloc->first_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish direct-read first-access cues");
  }
  if (!local_slot_regalloc->has_access_window || local_slot_regalloc->first_access_instruction_index != 4 ||
      local_slot_regalloc->last_access_instruction_index != 4 ||
      local_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish instruction-order access windows for direct local-slot reads");
  }
  const auto* carry_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "carry.slot");
  if (carry_slot_regalloc == nullptr || carry_slot_regalloc->contract_kind != "value_storage" ||
      carry_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep call-crossing local slots as register candidates");
  }
  if (carry_slot_regalloc->priority_bucket != "call_spanning_value") {
    return fail("semantic-BIR regalloc should classify call-spanning register candidates into the call-spanning priority bucket");
  }
  if (carry_slot_regalloc->assignment_readiness != "call_spanning_read_write_candidate") {
    return fail("semantic-BIR regalloc should expose a call-spanning read/write readiness cue for call-crossing value storage");
  }
  if (carry_slot_regalloc->access_shape != "direct_read_write") {
    return fail("semantic-BIR regalloc should summarize read/write local slots with a direct-read-write access shape");
  }
  if (carry_slot_regalloc->direct_read_count != 1 || carry_slot_regalloc->direct_write_count != 1 ||
      carry_slot_regalloc->addressed_access_count != 0 ||
      carry_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct read/write counts for call-crossing local slots");
  }
  if (carry_slot_regalloc->last_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for call-crossing local slots");
  }
  if (carry_slot_regalloc->first_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for call-crossing local slots");
  }
  if (!carry_slot_regalloc->has_access_window || carry_slot_regalloc->first_access_instruction_index != 0 ||
      carry_slot_regalloc->last_access_instruction_index != 5 ||
      !carry_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish call-crossing instruction-order cues for value-storage objects");
  }
  const auto* window_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "window.slot");
  if (window_slot_regalloc == nullptr || window_slot_regalloc->contract_kind != "value_storage" ||
      window_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep non-call-spanning local slots as register candidates");
  }
  if (window_slot_regalloc->priority_bucket != "multi_point_value") {
    return fail("semantic-BIR regalloc should classify non-call-spanning multi-access value storage into the multi-point priority bucket");
  }
  if (window_slot_regalloc->assignment_readiness != "multi_point_read_write_candidate") {
    return fail("semantic-BIR regalloc should expose a multi-point read/write readiness cue for non-call-spanning value storage");
  }
  if (window_slot_regalloc->access_shape != "direct_read_write") {
    return fail("semantic-BIR regalloc should summarize non-call-spanning multi-point value storage with a direct-read-write access shape");
  }
  if (window_slot_regalloc->direct_read_count != 1 || window_slot_regalloc->direct_write_count != 1 ||
      window_slot_regalloc->addressed_access_count != 0 ||
      window_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct read/write counts for non-call-spanning local slots");
  }
  if (window_slot_regalloc->last_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for non-call-spanning local slots");
  }
  if (window_slot_regalloc->first_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for non-call-spanning local slots");
  }
  if (!window_slot_regalloc->has_access_window || window_slot_regalloc->first_access_instruction_index != 7 ||
      window_slot_regalloc->last_access_instruction_index != 8 ||
      window_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point value storage");
  }
  const auto* readonly_slot_regalloc =
      find_regalloc_object(*regalloc_function, "local_slot", "readonly.slot");
  if (readonly_slot_regalloc == nullptr || readonly_slot_regalloc->contract_kind != "value_storage" ||
      readonly_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep non-call-spanning read-only local slots as register candidates");
  }
  if (readonly_slot_regalloc->priority_bucket != "multi_point_value") {
    return fail("semantic-BIR regalloc should classify non-call-spanning multi-read value storage into the multi-point priority bucket");
  }
  if (readonly_slot_regalloc->assignment_readiness != "multi_point_read_candidate") {
    return fail("semantic-BIR regalloc should expose a multi-point read readiness cue for non-call-spanning read-only value storage");
  }
  if (readonly_slot_regalloc->access_shape != "direct_read_only") {
    return fail("semantic-BIR regalloc should summarize non-call-spanning multi-point read-only value storage with a direct-read-only access shape");
  }
  if (readonly_slot_regalloc->direct_read_count != 2 ||
      readonly_slot_regalloc->direct_write_count != 0 ||
      readonly_slot_regalloc->addressed_access_count != 0 ||
      readonly_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct-read counts for non-call-spanning read-only local slots");
  }
  if (readonly_slot_regalloc->last_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for non-call-spanning read-only local slots");
  }
  if (readonly_slot_regalloc->first_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for non-call-spanning read-only local slots");
  }
  if (!readonly_slot_regalloc->has_access_window ||
      readonly_slot_regalloc->first_access_instruction_index != 9 ||
      readonly_slot_regalloc->last_access_instruction_index != 10 ||
      readonly_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point read-only value storage");
  }
  const auto* callread_slot_regalloc =
      find_regalloc_object(*regalloc_function, "local_slot", "callread.slot");
  if (callread_slot_regalloc == nullptr || callread_slot_regalloc->contract_kind != "value_storage" ||
      callread_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep call-spanning read-only local slots as register candidates");
  }
  if (callread_slot_regalloc->priority_bucket != "call_spanning_value") {
    return fail("semantic-BIR regalloc should classify call-spanning read-only value storage into the call-spanning priority bucket");
  }
  if (callread_slot_regalloc->assignment_readiness != "call_spanning_read_candidate") {
    return fail("semantic-BIR regalloc should expose a call-spanning read readiness cue for call-crossing read-only value storage");
  }
  if (callread_slot_regalloc->access_shape != "direct_read_only") {
    return fail("semantic-BIR regalloc should summarize call-spanning read-only value storage with a direct-read-only access shape");
  }
  if (callread_slot_regalloc->direct_read_count != 2 ||
      callread_slot_regalloc->direct_write_count != 0 ||
      callread_slot_regalloc->addressed_access_count != 0 ||
      callread_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct-read counts for call-spanning read-only local slots");
  }
  if (callread_slot_regalloc->last_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for call-spanning read-only local slots");
  }
  if (callread_slot_regalloc->first_access_kind != "direct_read") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for call-spanning read-only local slots");
  }
  if (!callread_slot_regalloc->has_access_window ||
      callread_slot_regalloc->first_access_instruction_index != 2 ||
      callread_slot_regalloc->last_access_instruction_index != 13 ||
      !callread_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish call-crossing instruction-order cues for read-only value storage");
  }
  const auto* multiwrite_slot_regalloc =
      find_regalloc_object(*regalloc_function, "local_slot", "multiwrite.slot");
  if (multiwrite_slot_regalloc == nullptr || multiwrite_slot_regalloc->contract_kind != "value_storage" ||
      multiwrite_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep non-call-spanning multi-write local slots as register candidates");
  }
  if (multiwrite_slot_regalloc->priority_bucket != "multi_point_value") {
    return fail("semantic-BIR regalloc should classify non-call-spanning multi-write value storage into the multi-point priority bucket");
  }
  if (multiwrite_slot_regalloc->assignment_readiness != "multi_point_write_candidate") {
    return fail("semantic-BIR regalloc should expose a multi-point write readiness cue for non-call-spanning write-only value storage");
  }
  if (multiwrite_slot_regalloc->access_shape != "direct_write_only") {
    return fail("semantic-BIR regalloc should summarize non-call-spanning multi-point write-only value storage with a direct-write-only access shape");
  }
  if (multiwrite_slot_regalloc->direct_read_count != 0 ||
      multiwrite_slot_regalloc->direct_write_count != 2 ||
      multiwrite_slot_regalloc->addressed_access_count != 0 ||
      multiwrite_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct-write counts for non-call-spanning multi-write local slots");
  }
  if (multiwrite_slot_regalloc->last_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for non-call-spanning multi-write local slots");
  }
  if (multiwrite_slot_regalloc->first_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for non-call-spanning multi-write local slots");
  }
  if (!multiwrite_slot_regalloc->has_access_window ||
      multiwrite_slot_regalloc->first_access_instruction_index != 11 ||
      multiwrite_slot_regalloc->last_access_instruction_index != 12 ||
      multiwrite_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point write-only value storage");
  }
  const auto* writeonly_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "writeonly.slot");
  if (writeonly_regalloc == nullptr || writeonly_regalloc->contract_kind != "value_storage" ||
      writeonly_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep write-only local slots in the value-storage register-candidate contract");
  }
  if (writeonly_regalloc->last_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish direct-write last-access cues");
  }
  if (writeonly_regalloc->assignment_readiness != "single_point_write_candidate") {
    return fail("semantic-BIR regalloc should expose a single-point write readiness cue for write-only local slots");
  }
  if (writeonly_regalloc->access_shape != "direct_write_only") {
    return fail("semantic-BIR regalloc should summarize write-only local slots with a direct-write-only access shape");
  }
  if (writeonly_regalloc->first_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish direct-write first-access cues");
  }
  const auto* address_taken_regalloc =
      find_regalloc_object(*regalloc_function, "address_taken_local_slot", "addressed.slot");
  if (address_taken_regalloc == nullptr ||
      address_taken_regalloc->contract_kind != "address_exposed_storage" ||
      address_taken_regalloc->allocation_kind != "fixed_stack_storage") {
    return fail("semantic-BIR regalloc should keep address-exposed storage on fixed stack storage");
  }
  if (address_taken_regalloc->priority_bucket != "non_value_storage") {
    return fail("semantic-BIR regalloc should keep non-value prepared objects out of value-storage priority buckets");
  }
  if (address_taken_regalloc->assignment_readiness != "fixed_stack_only") {
    return fail("semantic-BIR regalloc should keep address-exposed storage in the fixed-stack readiness contract");
  }
  if (address_taken_regalloc->access_shape != "addressed_access_only") {
    return fail("semantic-BIR regalloc should summarize addressed storage with an addressed-access-only shape");
  }
  if (address_taken_regalloc->direct_read_count != 0 || address_taken_regalloc->direct_write_count != 0 ||
      address_taken_regalloc->addressed_access_count != 1 ||
      address_taken_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish addressed-access counts for address-exposed storage");
  }
  if (address_taken_regalloc->last_access_kind != "addressed_access") {
    return fail("semantic-BIR regalloc should publish addressed-access last-access cues");
  }
  if (address_taken_regalloc->first_access_kind != "addressed_access") {
    return fail("semantic-BIR regalloc should publish addressed-access first-access cues");
  }
  const auto* call_result_regalloc =
      find_regalloc_object(*regalloc_function, "call_result_sret", "%call.result");
  if (call_result_regalloc == nullptr ||
      call_result_regalloc->contract_kind != "address_exposed_storage" ||
      call_result_regalloc->allocation_kind != "fixed_stack_storage") {
    return fail("semantic-BIR regalloc should keep aggregate call-result storage on fixed stack storage");
  }
  if (call_result_regalloc->direct_read_count != 0 || call_result_regalloc->direct_write_count != 0 ||
      call_result_regalloc->addressed_access_count != 0 ||
      call_result_regalloc->call_arg_exposure_count != 1) {
    return fail("semantic-BIR regalloc should publish call-argument exposure counts for call-result storage");
  }
  if (call_result_regalloc->assignment_readiness != "fixed_stack_only") {
    return fail("semantic-BIR regalloc should keep call-result storage in the fixed-stack readiness contract");
  }
  if (call_result_regalloc->access_shape != "call_argument_exposure_only") {
    return fail("semantic-BIR regalloc should summarize call-result storage with a call-exposure-only shape");
  }
  if (call_result_regalloc->last_access_kind != "call_argument_exposure") {
    return fail("semantic-BIR regalloc should publish call-exposure last-access cues");
  }
  if (call_result_regalloc->first_access_kind != "call_argument_exposure") {
    return fail("semantic-BIR regalloc should publish call-exposure first-access cues");
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
