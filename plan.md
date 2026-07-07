# RV64 Prepared Stack-Destination Move-Bundle Authority Runbook

Status: Active
Source Idea: ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md

## Purpose

Repair or narrow the RV64 object-route prepared move-bundle classifier blocker
reached after floating-point binary lowering advanced the representative route.

## Goal

Make the valid prepared stack-destination move-bundle authority shape lower
semantically, or prove it is invalid with a narrower fail-closed diagnostic.

## Core Rule

Classify and lower the move-bundle authority by storage semantics, not by
`src/20000605-1.c`, `render_image_rgb_a`, temporary names, or a broad bypass of
prepared validation.

## Read First

- `ideas/open/579_rv64_prepared_stack_destination_move_bundle_authority.md`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.log`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/object-route.rc`
- `build/agent_state/574_rv64_floating_point_binary_lowering/step4c/src_20000605-1.c/prepared-focus.txt`
- `src/backend/bir/`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

## Current Targets

- The first prepared move-bundle classifier rejection reached in the 574
  Step 4c representative artifact.
- The move-bundle authority, source values, destination stack slot, and owner
  that make the classifier report
  `ambiguous_non_parallel_multi_source_stack_destination`.
- Focused coverage for the valid supported authority shape and for a genuinely
  ambiguous fail-closed shape.

## Non-Goals

- Do not implement additional FP binary, F128, cast, truncation, runtime
  comparison, pointer, call, inline asm, or select repairs.
- Do not weaken prepared move-bundle validation so invalid stack authority is
  silently accepted.
- Do not change expected runtime output, unsupported markers, allowlists, or
  route classification as proof of progress.
- Do not perform broad prepared-move rewrites beyond the authority shape proven
  by this source idea.

## Working Model

- The representative blocker is currently a prepared consumer classification
  failure, not a `BinaryInst` lowering failure.
- The repair must distinguish a semantically ordered or mutually exclusive
  stack-destination move bundle from a genuinely ambiguous multi-source stack
  destination.
- If the shape is valid, RV64 object emission should materialize the required
  sources and publish/store the destination consistently with prepared
  authority.
- If the shape is invalid, the route should fail closed with a narrower
  diagnostic that explains why this stack-destination authority cannot be
  lowered.

## Execution Rules

- Keep each code-changing step paired with focused backend proof.
- Prefer existing prepared move, publication, and RV64 object-emission helpers
  before adding new abstractions.
- Preserve existing FP binary, integer, pointer, select, inline asm, and call
  behavior.
- Treat diagnostic-only edits as insufficient unless they prove a deliberately
  narrower fail-closed path for an invalid authority shape.
- Keep `todo.md` as the packet scratchpad; do not edit the source idea unless
  source intent changes or a separate follow-up must be recorded.

## Ordered Steps

### Step 1: Reproduce And Localize The Move-Bundle Authority

Goal: Confirm the current prepared move-bundle classifier rejection and identify
the semantic authority shape before implementation.

Primary target: RV64 object route for `src/20000605-1.c`.

Actions:

- Inspect the 574 Step 4c route artifacts named above.
- Reproduce the representative object route or an equivalent focused prepared
  dump if the existing evidence is stale.
- Identify the move bundle, source values, destination storage, owner
  instruction, and classifier branch that reports
  `ambiguous_non_parallel_multi_source_stack_destination`.
- Record in `todo.md` whether the sources appear ordered, mutually exclusive,
  or genuinely ambiguous.

Completion check:

- `todo.md` names the exact move-bundle authority shape and implementation
  surface for Step 2 without changing code.

### Step 2: Add Focused Move-Bundle Authority Coverage

Goal: Pin the desired supported and fail-closed behavior before changing the
classifier or materialization path.

Primary target: `tests/backend/mir/backend_riscv_object_emission_test.cpp`.

Actions:

- Add focused coverage for the supported stack-destination authority shape if
  Step 1 proves it is semantically valid.
- Add or preserve focused coverage for a genuinely ambiguous non-parallel
  multi-source stack-destination shape that must remain fail-closed.
- Keep tests semantic and storage/authority based, not tied to the
  representative filename or value names.

Completion check:

- The focused backend test target exposes the missing supported behavior or
  verifies the intended narrower fail-closed diagnostic contract.

### Step 3: Repair Or Narrow The Stack-Destination Authority Path

Goal: Lower the valid prepared stack-destination move bundle, or fail closed
with a precise diagnostic when the authority is invalid.

Primary target: `src/backend/mir/riscv/codegen/object_emission.cpp`.

Actions:

- Reuse existing prepared move-bundle classification, publication, and RV64
  materialization helpers where possible.
- If the shape is valid, materialize sources and store/publish the stack
  destination in the order required by prepared authority.
- If the shape is invalid, replace the broad ambiguous rejection with a
  narrower diagnostic that names the unsupported authority condition.
- Avoid changing unrelated FP binary, integer, pointer, select, call, or inline
  asm paths.

Completion check:

- Focused backend object-emission tests pass and the implementation does not
  bypass prepared validation or rely on testcase-shaped identifiers.

### Step 4: Repair Producer-Side Authority Or Narrow The Diagnostic

Goal: Address the unchanged representative authority shape that remains after
the consumer-side classifier narrowing, before claiming representative-route
progress.

Primary targets: prepared-BIR producer/publication surfaces under
`src/backend/bir/` and the RV64 prepared consumer/classifier surfaces under
`src/backend/mir/riscv/codegen/`.

Actions:

- Reinspect the Step 4 prepared dump and direct object-route evidence under
  `build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/step4/src_20000605-1.c/`.
- Identify where the non-parallel multi-source stack-destination authority is
  created or preserved: producer publication, prepared move-bundle formation,
  stack destination ownership, or RV64 consumer classification.
- If the representative shape is semantically lowerable, repair the
  producer/consumer authority contract so the prepared bundle records ordered,
  mutually exclusive, select-backed, or otherwise lowerable stack-destination
  semantics instead of reaching the broad ambiguity bucket.
- If the representative shape is genuinely invalid, narrow the diagnostic at
  the producer or earliest authoritative consumer so it explains the specific
  unsupported authority condition, not just the generic
  `ambiguous_non_parallel_multi_source_stack_destination` category.
- Keep the existing genuinely ambiguous fail-closed coverage intact and add or
  adjust focused coverage only for semantic authority behavior, not the
  representative filename or temporary names.

Completion check:

- Focused backend/prepared-object proof shows the producer-side authority
  shape is repaired or the diagnostic is narrowed before the representative
  proof is rerun; the old generic ambiguity remains covered for genuinely
  ambiguous bundles.

### Step 5: Prove The Representative Route

Goal: Verify the 574 representative no longer fails first on the same prepared
move-bundle classifier rejection.

Primary target: RV64 object route for `src/20000605-1.c`.

Actions:

- Rerun the representative object route and save the log under
  `build/agent_state/579_rv64_prepared_stack_destination_move_bundle_authority/`.
- Compare the new result with the 574 Step 4c object-route evidence.
- If the route advances to a later FP cast, runtime mismatch, pointer, call,
  select, or unrelated owner, record that as a distinct later owner in
  `todo.md`.

Completion check:

- The old `ambiguous_non_parallel_multi_source_stack_destination` classifier
  rejection is gone for the representative shape, or the failure is narrowed to
  a specific unsupported stack-destination authority diagnostic.

### Step 6: Backend Guard And Closure Readiness

Goal: Establish that the move-bundle authority slice is acceptance-ready
without regressing nearby backend coverage.

Primary target: backend CTest subset.

Actions:

- Run `cmake --build --preset default`.
- Run the focused backend object-emission test.
- Run the supervisor-selected broader backend subset, expected to be at least
  `ctest --test-dir build -j --output-on-failure -R '^backend_'` before
  closure.
- Write proof results to `test_after.log` when delegated by the supervisor or
  executor protocol.

Completion check:

- Focused and broader backend proof is green, `todo.md` records any follow-up
  owner, and the source idea acceptance criteria can be evaluated for closure.
