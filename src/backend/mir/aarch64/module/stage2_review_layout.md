# AArch64 Module Stage 2 Review Layout

## Scope

This document reconstructs the current AArch64 module subsystem shape from the
accepted Stage 1 evidence. It describes the legacy emitter as evidence for
later layout decisions, not as the replacement architecture and not as a set of
helper boundaries to preserve.

This Step 2 artifact intentionally stops before the replacement architecture,
parent-224 judgment, Stage 3 draft map, and Stage 2 handoff. Those belong to
later Stage 2 steps.

## Current Entry Points

The current public entry point is:

```cpp
BuildResult build(const prepare::PreparedBirModule& prepared);
```

`build` resolves the target profile from prepared state, validates the AArch64
prepared-module handoff, builds data and function records, and returns either a
`Module` or an `abi::HandoffError`. The implementation then fans out through
internal `build_*_record(s)` helpers. Those helpers are an implementation
ordering device: they should not be treated as durable replacement seams.

The current implementation driver for functions is
`prepared.control_flow.functions`. Source BIR functions and blocks are searched
afterward to enrich records, recover labels, attach source pointers, and create
limited machine-node evidence.

## Prepared BIR Inputs

`prepare::PreparedBirModule` is the main input authority. The current subsystem
reads prepared target profile data, prepared control-flow functions, frame
plans, call plans, value homes, storage plans, regalloc assignments,
spill/reload plans, dynamic stack plans, parallel-copy bundles, globals,
strings, and relocation needs.

The implementation also consults original BIR functions, blocks, values,
globals, string constants, terminators, and selected binary instructions. In
the current shape, BIR is not the driver; it is used to recover identity,
preserve provenance, and opportunistically synthesize a narrow subset of
machine-node records.

Several current lookups compensate for prepared data being split across
independent tables. Function labels, block labels, value homes, stack objects,
frame slots, destination slots, and regalloc values can be recovered by
combining structured IDs with fallback string spellings.

## Prepared Authority Records

The current module records merge multiple prepared authorities into exported
inspection views:

- value homes, regalloc assignments, and storage-plan values are merged into
  `OperandRecord`
- frame plans, stack layout slots, callee saves, and dynamic stack operations
  are merged into `FrameRecord`
- call plans are merged with ABI argument/result locations, clobbers,
  preserved values, memory returns, and indirect callee metadata
- value-location moves, ABI bindings, regalloc move resolution, spill/reload
  records, and parallel-copy records are collected beside each other
- globals, strings, initializer symbols, and relocation needs are projected
  into module-level data records

Authority is often represented by broad records plus enum tags, optional
fields, and snapshot booleans. That makes the current product useful as
evidence, but it leaves consumers responsible for deciding which populated
field is authoritative.

## Codegen Records

The current subsystem only partially lowers to codegen records. It stores
`codegen::InstructionRecord` inside the canonical
`FunctionRecord::mir` typed MIR function and duplicates selected instructions
in `FunctionRecord::machine_nodes` as a flat compatibility view.

Current machine-node sources are narrow:

- spill/reload records can become provisional spill/reload instruction records
- selected returned `add` and `sub` scalar binary instructions can become
  scalar ALU records when storage and locations line up
- return lowering can produce return machine nodes, including immediate return
  operands or reuse of a scalar ALU result register

This mixed record-plus-node shape is the important fact. The legacy subsystem
does not yet provide general prepared-BIR-to-MIR instruction lowering; it
mostly assembles prepared evidence and synthesizes a few instruction records.

## Public Assembly/API Exposure

`module.hpp` is the directory index surface and exposes `BuildResult`,
`Module`, `FunctionRecord`, and the large record vocabulary used by downstream
AArch64 MIR/codegen and API clients. `Module` retains a pointer to the
originating prepared module, target profile, globals, strings, relocation
needs, and function records.

The public assembly relationship is indirect. The module records and
`FunctionRecord::mir` are the target-side evidence later assembly/codegen
consumers inspect. The canonical machine product is the typed MIR function;
the flat `machine_nodes` vector is a legacy compatibility view beside MIR
blocks. Current public exposure therefore mixes durable MIR output, record
inspection data, compatibility fields, debug provenance, and source/prepared
pointers in one namespace.

## Hidden Cross-Helper State

The current implementation relies on state and assumptions that cross helper
boundaries:

- the returned `Module` stores raw pointers and string views into prepared and
  BIR storage, so caller-owned lifetime is implicit
- register conversion accepts structured prepared placements but can fall back
  to legacy register-name strings and reparsing
- source function and block recovery can fall back through link names, labels,
  prepared-name lookup, and spellings
- stack offsets and storage locations are treated as authoritative snapshots
  when prepared data provides them
- value homes, regalloc assignments, storage plans, moves, ABI bindings,
  spill/reload data, and parallel copies are reconciled by collection order
  and field tags rather than by one typed authority model
- some machine nodes are provisional record evidence rather than complete
  target instruction lowering

These hidden dependencies are the current subsystem shape. They are not a
replacement decomposition.

## Special-Case Classification

### Core Lowering

- AArch64 prepared-module handoff validation through the ABI layer.
- Structured register/storage placement conversion when prepared data carries
  authoritative facts.
- Projection of prepared frame, call, move, spill/reload, parallel-copy,
  operand, target-register, global, string, and relocation facts.
- `FunctionRecord::mir` as the canonical typed MIR function product.

### Optional Fast Path

- Branch fusion metadata such as predicate, compare type, operands, and
  `can_fuse_with_branch`.
- Returned scalar `add` and `sub` recognition when the returned value storage
  and locations agree.
- Immediate return operands when the prepared return path already carries an
  immediate.
- Reusing a scalar ALU result register for return when the current narrow path
  proves it is the return value.

### Legacy Compatibility

- Legacy register-name parsing when structured placement is absent or
  insufficient.
- String-based source function, block, and label recovery.
- Raw source/prepared pointers and `std::string_view` labels retained for
  diagnostics and inspection.
- `FunctionRecord::machine_nodes` as a flat migration view beside canonical
  MIR blocks.
- Snapshot booleans and repeated stack/register destination fields that expose
  transitional authority from older lowering paths.

### Overfit To Reject

- Extending the returned-`add`/`sub` path one named operation at a time instead
  of designing general prepared-BIR-to-MIR instruction lowering.
- Adding consumers that choose whichever optional authority field satisfies a
  narrow testcase instead of using a typed storage-location contract.
- Forcing structured replacement paths through legacy register strings or
  spelling-based identity recovery.
- Treating the flat `machine_nodes` compatibility vector as the primary
  rebuilt output instead of canonical MIR block/instruction structure.
- Preserving the current catch-all record assembler under renamed helper
  boundaries.

## Step 2 Decision Support

The current subsystem is a broad adapter from prepared BIR evidence to an
exported AArch64 module record pile, with limited MIR machine-node synthesis
inside that pile. Its entry point and public product are stable enough to
review, but its helper clusters mix lookup, validation, authority projection,
compatibility recovery, data records, and partial instruction lowering.

Later layout decisions should therefore use this document to understand which
facts must survive, while rejecting the current helper grouping as replacement
design.
