# Shared CFG, Branch, Join, And Loop Materialization Before X86 Emission

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md

## Purpose

Turn idea 58 into an execution runbook that strengthens prepared control-flow
ownership before x86 emission.

## Goal

Make shared lowering authoritative for branch conditions, join transfers, and
loop-carried values so x86 consumes prepared control-flow facts instead of
recovering meaning from CFG shape.

## Core Rule

Do not grow x86 whole-function matchers or testcase-shaped shortcuts. Semantic
control-flow meaning must be produced in shared prepare/prealloc code and
consumed by x86 as data.

## Read First

- `ideas/open/58_bir_cfg_and_join_materialization_for_x86.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

## Scope

- strengthen the prepared control-flow data model
- produce branch-condition and join-transfer facts in shared lowering
- teach x86 to consume those facts directly
- prove the route with build plus narrow x86/backend coverage, then broaden as
  needed

## Non-Goals

- activating idea 59 instruction-selection work
- value-home or move-bundle work from idea 60
- frame or addressing work from idea 61
- adding new x86 `try_*` helpers whose practical scope is one testcase family

## Working Model

- shared lowering owns branch semantics, join semantics, and loop-carry facts
- prepared control-flow data is keyed by function and block identity, not by
  emitter-local pattern recognition
- x86 may reject unsupported operand forms, but it must not rediscover CFG
  meaning when prepared data exists

## Execution Rules

- prefer semantic lowering and consumer lookups over emitter-local inference
- keep packet boundaries small enough for `todo.md` tracking
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for each code slice
- escalate to broader validation when shared control-flow helpers or x86
  consumer paths change across multiple families

## Step 1: Lock The Prepared Control-Flow Contract

Goal: define the data and lookup surfaces that make branch and join meaning
explicit.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`

Actions:

- add or refine prepared control-flow enums and structs for branch conditions,
  edge transfers, and join transfers
- make predecessor/successor identity explicit in transfer records
- expose lookup helpers for prepared branch conditions and join inputs
- keep names flexible, but preserve the source-idea semantics and invariants

Completion check:

- the header exposes a consumer-oriented control-flow contract that can answer
  branch meaning and join-input questions without CFG rediscovery

## Step 2: Produce Control-Flow Facts In Shared Lowering

Goal: populate the contract from shared prepare/prealloc code.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- related shared prepare/prealloc helpers if extraction is needed

Actions:

- build prepared branch conditions from conditional branches
- collect and normalize join transfers, including phi replacement,
  select-materialization joins, and loop-carry traffic
- keep transfer ownership in shared lowering rather than x86-local code
- ensure the producer path records enough information for later consumer
  lookups by function and block label

Completion check:

- prepared modules carry branch and join facts that cover ordinary branch/join
  lowering cases without x86 topology inference

## Step 3: Consume Prepared Control-Flow In X86

Goal: remove x86 dependence on whole-function CFG pattern recovery for the
covered control-flow cases.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

Actions:

- execute this step through the ordered substeps below rather than treating all
  x86 control-flow consumption as one undifferentiated packet stream
- keep target-specific decisions limited to x86 legality and spelling
- do not widen this step into generic instruction selection, idea 60
  value-location work, idea 61 frame/addressing work, or Step 4 emitter-file
  organization
- delete or simplify matcher logic only when the prepared branch/join consumer
  path makes the emitter-local semantic recovery unnecessary

Completion check:

- Step 3.1 through Step 3.4 are all complete
- x86 branch/join emission for the covered cases consults prepared control-flow
  data instead of recovering those semantics from CFG shape

### Step 3.1: Branch-Condition Consumer Lookups

Goal: replace emitter-local reconstruction of branch meaning with prepared
branch-condition lookups.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`

Actions:

- add or refine one shared branch-condition lookup/helper path in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- route covered branch emission through prepared condition facts instead of
  local CFG-shape recovery
- keep this substep focused on authoritative condition consumption, not join
  materialization breadth

Completion check:

- the covered x86 branch-emission paths consume prepared branch-condition data
  as the authoritative source of branch meaning

### Step 3.2: Compare-Join Consumer Migration

Goal: move the compare-join consumer path onto prepared branch/join helpers.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Actions:

- migrate `render_materialized_compare_join_if_supported()` and adjacent
  authoritative-join consumer code toward shared prepared lookup helpers
- prove compare-join packets against adjacent route variants rather than one
  named testcase shape
- keep this substep on compare-join consumer semantics; do not widen into
  countdown-loop exceptions or file-organization cleanup

Completion check:

- compare-join emission for the covered routes consults prepared branch/join
-contract data rather than re-deriving meaning from entry compare carriers or
  join-block topology

### Step 3.3: Join-Transfer Carrier Coverage

Goal: consume prepared join-transfer carriers as shared join-contract data in
the x86 consumer.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Actions:

- add one shared join-transfer lookup/helper path in
  `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- begin consuming `PreparedJoinTransferKind::EdgeStoreSlot` where the same
  prepared join-consumer contract applies
- cover selected-value, same-module-global, fixed-offset, and adjacent
  non-pointer-backed or pointer-backed join-carrier variants through prepared
  transfer ownership rather than matcher growth

Completion check:

- the covered x86 join-materialization paths consume prepared join-transfer
  data, including `EdgeStoreSlot` carrier cases, without emitter-local
  semantic recovery

### Step 3.4: Loop-Carry And Residual Consumer Cleanup

Goal: finish the remaining Step 3 control-flow consumer paths without widening
scope beyond prepared-control-flow consumption.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- focused backend/x86 proof coverage that matches the changed route

Actions:

- execute this step through the ordered substeps below rather than treating
  loop-carry and residual cleanup as one undifferentiated packet stream
- keep countdown-specific fallback cleanup isolated to its own substep and do
  not reopen that family once it is exhausted
- keep countdown-loop handling within prepared-control-flow consumption only;
  do not widen into value-home work, frame/addressing work, or Step 4
  extraction

Completion check:

- Step 3.4.1 through Step 3.4.3 are all complete
- the remaining covered loop-carry and adjacent join-consumer paths consult
  prepared control-flow data instead of CFG-shape recovery, and Step 3 can be
  treated as exhausted without hiding unrelated follow-on work inside it

### Step 3.4.1: Countdown Fallback Contract Exhaustion

Goal: close the residual countdown-specific fallback family without letting
authoritative prepared ownership fall back to raw CFG recovery.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `tests/backend/backend_x86_handoff_boundary_test.cpp`

Actions:

- reject residual local-countdown fallback paths once authoritative prepared
  branch ownership exists on the matched countdown blocks
- reject the same fallback family once authoritative prepared join ownership
  references the matched countdown region, not only when
  `PreparedJoinTransferKind::LoopCarry` appears
- stop this substep when the countdown-specific fallback family is exhausted
  rather than mining testcase-shaped countdown variants

Completion check:

- the countdown-specific fallback family no longer reopens raw CFG recovery
  when authoritative prepared ownership exists, and further countdown-only
  work would require widening scope

### Step 3.4.2: Loop-Carry Consumer Lookup Finishing

Goal: finish the remaining loop-carry consumer paths that should consult
prepared transfer ownership directly.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- focused backend/x86 proof coverage that matches the changed route

Actions:

- inspect residual loop-carried transfer handling outside the exhausted
  countdown-specific fallback family
- replace any remaining loop-carry topology recovery with prepared
  join-transfer or branch-condition lookups where the Step 3 contract already
  carries the needed semantics
- keep this substep on consumer-side prepared ownership; do not reopen
  countdown-only matcher cleanup or widen into Step 3.3 carrier expansion

Completion check:

- the remaining covered loop-carry consumer paths outside the exhausted
  countdown-specific fallback family use prepared transfer ownership directly
  instead of countdown-shaped CFG recovery

### Step 3.4.3: Residual Non-Countdown Consumer Cleanup

Goal: close the remaining Step 3 residual consumer paths outside the
countdown-specific fallback family once loop-carry lookups are stable.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- focused backend/x86 proof coverage that matches the changed route

Actions:

- inspect residual branch/join consumer helpers adjacent to Step 3.2 and
  Step 3.3 routes that still rely on emitter-local recovery but are not part
  of the exhausted countdown fallback family
- prove any remaining fixes against adjacent route variants rather than one
  named testcase shape
- if no honest bounded packet remains, treat Step 3.4 as exhausted instead of
  hiding unrelated follow-on work inside residual cleanup

Completion check:

- Step 3.4 can be declared exhausted without reopening the countdown-specific
  fallback family or masking unrelated work as residual cleanup

## Step 4: Organize `prepared_module_emit.cpp` For Prepared Control-Flow Consumption

Goal: shrink the emitter's mixed responsibilities so prepared control-flow
consumption can land in stable, reviewable seams instead of one growing file.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing x86 codegen translation units that can responsibly own extracted
  helper logic

Actions:

- execute this step through the ordered substeps below rather than treating
  all emitter organization as one undifferentiated cleanup stream
- identify helper groups in `prepared_module_emit.cpp` by responsibility, not
  by testcase or matcher family
- prefer extracting reusable comparison, shared consumer, local query, or
  module-wrapper logic into existing x86 codegen `.cpp` files before
  introducing any new filename
- create a new `.cpp` name only when no current translation unit can own the
  logic without violating responsibility boundaries
- keep branch, join, and loop-carry fact production in shared
  prepare/prealloc code; this step only reorganizes x86-side consumption
- reject cleanup that merely renames or relocates matcher growth without
  reducing emitter-local semantic recovery

Completion check:

- Step 4.1 through Step 4.3 are all complete
- `prepared_module_emit.cpp` has a narrower orchestration role, and the
  extracted logic lives primarily in existing x86 codegen `.cpp` files unless a
  new file was strictly necessary

### Step 4.1: Bounded Multi-Defined Call-Lane Wrapper Contraction

Goal: finish shrinking the bounded multi-defined call lane down to a narrow
top-level orchestration surface in `prepared_module_emit.cpp`.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`

Actions:

- keep the bounded multi-defined lane on the active prepared helper surface
  instead of widening ownership into inactive files such as `calls.cpp`
- extract remaining helper-prefix composition, fallback-contract checks,
  entry-lane wrapper code, or adjacent lane-only data plumbing by
  responsibility rather than by testcase shape
- leave prepared control-flow semantics unchanged; this substep is limited to
  x86 emitter organization around the bounded multi-defined route

Completion check:

- the bounded multi-defined lane uses shared helper seams for its remaining
  wrapper/data work, and `prepared_module_emit.cpp` keeps only the minimal
  top-level orchestration needed to select that route

### Step 4.2: Single-Function Entry Orchestration Extraction

Goal: isolate single-function entry orchestration from the surrounding
prepared-module surface without reopening Step 3 control-flow semantics.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing x86 codegen `.cpp` files that already own adjacent prepared helper
  logic

Actions:

- extract entry-routing orchestration, shared render-plan plumbing, or local
  helper groups out of `prepared_module_emit.cpp` when an existing x86 codegen
  seam can own them cleanly
- keep this work on organization of already-supported prepared consumer paths;
  do not hide unfinished producer-side control-flow gaps inside the cleanup
- prefer grouping by one coherent orchestration responsibility at a time
  rather than by one named testcase family

Completion check:

- the covered single-function entry path delegates its remaining helper groups
  through stable seams, and the emitter's central block no longer mixes those
  orchestration details with unrelated top-level dispatch

### Step 4.2.1: Local Arithmetic Guard Wrapper Extraction

Goal: move the remaining local arithmetic guard entry wrappers onto existing
helper seams so `prepared_module_emit.cpp` stops owning those route-specific
entry implementations directly.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/mir/x86/codegen/x86_codegen.hpp`

Actions:

- extract `render_local_i32_arithmetic_guard_if_supported()` onto the active
  local-slot helper seam if that seam can own the wrapper without widening
  into Step 3 semantics or idea 61 addressing work
- keep the moved helper focused on entry-wrapper orchestration and already
  supported guard behavior, not on producer-side capability changes
- prove the packet against the focused x86 handoff-boundary subset already
  used by adjacent Step 4.2 organization packets

Completion check:

- the local i32 arithmetic guard entry wrapper is delegated through a stable
  helper seam outside `prepared_module_emit.cpp`, and the top-level emitter
  keeps only route selection for that family

### Step 4.2.2: Residual Local Arithmetic Guard Exhaustion

Goal: finish the last honest Step 4.2 wrapper extraction work and leave only
Step 4.3-style top-level dispatcher narrowing behind.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing x86 codegen translation units that already own adjacent local guard
  helper logic

Actions:

- extract `render_local_i16_arithmetic_guard_if_supported()` or any tiny
  adjacent arithmetic guard entry wrapper that still belongs to the same
  single-function orchestration seam
- stop Step 4.2 once those residual local arithmetic wrappers no longer live
  directly in `prepared_module_emit.cpp`; do not keep mining cosmetic
  organization packets after that point
- keep this cleanup limited to wrapper ownership and route selection rather
  than reopening Step 3 control-flow work or broadening helper contracts

Completion check:

- the residual local arithmetic guard entry wrappers are owned by stable
  helper seams, and the remaining `prepared_module_emit.cpp` work is honestly
  Step 4.3 dispatcher narrowing rather than more Step 4.2 extraction

### Step 4.3: Residual Prepared-Module Dispatch Narrowing

Goal: finish the remaining top-level emitter cleanup needed before Step 4 can
be treated as exhausted.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing x86 codegen translation units chosen by responsibility

Actions:

- remove or relocate any residual prepared-module dispatch helpers whose main
  job is wrapper composition, route selection, or other top-level
  orchestration rather than semantic lowering
- keep the final narrowing bounded to existing Step 4 responsibilities and do
  not widen into Step 5 test splitting or ideas 59 through 61
- treat Step 4 as exhausted once only a narrow dispatcher remains in
  `prepared_module_emit.cpp`; do not keep landing indefinite cosmetic packets

Completion check:

- `prepared_module_emit.cpp` retains only a compact prepared-module dispatcher
  plus route selection glue, and no honest organization-only Step 4 packet
  remains

## Step 5: Split `backend_x86_handoff_boundary_test.cpp` Into Focused Translation Units

Goal: reduce the test file's mixed responsibilities so backend/x86 handoff
coverage stays reviewable and future packets can target narrower ownership
seams.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- sibling backend test translation units under `tests/backend/` that can
  responsibly own extracted coverage

Actions:

- execute this step through the ordered substeps below rather than treating
  all remaining test splitting as one undifferentiated packet stream
- keep shared fixtures, helpers, and registration mechanics coherent after the
  split
- do not weaken assertions, reclassify supported-path coverage, or use the
  split as cover for testcase-overfit cleanup
- keep this step on test translation-unit organization; do not widen it into
  emitter or semantic lowering changes

Completion check:

- Step 5.1 through Step 5.3 are all complete
- `tests/backend/backend_x86_handoff_boundary_test.cpp` no longer acts as one
  oversized mixed-responsibility translation unit, and the resulting test
  files preserve the existing handoff coverage with clearer ownership seams

### Step 5.1: Extract Broad Semantic Families

Goal: isolate the already-identified semantic coverage families that do not
need to stay attached to the monolithic helper harness.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- focused sibling translation units under `tests/backend/`

Actions:

- keep grouping by semantic coverage family rather than ad hoc testcase order
- preserve the completed compare-branch, joined-branch, short-circuit,
  countdown-loop, guard-chain, multi-defined, and scalar-smoke splits as the
  model for later Step 5 packets
- prefer small duplicated harness subsets inside focused files over a broad
  shared-helper rewrite when the seam is still bounded

Completion check:

- the completed broad semantic family splits remain focused and stable without
  reopening the monolithic file as their owner

### Step 5.2: Split Residual Local-Slot Guard Lane

Goal: remove the remaining local add/sub/address guard family from the
monolithic handoff file while keeping the helper seam bounded.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- a focused sibling translation unit under `tests/backend/`

Actions:

- extract the minimal local-slot add-chain, sub-guard, and byte
  addressed-guard routes into a clearly named focused translation unit
- keep the moved lane self-contained with only the bounded helper subset it
  genuinely needs
- avoid turning this packet into a central helper refactor or any emitter work

Completion check:

- `tests/backend/backend_x86_handoff_boundary_test.cpp` no longer owns the
  residual local add/sub/address guard family

### Step 5.3: Exhaust Residual Monolithic Ownership

Goal: finish Step 5 by deciding whether any coherent semantic lane still needs
  extraction after Step 5.2, or declare the step exhausted once only
  intentionally central harness ownership remains.

Primary targets:

- `tests/backend/backend_x86_handoff_boundary_test.cpp`
- focused sibling translation units under `tests/backend/` only if one honest
  last lane still exists

Actions:

- review the residual monolithic file after Step 5.2 instead of assuming it
  still hides another real family
- extract one final coherent lane only if the remaining ownership is still
  semantic rather than just central harness glue
- stop the step once the remaining monolithic ownership is intentionally
  central and reviewable

Completion check:

- the monolithic handoff file is reduced to intentionally central harness or
  registration ownership, not another hidden semantic family

## Step 6: Validate The Route

Goal: prove the new shared/consumer boundary without relying on a single named
testcase.

Actions:

- require a fresh build for every accepted slice
- choose the narrowest proving test that exercises the changed control-flow
  family
- broaden validation when shared lowering changes affect multiple backend
  buckets or when several narrow-only packets have landed
- reject slices whose main effect is test expectation weakening or
  testcase-specific matcher growth

Completion check:

- accepted slices have fresh proof logs and validation proportional to the
  control-flow blast radius
