# RV64 Register-Source Stack-Destination Prepared Move Bundles Runbook

Status: Active
Source Idea: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md

## Purpose

Resolve prepared move bundles whose destination is a stack value but whose
explicit source home is a register, without treating them as the coherent
stack-to-stack shape closed by idea 513.

## Goal

Either materialize the owned register-source to stack-destination prepared move
shape from explicit prepared facts, or reclassify it to the correct producer
owner with a precise fail-closed diagnostic.

## Core Rule

Consume explicit prepared move-bundle, source-home, destination-home, and
storage facts. Do not infer register or stack locations from row names, source
order, ABI formulas, argument indexes, value names, or raw BIR text.

## Read First

- ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
- ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/

## Current Targets

- Representative rows: `src/20010518-1.c` and `src/pr27073.c`
- Current rejection: `unsupported_move_bundle_target_shape`
- Owning layer: RV64 object emission consuming prepared move-bundle authority,
  with producer classification checked before materialization
- Required authority:
  - prepared move bundle identifies the value movement to emit
  - source home or storage plan explicitly identifies a register source
  - destination home explicitly identifies a stack destination
  - bundle cardinality, scalar type, width, bank, and sequencing are coherent
    enough for RV64 object emission to materialize without guessing

## Non-Goals

- Do not reopen idea 513's same-scalar stack-slot to stack-slot materializer.
- Do not infer source or destination locations from representative row names,
  argument indexes, source order, ABI formulas, local names, or textual BIR
  matching.
- Do not silently accept multi-move bundles by dropping moves, picking one
  source by order, or merging moves without an explicit contract.
- Do not broaden into general parallel-copy scheduling or cycle resolution
  unless the explicit prepared facts prove that minimal owner is required.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, reason strings, or scan accounting as proof of capability
  progress.

## Working Model

- Idea 513 closed the coherent same-scalar stack-slot to stack-slot move
  materializer and deliberately left register-source or ambiguous shapes
  fail-closed.
- `src/pr27073.c` currently presents a single move from `%p.count` in register
  `a4` to a stack destination despite the old `consumer_stack_to_stack` reason
  string.
- `src/20010518-1.c` currently presents a multi-move bundle with register
  sources `a0` and `a1` targeting one stack value.
- A valid implementation must first decide whether the explicit prepared facts
  authorize these shapes. If they do not, the producer or diagnostic owner must
  be clarified instead of patching RV64 around missing authority.

## Execution Rules

- Start by reproducing the two representative rows and recording the prepared
  move bundle, source home, destination home, storage plan, width, bank,
  cardinality, and rejection point in `todo.md`.
- Split single-move and multi-move evidence if they require different semantic
  owners. Do not force the multi-move case into the first implementation slice.
- Keep implementation in RV64 object emission only when producer facts are
  already explicit and coherent. If producer classification is wrong or
  incomplete, stop and record the producer-owned blocker.
- Add focused backend coverage for any accepted register-source to
  stack-destination path and for adjacent rejected shapes.
- For code-changing steps, run a fresh build, focused representative proof,
  focused backend coverage, and the relevant backend subset chosen by the
  supervisor.

## Step 1: Rebuild Register-Source Move Evidence

Goal: Reconfirm the current representative failures and capture the explicit
prepared facts that define the register-source to stack-destination owner.

Actions:

- Reproduce focused dumps or probes for `src/20010518-1.c` and
  `src/pr27073.c`.
- Record each prepared move bundle, source home, destination home, storage
  plan, scalar type, width, bank, cardinality, and current RV64 rejection site.
- Separate the single-move register-source case from the multi-move bundle
  case if the facts show different owners.
- Confirm that the failure is not the coherent stack-slot to stack-slot shape
  already accepted by idea 513.

Completion check:

- `todo.md` records the representative facts, the exact
  `unsupported_move_bundle_target_shape` site, and whether RV64 has enough
  explicit authority to attempt materialization.

## Step 2: Define The Minimal Accepted Or Rejected Contract

Goal: Decide whether the owned register-source to stack-destination shape is
authorized for RV64 materialization or must be reclassified to another owner.

Actions:

- Trace current move-bundle classification and materialization in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and neighboring helpers.
- Identify existing register-source move support, stack-destination storage
  helpers, scratch policy, width-specific store helpers, and diagnostics.
- Define the minimal accepted predicate for a single explicit register-source
  to stack-destination value move, if the facts are coherent.
- Define fail-closed variants for missing source authority, missing destination
  authority, unsupported register bank, unsupported width, multi-move
  ambiguity, or producer misclassification.

Completion check:

- `todo.md` names the implementation or reclassification boundary, the exact
  semantic predicate, and the adjacent shapes that must remain rejected.

## Step 3: Materialize Or Reclassify The Single-Move Shape

Goal: Handle the smallest authorized register-source to stack-destination move
without overfitting to the representative row.

Actions:

- If Step 2 proves the single-move shape is authorized, add the narrow RV64
  object-emission path that stores the explicit register source into the
  explicit stack destination.
- If Step 2 proves the shape is not authorized, repair the producer
  classification or diagnostic so the unsupported owner is precise.
- Preserve existing stack-to-stack behavior and all existing accepted move
  shapes.
- Keep multi-move bundles, unsupported banks, unsupported widths, missing
  facts, and contradictory facts fail-closed unless this step explicitly owns
  them.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused representative proof shows the single-move case either advances
  past the old ambiguous gate or reports the correct owner-specific diagnostic.
- `todo.md` records the handled boundary and remaining unsupported shapes.

## Step 4: Add Focused Backend Coverage

Goal: Cover accepted and rejected register-source to stack-destination prepared
move shapes without depending on gcc_torture row names.

Actions:

- Add or extend focused backend object-emission tests for the accepted
  single-move register-source to stack-destination shape, if implemented.
- Add reject coverage for missing authority, malformed destination facts, or
  unsupported adjacent shapes identified in Step 2.
- If the chosen route is diagnostic reclassification, add coverage that proves
  the owner-specific reject path.
- Keep assertions tied to semantic facts and diagnostics, not scan accounting
  or expectation rewrites.

Completion check:

- Focused backend tests pass.
- Reject coverage proves adjacent unsupported cases still fail closed.
- `git diff --check` passes for touched files.

## Step 5: Reassess Multi-Move Bundle Ownership

Goal: Decide whether the `src/20010518-1.c` multi-move bundle belongs in this
idea's remaining scope or needs a separate follow-up.

Actions:

- Rerun focused proof for `src/20010518-1.c` after the single-move boundary is
  settled.
- Inspect whether the multi-move bundle has explicit prepared sequencing and
  enough authority to materialize all moves without dropping or reordering
  sources incorrectly.
- If multi-move support is still a separate parallel-copy or producer
  classification problem, record the follow-up boundary instead of expanding
  this runbook.
- Preserve evidence that no row-name or source-order special case is being
  used.

Completion check:

- `todo.md` records whether the multi-move case is accepted into the current
  runbook, rejected with a precise owner, or split into a new idea.

## Step 6: Validate And Handoff

Goal: Prove the lifecycle slice is stable enough for closure or for a clearly
bounded follow-up.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend subset for the touched RV64 object-emission or
  producer-classification scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation results, residual risks, and whether the
  source idea acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The runbook can be closed, or remaining work is explicitly separated from
  the register-source stack-destination prepared move-bundle scope.
