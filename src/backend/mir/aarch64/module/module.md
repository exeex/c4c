# AArch64 Module Replacement Draft Index

## Stage 3 Scope

This directory is drafting the replacement AArch64 MIR module boundary. The
drafted boundary lowers `prepare::PreparedBirModule` directly into hierarchical
typed MIR with AArch64-owned machine instructions, operands, registers, memory
forms, labels, symbols, and immediates. The legacy `module.cpp` and existing
Stage 1 extraction notes remain evidence only; they are not the replacement
architecture and must not be preserved under renamed helper seams.

Stage 3 writes markdown replacement drafts only. It does not edit real `.cpp`
or `.hpp` files, build wiring, tests, expectations, or the compiled legacy
`src/backend/mir/aarch64/module/module.cpp`.

## Boundary Contract

Input authority starts at `prepare::PreparedBirModule`. The module dispatch
draft validates the target profile and AArch64 prepared handoff, then delegates
by dependency direction:

`PreparedBirModule -> module dispatch -> function traversal -> operand
resolution -> instruction, branch/control, and call lowering -> canonical MIR
carrier -> public assembly bridge -> compatibility projection`.

Canonical output is a typed MIR carrier organized by module, function, block,
and instruction. `FunctionRecord::machine_nodes`, broad inspection records, raw
prepared/source pointers, string labels, and legacy register spelling recovery
are compatibility projections only. They are derived after canonical lowering
and cannot decide semantic lowering.

## Canonical Vocabulary

The replacement drafts use these names as the shared vocabulary. The exact C++
spelling may be refined during implementation, but each later draft must keep
the same responsibilities.

- `BuildResult`: the public result for `build(prepared)`, containing either a
  completed `Module` or an `abi::HandoffError`.
- `Module`: the public AArch64 module product, containing the target profile,
  module data records, canonical MIR functions, and compatibility projections.
- `MachineModule`: the durable module-level MIR carrier after AArch64 lowering.
- `MachineFunction`: the durable per-function MIR carrier, ordered by prepared
  function traversal.
- `MachineBlock`: the durable block carrier with label identity, predecessor
  and successor metadata, and ordered machine instructions/control nodes.
- `MachineInstruction`: the AArch64 target-owned instruction node stored inside
  canonical MIR blocks.
- `MachineOperand`: the AArch64 target-owned operand variant for registers,
  immediates, memory references, labels, symbols, and relocation-aware forms.
- `MachineRegister`: the structured target register representation derived
  from prepared allocation and ABI facts, never from strings when structured
  authority is available.
- `Provenance`: the `MachineOrigin`-equivalent lightweight origin metadata
  using stable IDs, indices, labels, durable source locations, and reason tags
  for the prepared authority that produced a fact.
- `CompatibilityProjection`: migration-only records derived from canonical MIR
  and typed lowering facts for clients that still inspect legacy public shapes.

## Responsibility Seams

### Module Dispatch

Owns the public `build` entry point, target-profile resolution, AArch64
prepared-handoff validation, module-level data and relocation orchestration,
function traversal dispatch, and final `BuildResult` construction.

Must not own operation-specific lowering, register spelling recovery, call ABI
expansion internals, branch fusion internals, scalar ALU rules, spill/reload
rules, or return lowering rules.

### Function Traversal

Owns prepared function and block order, per-function lowering context
construction, creation of `MachineFunction` and `MachineBlock` carriers, and
optional source/debug provenance attachment.

Must not treat public flat `machine_nodes` as the primary output and must not
recover identity from source spellings except through compatibility metadata.

### Operand Resolution

Owns the single typed authority model for prepared value locations, converting
prepared homes, storage plans, regalloc assignments, frame slots, spill slots,
immediates, symbols, labels, and target registers into `MachineOperand` and
`MachineRegister` values.

Must not choose operation-specific instruction forms, branch sequencing, call
sequencing, final assembly punctuation, or string-based register authority
outside a fail-closed compatibility adapter.

### Instruction Lowering

Owns semantic non-control lowering families: scalar ALU, memory, moves,
spill/reload, parallel-copy steps, and return-value materialization.

Must not expand named testcase shortcuts, use source spellings as semantic
input, assemble public record piles, or depend on compatibility projections.

### Branch And Control Lowering

Owns terminators, conditional and unconditional branches, compare/condition
handling, return control flow, successor metadata, and valid branch-fusion
decisions.

Must not use public branch records as instruction authority or require
AArch64-specific traversal from the shared printer.

### Call Lowering

Owns prepared call-plan lowering: argument/result locations, call-adjacent
moves, preserved values, clobbers, indirect callees, memory returns, and
call-site ABI bindings.

Must not use broad public call records as semantic input or leak printer syntax
into call sequencing.

### Public Assembly Bridge

Owns the bridge from canonical MIR functions and module data to the shared
`mir_printer`. The shared printer owns traversal, ordering, labels, sections,
spacing mechanics, and file emission structure. AArch64 owns target rendering
for instructions, operands, registers, memory references, labels, symbols, and
immediates.

Target render APIs must use ordinary C++ names and must not introduce an API
named `__repr__`.

### Compatibility Projection

Owns migration-only projections for `FunctionRecord::machine_nodes`, broad
inspection records, raw source/prepared provenance pointers, label views, and
legacy register-string diagnostics. It derives these records after canonical
lowering and records removal conditions for clients that migrate to MIR blocks
and typed operands.

Must not participate in semantic lowering decisions.

## Mandatory Stage 3 Artifact Map

Directory index draft:

- `src/backend/mir/aarch64/module/module.md`: this replacement directory index
  and route contract.

Mandatory header draft:

- `src/backend/mir/aarch64/module/module.hpp.md`: the single non-helper public
  header contract for `BuildResult`, `Module`, canonical MIR carrier concepts,
  provenance, compatibility projections, and target-owned printable surfaces.

Mandatory implementation drafts:

- `src/backend/mir/aarch64/module/module.cpp.md`: module dispatch, target
  profile resolution, AArch64 prepared-handoff validation, top-level data and
  relocation orchestration, `BuildResult` construction, and public product
  assembly from completed canonical MIR functions.
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`: prepared
  function/block traversal, per-function lowering context construction, MIR
  function/block creation, and optional source/debug provenance attachment.
- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`: typed prepared
  value-location authority, target register/operand conversion, storage
  precedence normalization, immediates, labels, symbols, memory forms, and
  fail-closed compatibility register fallback.
- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`: non-control
  prepared operation lowering into AArch64 MIR machine nodes, including scalar
  ALU, memory, moves, spill/reload, parallel-copy steps, and return-value
  materialization as semantic families.
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`: prepared
  terminator lowering, conditional and unconditional branch nodes, return
  control flow, compare/condition handling, block successor metadata, and valid
  branch-fusion decisions.
- `src/backend/mir/aarch64/module/call_lowering.cpp.md`: prepared call-plan
  lowering, argument/result locations, call-adjacent moves, preserved values,
  clobbers, indirect callees, memory returns, and call-site ABI bindings.
- `src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md`: bridge from
  canonical MIR functions and module data to the shared `mir_printer`
  traversal, with AArch64 owning opcode, operand, register, memory, immediate,
  label, and symbol rendering.
- `src/backend/mir/aarch64/module/compatibility_projection.cpp.md`:
  migration-only projections for `FunctionRecord::machine_nodes`, broad
  inspection records, raw source/prepared provenance, label views, and legacy
  register-string fallback diagnostics derived after canonical lowering.

Stage 3 must also produce one review artifact for the complete replacement
draft set before Stage 4 implementation conversion.

## Header Policy

`module.hpp` remains the single non-helper public index header for this
directory. Stage 3 must not draft component-level public headers for module
dispatch, function traversal, operand resolution, instruction lowering,
branch/control lowering, call lowering, public assembly bridging, or
compatibility projection.

`helper.hpp` is the only allowed header exception. It may be drafted only if a
later packet explicitly proves that private declarations must be shared across
multiple implementation files, that the helper surface is not public API, and
that the helper contract cannot live in one implementation draft without
creating two-way coupling. No `helper.hpp.md` is part of the mandatory Stage 3
artifact map.

## Drift Rejection Rules

Reject any Stage 3 draft that:

- treats `FunctionRecord::machine_nodes` or another flat vector as the
  canonical machine product
- uses broad public inspection records, cached display strings, spelling
  recovery, or raw prepared/source object retention as semantic lowering
  authority
- routes common printer traversal or assembly file structure through
  AArch64-specific code
- preserves the legacy catch-all module emitter under renamed helpers
- expands returned `add`/`sub` behavior through named testcase shortcuts
  instead of semantic instruction families
- introduces extra files beyond the mandatory Stage 3 map without Stage 2
  contract repair
- adds component-level public headers or introduces a target render API named
  `__repr__`
