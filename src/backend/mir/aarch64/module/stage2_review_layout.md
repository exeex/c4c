# AArch64 Module Stage 2 Review Layout

## Scope

This document reconstructs the current AArch64 module subsystem shape from the
accepted Stage 1 evidence. It describes the legacy emitter as evidence for
later layout decisions, not as the replacement architecture and not as a set of
helper boundaries to preserve.

This Stage 2 artifact now covers the current subsystem reconstruction,
replacement architecture layout, and Stage 3 draft artifact map. It
intentionally stops before the Stage 2 handoff, which belongs to a later Stage
2 step.

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

## Replacement Architecture Layout

The replacement architecture lowers `prepare::PreparedBirModule` directly into
typed MIR machine nodes. The current record assembly shape remains evidence for
what facts must survive, but it must not become a renamed catch-all module
emitter that gathers every prepared table, source fallback, instruction rule,
and compatibility view in one implementation file.

Dependency direction is one way:

`prepare::PreparedBirModule` -> AArch64 module dispatch -> function traversal
-> value/operand resolution -> instruction, branch/control, and call lowering
-> MIR machine nodes -> shared `mir_printer` traversal -> target-owned
instruction/operand/register rendering.

Lower layers may receive typed facts from earlier layers, but must not reach
back into broad prepared tables, BIR source spellings, or public compatibility
records to rediscover authority. The module boundary may orchestrate and return
the public product; it must not own all lowering decisions.

## Responsibility And Dependency Seams

### Module Dispatch

Owns:

- accepting the `prepare::PreparedBirModule` entry point
- target-profile and AArch64 prepared-handoff validation
- dispatching data/module-level preparation and prepared function traversal
- constructing the public `BuildResult` and top-level module product

May know:

- the prepared module, target profile, top-level data records, and which
  function traversal component is used for AArch64
- the public compatibility fields that must be populated from completed MIR
  output

Must not know:

- individual BIR operation lowering rules
- register spelling fallbacks as semantic authority
- call argument/result ABI expansion details beyond passing the prepared call
  plan to call lowering
- branch fusion, scalar ALU, spill/reload, or return lowering internals

Outputs:

- a validated module-level product containing canonical MIR functions and any
  required data/relocation records
- compatibility views derived from canonical MIR and typed facts, not from a
  separate catch-all record pass

### Function Traversal

Owns:

- walking `prepared.control_flow.functions` and their prepared block order
- establishing per-function lowering context from prepared frame, storage,
  regalloc, spill/reload, parallel-copy, and call-plan authorities
- creating MIR function/block structure before instruction emission
- preserving source/debug provenance only as optional metadata

May know:

- prepared function identity, block IDs, prepared frame/control-flow records,
  and typed per-function authority tables
- the value/operand resolver and target instruction lowerers

Must not know:

- AArch64 assembly text syntax
- public flat `machine_nodes` compatibility as the primary output shape
- spelling-based source function or block recovery except through an explicit
  compatibility/provenance bridge

Outputs:

- a `mir::Function<mir::MachineNode<...>>` with ordered MIR blocks and target
  machine nodes
- per-function compatibility/provenance metadata derived from the same typed
  traversal

### Value And Operand Resolution

Owns:

- the single typed authority model for prepared value locations
- conversion from prepared value homes, storage-plan entries, regalloc
  assignments, frame slots, spill slots, immediates, symbols, labels, and
  target registers into AArch64 operand objects
- normalizing storage location precedence so consumers do not choose among
  broad optional record fields

May know:

- prepared value IDs, storage locations, frame/stack slot IDs, ABI register
  references, immediate values, symbols, and label identities
- AArch64 register classes and operand forms needed to build target operands

Must not know:

- operation-specific instruction selection
- branch or call sequencing rules
- final assembly punctuation, spacing, or line emission
- legacy register-name parsing except through a contained compatibility
  adapter with fail-closed behavior for structured misses

Outputs:

- target-owned operand/register representations for instructions, branches,
  calls, memory forms, immediates, labels, and symbols
- diagnostic/provenance records that explain which prepared authority produced
  each operand when compatibility clients still need that view

### Instruction Lowering

Owns:

- lowering prepared non-control operations into AArch64 MIR machine nodes
- scalar ALU, memory, move, spill/reload, parallel-copy step, and return-value
  materialization rules as semantic instruction families, not named testcase
  shortcuts
- selecting target instruction records from typed operands and prepared
  operation semantics

May know:

- prepared operation kind, typed operands from the resolver, target register
  classes, instruction constraints, and MIR block insertion points

Must not know:

- module-level public record assembly
- source BIR spelling recovery as semantic input
- printer traversal or `.s` file structure
- call ABI planning or branch edge ownership

Outputs:

- target instruction machine nodes inserted into canonical MIR blocks
- optional compatibility instruction records derived from those nodes when
  existing clients still require `FunctionRecord::machine_nodes`

### Branch And Control Lowering

Owns:

- prepared terminator lowering, block-edge emission, conditional/unconditional
  branch node creation, compare/condition handling, and return control flow
- branch-fusion decisions when prepared comparison facts and target constraints
  make fusion valid
- ensuring MIR block control nodes are complete enough for a shared printer to
  emit labels and branches without target-specific traversal logic

May know:

- prepared terminators, block labels, compare/condition metadata, target branch
  instruction forms, and typed operands from the resolver

Must not know:

- function traversal order beyond the current block context
- public compatibility branch records as instruction authority
- ad hoc source label spellings except as compatibility metadata

Outputs:

- target branch/control machine nodes and block successor metadata
- compatibility branch records derived from typed control lowering when needed

### Call Lowering

Owns:

- converting prepared call plans into target MIR call sequences
- materializing argument moves, result locations, preserved values, clobbers,
  indirect callees, memory returns, and call-site ABI bindings
- coordinating with value/operand resolution for locations while keeping ABI
  call semantics in one call-lowering component

May know:

- prepared call plans, ABI argument/result locations, clobber/preserve sets,
  memory-return metadata, indirect-callee data, and target call instruction
  forms

Must not know:

- generic operation lowering rules unrelated to calls
- public flat record layout as the source of call authority
- printer syntax or section/file emission

Outputs:

- target call and call-adjacent move machine nodes in canonical MIR
- call inspection records that are projections of the lowered call sequence and
  prepared ABI facts

### Public Assembly Bridging

Owns:

- the bridge from canonical MIR functions and module data to one shared,
  platform-independent `mir_printer` pass
- preserving the invariant that the common printer owns traversal, ordering,
  spacing mechanics, labels, sections, and file emission structure
- calling target-owned print/render methods on AArch64 instruction, operand,
  register, memory, label, symbol, and immediate representations

May know:

- common MIR containers, target-owned printable interfaces, module data records,
  and the minimal section/data hooks needed by the common printer

Must not know:

- AArch64 opcode syntax tables, register spelling rules, operand punctuation,
  or instruction-family formatting details
- lowering-time prepared authority tables
- cached display strings as semantic output

Outputs:

- `.s` text acceptable to `gcc`/`as` through a single common printer traversal
- no C++ API named `__repr__`; target-owned render methods should have ordinary
  C++ names that separate instruction printing from operand printing

### Compatibility Behavior

Owns:

- migration-only projections such as `FunctionRecord::machine_nodes`, raw
  source/prepared provenance pointers, label views, broad inspection records,
  and legacy register-string fallback
- clear removal conditions for compatibility views once clients consume
  canonical MIR blocks and typed operands directly

May know:

- legacy public record shapes and the structured canonical MIR facts needed to
  populate them
- source/prepared pointers and string views for diagnostics

Must not know:

- semantic lowering decisions that should live in instruction, branch, call, or
  operand components
- fallback strings as primary identity or register authority
- testcase-specific exceptions that bypass typed prepared data

Outputs:

- compatibility records derived after canonical lowering
- fail-closed diagnostics when structured authority is missing and no approved
  compatibility fallback applies

## Parent 224 Judgment

The layout resolves the parent 224 failure family if Stage 3 preserves these
seams:

- common MIR carrier confusion: canonical output is MIR blocks containing
  target-owned instruction nodes; public records and compatibility vectors are
  projections, not a second instruction carrier
- flat target-local vectors: `FunctionRecord::machine_nodes` may remain only as
  a derived migration view and must not drive printing, validation, or future
  lowering
- cached display strings: printable target forms render themselves when the
  shared printer asks; cached strings are diagnostics only and cannot become
  semantic instruction or operand authority
- target/common printer boundary drift: the common `mir_printer` owns traversal
  and emission mechanics, while AArch64 owns opcode, register, operand, memory,
  immediate, label, and symbol rendering

The parent 224 judgment is therefore positive for a phoenix replacement only
under the direct prepared-BIR-to-MIR layout above. A design that keeps the
legacy broad module emitter, renames its helpers, and asks later code to infer
instructions from record piles would fail parent 224 again.

## Stage 3 Draft Artifact Map

Stage 3 must draft the replacement for this directory through the artifacts
below. This is the complete mandatory set for the initial replacement draft;
Stage 3 should not invent additional layout scope while filling these files.

### Directory Index Draft

- `src/backend/mir/aarch64/module/module.md`: replacement directory-level
  index. It must describe the new direct prepared-BIR-to-MIR module boundary,
  list the draft artifacts in this map, and preserve the phoenix route
  constraints from this review.

### Mandatory Header Drafts

- `src/backend/mir/aarch64/module/module.hpp.md`: the only mandatory
  replacement `.hpp.md` artifact. It drafts the non-helper directory index
  header for the public build entry point, public module product, durable MIR
  carrier contract, compatibility projections, and target-owned printable
  type surfaces.

Phoenix header policy: `module.hpp` remains the single non-helper index header
for this replacement directory. Stage 3 must not add component-level public
headers for the seams below. `helper.hpp` is the only allowed header exception,
and it is not part of the mandatory draft set unless Stage 3 explicitly proves
that a private helper header is required without becoming a second public
index.

### Mandatory Implementation Drafts

- `src/backend/mir/aarch64/module/module.cpp.md`: module dispatch, target
  profile resolution, AArch64 prepared-handoff validation, top-level data and
  relocation orchestration, `BuildResult` construction, and public product
  assembly from completed canonical MIR functions.
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`: prepared
  function/block traversal, per-function lowering context construction, MIR
  function/block creation, and source/debug provenance attachment as optional
  metadata.
- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`: typed prepared
  value-location authority, target register/operand conversion, storage
  precedence normalization, immediates, labels, symbols, memory forms, and
  fail-closed compatibility register fallback.
- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`: non-control
  prepared operation lowering into AArch64 MIR machine nodes, including
  scalar ALU, memory, moves, spill/reload, parallel-copy steps, and
  return-value materialization as semantic families.
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`: prepared
  terminator lowering, conditional and unconditional branch nodes, return
  control flow, compare/condition handling, block successor metadata, and
  valid branch-fusion decisions.
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

### Evidence Repair Dependencies

No Stage 1 extraction repair is required before the artifacts above are
trustworthy to draft. The accepted Stage 1 correction remains a route
constraint: `src/backend/mir/aarch64/module/module.cpp` is still compiled
legacy evidence until replacement implementation and build wiring exist.
Stage 3 should use `module.cpp.md`, `module.hpp.md`, and `module.md` as
evidence, keep the correction visible, and avoid treating the legacy compiled
source as an already-replaced implementation.

## Step 2 Decision Support

The current subsystem is a broad adapter from prepared BIR evidence to an
exported AArch64 module record pile, with limited MIR machine-node synthesis
inside that pile. Its entry point and public product are stable enough to
review, but its helper clusters mix lookup, validation, authority projection,
compatibility recovery, data records, and partial instruction lowering.

Later layout decisions should therefore use this document to understand which
facts must survive, while rejecting the current helper grouping as replacement
design.

Step 3 extends that decision support with a replacement architecture: prepared
state drives typed MIR machine-node lowering through explicit module,
function, operand, instruction, branch, call, printer-bridge, and compatibility
seams. Step 4 maps those seams to exact Stage 3 draft artifacts; this document
does not provide draft implementation contents.
