# AArch64 Dispatch Edge-Copy Owner Contraction Runbook

Status: Active
Source Idea: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md

## Purpose

Contract the remaining AArch64 edge-copy owner surface after the prepared
edge-source and typed stack-source authority migrations, while preserving the
prepared facts that now own edge-copy source truth.

## Goal

Make edge-copy helper ownership match the helper's real responsibility without
changing edge-copy ordering, block-entry publication behavior, diagnostics, or
machine-record behavior.

## Core Rule

Keep prepared edge-copy, typed stack-source, and producer-context facts as the
authority; move or fold only target-local emission and orchestration helpers
whose ownership is clearer after classification.

## Read First

- `ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- Narrow destination owners only when classification justifies them:
  `memory.cpp`, `comparison.cpp`, `alu.cpp`, and publication-related owners.

## Current Scope

- Inventory remaining edge-copy helpers by responsibility.
- Decide which helpers remain in `dispatch_edge_copies.*`, which belong in
  `dispatch.cpp`, and which belong in a narrower owner that emits the records.
- Preserve prepared edge-source, typed stack-source, producer-context, and
  shared BIR/prealloc facts.
- Keep changes behavior-preserving and prove each contraction packet with a
  focused build plus backend test subset chosen by the supervisor.

## Non-Goals

- Do not replace prepared edge-copy facts with local re-derivation.
- Do not add x86 or RISC-V consumers.
- Do not combine this route with dispatch-publication relocation.
- Do not change edge-copy ordering behavior.
- Do not weaken edge-copy, block-entry publication, or typed stack-source test
  expectations.

## Working Model

- Producer-context lookup and prepared source validation may remain near
  dispatch when they coordinate multiple owner families.
- Pure load-local or typed stack-source emission helpers should move only when
  a narrower owner naturally emits the resulting record family.
- Block-entry move filtering and select parallel-copy lowering must keep the
  same ordering and source-selection semantics.
- Helpers that only coordinate edge-copy route order can move into
  `dispatch.cpp` if they do not justify an independent edge-copy owner.

## Execution Rules

- Start with a helper classification table before moving code.
- Keep one responsibility family per implementation packet unless the move is
  mechanically inseparable.
- Preserve prepared source facts and diagnostics exactly unless an intentional
  message change is justified in `todo.md`.
- Update headers and includes only after call sites are accounted for.
- Run build proof after each code-changing packet. Add focused backend tests
  for edge copies, join parallel copies, typed stack-source edge publication,
  select parallel-copy sources, or predecessor publication as affected.
- Escalate to regression guard before closure if the file split changes
  externally visible lowering behavior or multiple owner families move.

## Ordered Steps

### Step 1: Build the edge-copy helper classification table

Goal: produce a helper-by-helper ownership map before moving or folding code.

Primary targets:

- `dispatch_edge_copies.*`
- `dispatch.cpp`
- Candidate destination owners listed in `Read First`

Actions:

- Inventory remaining non-trivial helpers in `dispatch_edge_copies.*`.
- Classify each helper as producer-context lookup, source fact validation,
  load-local emission, typed stack-source emission, block-entry move filtering,
  select parallel-copy lowering, or dispatch orchestration.
- Record the classification and proposed first bounded contraction packet in
  `todo.md`.
- Identify the focused build and backend proof command the executor should run
  for the first implementation packet.

Completion check:

- `todo.md` contains a classification summary sufficient for the supervisor to
  delegate one bounded contraction packet.
- No implementation files are changed in this step unless the supervisor
  explicitly delegates the first move in the same packet.

### Step 2: Fold dispatch-only orchestration helpers

Goal: move helpers that only coordinate edge-copy routing into `dispatch.cpp`
when they do not need an independent owner surface.

Actions:

- Move orchestration-only helper implementations and any private support code
  into `dispatch.cpp`.
- Keep edge-copy source selection delegated to prepared facts.
- Remove stale declarations from `dispatch_edge_copies.hpp` only after all
  call sites are updated.

Completion check:

- The project builds.
- Focused backend proof covering affected edge-copy routing passes.
- `todo.md` records the moved helpers and any helpers intentionally left in
  `dispatch_edge_copies.*`.

### Step 3: Relocate narrow target-local emission helpers

Goal: move emission helpers only when a destination owner naturally owns the
emitted record family.

Actions:

- Move load-local, typed stack-source, select parallel-copy, or block-entry
  helper families into `memory.cpp`, `comparison.cpp`, `alu.cpp`, or a
  publication-related owner only when classification proves that owner already
  owns the emitted records.
- Keep prepared edge-source and typed stack-source facts consumed directly.
- Update headers, includes, and call sites without changing edge-copy order.

Completion check:

- Each relocated family has focused build and backend proof.
- No route locally re-derives prepared source facts.
- `todo.md` records remaining helper families and their ownership rationale.

### Step 4: Contract the remaining edge-copy owner surface

Goal: leave `dispatch_edge_copies.*` with only the helpers that still have a
clear edge-copy ownership purpose.

Actions:

- Delete dead declarations and private helpers after relocation or folding.
- Thin headers to the externally needed edge-copy entry points.
- Update build files only if a source file can be removed safely.
- Keep any retained helper documented in `todo.md` by responsibility.

Completion check:

- No stale declarations, includes, or build source-list entries remain.
- The remaining edge-copy surface has a clear owner explanation.
- The project builds after any file-list change.

### Step 5: Validate closure readiness

Goal: prove the source idea is satisfied, not merely that one contraction
packet passed.

Actions:

- Run the supervisor-chosen backend tests covering AArch64 edge copies, join
  parallel copies, typed stack-source edge publication, select parallel-copy
  sources, and predecessor publication.
- Run regression guard before closure if helper movement changed externally
  visible lowering behavior or crossed multiple owner families.
- Review the final diff against the source idea's reject signals.

Completion check:

- Focused backend tests pass.
- Required regression guard passes when applicable.
- No helper bypasses prepared source facts.
- No edge-copy or block-entry publication expectation was weakened.
- No dispatch-publication, printer, instruction-record, x86, or RISC-V cleanup
  was pulled into this route.
