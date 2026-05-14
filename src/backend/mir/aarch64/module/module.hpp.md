# `src/backend/mir/aarch64/module/module.hpp` Extraction

## Purpose And Current Responsibility

This header is the non-helper directory index surface for `backend/mir/aarch64/module`.
It exposes the public AArch64 MIR module build product and the single entry point that
turns a prepared BIR module into that product.

The file is almost entirely a schema boundary. It does not implement lowering, but it
defines the records that downstream AArch64 MIR/codegen clients inspect after lowering:
allocation provenance, frame layout, calls, moves, spill/reload plans, parallel-copy
execution, globals, strings, blocks, functions, and the top-level module.

Treat these structures as evidence of the current contract shape, not as the desired
rebuild design. The current surface mixes durable module facts, debug/provenance fields,
compatibility views, source pointers, backend allocation authority, and partially lowered
machine-node state in one exported namespace.

## Important APIs And Contract Surfaces

The only function API is the module builder:

```cpp
struct BuildResult {
  std::optional<Module> module;
  std::optional<abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const prepare::PreparedBirModule& prepared);
```

The top-level product retains the prepared-module pointer, target profile, and all
materialized data/function records:

```cpp
struct Module {
  const prepare::PreparedBirModule* prepared;
  c4c::TargetProfile target_profile;
  std::vector<GlobalDataRecord> globals;
  std::vector<StringDataRecord> strings;
  std::vector<DataRelocationNeedRecord> relocation_needs;
  std::vector<FunctionRecord> functions;
};
```

`FunctionRecord` is the largest contract surface. It aggregates source identity, prepared
control-flow linkage, frame data, branch/call/move/spill/parallel-copy side records,
operand/register views, the canonical MIR function, a compatibility flat machine-node
view, and block metadata.

```cpp
struct FunctionRecord {
  c4c::FunctionNameId function_name;
  std::string_view label;
  const bir::Function* source_function;
  const prepare::PreparedControlFlowFunction* control_flow;
  FrameRecord frame;
  std::vector<BranchRecord> branches;
  std::vector<CallRecord> calls;
  std::vector<MoveRecord> moves;
  std::vector<AbiBindingRecord> abi_bindings;
  std::vector<SpillReloadRecord> spill_reloads;
  std::vector<ParallelCopyRecord> parallel_copies;
  std::vector<OperandRecord> operands;
  std::vector<TargetRegisterRecord> target_registers;
  mir::Function<mir::MachineNode<codegen::InstructionRecord>> mir;
  std::vector<codegen::InstructionRecord> machine_nodes;
  std::vector<BlockRecord> blocks;
};
```

Core record families:

- Allocation/register contract: `TargetRegisterRecord`, `OperandRecord`, and enums for
  register reference kind, allocation snapshot, allocation location, and allocation
  authority.
- Frame contract: `FrameRecord`, `FrameSlotRecord`, `CalleeSaveRecord`, and
  `DynamicStackRecord`.
- Control-flow and instruction-adjacent contract: `BlockRecord`, `BranchRecord`,
  `MoveRecord`, `AbiBindingRecord`, `SpillReloadRecord`, `ParallelCopyRecord`, and its
  move/step children.
- ABI call contract: `CallRecord`, `CallArgumentRecord`, `CallResultRecord`, and
  `CallPreservedValueRecord`.
- Data contract: `GlobalDataRecord`, `StringDataRecord`, `DataRelocationNeedRecord`, and
  visibility/relocation enums.

## Dependency Direction And Hidden Inputs

Public dependency direction is from this module surface to:

- `../abi/abi.hpp` for register references, allocation pools, and handoff errors.
- `../codegen/records.hpp` for machine instruction records.
- `../../mir.hpp` for the target MIR function and machine-node container.
- `backend::prepare` records via `PreparedBirModule` and many prepared-plan references.
- `backend::bir` value/type/global/function/block shapes.
- shared ID namespaces such as `ValueNameId`, `FunctionNameId`, `LinkNameId`,
  `BlockLabelId`, `SlotNameId`, `TextId`, and `TargetProfile`.

The hidden inputs are the lifetime and semantic stability of the prepared objects. Many
records keep raw `const prepare::*` or `const bir::*` source pointers. The contract
therefore assumes the built `Module` is inspected while the originating prepared/BIR
storage remains alive. It also assumes `std::string_view` labels outlive the module
record, usually through upstream interning or prepared-module storage.

Several records carry duplicated facts with different authority labels. For example an
operand can expose value home, regalloc assignment, spill authority, storage plan,
physical register text, stack offsets, frame/spill slot IDs, immediates, symbols, and
pointer-base information at once. The consumer must infer which fields are authoritative
from enum tags rather than from type separation.

## Responsibility Buckets

### Directory Index Surface

This header is the single non-helper `.hpp` for the directory and should remain the
formal index surface in a phoenix rebuild. It should describe what the module builder
exports, but not become the implementation home for every internal lowering concern.

### Prepared Evidence Projection

Most records are a projection of prepared lowering facts into target-facing evidence:
prepared value IDs, value homes, storage encoding, frame objects, control-flow blocks,
call plans, move bundles, spill/reload plans, and dynamic stack plans.

### Target ABI Projection

AArch64 ABI-specific fields appear throughout the records: register references,
contiguous register widths, occupied physical registers, callee-save save indices,
destination/source register banks, and stack offsets. These are important contract data,
but they are currently repeated across call, move, spill, operand, and register records.

### Machine MIR Product

The actual target MIR product is held inside `FunctionRecord::mir` as a typed MIR
function over `codegen::InstructionRecord`. This is the durable product that rebuilt
clients should prefer.

### Compatibility And Inspection Views

`FunctionRecord::machine_nodes` is explicitly a compatibility flat view while clients
migrate to `mir.blocks[].instructions`. Many source pointer and label fields also serve
inspection/debug use cases more than semantic lowering.

## Fast Paths, Compatibility Paths, And Overfit Pressure

- Core lowering: `Module`, `FunctionRecord::mir`, frame/call/move/spill/parallel-copy
  records are legitimate outputs if they reflect general prepared-plan semantics.
- Optional fast path: branch records carry `can_fuse_with_branch`, predicate, compare
  type, lhs, and rhs, allowing branch fusion when a prepared condition can be consumed
  directly.
- Legacy compatibility: `FunctionRecord::machine_nodes` duplicates machine
  instructions as a flat vector and should be treated as a migration bridge, not a
  rebuilt primary surface.
- Legacy compatibility: pervasive `source_*` raw pointers preserve traceability to BIR
  and prepared plans. They are useful for diagnostics but make lifetime and ownership
  implicit.
- Overfit pressure: authority is often represented by optional fields plus enum tags in
  broad records instead of smaller typed variants. This invites narrow consumers to read
  whichever field happens to be populated for a testcase.
- Overfit pressure: repeated register/stack destination shapes across call arguments,
  call results, ABI bindings, moves, preserved values, and spill/reloads can encourage
  local shape patches instead of one semantic storage-location model.
- Overfit pressure: `offset_is_prepared_snapshot` and similar snapshot booleans appear
  in several records, exposing transitional offset authority rather than a single frame
  layout contract.

## Rebuild Ownership Guidance

This file should own the directory index contract: the public module build entry point,
the exported module/function/data record vocabulary, and the narrow set of durable types
that downstream clients are expected to consume.

It should not own lowering algorithms, register-allocation policy, frame planning,
parallel-copy resolution, call ABI planning, spill/reload selection, or compatibility
storage for clients that can consume canonical MIR blocks directly. In a rebuild, keep
this header as the directory index surface, but push repeated storage-location shapes,
debug provenance, and compatibility views behind clearer typed seams.
