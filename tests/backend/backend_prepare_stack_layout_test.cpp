#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/target.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

using c4c::backend::Target;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedStackObject* find_stack_object(const prepare::PreparedBirModule& prepared,
                                                      std::string_view source_name) {
  for (const auto& object : prepared.stack_layout.objects) {
    if (object.source_name == source_name) {
      return &object;
    }
  }
  return nullptr;
}

const prepare::PreparedFrameSlot* find_frame_slot(const prepare::PreparedBirModule& prepared,
                                                  prepare::PreparedObjectId object_id) {
  for (const auto& slot : prepared.stack_layout.frame_slots) {
    if (slot.object_id == object_id) {
      return &slot;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule prepare_stack_layout_module() {
  bir::Module module;

  bir::Function function;
  function.name = "stack_layout_dead_slot_elision";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.live",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "lv.dead",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "lv.live",
      .value = bir::Value::immediate_i32(41),
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .slot_name = "lv.live",
      .align_bytes = 4,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loaded"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  return std::move(planner.prepared());
}

int check_dead_local_slot_elision(const prepare::PreparedBirModule& prepared) {
  const auto* live_object = find_stack_object(prepared, "lv.live");
  const auto* dead_object = find_stack_object(prepared, "lv.dead");
  if (live_object == nullptr || dead_object == nullptr) {
    return fail("expected stack-layout objects for both live and dead local slots");
  }

  if (!live_object->requires_home_slot) {
    return fail("expected the live local slot to keep a home-slot requirement");
  }
  if (dead_object->requires_home_slot) {
    return fail("expected the dead local slot to drop its home-slot requirement");
  }

  const auto* live_slot = find_frame_slot(prepared, live_object->object_id);
  const auto* dead_slot = find_frame_slot(prepared, dead_object->object_id);
  if (live_slot == nullptr) {
    return fail("expected the live local slot to receive a frame slot");
  }
  if (dead_slot != nullptr) {
    return fail("expected the dead local slot to be elided from frame-slot assignment");
  }

  if (prepared.stack_layout.frame_slots.size() != 1) {
    return fail("expected only one frame slot after dead local-slot elision");
  }
  if (prepared.stack_layout.frame_size_bytes != 4 ||
      prepared.stack_layout.frame_alignment_bytes != 4) {
    return fail("expected frame metrics to reflect only the live local slot");
  }

  return 0;
}

}  // namespace

int main() {
  const auto prepared = prepare_stack_layout_module();
  if (const int rc = check_dead_local_slot_elision(prepared); rc != 0) {
    return rc;
  }
  return 0;
}
