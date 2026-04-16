#pragma once

#include "../bir/bir.hpp"
#include "../target.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

using PreparedObjectId = std::size_t;
using PreparedValueId = std::size_t;
using PreparedFrameSlotId = std::size_t;

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
  PreparedObjectId object_id = 0;
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool address_exposed = false;
  bool requires_home_slot = false;
};

struct PreparedFrameSlot {
  PreparedFrameSlotId slot_id = 0;
  PreparedObjectId object_id = 0;
  std::string function_name;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

struct PreparedStackLayout {
  std::vector<PreparedStackObject> objects;
  std::vector<PreparedFrameSlot> frame_slots;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
};

enum class PreparedValueKind {
  StackObject,
  Parameter,
  CallResult,
  Phi,
  Temporary,
};

[[nodiscard]] constexpr std::string_view prepared_value_kind_name(PreparedValueKind kind) {
  switch (kind) {
    case PreparedValueKind::StackObject:
      return "stack_object";
    case PreparedValueKind::Parameter:
      return "parameter";
    case PreparedValueKind::CallResult:
      return "call_result";
    case PreparedValueKind::Phi:
      return "phi";
    case PreparedValueKind::Temporary:
      return "temporary";
  }
  return "unknown";
}

enum class PreparedUseKind {
  Read,
  Write,
  ReadWrite,
  Address,
  CallArgument,
  ReturnValue,
};

[[nodiscard]] constexpr std::string_view prepared_use_kind_name(PreparedUseKind kind) {
  switch (kind) {
    case PreparedUseKind::Read:
      return "read";
    case PreparedUseKind::Write:
      return "write";
    case PreparedUseKind::ReadWrite:
      return "read_write";
    case PreparedUseKind::Address:
      return "address";
    case PreparedUseKind::CallArgument:
      return "call_argument";
    case PreparedUseKind::ReturnValue:
      return "return_value";
  }
  return "unknown";
}

struct PreparedLivenessDefSite {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::string definition_kind;
};

struct PreparedLivenessUseSite {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t operand_index = 0;
  PreparedUseKind use_kind = PreparedUseKind::Read;
};

struct PreparedLivenessLiveSegment {
  std::size_t start_block_index = 0;
  std::size_t start_instruction_index = 0;
  std::size_t end_block_index = 0;
  std::size_t end_instruction_index = 0;
};

struct PreparedLivenessBlock {
  std::string block_name;
  std::size_t block_index = 0;
  std::size_t instruction_start_index = 0;
  std::size_t instruction_end_index = 0;
  std::vector<std::size_t> predecessor_block_indices;
  std::vector<std::size_t> successor_block_indices;
  std::vector<PreparedValueId> defs;
  std::vector<PreparedValueId> uses;
  std::vector<PreparedValueId> live_in;
  std::vector<PreparedValueId> live_out;
};

struct PreparedLivenessValue {
  PreparedValueId value_id = 0;
  std::optional<PreparedObjectId> stack_object_id;
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  bool address_taken = false;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::optional<PreparedLivenessDefSite> definition_site;
  std::vector<PreparedLivenessUseSite> use_sites;
  std::vector<PreparedLivenessLiveSegment> live_segments;
  std::vector<std::size_t> crossed_call_instruction_indices;
};

struct PreparedLivenessFunction {
  std::string function_name;
  std::size_t instruction_count = 0;
  std::size_t call_instruction_count = 0;
  std::vector<std::size_t> call_instruction_indices;
  std::vector<PreparedLivenessBlock> blocks;
  std::vector<PreparedLivenessValue> values;
};

struct PreparedLiveness {
  std::vector<PreparedLivenessFunction> functions;
};

enum class PreparedRegisterClass {
  None,
  General,
  Float,
  Vector,
  AggregateAddress,
};

[[nodiscard]] constexpr std::string_view prepared_register_class_name(
    PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::None:
      return "none";
    case PreparedRegisterClass::General:
      return "general";
    case PreparedRegisterClass::Float:
      return "float";
    case PreparedRegisterClass::Vector:
      return "vector";
    case PreparedRegisterClass::AggregateAddress:
      return "aggregate_address";
  }
  return "unknown";
}

enum class PreparedAllocationStatus {
  Unallocated,
  AssignedRegister,
  AssignedStackSlot,
  Split,
  Spilled,
};

[[nodiscard]] constexpr std::string_view prepared_allocation_status_name(
    PreparedAllocationStatus status) {
  switch (status) {
    case PreparedAllocationStatus::Unallocated:
      return "unallocated";
    case PreparedAllocationStatus::AssignedRegister:
      return "assigned_register";
    case PreparedAllocationStatus::AssignedStackSlot:
      return "assigned_stack_slot";
    case PreparedAllocationStatus::Split:
      return "split";
    case PreparedAllocationStatus::Spilled:
      return "spilled";
  }
  return "unknown";
}

enum class PreparedSpillReloadOpKind {
  Spill,
  Reload,
  Rematerialize,
};

[[nodiscard]] constexpr std::string_view prepared_spill_reload_op_kind_name(
    PreparedSpillReloadOpKind kind) {
  switch (kind) {
    case PreparedSpillReloadOpKind::Spill:
      return "spill";
    case PreparedSpillReloadOpKind::Reload:
      return "reload";
    case PreparedSpillReloadOpKind::Rematerialize:
      return "rematerialize";
  }
  return "unknown";
}

struct PreparedPhysicalRegisterAssignment {
  PreparedRegisterClass reg_class = PreparedRegisterClass::None;
  std::string register_name;
};

struct PreparedStackSlotAssignment {
  PreparedFrameSlotId slot_id = 0;
  std::size_t offset_bytes = 0;
};

struct PreparedAllocationConstraint {
  PreparedValueId value_id = 0;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  bool requires_register = false;
  bool requires_home_slot = false;
  bool cannot_cross_call = false;
  std::optional<std::string> fixed_register_name;
  std::vector<std::string> preferred_register_names;
  std::vector<std::string> forbidden_register_names;
};

struct PreparedInterferenceEdge {
  PreparedValueId lhs_value_id = 0;
  PreparedValueId rhs_value_id = 0;
  std::string reason;
};

struct PreparedMoveResolution {
  PreparedValueId from_value_id = 0;
  PreparedValueId to_value_id = 0;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::string reason;
};

struct PreparedSpillReloadOp {
  PreparedValueId value_id = 0;
  PreparedSpillReloadOpKind op_kind = PreparedSpillReloadOpKind::Spill;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

struct PreparedRegallocValue {
  PreparedValueId value_id = 0;
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  PreparedAllocationStatus allocation_status = PreparedAllocationStatus::Unallocated;
  bool spillable = true;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::size_t priority = 0;
  double spill_weight = 0.0;
  std::vector<PreparedLivenessLiveSegment> live_segments;
  std::optional<PreparedPhysicalRegisterAssignment> assigned_register;
  std::optional<PreparedStackSlotAssignment> assigned_stack_slot;
};

struct PreparedRegallocFunction {
  std::string function_name;
  std::vector<PreparedRegallocValue> values;
  std::vector<PreparedAllocationConstraint> constraints;
  std::vector<PreparedInterferenceEdge> interference;
  std::vector<PreparedMoveResolution> move_resolution;
  std::vector<PreparedSpillReloadOp> spill_reload_ops;
};

struct PreparedRegalloc {
  std::vector<PreparedRegallocFunction> functions;
};

enum class PrepareRoute {
  SemanticBirShared,
};

[[nodiscard]] constexpr std::string_view prepare_route_name(PrepareRoute route) {
  switch (route) {
    case PrepareRoute::SemanticBirShared:
      return "semantic_bir_shared";
  }
  return "unknown";
}

enum class PreparedBirInvariant {
  NoTargetFacingI1,
  NoPhiNodes,
};

[[nodiscard]] constexpr std::string_view prepared_bir_invariant_name(
    PreparedBirInvariant invariant) {
  switch (invariant) {
    case PreparedBirInvariant::NoTargetFacingI1:
      return "no_target_facing_i1";
    case PreparedBirInvariant::NoPhiNodes:
      return "no_phi_nodes";
  }
  return "unknown";
}

struct PreparedBirModule {
  c4c::backend::bir::Module module;
  Target target = Target::X86_64;
  PrepareRoute route = PrepareRoute::SemanticBirShared;
  std::vector<PreparedBirInvariant> invariants;
  PreparedStackLayout stack_layout;
  PreparedLiveness liveness;
  PreparedRegalloc regalloc;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

class BirPreAlloc {
 public:
  BirPreAlloc(const c4c::backend::bir::Module& module,
              Target target,
              const PrepareOptions& options = {})
      : options_(options),
        prepared_{
            .module = module,
            .target = target,
            .route = PrepareRoute::SemanticBirShared,
            .completed_phases = {},
            .notes = {},
        } {}
  explicit BirPreAlloc(PreparedBirModule prepared,
                       const PrepareOptions& options = {})
      : options_(options), prepared_(std::move(prepared)) {}

  void run_legalize();
  void run_stack_layout();
  void run_liveness();
  void run_regalloc();

  PreparedBirModule& prepared() { return prepared_; }
  const PreparedBirModule& prepared() const { return prepared_; }
  PreparedBirModule run();

 private:
  void note(std::string_view message);

  PrepareOptions options_;
  PreparedBirModule prepared_;
};

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    Target target,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare
