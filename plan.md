# Common MIR Container And Target Printer Boundary Runbook

Status: Active
Source Idea: ideas/open/224_common_mir_container_and_target_printer_boundary.md

## Purpose

Make backend MIR an assembly-shaped, target-independent instruction stream
that common code can walk while each target owns instruction, operand, memory,
register, symbol, immediate, and label spelling.

## Goal

Use `Function -> Block -> Instruction` MIR under `src/backend/mir/` as the
primary carrier for AArch64 public assembly generation, with generic walking
and target-specific printing instead of flat target-local vectors or cached
display strings.

## Core Rule

Common MIR owns structure, ordering, labels, indentation, sections, function
and block traversal, and newline policy. Target code owns target opcode
meaning and all textual spelling decisions.

## Read First

- `ideas/open/224_common_mir_container_and_target_printer_boundary.md`
- `src/backend/mir/mir.hpp`
- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- AArch64 MIR and public assembly tests under `tests/backend/mir/`

## Current Targets And Scope

- Common MIR container and printer/walker code under `src/backend/mir/`.
- AArch64 module records that currently maintain both
  `Function<MachineNode<InstructionRecord>> mir` and compatibility
  `machine_nodes`.
- Public AArch64 assembly generation in `src/backend/backend.cpp`.
- AArch64 instruction and operand printers in
  `src/backend/mir/aarch64/codegen/`.
- Tests that prove public AArch64 `.c -> .s` output and target MIR record
  behavior.

## Non-Goals

- Do not add broad new AArch64 instruction coverage beyond what is needed to
  prove the container/printer boundary.
- Do not change prepared authority semantics for frame, call, register
  allocation, spill/reload, storage plans, or control-flow facts.
- Do not replace AArch64 instruction selection with an ISA-description system.
- Do not implement object encoding, ELF relocation emission, linker behavior,
  or assembler handoff beyond preserving the structured MIR path.
- Do not migrate x86 or RISC-V beyond keeping the common interfaces usable for
  future target consumers.
- Do not downgrade tests, mark supported paths unsupported, or claim progress
  through expectation rewrites.

## Working Model

Execution should move from boundary inventory to common printing, then migrate
AArch64 public assembly generation onto the common stream:

1. inspect the current common MIR container, AArch64 flat compatibility view,
   and printer entry points;
2. define a common MIR printer/walker contract that delegates all target
   spelling;
3. adapt AArch64 target printing to satisfy that contract;
4. route public AArch64 assembly generation through the common MIR function and
   block stream;
5. narrow or remove compatibility flat views and display-cache fields only when
   callers no longer depend on them;
6. validate with build, narrow MIR/AArch64 assembly proof, and broader backend
   regression checks when the migration reaches a milestone.

Keep each step behavior-preserving unless the source idea explicitly requires
removing stale cached spelling or compatibility-only access paths.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve the source idea; do not edit it for ordinary execution notes.
- Prefer small migration packets that leave AArch64 assembly output unchanged.
- Any common MIR code must avoid target mnemonic, register spelling, and memory
  syntax knowledge.
- Target printers should derive spelling from IDs, register references,
  operands, and target print methods, not from precomputed display strings.
- Keep compatibility flat views only as temporary migration aids and document
  any remaining callers in `todo.md`.
- For code-changing steps, run a fresh build before narrow proof.
- Treat named-case shortcuts, unsupported downgrades, and expectation-only
  rewrites as route drift.

## Steps

### Step 1: Inventory Current MIR Carrier And Printer Boundary

Goal: establish the current AArch64 MIR carrier, flat compatibility paths, and
printer ownership before changing interfaces.

Primary targets:
- `src/backend/mir/mir.hpp`
- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.*`
- relevant MIR/AArch64 backend tests

Actions:
- Identify every caller that reads `machine_nodes` directly or flattens
  `function.mir` back into target-local records.
- Identify which target records still contain recoverable display strings or
  target spelling cached outside the printer.
- Record the minimum common printer/walker surface needed for current AArch64
  public assembly output.
- Choose the first implementation packet and narrow proof command.

Completion check:
- `todo.md` records the direct flat-vector callers, display-cache candidates,
  first implementation packet, and exact proof command.
- No implementation behavior has changed except optional lifecycle-only
  inventory notes.

### Step 2: Define The Common MIR Printer Contract

Goal: add common walking/printing code that owns MIR structure and delegates
target spelling.

Primary targets:
- `src/backend/mir/`
- build wiring for common MIR sources, if needed
- tests for common printer traversal, if an existing test bucket fits

Actions:
- Define a target printer interface or template contract for instruction,
  operand, memory, register, symbol, immediate, and label spelling.
- Add a generic MIR printer/walker for functions, blocks, instructions,
  labels, indentation, section/function/block structure, and newline policy.
- Keep the contract target-neutral; common code must not inspect AArch64
  opcode families or register names.
- Preserve existing `mir.hpp` container semantics unless the implementation
  proves a narrow type-shape correction is needed.

Completion check:
- Common MIR printer code builds without target-specific spelling embedded in
  common files.
- New or existing tests prove ordering and formatting policy at the common
  layer.
- Build plus narrow common-MIR proof is recorded.

### Step 3: Adapt AArch64 To The Target Printer Boundary

Goal: make AArch64 instruction, operand, register, and memory spelling
available through the target-printer contract.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- AArch64 record helpers that map IDs and register references to text

Actions:
- Wrap or refactor existing AArch64 machine-printer functions behind the common
  target-printer interface.
- Ensure instruction spelling is derived from AArch64 opcode/status/payload
  records and target methods, not cached display text.
- Keep existing printable-subset diagnostics intact or improve them without
  weakening contracts.
- Do not add unrelated AArch64 instruction families.

Completion check:
- Existing AArch64 machine-printer tests still pass through the new boundary.
- No common MIR file contains AArch64 mnemonic, register, or memory syntax
  knowledge.
- Build plus narrow AArch64 printer proof is recorded.

### Step 4: Route Public AArch64 Assembly Through Common MIR

Goal: make public AArch64 assembly generation walk `function.mir` directly
instead of reconstructing a flat target-local vector first.

Primary targets:
- `src/backend/backend.cpp`
- `src/backend/mir/aarch64/module/module.*`
- common MIR printer entry points
- AArch64 public assembly tests

Actions:
- Replace public assembly generation's flatten-and-print path with common MIR
  function/block traversal.
- Keep target-specific instruction records as instruction payload while the
  common container settles.
- Preserve `.text`, `.globl`, `.type`, function label, `.size`, and
  `.note.GNU-stack` output contracts.
- Keep compatibility flat views only for callers that still need them during
  migration.

Completion check:
- AArch64 public assembly generation reads the common MIR container as its
  primary source.
- Existing public `.c -> .s` output remains behavior-preserving.
- Build plus narrow AArch64 public assembly proof is recorded.

### Step 5: Narrow Compatibility Views And Display Caches

Goal: remove or demote target-local flat views and recoverable display strings
after common-stream callers are in place.

Primary targets:
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- AArch64 codegen records and printer helpers
- tests that inspect AArch64 MIR/module records

Actions:
- Remove direct `machine_nodes` reads that have common MIR replacements.
- Retain flat compatibility helpers only when a documented caller still needs
  a temporary migration path.
- Remove display-cache fields when spelling can be derived from IDs, name
  tables, register references, operands, and target print methods.
- Keep prepared frame, call, allocation, spill/reload, and storage-plan facts
  semantically unchanged.

Completion check:
- Remaining flat or cached-spelling paths are documented in `todo.md` with an
  owner and reason, or are removed.
- Tests prove target MIR metadata still carries required prepared-contract
  facts.
- Build plus narrow MIR metadata/printer proof is recorded.

### Step 6: Validate The Boundary And Close Readiness

Goal: prove the migrated boundary is stable enough for supervisor close review.

Primary targets:
- backend MIR and AArch64 test subsets
- public assembly smoke tests
- canonical regression logs

Actions:
- Run the supervisor-selected build and narrow MIR/AArch64 backend subset.
- Escalate to broader backend or full CTest validation if the migration touched
  shared common MIR interfaces or multiple target-facing contracts.
- Compare implementation against the source idea's completion signal.
- Record remaining compatibility paths or blockers in `todo.md`.

Completion check:
- Build, narrow proof, and any broader required validation are green and
  recorded.
- AArch64 public assembly generation walks the common MIR carrier.
- Instruction, operand, register, and memory spelling is delegated through
  target printer methods.
- Source-idea completion or remaining scope is explicit for supervisor review.
