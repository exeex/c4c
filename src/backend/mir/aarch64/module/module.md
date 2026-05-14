# AArch64 Module Directory Index

## Scope

This directory currently contains the AArch64 MIR module boundary. The public
surface is `module.hpp`, and the legacy implementation evidence is
`module.cpp`. Stage 1 treats both source files as extraction inputs only: the
markdown artifacts below are the review surface for later phoenix design work.

Per-file evidence:

- `module.cpp.md`: implementation evidence for building an AArch64 module from
  `prepare::PreparedBirModule`, including prepared-state lookup, record
  assembly, limited MIR machine-node construction, data records, relocation
  needs, and compatibility paths.
- `module.hpp.md`: public surface evidence for the exported build entry point,
  `BuildResult`, `Module`, `FunctionRecord`, data records, frame/call/move
  records, target register and operand records, and the canonical MIR function
  product.

## Current Responsibilities

The module emitter validates the prepared AArch64 handoff, projects prepared
BIR state into target-facing records, preserves source and prepared pointers for
inspection, and returns a `Module` containing globals, strings, relocation
needs, and function records. Each `FunctionRecord` aggregates frame, block,
branch, call, move, ABI binding, spill/reload, parallel-copy, operand, target
register, canonical MIR, compatibility machine-node, and block metadata.

The implementation is currently a broad adapter rather than a narrow lowering
owner. It stitches together prepared control-flow, frame, call, allocation,
storage-plan, spill/reload, data, and source-recovery facts; normalizes AArch64
register references; and opportunistically creates a small set of
`codegen::InstructionRecord` machine nodes. The public header exports this
mixed record vocabulary as the module inspection contract.

## Relationship To Adjacent Inputs And Outputs

- Public AArch64 assembly: the module records are the target-side evidence that
  later AArch64 assembly/codegen consumers inspect. `FunctionRecord::mir` is
  the durable typed MIR product; `FunctionRecord::machine_nodes` is a legacy
  flat compatibility view for clients not yet using MIR blocks directly.
- `api/api.hpp`: this module sits behind the backend API path as the AArch64
  target module product. Its public build result must remain a stable handoff
  surface, while replacement internals should avoid leaking temporary
  compatibility details into broader API contracts.
- Codegen records: the current module surface stores
  `codegen::InstructionRecord` in both canonical MIR machine nodes and the flat
  compatibility vector. The legacy implementation also synthesizes limited
  spill/reload, scalar ALU return, and return instruction records.
- Prepared BIR inputs: `prepare::PreparedBirModule` is the authority for target
  profile, prepared control-flow functions, frame plans, call plans, storage
  plans, value homes, regalloc assignments, spill/reload plans, parallel-copy
  bundles, globals, strings, and relocation needs. Many module records keep
  raw pointers or `string_view` references into prepared/BIR storage, so the
  built module assumes that upstream storage remains alive.

## Evidence Classification

core lowering:

- Handoff validation through the AArch64 ABI layer before module construction.
- Projection of prepared frame, call, move, spill/reload, parallel-copy,
  operand, target-register, global, string, and relocation facts into module
  records.
- `FunctionRecord::mir` as the canonical typed MIR function product.
- Structured register and storage placement conversion when prepared data
  provides authoritative facts.

optional fast path:

- Branch records expose fusion-related condition fields for consumers that can
  fold a prepared condition into a branch.
- Return scalar ALU recognition can create provisional `add`/`sub` machine
  nodes when return value storage and locations agree.
- Immediate return operands can be materialized directly when the prepared
  return path already carries an immediate.

legacy compatibility:

- Legacy register-name parsing when structured register placement is absent or
  insufficient.
- String-based fallback recovery for function labels, block labels, and source
  identity.
- Raw source/prepared pointers and `string_view` labels used for diagnostics
  and inspection.
- `FunctionRecord::machine_nodes` as a flat view beside canonical MIR blocks.
- Snapshot booleans and repeated stack/register destination shapes that expose
  transitional authority from older lowering paths.

overfit to reject:

- Extending the narrow returned-`add`/`sub` scalar ALU path with more
  named-operation shortcuts instead of designing general prepared-BIR-to-MIR
  lowering.
- Adding new consumers that pick whichever optional authority field happens to
  satisfy a testcase rather than using a typed storage-location contract.
- Forcing rebuilt structured paths through legacy register strings or
  spelling-based identity recovery.
- Treating compatibility flat machine nodes as the primary rebuild product
  instead of the canonical MIR block/instruction structure.

## Phoenix Boundary

The rebuilt module should keep `module.hpp` as the directory-level public
surface and preserve the stable module build handoff. Stage 2 design should use
this index plus `module.cpp.md` and `module.hpp.md` as evidence, not reopen the
legacy source as the main reference. The rebuild route should lower prepared
BIR inputs directly into MIR machine nodes through explicit contracts, while
retaining only compatibility views that are required for existing clients.
