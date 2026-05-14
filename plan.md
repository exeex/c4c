# Common MIR Container And Target Printer Boundary Runbook

Status: Active
Source Idea: ideas/open/224_common_mir_container_and_target_printer_boundary.md
Activated after: ideas/closed/228_aarch64_module_phoenix_drafts_to_implementation.md

## Purpose

Make backend MIR an assembly-shaped, target-independent instruction stream
with common walking and printing mechanics. AArch64 should own target
instruction, operand, register, memory, symbol, immediate, and label spelling,
while shared MIR code owns traversal order, function/block structure,
indentation, labels, sections, and newline policy.

Goal: route AArch64 public assembly generation through a common MIR
function/block/instruction carrier and shared printer boundary instead of
printing a flat target-local node vector directly.

## Core Rule

Do not grow `src/backend/mir/aarch64/codegen/machine_printer.*` into the
permanent terminal assembly owner. It is the temporary AArch64 spelling route
deferred by idea 228. Move common traversal and emission policy under
`src/backend/mir/`, and make AArch64 provide target printer hooks.

## Read First

- `ideas/open/224_common_mir_container_and_target_printer_boundary.md`
- `ideas/closed/228_aarch64_module_phoenix_drafts_to_implementation.md`
- `src/backend/mir/mir.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`

## Current Targets

- shared MIR carrier and printer surfaces under `src/backend/mir/`
- AArch64 target printer hooks under `src/backend/mir/aarch64/codegen/`
- AArch64 module and compatibility projection surfaces under
  `src/backend/mir/aarch64/module/` and
  `src/backend/mir/aarch64/codegen/`
- build wiring needed for new shared MIR printer implementation files
- focused backend C fixtures that already prove AArch64 assembly output, plus
  any small fixture needed to prove block ordering or printer delegation

## Current State Context

Idea 228 left AArch64 with a typed `module::MachineModule` alias over the
common `mir::MachineModule<codegen::InstructionRecord>` carrier and a derived
compatibility `FunctionRecord::machine_nodes` flat view. Public assembly
printing still depends on AArch64 `machine_printer.*` and selected target
instruction records.

This plan should use that state as the starting point. Keep compatibility flat
views only as migration aids while the common printer boundary becomes the
terminal public assembly path.

## Non-Goals

- Do not add broad new AArch64 instruction coverage beyond what is needed to
  prove the common printer/container boundary.
- Do not change prepared authority semantics for frame, call, register
  allocation, spill/reload, storage plans, data, or control-flow facts.
- Do not replace target-specific instruction selection with a TableGen-like
  ISA description.
- Do not implement object encoding, ELF relocation emission, or linker
  behavior after assembly text production.
- Do not migrate x86 or RISC-V beyond keeping shared MIR interfaces suitable
  for future consumers.
- Do not weaken tests, mark supported paths unsupported, or accept
  expectation-only progress.
- Do not add named-case printer shortcuts for one fixture.

## Working Model

- Common MIR owns `module -> function -> block -> instruction` traversal and
  generic operand carrier types.
- Target instruction records may remain the `MachineInstruction<Target>` target
  payload while the boundary settles.
- AArch64 owns target opcode, operand, register, memory, symbol, immediate,
  and label spelling through target printer methods.
- The shared printer owns ordering, labels, indentation, section/function/block
  structure, and newline policy.
- `FunctionRecord::machine_nodes` and similar flat views are compatibility
  projections, not the terminal assembly source of truth.
- Recoverable display strings should be derived from IDs, register references,
  operands, and target printer hooks rather than cached in MIR records.

## Execution Rules

- Keep each implementation step behavior-preserving unless the source idea
  explicitly requires a boundary change.
- Build after each code-changing packet and run the supervisor-delegated
  narrow proof.
- Escalate to broader backend or full CTest when shared MIR headers, printer
  APIs, or public assembly routing affect more than one narrow fixture.
- Treat printer output expectation rewrites as insufficient unless the diff
  also moves real traversal or spelling authority to the intended layer.
- Preserve temporary compatibility projections only with clear ownership and a
  later retirement check.
- Stop for lifecycle repair if the route turns into markdown-shard conversion
  work owned by idea 229.

## Ordered Steps

### Step 1: Audit Current MIR Carrier And Assembly Route

Goal: map the exact post-228 path from prepared BIR through AArch64 MIR
records to public assembly text.

Primary targets:

- `src/backend/mir/mir.hpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.*`
- `src/backend/mir/aarch64/codegen/compatibility_projection.*`

Actions:

- Inspect which public assembly API still consumes flat
  `FunctionRecord::machine_nodes` or `std::vector<InstructionRecord>`.
- Identify which traversal and formatting responsibilities are currently
  target-local in `machine_printer.*`.
- Identify the smallest existing backend C proof that exercises selected
  AArch64 machine nodes through public assembly output.
- Record the chosen proof command and any compatibility-flat-view callers in
  `todo.md`.

Completion check:

- `todo.md` names the active public assembly route, the flat compatibility
  callers, and the narrow proof command for the first code-changing packet.

### Step 2: Normalize The Common MIR Stream Contract

Goal: ensure the shared MIR carrier exposes the function/block/instruction
shape needed by a common printer without making common MIR know AArch64
mnemonics or register names.

Primary target:

- `src/backend/mir/mir.hpp`

Actions:

- Keep or refine `MachineModule`, `MachineFunction`, `MachineBlock`, and
  `MachineInstruction` as the canonical hierarchical stream.
- Preserve target instruction payloads as target-owned data.
- Keep any flattening helpers explicitly classified as compatibility helpers,
  not as printer authority.
- Avoid moving AArch64-specific enums, mnemonics, or register spelling into
  common MIR.

Completion check:

- The shared carrier can be walked by common code while target-specific
  spelling remains target-owned, and the project builds.

### Step 3: Add The Shared MIR Printer Boundary

Goal: introduce common MIR printer code that walks the shared carrier and
delegates all target spelling decisions.

Primary targets:

- new or existing files under `src/backend/mir/`
- build wiring for the shared MIR printer files

Actions:

- Define a target printer interface or callback surface for instruction,
  operand, memory, register, symbol, immediate, and label spelling.
- Implement a common printer/walker that owns module/function/block order,
  labels, indentation, separators, and newline policy.
- Make unsupported target spelling return structured diagnostics instead of
  silently emitting partial assembly.
- Keep the interface narrow enough that AArch64 can adapt
  `InstructionRecord` without broad instruction-selection rewrites.

Completion check:

- Common MIR printer code compiles independently of AArch64 target mnemonics
  and can be instantiated by an AArch64 adapter.

### Step 4: Adapt AArch64 Spelling To The Target Printer Interface

Goal: turn AArch64 `machine_printer.*` from a whole-route flat-vector printer
into target spelling hooks used by the shared printer.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`

Actions:

- Preserve existing selected-node validation and diagnostic behavior.
- Move instruction-line spelling behind the target printer interface.
- Route register, immediate, memory, branch label, and mnemonic spelling
  through target-owned helpers.
- Avoid precomputing display strings in MIR records when spelling is
  recoverable from structured target records.

Completion check:

- Existing printable AArch64 machine node families still print through the
  adapter, and unsupported families still fail with diagnostics.

### Step 5: Route Public AArch64 Assembly Through The Common Printer

Goal: make terminal AArch64 assembly generation walk the hierarchical
`module::MachineModule` instead of a flat target-local vector.

Primary targets:

- `src/backend/mir/aarch64/codegen/emit.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.*`
- AArch64 API/public assembly call sites discovered in Step 1

Actions:

- Wire the public AArch64 assembly path to call the shared MIR printer over
  `built_module.mir`.
- Keep compatibility `FunctionRecord::machine_nodes` available only where
  existing callers still require it.
- Preserve function and block ordering from the common carrier.
- Add or adjust a focused backend C proof only when needed to prove
  hierarchical block walking or printer delegation.

Completion check:

- Public AArch64 assembly output for the delegated proof is produced by
  walking `MachineModule`/`MachineFunction`/`MachineBlock`, not by printing a
  flat vector directly.

### Step 6: Retire Or Fence Temporary Display And Flat-Printer Paths

Goal: remove or clearly fence target-local display caches and flat-printer
authority that conflict with the common boundary.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.*`
- `src/backend/mir/aarch64/codegen/compatibility_projection.*`
- `src/backend/mir/aarch64/module/module.hpp`

Actions:

- Delete flat-vector terminal assembly entrypoints once public routing no
  longer needs them, or mark them as compatibility-only if a caller remains.
- Remove cached display fields only when structured spelling fully replaces
  them.
- Keep derived flat views separate from canonical MIR stream ownership.
- Update tests only to reflect real traversal/spelling ownership changes.

Completion check:

- The remaining flat views and target-local printer helpers are either gone or
  explicitly non-authoritative, and no public assembly route depends on them as
  the main carrier.

### Step 7: Broader Validation And Handoff

Goal: prove the shared MIR printer/container boundary is stable enough to
close or hand off follow-up work.

Actions:

- Run the supervisor-selected broader backend or full CTest check after the
  public assembly route moves to the common printer.
- Confirm idea 229 remains a markdown-shard conversion follow-up rather than a
  blocker for the shared printer boundary.
- Record any remaining target migration or markdown conversion work in
  `todo.md` without expanding this source idea.

Completion check:

- Backend MIR has a common instruction-stream carrier under `src/backend/mir/`,
  AArch64 public assembly generation walks that carrier, target spelling is
  delegated through AArch64 printer hooks, and validation results are recorded
  in `todo.md`.
