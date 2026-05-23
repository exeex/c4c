# AArch64 Calls Mechanical Split Runbook

Status: Active
Source Idea: ideas/open/386_aarch64_calls_mechanical_split.md

## Purpose

Make `src/backend/mir/aarch64/codegen/calls.cpp` reviewable by mechanically
extracting private implementation clusters into smaller calls-related files
while preserving behavior.

## Goal

Keep `calls.cpp` and every newly extracted calls-related implementation file
below 4000 lines without changing generated assembly, MIR records, or backend
capability.

## Core Rule

This is a mechanical split. Do not redesign call lowering, add testcase-shaped
special cases, weaken tests, or claim progress through expectation rewrites.

## Read First

- `ideas/open/386_aarch64_calls_mechanical_split.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `.codex/skills/c4c-clang-tools/SKILL.md`

## Current Targets

Extract private call-lowering clusters from `calls.cpp` into provisional
implementation/helper files:

- `calls_common.cpp/hpp`
- `calls_argument_sources.cpp/hpp`
- `calls_byval_aggregates.cpp/hpp`
- `calls_preservation.cpp/hpp`
- `calls_moves.cpp/hpp`
- `calls_effects.cpp/hpp`
- `calls_printing.cpp/hpp`

Keep these high-level entrypoints in `calls.cpp` initially:

- `publish_prepared_call_preserve_effects`
- `find_prepared_call_plan`
- `require_prepared_call_plan`
- `lower_prepared_call_instruction`
- `ScopedPreparedCallPreserveEffectPublication`

## Non-Goals

- Do not redesign AArch64 call lowering.
- Do not change generated assembly or MIR records.
- Do not rename the public call API unless mechanically required.
- Do not merge call-boundary material back into dispatch files in this pass.
- Do not add backend capability or testcase-shaped special cases.
- Do not touch unrelated dirty work already present in the worktree unless a
  supervisor packet explicitly owns it.

## Working Model

Use AST-backed symbol queries before each move. For each extracted cluster,
inspect signatures plus at least one direct caller or callee query so the move
is driven by dependencies instead of line-range guessing.

Canonical queries:

```bash
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callers /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp <function-name> build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/aarch64/codegen/calls.cpp <function-name> build/compile_commands.json
```

Record awkward dependencies in `todo.md` first. Create a later cleanup idea only
when the dependency is durable and outside the mechanical split.

## Execution Rules

- Move one coherent cluster per executor packet unless dependency size forces a
  smaller split.
- Keep new declarations private to calls implementation helpers unless a public
  API move is mechanically unavoidable.
- Prefer preserving existing helper names and call order.
- After each extraction, run the required narrow proof before reporting the
  packet complete.
- Escalate to broader validation when multiple clusters have landed or the
  touched dependency graph crosses cluster boundaries.
- Before closing the source idea, run the full close proof from the source idea.

## Steps

### Step 1: Map Calls Dependencies

Goal: build a dependency-aware extraction order for `calls.cpp`.

Primary target: `src/backend/mir/aarch64/codegen/calls.cpp`

Actions:

- Load `.codex/skills/c4c-clang-tools/SKILL.md`.
- Run the required function signature query for `calls.cpp`.
- Query callers or callees for representative functions in each proposed
  cluster from the source idea.
- Identify the first extraction packet that can be moved mechanically with the
  least cross-cluster churn.
- Record any awkward dependency in `todo.md`.

Completion check:

- `todo.md` names the selected first extraction cluster and the proof command
  the executor should run after moving it.

### Step 2: Extract Common Low-Level Helpers

Goal: move broadly shared low-level helpers into `calls_common.cpp/hpp` when the
dependency map shows the move is mechanical.

Primary target: common helpers listed under `calls_common.cpp/hpp` in the
source idea.

Actions:

- Move only helpers with clear shared utility and no semantic change.
- Add includes/declarations needed by later call clusters.
- Keep public `calls.hpp` call-oriented.
- Build and run the narrow proof.

Completion check:

- Common helpers compile from their new location, narrow proof is green, and
  no generated behavior changes are introduced.

### Step 3: Extract Argument Sources And Bindings

Goal: move prepared argument/result binding lookup and argument source
construction into `calls_argument_sources.cpp/hpp`.

Primary target: functions listed under `calls_argument_sources.cpp/hpp` in the
source idea.

Actions:

- Move lookup helpers and source constructors as a coherent mechanical group or
  as smaller dependency-preserving packets.
- Preserve existing authority, frame-slot, local-frame, and aggregate source
  behavior.
- Record any dependency that prevents a clean move.
- Build and run the narrow proof.

Completion check:

- Argument source construction lives outside `calls.cpp` where practical and
  narrow proof is green.

### Step 4: Extract Byval And Aggregate Lane Helpers

Goal: move aggregate/byval lane copy, lane publication, and stack/register lane
helpers into `calls_byval_aggregates.cpp/hpp`.

Primary target: functions listed under `calls_byval_aggregates.cpp/hpp` in the
source idea.

Actions:

- Move aggregate stack-copy helpers and fragmented lane publication helpers
  mechanically.
- Keep byval size and indirect-register behavior unchanged.
- Build and run the narrow proof.

Completion check:

- Aggregate/byval helper implementation is separated and narrow proof is green.

### Step 5: Extract Preservation Analysis

Goal: move preserved-value and move-bundle analysis into
`calls_preservation.cpp/hpp`.

Primary target: functions listed under `calls_preservation.cpp/hpp` in the
source idea.

Actions:

- Move prepared-block, value-home, and preserved-value search helpers.
- Preserve dominance, ordering, and later-use checks exactly.
- Build and run the narrow proof.

Completion check:

- Preservation analysis compiles from its new location and narrow proof is
  green.

### Step 6: Extract Move Lowering

Goal: move call-boundary value move construction and before/after call move
lowering into `calls_moves.cpp/hpp`.

Primary target: functions listed under `calls_moves.cpp/hpp` in the source
idea.

Actions:

- Move value move mnemonic, scratch register, frame-slot address, and lowering
  helpers mechanically.
- Keep instruction construction behavior and ordering unchanged.
- Build and run the narrow proof.

Completion check:

- Move lowering is separated from `calls.cpp` and narrow proof is green.

### Step 7: Extract Effect Derivation

Goal: move machine effect resource derivation into `calls_effects.cpp/hpp`.

Primary target: functions listed under `calls_effects.cpp/hpp` in the source
idea.

Actions:

- Move clobber, preserved-value, operand, and prepared-value effect helpers.
- Preserve effect resource identity and ordering.
- Build and run the narrow proof.

Completion check:

- Effect derivation compiles from its new location and narrow proof is green.

### Step 8: Extract Call Printing And Record Construction

Goal: move call record construction and target printing helpers into
`calls_printing.cpp/hpp`.

Primary target: functions listed under `calls_printing.cpp/hpp` in the source
idea.

Actions:

- Move call-boundary print helpers, immediate materialization helpers, and
  final `print_call*` helpers mechanically.
- Preserve printed assembly and unsupported/printed status behavior.
- Build and run the narrow proof.

Completion check:

- Printing helpers are separated and narrow proof is green.

### Step 9: Size And Integration Checkpoint

Goal: verify the split meets the source idea's size and behavior constraints.

Actions:

- Confirm `calls.cpp` is below 4000 lines.
- Confirm no extracted calls-related implementation file is over 4000 lines.
- Inspect `calls.hpp` for public API coherence.
- Run the source idea's full close proof:

```bash
ctest --test-dir build -j10 --output-on-failure
```

Completion check:

- Size targets are met, full-suite baseline is green, and any leftover naming
  or dependency cleanup is recorded for a later idea rather than hidden in the
  runbook.

## Required Packet Proof

Each extraction packet must at minimum run:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```
