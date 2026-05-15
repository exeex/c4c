# AArch64 Atomic Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/238_aarch64_atomic_machine_nodes.md

## Purpose

Activate the AArch64 atomic backend route from the archived atomic semantics
shard. This runbook should introduce structured ownership for atomic facts
before AArch64 selection, then lower and print machine records from those facts.

## Goal

Represent ordered atomic loads and stores, fences, read-modify-write operations,
and compare-exchange as structured backend carriers and selected AArch64
machine nodes, with proof that old-value and ordering semantics are preserved.

## Core Rule

Atomic backend progress must come from structured operation facts: ordering,
width, pointer, value, result mode, and compare-exchange failure ordering. Do
not infer semantics from volatile flags, rendered assembler text, fixed scratch
snippets, or named testcase shortcuts.

## Read First

- `ideas/open/238_aarch64_atomic_machine_nodes.md`
- `docs/backend/x86_codegen_legacy/atomics.cpp.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- atomic, prepared, dispatch, and printer tests under `tests/backend`

## Current Targets

- Target-neutral or prepared atomic operation carriers with explicit ordering,
  width, pointer/value, result, and compare-exchange facts.
- AArch64 selected records for ordered loads, ordered stores, fences, atomic
  RMW loops, and compare-exchange loops.
- Printer support for acquire/release instructions, exclusive-load/store retry
  loops, compare-exchange failure paths, and non-relaxed barriers.
- Tests that reject unsupported or partial atomic facts instead of fabricating
  instruction sequences.

## Non-Goals

- Do not rebuild the archived fixed `x0`/`x1`/`x2`/`x3`/`w4` scratch contract.
- Do not infer atomic ordering from text templates or volatile memory flags.
- Do not add named-case shortcuts for one atomic fixture.
- Do not weaken unsupported atomic tests or external contracts to claim backend
  progress.
- Do not merge this route with intrinsic, inline-assembly, binary128, scalar FP,
  or i128 work.

## Working Model

- Treat atomics as a semantic backend family with explicit carrier facts before
  target-specific selection.
- Start fail-closed: missing ordering, width, operand, or result-mode authority
  should diagnose rather than silently choose a default instruction sequence.
- Prefer narrow, reviewable packets: first inventory and carrier shape, then
  simple ordered memory operations, then RMW/compare-exchange loops, then final
  printer and route proof.
- Keep old-value result semantics visible in selected records and tests for
  fetch operations and compare-exchange.

## Execution Rules

- Each code-changing step must run the supervisor-delegated build or compile
  proof and the matching backend test subset.
- Add tests beside the backend layer changed: BIR/prepared carrier tests before
  dispatch tests, and dispatch tests before final assembly printer tests.
- Preserve existing non-atomic memory, call-boundary, scalar, i128, intrinsic,
  inline-asm, and binary128 behavior after shared backend changes.
- If execution discovers that frontend/HIR lacks atomic semantic authority,
  record the missing prerequisite in `todo.md` or ask the supervisor to split a
  separate source idea instead of inventing target-local semantics.

## Ordered Steps

### Step 1: Inventory Atomic Authority And Fail-Closed Gaps

Goal: identify where atomic semantics currently exist or are lost before AArch64
selection.

Primary targets:
- `docs/backend/x86_codegen_legacy/atomics.cpp.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- current backend atomic-related tests

Actions:
- Inspect existing BIR, prepared, MIR, and printer support for atomic ordering,
  fences, RMW, and compare-exchange facts.
- Add focused diagnostics or tests that document missing structured atomic
  authority without downgrading unsupported expectations.
- Identify the smallest carrier boundary that can preserve ordering, width,
  pointer/value, result mode, and compare-exchange failure ordering.
- Keep all observed gaps fail-closed; do not add target-local reconstruction.

Completion check:
- Tests or notes in `todo.md` identify current atomic capability and the first
  carrier boundary to implement.
- No unsupported atomic expectation is weakened.
- The executor proof command and result are recorded in `todo.md`.

### Step 2: Define Structured Atomic Operation Carriers

Goal: create the target-neutral or prepared carrier facts needed by AArch64
selection.

Primary targets:
- `src/backend/bir/bir.hpp`
- BIR or prepared lowering files identified in Step 1
- prepared printer and carrier tests

Actions:
- Add structured atomic operation records for ordered loads, ordered stores,
  fences, RMW operations, and compare-exchange where source authority exists.
- Include explicit ordering, width, pointer, value, result mode, and
  compare-exchange failure-ordering fields.
- Print or dump the carrier facts in existing backend debug/printer surfaces
  when those surfaces already cover comparable structured records.
- Preserve fail-closed diagnostics for incomplete carrier facts.

Completion check:
- BIR/prepared tests prove atomic carrier facts preserve all required fields.
- Adjacent non-atomic memory and call-preparation tests remain green.
- Missing or partial atomic facts still fail closed.

### Step 3: Select Ordered Loads, Stores, And Fences

Goal: lower the simple atomic memory operations to structured AArch64 selected
records.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- AArch64 dispatch tests

Actions:
- Add selected records for atomic loads, atomic stores, and fences from the
  structured carriers.
- Map acquire, release, acquire-release, sequentially consistent, and relaxed
  orderings according to the source idea's semantics.
- Keep instruction selection width-correct for supported narrow and full-width
  operations.
- Reject unsupported ordering/width combinations explicitly.

Completion check:
- Dispatch tests prove ordered loads/stores and non-relaxed fences select from
  structured facts.
- Unsupported combinations diagnose instead of falling back to plain volatile
  memory operations.
- The supervisor-selected backend proof is green.

### Step 4: Select Atomic RMW And Compare-Exchange Loops

Goal: lower structured RMW and compare-exchange operations into retrying
exclusive-access machine records while preserving result semantics.

Primary targets:
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- AArch64 dispatch tests for old-value and boolean result modes

Actions:
- Add selected records for exclusive-load/store retry loops used by atomic RMW
  operations.
- Preserve old-value result semantics for fetch operations.
- Add compare-exchange records that model expected value, desired value,
  success ordering, failure ordering, and result mode.
- Model exclusive-monitor clearing on compare failure when required.

Completion check:
- Dispatch tests prove RMW returns the old value and compare-exchange supports
  boolean and old-value result modes.
- Failure-ordering and monitor-clear facts are visible in selected records or
  diagnostics.
- No fixed scratch-register contract is introduced.

### Step 5: Print AArch64 Atomic Machine Nodes

Goal: emit final AArch64 assembly only from the structured selected records.

Primary targets:
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- AArch64 machine-printer tests

Actions:
- Print acquire/release load/store forms, `dmb` fences for non-relaxed
  orderings, and exclusive-access retry loops.
- Print compare-exchange success and failure paths from selected record facts.
- Keep register names, labels, and temporaries sourced from structured selected
  records rather than archived fixed scratch conventions.
- Preserve diagnostics for selected records that lack required operands.

Completion check:
- Printer tests prove ordered load/store, fence, RMW, and compare-exchange
  emission from structured nodes.
- Unsupported or incomplete records remain fail-closed.
- No assembler text is parsed to recover backend semantics.

### Step 6: Allocate Atomic Loop Printer Facts In Selection

Goal: make the prepared-to-selected route publish the structured scratch,
status, and loaded-value register facts required by printable atomic RMW and
compare-exchange loops.

Primary targets:
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- backend prepared, dispatch, and printer route tests under `tests/backend`

Actions:
- Allocate or carry explicit structured loop temporary facts for RMW and
  compare-exchange selected records instead of relying on fixed archived
  scratch registers.
- Preserve pointer, input, expected, desired, result, old-value, status, and
  loaded-value register authority from prepared or selected records through the
  final printer surface.
- Keep incomplete prepared or selected atomic loop facts fail-closed with
  explicit diagnostics.
- Do not add final assembly text inference, volatile-memory reconstruction, or
  named testcase shortcuts.

Completion check:
- Representative prepared-to-selected tests prove RMW and compare-exchange
  records carry the loop scratch/status/loaded-value facts needed by the
  printer.
- Existing selected-record printer tests continue to pass without fixed
  scratch-register assumptions.
- Unsupported or partial atomic loop carriers still diagnose.

### Step 7: Prove Atomic Route And Decide Lifecycle

Goal: validate that the atomic source idea is satisfied after selected atomic
loop facts are generated by the real route, or explicitly park any remaining
durable gap.

Primary targets:
- backend atomic route tests under `tests/backend`
- `ideas/open/238_aarch64_atomic_machine_nodes.md`
- `plan.md`
- `todo.md`

Actions:
- Run the supervisor-selected narrow proof and broader backend validation when
  shared carrier, selection, or printer surfaces changed.
- Compare the completed route against the source idea's proof direction:
  ordered loads/stores, fences, RMW old-value results, compare-exchange result
  modes, failure ordering, and signed narrow-load normalization.
- Ask the plan owner to close the source idea only if the source scope is
  satisfied; otherwise deactivate or rewrite the runbook with precise leftover
  work.

Completion check:
- Atomic route proof is green at the selected scope.
- The lifecycle decision is explicit: close, deactivate, or continue with a
  narrowed follow-on runbook.
