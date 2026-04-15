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

const prepare::PreparedLivenessFunction* find_liveness_function(
    const prepare::PreparedBirModule& module,
    std::string_view function_name) {
  for (const auto& function : module.liveness.functions) {
    if (function.function_name == function_name) {
      return &function;
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

const prepare::PreparedRegallocAllocationDecision* find_regalloc_allocation_decision(
    const prepare::PreparedRegallocFunction& function,
    std::string_view source_kind,
    std::string_view source_name) {
  for (const auto& decision : function.allocation_sequence) {
    if (decision.source_kind == source_kind && decision.source_name == source_name) {
      return &decision;
    }
  }
  return nullptr;
}

std::size_t regalloc_allocation_index(const prepare::PreparedRegallocFunction& function,
                                      std::string_view source_kind,
                                      std::string_view source_name) {
  for (std::size_t index = 0; index < function.allocation_sequence.size(); ++index) {
    const auto& decision = function.allocation_sequence[index];
    if (decision.source_kind == source_kind && decision.source_name == source_name) {
      return index;
    }
  }
  return function.allocation_sequence.size();
}

const prepare::PreparedRegallocReservationSummary* find_regalloc_reservation_summary(
    const prepare::PreparedRegallocFunction& function,
    std::string_view allocation_stage) {
  for (const auto& summary : function.reservation_summary) {
    if (summary.allocation_stage == allocation_stage) {
      return &summary;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocContentionSummary* find_regalloc_contention_summary(
    const prepare::PreparedRegallocFunction& function,
    std::string_view allocation_stage) {
  for (const auto& summary : function.contention_summary) {
    if (summary.allocation_stage == allocation_stage) {
      return &summary;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocBindingDecision* find_regalloc_binding_decision(
    const prepare::PreparedRegallocFunction& function,
    std::string_view source_kind,
    std::string_view source_name) {
  for (const auto& decision : function.binding_sequence) {
    if (decision.source_kind == source_kind && decision.source_name == source_name) {
      return &decision;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocBindingBatchSummary* find_regalloc_binding_batch(
    const prepare::PreparedRegallocFunction& function,
    std::string_view binding_batch_kind) {
  for (const auto& summary : function.binding_batches) {
    if (summary.binding_batch_kind == binding_batch_kind) {
      return &summary;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocDeferredBindingBatchSummary* find_regalloc_deferred_binding_batch(
    const prepare::PreparedRegallocFunction& function,
    std::string_view binding_batch_kind) {
  for (const auto& summary : function.deferred_binding_batches) {
    if (summary.binding_batch_kind == binding_batch_kind) {
      return &summary;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocBindingHandoffSummary* find_regalloc_binding_handoff_summary(
    const prepare::PreparedRegallocFunction& function,
    std::string_view binding_frontier_kind,
    std::string_view binding_batch_kind) {
  for (const auto& summary : function.binding_handoff_summary) {
    if (summary.binding_frontier_kind == binding_frontier_kind &&
        summary.binding_batch_kind == binding_batch_kind) {
      return &summary;
    }
  }
  return nullptr;
}

template <typename Summary>
std::size_t regalloc_candidate_count_sum(const std::vector<Summary>& summaries) {
  std::size_t total = 0;
  for (const auto& summary : summaries) {
    total += summary.candidate_count;
  }
  return total;
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
      .name = "callwrite.slot",
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
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "callwrite.slot",
      .value = bir::Value::immediate_i32(23),
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
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "callwrite.slot",
      .value = bir::Value::immediate_i32(29),
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
  if (!contains_note(prepared_bir.notes,
                     "liveness",
                     "direct read/write, addressed-access, and call-argument exposure counts")) {
    return fail("semantic-BIR prepare liveness note should mention prepared access/exposure counts");
  }
  if (!contains_note(prepared_bir.notes, "liveness", "instruction-order access windows with call-crossing cues")) {
    return fail("semantic-BIR prepare liveness note should mention access windows and call-crossing cues");
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
  if (!contains_note(prepared_bir.notes, "regalloc", "home-slot stability hints")) {
    return fail("semantic-BIR prepare regalloc note should mention home-slot stability hints");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "allocation-sequence decisions")) {
    return fail("semantic-BIR prepare regalloc note should mention allocation-sequence decisions");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "first-pass reservation decisions")) {
    return fail("semantic-BIR prepare regalloc note should mention first-pass reservation decisions");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "reservation pressure/collision summaries")) {
    return fail("semantic-BIR prepare regalloc note should mention reservation pressure/collision summaries");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "object-level allocation states")) {
    return fail("semantic-BIR prepare regalloc note should mention object-level allocation states");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "ready-vs-deferred binding frontier")) {
    return fail("semantic-BIR prepare regalloc note should mention the prepared binding frontier");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "binding batch/order artifact")) {
    return fail("semantic-BIR prepare regalloc note should mention the ready-only binding batch/order artifact");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "deferred binding batch artifacts")) {
    return fail("semantic-BIR prepare regalloc note should mention deferred binding batch artifacts");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "per-object binding batch/order")) {
    return fail("semantic-BIR prepare regalloc note should mention uniform per-object binding cues");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "downstream handoff summaries")) {
    return fail("semantic-BIR prepare regalloc note should mention downstream handoff summaries");
  }
  if (!contains_note(prepared_bir.notes, "regalloc", "compact access-shape summaries")) {
    return fail("semantic-BIR prepare regalloc note should mention compact access-shape summaries");
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
  if (prepared_bir.stack_layout.objects.size() != 20) {
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
  const auto* callwrite_slot = find_stack_object(prepared_bir, "local_slot", "callwrite.slot");
  if (callwrite_slot == nullptr || callwrite_slot->function_name != "id_pair" ||
      callwrite_slot->type != bir::TypeKind::I32 || callwrite_slot->size_bytes != 4 ||
      callwrite_slot->align_bytes != 4) {
    return fail("semantic-BIR stack-layout should preserve call-spanning write-only local slots as prepared frame objects");
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
  if (prepared_bir.liveness.functions.size() != 1) {
    return fail("semantic-BIR liveness should publish one per-function analysis grouping for this module");
  }
  const auto* liveness_function = find_liveness_function(prepared_bir, "id_pair");
  if (liveness_function == nullptr) {
    return fail("semantic-BIR liveness should publish function-level analysis metadata by semantic function name");
  }
  if (liveness_function->instruction_count != 16 || liveness_function->call_instruction_count != 2 ||
      liveness_function->object_count != prepared_bir.liveness.objects.size() ||
      liveness_function->call_instruction_indices.size() != 2 ||
      liveness_function->call_instruction_indices[0] != 1 ||
      liveness_function->call_instruction_indices[1] != 4) {
    return fail("semantic-BIR liveness should publish instruction-count and call-point metadata for prepared functions");
  }
  const auto* local_slot_liveness = find_liveness_object(prepared_bir, "local_slot", "flag.slot");
  if (local_slot_liveness == nullptr || local_slot_liveness->function_name != "id_pair" ||
      local_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should classify local stack objects as value storage");
  }
  if (local_slot_liveness->access_shape != "direct_read_only" ||
      local_slot_liveness->first_access_kind != "direct_read" ||
      local_slot_liveness->last_access_kind != "direct_read" ||
      local_slot_liveness->direct_read_count != 1 ||
      local_slot_liveness->direct_write_count != 0 ||
      local_slot_liveness->addressed_access_count != 0 ||
      local_slot_liveness->call_arg_exposure_count != 0 ||
      !local_slot_liveness->has_access_window ||
      local_slot_liveness->first_access_instruction_index != 5 ||
      local_slot_liveness->last_access_instruction_index != 5 ||
      local_slot_liveness->crosses_call_boundary) {
    return fail("semantic-BIR liveness should publish direct-read access-window metadata for local slots");
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
  if (carry_slot_liveness->access_shape != "direct_read_write" ||
      carry_slot_liveness->first_access_kind != "direct_write" ||
      carry_slot_liveness->last_access_kind != "direct_read" ||
      carry_slot_liveness->direct_read_count != 1 ||
      carry_slot_liveness->direct_write_count != 1 ||
      carry_slot_liveness->addressed_access_count != 0 ||
      carry_slot_liveness->call_arg_exposure_count != 0 ||
      !carry_slot_liveness->has_access_window ||
      carry_slot_liveness->first_access_instruction_index != 0 ||
      carry_slot_liveness->last_access_instruction_index != 6 ||
      !carry_slot_liveness->crosses_call_boundary) {
    return fail("semantic-BIR liveness should publish call-crossing access windows for read/write local slots");
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
  const auto* callwrite_slot_liveness =
      find_liveness_object(prepared_bir, "local_slot", "callwrite.slot");
  if (callwrite_slot_liveness == nullptr || callwrite_slot_liveness->function_name != "id_pair" ||
      callwrite_slot_liveness->contract_kind != "value_storage") {
    return fail("semantic-BIR liveness should keep call-spanning write-only local slots in the value-storage contract");
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
  if (address_taken_liveness->access_shape != "addressed_access_only" ||
      address_taken_liveness->first_access_kind != "addressed_access" ||
      address_taken_liveness->last_access_kind != "addressed_access" ||
      address_taken_liveness->direct_read_count != 0 ||
      address_taken_liveness->direct_write_count != 0 ||
      address_taken_liveness->addressed_access_count != 1 ||
      address_taken_liveness->call_arg_exposure_count != 0 ||
      !address_taken_liveness->has_access_window ||
      address_taken_liveness->first_access_instruction_index != 5 ||
      address_taken_liveness->last_access_instruction_index != 5 ||
      address_taken_liveness->crosses_call_boundary) {
    return fail("semantic-BIR liveness should publish addressed-access metadata for address-exposed stack objects");
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
  if (call_result_liveness->access_shape != "call_argument_exposure_only" ||
      call_result_liveness->first_access_kind != "call_argument_exposure" ||
      call_result_liveness->last_access_kind != "call_argument_exposure" ||
      call_result_liveness->direct_read_count != 0 ||
      call_result_liveness->direct_write_count != 0 ||
      call_result_liveness->addressed_access_count != 0 ||
      call_result_liveness->call_arg_exposure_count != 1 ||
      !call_result_liveness->has_access_window ||
      call_result_liveness->first_access_instruction_index != 1 ||
      call_result_liveness->last_access_instruction_index != 1 ||
      call_result_liveness->crosses_call_boundary) {
    return fail("semantic-BIR liveness should publish call-exposure metadata for aggregate call-result storage");
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
  if (regalloc_function->register_candidate_count != 15 ||
      regalloc_function->fixed_stack_storage_count != 5) {
    return fail("semantic-BIR regalloc should summarize register-candidate vs fixed-stack prepared objects");
  }
  if (regalloc_function->binding_sequence.size() != 6 ||
      regalloc_function->binding_batches.size() != 2 ||
      regalloc_candidate_count_sum(regalloc_function->binding_batches) !=
          regalloc_function->binding_sequence.size()) {
    return fail("semantic-BIR regalloc should publish ready-only binding batches and one binding order entry per ready candidate");
  }
  if (regalloc_function->deferred_binding_batches.size() != 2 ||
      regalloc_candidate_count_sum(regalloc_function->deferred_binding_batches) != 9 ||
      find_regalloc_deferred_binding_batch(*regalloc_function,
                                           "deferred_access_window_binding_batch") == nullptr ||
      find_regalloc_deferred_binding_batch(*regalloc_function,
                                           "deferred_coordination_binding_batch") == nullptr) {
    return fail("semantic-BIR regalloc should publish explicit deferred binding batches for the current binding-deferred frontier");
  }
  if (regalloc_function->allocation_sequence.size() != regalloc_function->register_candidate_count) {
    return fail("semantic-BIR regalloc should publish one allocation-sequence decision per register candidate");
  }
  const auto* scratch_slot_regalloc =
      find_regalloc_object(*regalloc_function, "lowering_scratch_slot", "scratch.slot");
  const auto* local_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "flag.slot");
  if (scratch_slot_regalloc == nullptr || scratch_slot_regalloc->contract_kind != "value_storage" ||
      scratch_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep lowering-created scratch slots in the value-storage register-candidate contract");
  }
  if (scratch_slot_regalloc->allocation_state_kind != "deferred_single_point_candidate" ||
      scratch_slot_regalloc->reservation_kind != "single_point_value_opportunity" ||
      scratch_slot_regalloc->reservation_scope != "unobserved_instruction_window" ||
      scratch_slot_regalloc->home_slot_mode != "home_slot_needs_future_analysis" ||
      scratch_slot_regalloc->sync_policy != "sync_policy_needs_future_analysis" ||
      scratch_slot_regalloc->follow_up_category != "batched_single_point_coordination" ||
      scratch_slot_regalloc->sync_coordination_category != "read_write_coordination" ||
      scratch_slot_regalloc->home_slot_category != "mixed_home_slot_modes" ||
      scratch_slot_regalloc->window_coordination_category != "mixed_sparse_windows" ||
      scratch_slot_regalloc->deferred_reason != "awaiting_access_window_observation") {
    return fail("semantic-BIR regalloc should publish a deferred opportunistic allocation state for unobserved single-point prepared objects");
  }
  if (scratch_slot_regalloc->binding_frontier_kind != "binding_deferred") {
    return fail("semantic-BIR regalloc should defer stable binding when prepared access-window facts are still missing");
  }
  if (scratch_slot_regalloc->binding_batch_kind != "deferred_access_window_binding_batch") {
    return fail(
        "semantic-BIR regalloc should keep deferred objects attached to their deferred binding batch without mirroring batch-owned prerequisites onto each object");
  }
  if (local_slot_regalloc == nullptr || local_slot_regalloc->contract_kind != "value_storage" ||
      local_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should treat value-storage objects as register candidates");
  }
  if (local_slot_regalloc->priority_bucket != "single_point_value") {
    return fail("semantic-BIR regalloc should classify single-touch value storage into the single-point priority bucket");
  }
  if (local_slot_regalloc->spill_restore_locality_hint != "single_instruction_reuse_window") {
    return fail("semantic-BIR regalloc should expose a single-instruction spill/restore locality hint for single-read value storage");
  }
  if (local_slot_regalloc->spill_sync_hint != "restore_only_single_use") {
    return fail("semantic-BIR regalloc should expose a restore-only spill-sync hint for single-read value storage");
  }
  if (local_slot_regalloc->home_slot_stability_hint != "single_use_read_home_slot") {
    return fail("semantic-BIR regalloc should expose a single-use-read home-slot stability hint for single-read value storage");
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
  if (!local_slot_regalloc->has_access_window || local_slot_regalloc->first_access_instruction_index != 5 ||
      local_slot_regalloc->last_access_instruction_index != 5 ||
      local_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish instruction-order access windows for direct local-slot reads");
  }
  if (local_slot_regalloc->allocation_state_kind != "opportunistic_single_point_candidate" ||
      local_slot_regalloc->reservation_kind != "single_read_cache_opportunity" ||
      local_slot_regalloc->reservation_scope != "single_instruction_window" ||
      local_slot_regalloc->home_slot_mode != "single_use_home_slot_ok" ||
      local_slot_regalloc->sync_policy != "restore_before_read" ||
      local_slot_regalloc->follow_up_category != "batched_single_point_coordination" ||
      local_slot_regalloc->sync_coordination_category != "read_write_coordination" ||
      local_slot_regalloc->home_slot_category != "mixed_home_slot_modes" ||
      local_slot_regalloc->window_coordination_category != "mixed_sparse_windows" ||
      local_slot_regalloc->deferred_reason != "not_deferred") {
    return fail("semantic-BIR regalloc should publish an opportunistic object-level allocation state for observed single-point reads");
  }
  if (local_slot_regalloc->binding_frontier_kind != "binding_deferred") {
    return fail("semantic-BIR regalloc should keep observed single-point candidates deferred when coordination remains batched");
  }
  if (local_slot_regalloc->binding_batch_kind != "deferred_coordination_binding_batch") {
    return fail(
        "semantic-BIR regalloc should keep coordination-deferred objects attached to their deferred binding batch without mirroring batch-owned prerequisites onto each object");
  }
  const auto* carry_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "carry.slot");
  if (carry_slot_regalloc == nullptr || carry_slot_regalloc->contract_kind != "value_storage" ||
      carry_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep call-crossing local slots as register candidates");
  }
  if (carry_slot_regalloc->priority_bucket != "call_spanning_value") {
    return fail("semantic-BIR regalloc should classify call-spanning register candidates into the call-spanning priority bucket");
  }
  if (carry_slot_regalloc->spill_restore_locality_hint != "call_split_reuse_window") {
    return fail("semantic-BIR regalloc should expose a call-split spill/restore locality hint for call-crossing value storage");
  }
  if (carry_slot_regalloc->spill_sync_hint != "bidirectional_sync_call_window") {
    return fail("semantic-BIR regalloc should expose a bidirectional call-window spill-sync hint for call-crossing value storage");
  }
  if (carry_slot_regalloc->home_slot_stability_hint != "call_preserved_read_write_home_slot") {
    return fail("semantic-BIR regalloc should expose a call-preserved read/write home-slot stability hint for call-crossing value storage");
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
      carry_slot_regalloc->last_access_instruction_index != 6 ||
      !carry_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish call-crossing instruction-order cues for value-storage objects");
  }
  if (carry_slot_regalloc->allocation_state_kind != "reserved_call_preserved_candidate" ||
      carry_slot_regalloc->reservation_kind != "call_preserved_value_reservation" ||
      carry_slot_regalloc->reservation_scope != "call_boundary_window" ||
      carry_slot_regalloc->home_slot_mode != "stable_home_slot_required" ||
      carry_slot_regalloc->sync_policy != "sync_on_read_write_boundaries" ||
      carry_slot_regalloc->follow_up_category != "call_boundary_preservation" ||
      carry_slot_regalloc->sync_coordination_category != "mixed_sync_coordination" ||
      carry_slot_regalloc->home_slot_category != "stable_home_slot_required" ||
      carry_slot_regalloc->window_coordination_category != "overlapping_call_boundary_windows" ||
      carry_slot_regalloc->deferred_reason != "not_deferred") {
    return fail("semantic-BIR regalloc should publish a call-preserved object-level allocation state for across-call candidates");
  }
  if (carry_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark across-call candidates as ready for stable prepared binding");
  }
  if (carry_slot_regalloc->binding_batch_kind != "call_boundary_binding_batch" ||
      carry_slot_regalloc->binding_order_index != 0) {
    return fail(
        "semantic-BIR regalloc should keep ready call-boundary objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
  }
  const auto* window_slot_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "window.slot");
  if (window_slot_regalloc == nullptr || window_slot_regalloc->contract_kind != "value_storage" ||
      window_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep non-call-spanning local slots as register candidates");
  }
  if (window_slot_regalloc->priority_bucket != "multi_point_value") {
    return fail("semantic-BIR regalloc should classify non-call-spanning multi-access value storage into the multi-point priority bucket");
  }
  if (window_slot_regalloc->spill_restore_locality_hint != "adjacent_instruction_reuse_window") {
    return fail("semantic-BIR regalloc should expose an adjacent-instruction spill/restore locality hint for non-call-spanning read/write value storage");
  }
  if (window_slot_regalloc->spill_sync_hint != "bidirectional_sync_local_window") {
    return fail("semantic-BIR regalloc should expose a bidirectional local-window spill-sync hint for non-call-spanning read/write value storage");
  }
  if (window_slot_regalloc->home_slot_stability_hint != "tight_read_write_home_slot") {
    return fail("semantic-BIR regalloc should expose a tight read/write home-slot stability hint for non-call-spanning read/write value storage");
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
  if (!window_slot_regalloc->has_access_window || window_slot_regalloc->first_access_instruction_index != 8 ||
      window_slot_regalloc->last_access_instruction_index != 9 ||
      window_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point value storage");
  }
  if (window_slot_regalloc->allocation_state_kind != "reserved_local_reuse_candidate" ||
      window_slot_regalloc->reservation_kind != "local_reuse_value_reservation" ||
      window_slot_regalloc->reservation_scope != "adjacent_instruction_window" ||
      window_slot_regalloc->home_slot_mode != "stable_home_slot_preferred" ||
      window_slot_regalloc->sync_policy != "sync_on_read_write_boundaries" ||
      window_slot_regalloc->follow_up_category != "sequenced_local_reuse_coordination" ||
      window_slot_regalloc->sync_coordination_category != "mixed_sync_coordination" ||
      window_slot_regalloc->home_slot_category != "stable_home_slot_preferred" ||
      window_slot_regalloc->window_coordination_category != "adjacent_local_windows" ||
      window_slot_regalloc->deferred_reason != "not_deferred") {
    return fail("semantic-BIR regalloc should publish a local-reuse object-level allocation state for nearby multi-point candidates");
  }
  if (window_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark sequenced local-reuse candidates as ready for stable prepared binding");
  }
  if (window_slot_regalloc->binding_batch_kind != "local_reuse_binding_batch" ||
      window_slot_regalloc->binding_order_index != 0) {
    return fail(
        "semantic-BIR regalloc should keep ready local-reuse objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
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
  if (readonly_slot_regalloc->spill_restore_locality_hint != "adjacent_instruction_reuse_window") {
    return fail("semantic-BIR regalloc should expose an adjacent-instruction spill/restore locality hint for non-call-spanning multi-read value storage");
  }
  if (readonly_slot_regalloc->spill_sync_hint != "restore_only_reuse_window") {
    return fail("semantic-BIR regalloc should expose a restore-only reuse-window spill-sync hint for non-call-spanning multi-read value storage");
  }
  if (readonly_slot_regalloc->home_slot_stability_hint != "adjacent_read_home_slot") {
    return fail("semantic-BIR regalloc should expose an adjacent-read home-slot stability hint for non-call-spanning multi-read value storage");
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
      readonly_slot_regalloc->first_access_instruction_index != 10 ||
      readonly_slot_regalloc->last_access_instruction_index != 11 ||
      readonly_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point read-only value storage");
  }
  if (readonly_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark non-call-spanning multi-read candidates as ready for stable prepared binding");
  }
  if (readonly_slot_regalloc->binding_batch_kind != "local_reuse_binding_batch" ||
      readonly_slot_regalloc->binding_order_index != 1) {
    return fail(
        "semantic-BIR regalloc should keep ready local-reuse objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
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
  if (callread_slot_regalloc->spill_sync_hint != "restore_only_call_window") {
    return fail("semantic-BIR regalloc should expose a restore-only call-window spill-sync hint for call-spanning read-only value storage");
  }
  if (callread_slot_regalloc->home_slot_stability_hint != "call_preserved_read_home_slot") {
    return fail("semantic-BIR regalloc should expose a call-preserved read home-slot stability hint for call-spanning read-only value storage");
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
      callread_slot_regalloc->last_access_instruction_index != 14 ||
      !callread_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish call-crossing instruction-order cues for read-only value storage");
  }
  if (callread_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark call-spanning read-only candidates as ready for stable prepared binding");
  }
  if (callread_slot_regalloc->binding_batch_kind != "call_boundary_binding_batch" ||
      callread_slot_regalloc->binding_order_index != 1) {
    return fail(
        "semantic-BIR regalloc should keep ready call-boundary objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
  }
  const auto* callwrite_slot_regalloc =
      find_regalloc_object(*regalloc_function, "local_slot", "callwrite.slot");
  if (callwrite_slot_regalloc == nullptr || callwrite_slot_regalloc->contract_kind != "value_storage" ||
      callwrite_slot_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep call-spanning write-only local slots as register candidates");
  }
  if (callwrite_slot_regalloc->priority_bucket != "call_spanning_value") {
    return fail("semantic-BIR regalloc should classify call-spanning write-only value storage into the call-spanning priority bucket");
  }
  if (callwrite_slot_regalloc->spill_restore_locality_hint != "call_split_reuse_window") {
    return fail("semantic-BIR regalloc should expose a call-split spill/restore locality hint for call-spanning write-only value storage");
  }
  if (callwrite_slot_regalloc->spill_sync_hint != "writeback_only_call_window") {
    return fail("semantic-BIR regalloc should expose a writeback-only call-window spill-sync hint for call-spanning write-only value storage");
  }
  if (callwrite_slot_regalloc->home_slot_stability_hint != "call_preserved_write_home_slot") {
    return fail("semantic-BIR regalloc should expose a call-preserved write home-slot stability hint for call-spanning write-only value storage");
  }
  if (callwrite_slot_regalloc->access_shape != "direct_write_only") {
    return fail("semantic-BIR regalloc should summarize call-spanning write-only value storage with a direct-write-only access shape");
  }
  if (callwrite_slot_regalloc->direct_read_count != 0 ||
      callwrite_slot_regalloc->direct_write_count != 2 ||
      callwrite_slot_regalloc->addressed_access_count != 0 ||
      callwrite_slot_regalloc->call_arg_exposure_count != 0) {
    return fail("semantic-BIR regalloc should publish direct-write counts for call-spanning write-only local slots");
  }
  if (callwrite_slot_regalloc->last_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the latest direct access kind for call-spanning write-only local slots");
  }
  if (callwrite_slot_regalloc->first_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish the opening direct access kind for call-spanning write-only local slots");
  }
  if (!callwrite_slot_regalloc->has_access_window ||
      callwrite_slot_regalloc->first_access_instruction_index != 3 ||
      callwrite_slot_regalloc->last_access_instruction_index != 15 ||
      !callwrite_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish call-crossing instruction-order cues for write-only value storage");
  }
  if (callwrite_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark call-spanning write-only candidates as ready for stable prepared binding");
  }
  if (callwrite_slot_regalloc->binding_batch_kind != "call_boundary_binding_batch" ||
      callwrite_slot_regalloc->binding_order_index != 2) {
    return fail(
        "semantic-BIR regalloc should keep ready call-boundary objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
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
  if (multiwrite_slot_regalloc->spill_restore_locality_hint != "adjacent_instruction_reuse_window") {
    return fail("semantic-BIR regalloc should expose an adjacent-instruction spill/restore locality hint for non-call-spanning multi-write value storage");
  }
  if (multiwrite_slot_regalloc->spill_sync_hint != "writeback_only_redefinition_window") {
    return fail("semantic-BIR regalloc should expose a repeated-writeback spill-sync hint for non-call-spanning multi-write value storage");
  }
  if (multiwrite_slot_regalloc->home_slot_stability_hint != "adjacent_write_home_slot") {
    return fail("semantic-BIR regalloc should expose an adjacent-write home-slot stability hint for non-call-spanning multi-write value storage");
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
      multiwrite_slot_regalloc->first_access_instruction_index != 12 ||
      multiwrite_slot_regalloc->last_access_instruction_index != 13 ||
      multiwrite_slot_regalloc->crosses_call_boundary) {
    return fail("semantic-BIR regalloc should publish non-call-spanning instruction-order cues for multi-point write-only value storage");
  }
  if (multiwrite_slot_regalloc->binding_frontier_kind != "binding_ready") {
    return fail("semantic-BIR regalloc should mark non-call-spanning multi-write candidates as ready for stable prepared binding");
  }
  if (multiwrite_slot_regalloc->binding_batch_kind != "local_reuse_binding_batch" ||
      multiwrite_slot_regalloc->binding_order_index != 2) {
    return fail(
        "semantic-BIR regalloc should keep ready local-reuse objects attached to their batch and sequence position without mirroring batch-owned summaries onto each object");
  }
  const auto* writeonly_regalloc = find_regalloc_object(*regalloc_function, "local_slot", "writeonly.slot");
  if (writeonly_regalloc == nullptr || writeonly_regalloc->contract_kind != "value_storage" ||
      writeonly_regalloc->allocation_kind != "register_candidate") {
    return fail("semantic-BIR regalloc should keep write-only local slots in the value-storage register-candidate contract");
  }
  if (writeonly_regalloc->spill_restore_locality_hint != "single_instruction_reuse_window") {
    return fail("semantic-BIR regalloc should expose a single-instruction spill/restore locality hint for single-point write-only local slots");
  }
  if (writeonly_regalloc->spill_sync_hint != "writeback_only_single_definition") {
    return fail("semantic-BIR regalloc should expose a single-definition writeback-only spill-sync hint for single-point write-only local slots");
  }
  if (writeonly_regalloc->home_slot_stability_hint != "single_definition_write_home_slot") {
    return fail("semantic-BIR regalloc should expose a single-definition-write home-slot stability hint for single-point write-only local slots");
  }
  if (writeonly_regalloc->last_access_kind != "direct_write") {
    return fail("semantic-BIR regalloc should publish direct-write last-access cues");
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
  if (address_taken_regalloc->spill_restore_locality_hint != "fixed_stack_address_anchor") {
    return fail("semantic-BIR regalloc should expose an address-anchored spill/restore locality hint for fixed-stack storage");
  }
  if (address_taken_regalloc->spill_sync_hint != "fixed_stack_memory_authoritative") {
    return fail("semantic-BIR regalloc should expose a memory-authoritative spill-sync hint for address-exposed fixed-stack storage");
  }
  if (address_taken_regalloc->home_slot_stability_hint != "memory_anchor_home_slot") {
    return fail("semantic-BIR regalloc should expose a memory-anchor home-slot stability hint for address-exposed fixed-stack storage");
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
  if (address_taken_regalloc->allocation_state_kind != "fixed_stack_authoritative" ||
      address_taken_regalloc->reservation_kind != "fixed_stack_storage" ||
      address_taken_regalloc->reservation_scope != "fixed_stack_memory_anchor" ||
      address_taken_regalloc->home_slot_mode != "stable_home_slot_required" ||
      address_taken_regalloc->sync_policy != "memory_authoritative" ||
      address_taken_regalloc->follow_up_category != "fixed_stack_authoritative" ||
      address_taken_regalloc->sync_coordination_category != "fixed_stack_authoritative" ||
      address_taken_regalloc->home_slot_category != "stable_home_slot_required" ||
      address_taken_regalloc->window_coordination_category != "fixed_stack_memory_anchor" ||
      address_taken_regalloc->deferred_reason != "not_applicable_fixed_stack") {
    return fail("semantic-BIR regalloc should publish a fixed-stack authoritative allocation state for address-exposed prepared objects");
  }
  if (address_taken_regalloc->binding_frontier_kind != "fixed_stack_authoritative") {
    return fail("semantic-BIR regalloc should keep fixed-stack storage outside the register-binding frontier");
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
  if (call_result_regalloc->spill_restore_locality_hint != "fixed_stack_call_boundary_anchor") {
    return fail("semantic-BIR regalloc should expose a call-boundary spill/restore locality hint for call-exposed fixed-stack storage");
  }
  if (call_result_regalloc->spill_sync_hint != "fixed_stack_call_boundary_authoritative") {
    return fail("semantic-BIR regalloc should expose a call-boundary-authoritative spill-sync hint for call-exposed fixed-stack storage");
  }
  if (call_result_regalloc->home_slot_stability_hint != "call_boundary_anchor_home_slot") {
    return fail("semantic-BIR regalloc should expose a call-boundary-anchor home-slot stability hint for call-exposed fixed-stack storage");
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
  if (find_regalloc_allocation_decision(*regalloc_function, "address_taken_local_slot", "addressed.slot") !=
      nullptr) {
    return fail("semantic-BIR regalloc allocation sequence should exclude fixed-stack address-exposed storage");
  }
  if (find_regalloc_allocation_decision(*regalloc_function, "call_result_sret", "%call.result") != nullptr) {
    return fail("semantic-BIR regalloc allocation sequence should exclude fixed-stack call-exposed storage");
  }
  const auto* carry_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "carry.slot");
  const auto* callread_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "callread.slot");
  const auto* callwrite_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "callwrite.slot");
  const auto* window_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "window.slot");
  const auto* readonly_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "readonly.slot");
  const auto* multiwrite_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "multiwrite.slot");
  const auto* local_slot_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "flag.slot");
  const auto* writeonly_sequence =
      find_regalloc_allocation_decision(*regalloc_function, "local_slot", "writeonly.slot");
  const auto* across_calls_summary =
      find_regalloc_reservation_summary(*regalloc_function, "stabilize_across_calls");
  const auto* local_reuse_summary =
      find_regalloc_reservation_summary(*regalloc_function, "stabilize_local_reuse");
  const auto* opportunistic_summary =
      find_regalloc_reservation_summary(*regalloc_function, "opportunistic_single_point");
  const auto* across_calls_contention =
      find_regalloc_contention_summary(*regalloc_function, "stabilize_across_calls");
  const auto* local_reuse_contention =
      find_regalloc_contention_summary(*regalloc_function, "stabilize_local_reuse");
  const auto* opportunistic_contention =
      find_regalloc_contention_summary(*regalloc_function, "opportunistic_single_point");
  const auto* carry_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "carry.slot");
  const auto* callread_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "callread.slot");
  const auto* callwrite_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "callwrite.slot");
  const auto* window_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "window.slot");
  const auto* readonly_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "readonly.slot");
  const auto* multiwrite_binding =
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "multiwrite.slot");
  const auto* call_boundary_binding_batch =
      find_regalloc_binding_batch(*regalloc_function, "call_boundary_binding_batch");
  const auto* local_reuse_binding_batch =
      find_regalloc_binding_batch(*regalloc_function, "local_reuse_binding_batch");
  const auto* deferred_access_window_binding_batch =
      find_regalloc_deferred_binding_batch(*regalloc_function,
                                           "deferred_access_window_binding_batch");
  const auto* deferred_coordination_binding_batch =
      find_regalloc_deferred_binding_batch(*regalloc_function,
                                           "deferred_coordination_binding_batch");
  const auto* call_boundary_handoff_summary =
      find_regalloc_binding_handoff_summary(*regalloc_function,
                                            "binding_ready",
                                            "call_boundary_binding_batch");
  const auto* local_reuse_handoff_summary =
      find_regalloc_binding_handoff_summary(*regalloc_function,
                                            "binding_ready",
                                            "local_reuse_binding_batch");
  const auto* deferred_access_window_handoff_summary =
      find_regalloc_binding_handoff_summary(*regalloc_function,
                                            "binding_deferred",
                                            "deferred_access_window_binding_batch");
  const auto* deferred_coordination_handoff_summary =
      find_regalloc_binding_handoff_summary(*regalloc_function,
                                            "binding_deferred",
                                            "deferred_coordination_binding_batch");
  if (carry_sequence == nullptr || callread_sequence == nullptr || callwrite_sequence == nullptr ||
      window_sequence == nullptr || readonly_sequence == nullptr || multiwrite_sequence == nullptr ||
      local_slot_sequence == nullptr || writeonly_sequence == nullptr) {
    return fail("semantic-BIR regalloc allocation sequence should cover nearby register-candidate shapes");
  }
  if (across_calls_summary == nullptr || local_reuse_summary == nullptr ||
      opportunistic_summary == nullptr) {
    return fail("semantic-BIR regalloc should publish per-function reservation summaries for the active allocation stages");
  }
  if (across_calls_contention == nullptr || local_reuse_contention == nullptr ||
      opportunistic_contention == nullptr) {
    return fail("semantic-BIR regalloc should publish explicit per-function contention follow-up categories for the active allocation stages");
  }
  if (carry_binding == nullptr || callread_binding == nullptr || callwrite_binding == nullptr ||
      window_binding == nullptr || readonly_binding == nullptr || multiwrite_binding == nullptr ||
      call_boundary_binding_batch == nullptr || local_reuse_binding_batch == nullptr ||
      deferred_access_window_binding_batch == nullptr ||
      deferred_coordination_binding_batch == nullptr) {
    return fail("semantic-BIR regalloc should publish binding decisions and batch summaries for the current binding-ready frontier");
  }
  if (regalloc_function->binding_handoff_summary.size() !=
          regalloc_function->binding_batches.size() +
              regalloc_function->deferred_binding_batches.size() ||
      call_boundary_handoff_summary == nullptr || local_reuse_handoff_summary == nullptr ||
      deferred_access_window_handoff_summary == nullptr ||
      deferred_coordination_handoff_summary == nullptr) {
    return fail(
        "semantic-BIR regalloc should publish unified downstream handoff summaries across ready and deferred binding frontiers");
  }
  if (find_regalloc_binding_decision(*regalloc_function, "local_slot", "flag.slot") != nullptr ||
      find_regalloc_binding_decision(*regalloc_function, "local_slot", "writeonly.slot") != nullptr) {
    return fail("semantic-BIR regalloc binding sequence should exclude deferred single-point candidates");
  }
  if (carry_sequence->allocation_stage != "stabilize_across_calls" ||
      callread_sequence->allocation_stage != "stabilize_across_calls" ||
      callwrite_sequence->allocation_stage != "stabilize_across_calls") {
    return fail("semantic-BIR regalloc should stage call-spanning candidates for across-call stabilization");
  }
  if (window_sequence->allocation_stage != "stabilize_local_reuse" ||
      readonly_sequence->allocation_stage != "stabilize_local_reuse" ||
      multiwrite_sequence->allocation_stage != "stabilize_local_reuse") {
    return fail("semantic-BIR regalloc should stage local multi-point candidates for local-reuse stabilization");
  }
  if (local_slot_sequence->allocation_stage != "opportunistic_single_point" ||
      writeonly_sequence->allocation_stage != "opportunistic_single_point") {
    return fail("semantic-BIR regalloc should stage single-point candidates as opportunistic assignments");
  }
  if (carry_sequence->reservation_kind != "call_preserved_value_reservation" ||
      carry_sequence->reservation_scope != "call_boundary_window" ||
      carry_sequence->home_slot_mode != "stable_home_slot_required" ||
      carry_sequence->sync_policy != "sync_on_read_write_boundaries") {
    return fail("semantic-BIR regalloc should turn call-spanning read/write staging into a call-preserved first-pass reservation");
  }
  if (callread_sequence->reservation_kind != "call_preserved_read_cache" ||
      callread_sequence->reservation_scope != "call_boundary_window" ||
      callread_sequence->home_slot_mode != "stable_home_slot_required" ||
      callread_sequence->sync_policy != "restore_before_read") {
    return fail("semantic-BIR regalloc should turn call-spanning read-only staging into a call-preserved read-cache reservation");
  }
  if (callwrite_sequence->reservation_kind != "call_preserved_writeback_buffer" ||
      callwrite_sequence->reservation_scope != "call_boundary_window" ||
      callwrite_sequence->home_slot_mode != "stable_home_slot_required" ||
      callwrite_sequence->sync_policy != "writeback_after_write") {
    return fail("semantic-BIR regalloc should turn call-spanning write-only staging into a call-preserved writeback reservation");
  }
  if (window_sequence->reservation_kind != "local_reuse_value_reservation" ||
      window_sequence->reservation_scope != "adjacent_instruction_window" ||
      window_sequence->home_slot_mode != "stable_home_slot_preferred" ||
      window_sequence->sync_policy != "sync_on_read_write_boundaries") {
    return fail("semantic-BIR regalloc should turn local read/write reuse staging into a local reuse reservation");
  }
  if (readonly_sequence->reservation_kind != "local_read_cache_reservation" ||
      readonly_sequence->reservation_scope != "adjacent_instruction_window" ||
      readonly_sequence->home_slot_mode != "stable_home_slot_preferred" ||
      readonly_sequence->sync_policy != "restore_before_read") {
    return fail("semantic-BIR regalloc should turn local read-only reuse staging into a local read-cache reservation");
  }
  if (multiwrite_sequence->reservation_kind != "local_write_buffer_reservation" ||
      multiwrite_sequence->reservation_scope != "adjacent_instruction_window" ||
      multiwrite_sequence->home_slot_mode != "stable_home_slot_preferred" ||
      multiwrite_sequence->sync_policy != "writeback_after_write") {
    return fail("semantic-BIR regalloc should turn local write-only reuse staging into a local write-buffer reservation");
  }
  if (local_slot_sequence->reservation_kind != "single_read_cache_opportunity" ||
      local_slot_sequence->reservation_scope != "single_instruction_window" ||
      local_slot_sequence->home_slot_mode != "single_use_home_slot_ok" ||
      local_slot_sequence->sync_policy != "restore_before_read") {
    return fail("semantic-BIR regalloc should turn single-point read staging into a single-read opportunistic reservation");
  }
  if (writeonly_sequence->reservation_kind != "single_write_buffer_opportunity" ||
      writeonly_sequence->reservation_scope != "single_instruction_window" ||
      writeonly_sequence->home_slot_mode != "single_use_home_slot_ok" ||
      writeonly_sequence->sync_policy != "writeback_after_write") {
    return fail("semantic-BIR regalloc should turn single-point write staging into a single-write opportunistic reservation");
  }
  const std::size_t carry_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "carry.slot");
  const std::size_t callread_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "callread.slot");
  const std::size_t callwrite_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "callwrite.slot");
  const std::size_t window_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "window.slot");
  const std::size_t readonly_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "readonly.slot");
  const std::size_t multiwrite_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "multiwrite.slot");
  const std::size_t local_slot_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "flag.slot");
  const std::size_t writeonly_sequence_index =
      regalloc_allocation_index(*regalloc_function, "local_slot", "writeonly.slot");
  if (!(carry_sequence_index < callread_sequence_index &&
        callread_sequence_index < callwrite_sequence_index &&
        callwrite_sequence_index < window_sequence_index &&
        window_sequence_index < readonly_sequence_index &&
        readonly_sequence_index < multiwrite_sequence_index &&
        multiwrite_sequence_index < local_slot_sequence_index &&
        local_slot_sequence_index < writeonly_sequence_index)) {
    return fail(
        "semantic-BIR regalloc should order register candidates from call-stable reuse through local reuse to opportunistic single-point cases");
  }
  if (across_calls_summary->candidate_count != 3 ||
      across_calls_summary->overlapping_window_count != 3 ||
      across_calls_summary->call_boundary_window_count != 3 ||
      across_calls_summary->stable_home_slot_required_count != 3 ||
      across_calls_summary->restore_before_read_count != 1 ||
      across_calls_summary->writeback_after_write_count != 1 ||
      across_calls_summary->sync_on_read_write_boundaries_count != 1 ||
      across_calls_summary->pressure_signal != "call_boundary_stable_home_pressure" ||
      across_calls_summary->collision_signal != "mixed_sync_window_collision_signal") {
    return fail("semantic-BIR regalloc should summarize call-spanning reservation pressure and mixed sync-policy collisions");
  }
  if (local_reuse_summary->candidate_count != 3 ||
      local_reuse_summary->overlapping_window_count != 0 ||
      local_reuse_summary->adjacent_instruction_window_count != 3 ||
      local_reuse_summary->stable_home_slot_preferred_count != 3 ||
      local_reuse_summary->restore_before_read_count != 1 ||
      local_reuse_summary->writeback_after_write_count != 1 ||
      local_reuse_summary->sync_on_read_write_boundaries_count != 1 ||
      local_reuse_summary->pressure_signal != "sequenced_local_reuse_pressure" ||
      local_reuse_summary->collision_signal != "sequenced_local_reuse_no_collision") {
    return fail("semantic-BIR regalloc should summarize nearby local-reuse pressure without inventing collisions for disjoint windows");
  }
  if (opportunistic_summary->candidate_count != 9 ||
      opportunistic_summary->overlapping_window_count != 0 ||
      opportunistic_summary->unobserved_window_count != 7 ||
      opportunistic_summary->single_instruction_window_count != 2 ||
      opportunistic_summary->single_use_home_slot_ok_count != 2 ||
      opportunistic_summary->restore_before_read_count != 1 ||
      opportunistic_summary->writeback_after_write_count != 1 ||
      opportunistic_summary->pressure_signal != "batched_single_point_pressure" ||
      opportunistic_summary->collision_signal != "single_instruction_collision_watch") {
    return fail("semantic-BIR regalloc should summarize opportunistic single-point pressure and same-shape collision watch signals");
  }
  if (across_calls_contention->follow_up_category != "call_boundary_preservation" ||
      across_calls_contention->sync_coordination_category != "mixed_sync_coordination" ||
      across_calls_contention->home_slot_category != "stable_home_slot_required" ||
      across_calls_contention->window_coordination_category !=
          "overlapping_call_boundary_windows") {
    return fail("semantic-BIR regalloc should reduce call-spanning reservation summaries into explicit contention follow-up categories");
  }
  if (local_reuse_contention->follow_up_category != "sequenced_local_reuse_coordination" ||
      local_reuse_contention->sync_coordination_category != "mixed_sync_coordination" ||
      local_reuse_contention->home_slot_category != "stable_home_slot_preferred" ||
      local_reuse_contention->window_coordination_category != "adjacent_local_windows") {
    return fail("semantic-BIR regalloc should reduce local-reuse reservation summaries into explicit contention follow-up categories");
  }
  if (opportunistic_contention->follow_up_category != "batched_single_point_coordination" ||
      opportunistic_contention->sync_coordination_category != "read_write_coordination" ||
      opportunistic_contention->home_slot_category != "mixed_home_slot_modes" ||
      opportunistic_contention->window_coordination_category != "mixed_sparse_windows") {
    return fail("semantic-BIR regalloc should reduce opportunistic reservation summaries into explicit sparse-window follow-up categories");
  }
  if (carry_binding->binding_batch_kind != "call_boundary_binding_batch" ||
      callread_binding->binding_batch_kind != "call_boundary_binding_batch" ||
      callwrite_binding->binding_batch_kind != "call_boundary_binding_batch" ||
      carry_binding->binding_order_index != 0 || callread_binding->binding_order_index != 1 ||
      callwrite_binding->binding_order_index != 2) {
    return fail(
        "semantic-BIR regalloc should keep binding-ready across-call entries focused on sequencing identity and order");
  }
  if (window_binding->binding_batch_kind != "local_reuse_binding_batch" ||
      readonly_binding->binding_batch_kind != "local_reuse_binding_batch" ||
      multiwrite_binding->binding_batch_kind != "local_reuse_binding_batch" ||
      window_binding->binding_order_index != 0 || readonly_binding->binding_order_index != 1 ||
      multiwrite_binding->binding_order_index != 2) {
    return fail(
        "semantic-BIR regalloc should keep binding-ready local-reuse entries focused on sequencing identity and order");
  }
  if (call_boundary_binding_batch->allocation_stage != "stabilize_across_calls" ||
      call_boundary_binding_batch->follow_up_category != "call_boundary_preservation" ||
      call_boundary_binding_batch->ordering_policy != "preserve_allocation_sequence" ||
      call_boundary_binding_batch->home_slot_prerequisite_category !=
          "stable_home_slot_required" ||
      call_boundary_binding_batch->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_satisfied" ||
      call_boundary_binding_batch->sync_handoff_prerequisite_category !=
          "mixed_sync_coordination" ||
      call_boundary_binding_batch->sync_handoff_state != "prepare_sync_handoff_ready" ||
      call_boundary_binding_batch->candidate_count != 3) {
    return fail(
        "semantic-BIR regalloc should summarize call-boundary batch prerequisites and ready sync/home-slot handoff from the existing reservation/contention frontier");
  }
  if (local_reuse_binding_batch->allocation_stage != "stabilize_local_reuse" ||
      local_reuse_binding_batch->follow_up_category != "sequenced_local_reuse_coordination" ||
      local_reuse_binding_batch->ordering_policy != "preserve_allocation_sequence" ||
      local_reuse_binding_batch->home_slot_prerequisite_category !=
          "stable_home_slot_preferred" ||
      local_reuse_binding_batch->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_satisfied" ||
      local_reuse_binding_batch->sync_handoff_prerequisite_category !=
          "mixed_sync_coordination" ||
      local_reuse_binding_batch->sync_handoff_state != "prepare_sync_handoff_ready" ||
      local_reuse_binding_batch->candidate_count != 3) {
    return fail(
        "semantic-BIR regalloc should summarize local-reuse batch prerequisites and ready sync/home-slot handoff from the existing reservation/contention frontier");
  }
  if (deferred_access_window_binding_batch->allocation_stage != "opportunistic_single_point" ||
      deferred_access_window_binding_batch->deferred_reason !=
          "awaiting_access_window_observation" ||
      deferred_access_window_binding_batch->follow_up_category !=
          "batched_single_point_coordination" ||
      deferred_access_window_binding_batch->ordering_policy !=
          "defer_until_access_window_observed" ||
      deferred_access_window_binding_batch->access_window_prerequisite_category !=
          "unobserved_instruction_window" ||
      deferred_access_window_binding_batch->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_deferred" ||
      deferred_access_window_binding_batch->home_slot_prerequisite_category !=
          "home_slot_needs_future_analysis" ||
      deferred_access_window_binding_batch->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_deferred" ||
      deferred_access_window_binding_batch->sync_handoff_prerequisite_category !=
          "sync_policy_needs_future_analysis" ||
      deferred_access_window_binding_batch->sync_handoff_state !=
          "prepare_sync_handoff_deferred" ||
      deferred_access_window_binding_batch->candidate_count != 7) {
    return fail(
        "semantic-BIR regalloc should group deferred single-point candidates waiting on access-window observation into an explicit deferred binding batch");
  }
  if (deferred_coordination_binding_batch->allocation_stage != "opportunistic_single_point" ||
      deferred_coordination_binding_batch->deferred_reason !=
          "batched_single_point_coordination" ||
      deferred_coordination_binding_batch->follow_up_category !=
          "batched_single_point_coordination" ||
      deferred_coordination_binding_batch->ordering_policy != "defer_until_frontier_ready" ||
      deferred_coordination_binding_batch->access_window_prerequisite_category !=
          "mixed_sparse_windows" ||
      deferred_coordination_binding_batch->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_satisfied" ||
      deferred_coordination_binding_batch->home_slot_prerequisite_category !=
          "mixed_home_slot_modes" ||
      deferred_coordination_binding_batch->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_deferred" ||
      deferred_coordination_binding_batch->sync_handoff_prerequisite_category !=
          "read_write_coordination" ||
      deferred_coordination_binding_batch->sync_handoff_state !=
          "prepare_sync_handoff_ready" ||
      deferred_coordination_binding_batch->candidate_count != 2) {
    return fail(
        "semantic-BIR regalloc should group deferred single-point candidates waiting on coordination into an explicit deferred binding batch");
  }
  if (call_boundary_handoff_summary->binding_frontier_reason != "call_boundary_preservation" ||
      call_boundary_handoff_summary->allocation_stage != "stabilize_across_calls" ||
      call_boundary_handoff_summary->follow_up_category != "call_boundary_preservation" ||
      call_boundary_handoff_summary->ordering_policy != "preserve_allocation_sequence" ||
      call_boundary_handoff_summary->access_window_prerequisite_category !=
          "overlapping_call_boundary_windows" ||
      call_boundary_handoff_summary->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_satisfied" ||
      call_boundary_handoff_summary->home_slot_prerequisite_category !=
          "stable_home_slot_required" ||
      call_boundary_handoff_summary->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_satisfied" ||
      call_boundary_handoff_summary->sync_handoff_prerequisite_category !=
          "mixed_sync_coordination" ||
      call_boundary_handoff_summary->sync_handoff_state !=
          "prepare_sync_handoff_ready" ||
      call_boundary_handoff_summary->candidate_count != 3) {
    return fail(
        "semantic-BIR regalloc should collapse ready call-boundary bindings into one downstream handoff summary");
  }
  if (local_reuse_handoff_summary->binding_frontier_reason !=
          "sequenced_local_reuse_coordination" ||
      local_reuse_handoff_summary->allocation_stage != "stabilize_local_reuse" ||
      local_reuse_handoff_summary->follow_up_category !=
          "sequenced_local_reuse_coordination" ||
      local_reuse_handoff_summary->ordering_policy != "preserve_allocation_sequence" ||
      local_reuse_handoff_summary->access_window_prerequisite_category !=
          "adjacent_local_windows" ||
      local_reuse_handoff_summary->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_satisfied" ||
      local_reuse_handoff_summary->home_slot_prerequisite_category !=
          "stable_home_slot_preferred" ||
      local_reuse_handoff_summary->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_satisfied" ||
      local_reuse_handoff_summary->sync_handoff_prerequisite_category !=
          "mixed_sync_coordination" ||
      local_reuse_handoff_summary->sync_handoff_state != "prepare_sync_handoff_ready" ||
      local_reuse_handoff_summary->candidate_count != 3) {
    return fail(
        "semantic-BIR regalloc should collapse ready local-reuse bindings into one downstream handoff summary");
  }
  if (deferred_access_window_handoff_summary->binding_frontier_reason !=
          "awaiting_access_window_observation" ||
      deferred_access_window_handoff_summary->allocation_stage !=
          "opportunistic_single_point" ||
      deferred_access_window_handoff_summary->follow_up_category !=
          "batched_single_point_coordination" ||
      deferred_access_window_handoff_summary->ordering_policy !=
          "defer_until_access_window_observed" ||
      deferred_access_window_handoff_summary->access_window_prerequisite_category !=
          "unobserved_instruction_window" ||
      deferred_access_window_handoff_summary->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_deferred" ||
      deferred_access_window_handoff_summary->home_slot_prerequisite_category !=
          "home_slot_needs_future_analysis" ||
      deferred_access_window_handoff_summary->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_deferred" ||
      deferred_access_window_handoff_summary->sync_handoff_prerequisite_category !=
          "sync_policy_needs_future_analysis" ||
      deferred_access_window_handoff_summary->sync_handoff_state !=
          "prepare_sync_handoff_deferred" ||
      deferred_access_window_handoff_summary->candidate_count != 7) {
    return fail(
        "semantic-BIR regalloc should collapse access-window-deferred bindings into one downstream handoff summary");
  }
  if (deferred_coordination_handoff_summary->binding_frontier_reason !=
          "batched_single_point_coordination" ||
      deferred_coordination_handoff_summary->allocation_stage !=
          "opportunistic_single_point" ||
      deferred_coordination_handoff_summary->follow_up_category !=
          "batched_single_point_coordination" ||
      deferred_coordination_handoff_summary->ordering_policy != "defer_until_frontier_ready" ||
      deferred_coordination_handoff_summary->access_window_prerequisite_category !=
          "mixed_sparse_windows" ||
      deferred_coordination_handoff_summary->access_window_prerequisite_state !=
          "prepare_access_window_prerequisite_satisfied" ||
      deferred_coordination_handoff_summary->home_slot_prerequisite_category !=
          "mixed_home_slot_modes" ||
      deferred_coordination_handoff_summary->home_slot_prerequisite_state !=
          "prepare_home_slot_prerequisite_deferred" ||
      deferred_coordination_handoff_summary->sync_handoff_prerequisite_category !=
          "read_write_coordination" ||
      deferred_coordination_handoff_summary->sync_handoff_state !=
          "prepare_sync_handoff_ready" ||
      deferred_coordination_handoff_summary->candidate_count != 2) {
    return fail(
        "semantic-BIR regalloc should collapse coordination-deferred bindings into one downstream handoff summary");
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
