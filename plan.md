# RV64 Bankless Or Conversion-Adjacent Stack Slot Moves Runbook

Status: Active
Source Idea: ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
Activated From: post-514 successor after `ideas/closed/514_rv64_register_source_stack_destination_move_bundles.md`

## Purpose

Resolve the next prepared move-bundle boundary left after the stack-to-stack
and register-source move work: stack-slot moves whose prepared storage facts
are bankless or whose nearby scalar types imply a conversion instead of a
same-scalar copy.

## Goal

Make the `src/pr69447.c` bankless or conversion-adjacent prepared stack-slot
move route either publish explicit coherent facts and materialize correctly, or
reject before the generic stack-copy materializer with an owner-specific
diagnostic.

## Core Rule

Consume only explicit prepared move-bundle, home, storage-plan, scalar type,
width, bank, conversion, sequencing, and authority facts. Do not infer a GPR
bank from scalar type alone, and do not treat conversion-adjacent movement as a
raw same-width stack copy without an explicit conversion contract.

## Read First

- ideas/open/515_rv64_bankless_conversion_adjacent_stack_slot_moves.md
- ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md
- ideas/closed/514_rv64_register_source_stack_destination_move_bundles.md
- ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md
- src/backend/mir/riscv/codegen/object_emission.cpp
- src/backend/mir/riscv/
- tests/backend/mir/backend_riscv_object_emission_test.cpp
- tests/backend/

## Current Targets

- Primary representative: `src/pr69447.c`
- Current failure family: bankless or conversion-adjacent prepared stack-slot
  movement
- Owning layer: prepared storage classification and RV64 object emission
- Required authority:
  - prepared move bundle identifies the value movement to emit
  - source and destination homes identify stack slots explicitly
  - storage-plan entries publish a coherent bank when RV64 lowering needs one
  - scalar source and destination facts distinguish same-scalar copies from
    explicit conversions
  - width, sequencing, and authority facts are coherent enough for RV64 object
    emission to act without guessing

## Non-Goals

- Do not reopen idea 513's coherent same-scalar stack-slot to stack-slot
  materializer.
- Do not reopen idea 514's register-source stack-destination boundary.
- Do not reopen idea 516's multi-source producer/classification boundary.
- Do not infer GPR bank from scalar type alone when the prepared storage plan
  says `bank=none`.
- Do not emit a raw stack copy for conversion-adjacent movement unless an
  explicit conversion materialization contract owns the semantics.
- Do not broaden into unrelated integer conversion, ABI, or stack-frame work.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, reason strings, or scan accounting as proof of capability
  progress.

## Working Model

- Idea 513 accepted only coherent same-scalar stack-slot to stack-slot copies
  with explicit prepared home and storage-plan facts.
- Idea 514 closed adjacent register-source stack-destination behavior with
  precise accepted and rejected boundaries.
- Idea 516 closed the multi-source producer/classifier boundary for ambiguous
  non-parallel stack-destination bundles.
- Idea 515 starts from `src/pr69447.c`, where stack homes exist but at least
  one source storage-plan entry reports `frame_slot bank=none`, and the nearby
  scalar shape looks like an `i16` to `i64` conversion rather than the
  same-scalar copy accepted by idea 513.

## Execution Rules

- Start by reproducing current `src/pr69447.c` prepared move, storage-plan,
  type, conversion, and rejection facts, and record them in `todo.md`.
- Keep bankless storage classification and conversion-adjacent movement as
  separate decisions unless evidence proves one contract owns both.
- Keep implementation in RV64 object emission only when producer facts are
  already explicit and coherent. If the producer storage classification is
  wrong or incomplete, stop and record the producer-owned blocker.
- Add focused backend coverage for accepted behavior and adjacent fail-closed
  shapes.
- For code-changing steps, run a fresh build, focused representative proof,
  focused backend coverage, and the relevant backend subset chosen by the
  supervisor.

## Step 1: Rebuild Bankless Conversion Evidence

Goal: Reproduce the current `src/pr69447.c` facts and identify whether the
first owner is bankless storage classification, conversion materialization, or
an already precise rejection path.

Actions:

- Reproduce focused dumps or probes for `src/pr69447.c`.
- Record the prepared move bundle, source home, destination home, storage-plan
  entries, scalar source type, scalar destination type, widths, banks,
  cardinality, authority, and current rejection site.
- Identify whether the move is a same-scalar stack-slot copy candidate,
  bankless stack-slot movement, conversion-adjacent movement, or a malformed
  producer fact.
- Confirm the route is not the coherent stack-to-stack shape accepted by idea
  513, the register-source shape closed by idea 514, or the multi-source shape
  closed by idea 516.

Completion check:

- `todo.md` records the representative facts, the exact current rejection
  site, and the first owner to investigate.

## Step 2: Define The Bankless And Conversion Boundaries

Goal: Decide which facts must be explicit before RV64 may materialize this
family, and which malformed or under-authorized variants must reject.

Actions:

- Trace current prepared storage classification and RV64 materialization in
  `src/backend/mir/riscv/codegen/object_emission.cpp` and neighboring helpers.
- Define the accepted predicate, if any, for a stack-slot move whose storage
  plan publishes coherent banks and same-scalar semantics.
- Define fail-closed diagnostics for `bank=none`, missing storage authority,
  contradictory home/storage facts, and unsupported banks or widths.
- Define whether conversion-adjacent stack-slot movement has enough explicit
  source and destination type facts to become a separate conversion
  materialization contract.
- If the conversion contract is not explicit, define the rejection owner before
  the generic stack-copy materializer.

Completion check:

- `todo.md` names the accepted or rejected semantic boundaries, the predicate
  or diagnostic owner, and adjacent shapes that must remain fail-closed.

## Step 3: Repair Producer Classification Or Rejection

Goal: Ensure bankless prepared frame-slot facts are either published coherently
by the producer or rejected with an owner-specific diagnostic before RV64 stack
copy materialization.

Actions:

- If Step 2 proves `bank=none` is a producer omission, repair the narrow
  storage classification so the explicit bank is published from real authority.
- If Step 2 proves `bank=none` is intentionally unsupported, add or refine the
  precise reject path and diagnostic owner.
- Preserve idea 513 same-scalar stack-slot behavior and idea 514/516 adjacent
  move-bundle boundaries.
- Keep scalar-type-only bank inference rejected.

Completion check:

- A fresh build passes.
- Focused proof shows `src/pr69447.c` no longer reaches the generic
  stack-copy path with incoherent bankless facts.
- `todo.md` records whether the bankless owner was repaired or rejected.

## Step 4: Materialize Or Reject Conversion-Adjacent Movement

Goal: Handle the conversion-adjacent stack-slot movement only if explicit
conversion semantics are available; otherwise reject it before raw stack-copy
lowering.

Actions:

- If Step 2 proves the conversion contract is explicit and narrow, materialize
  the conversion-adjacent stack-slot movement under that contract.
- If Step 2 proves the conversion contract is missing, add or refine the
  owner-specific rejection before the generic stack-copy materializer.
- Preserve all same-scalar, register-source, multi-source, unsupported-bank,
  unsupported-width, and malformed-fact boundaries.
- Avoid broad integer conversion or ABI rewrites outside prepared stack-slot
  movement.

Completion check:

- A fresh build passes.
- Focused proof shows `src/pr69447.c` either advances through explicit
  conversion materialization or reports the correct owner-specific diagnostic.
- `todo.md` records the handled boundary and remaining unsupported shapes.

## Step 5: Add Focused Backend Coverage

Goal: Cover accepted and rejected bankless or conversion-adjacent stack-slot
move behavior without depending on gcc_torture row names.

Actions:

- Add or extend focused backend object-emission tests for any accepted
  bank-published same-scalar stack-slot move or explicit conversion-adjacent
  materialization.
- Add reject coverage for `bank=none`, missing storage authority,
  contradictory home/storage facts, unsupported banks or widths, and missing
  conversion authority.
- Preserve coverage for the idea 513, 514, and 516 boundaries that this route
  must not weaken.
- Keep assertions tied to semantic facts and diagnostics, not scan accounting
  or expectation rewrites.

Completion check:

- Focused backend tests pass.
- Reject coverage proves adjacent unsupported cases still fail closed.
- `git diff --check` passes for touched files.

## Step 6: Validate And Handoff

Goal: Prove the lifecycle slice is stable enough for closure or for a clearly
bounded follow-up.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend subset for touched RV64 object-emission or producer
  classification scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation results, residual risks, and whether the
  source idea acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The runbook can be closed, or remaining work is explicitly separated from
  the bankless or conversion-adjacent stack-slot move scope.
