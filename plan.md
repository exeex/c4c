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

- drive covered loop-carried handling from prepared transfer data
- finish any residual Step 3 consumer cleanup needed after Step 3.1 through
  Step 3.3 land
- keep countdown-loop handling within prepared-control-flow consumption only;
  do not widen into value-home work, frame/addressing work, or Step 4
  extraction

Completion check:

- the remaining covered loop-carry and adjacent join-consumer paths consult
  prepared control-flow data instead of CFG-shape recovery, and Step 3 can be
  treated as exhausted without hiding unrelated follow-on work inside it

## Step 4: Organize `prepared_module_emit.cpp` For Prepared Control-Flow Consumption

Goal: shrink the emitter's mixed responsibilities so prepared control-flow
consumption can land in stable, reviewable seams instead of one growing file.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- existing x86 codegen translation units that can responsibly own extracted
  helper logic

Actions:

- identify helper groups in `prepared_module_emit.cpp` by responsibility, not
  by testcase or matcher family
- prefer extracting reusable comparison, shared consumer, or local query logic
  into existing x86 codegen `.cpp` files before introducing any new filename
- create a new `.cpp` name only when no current translation unit can own the
  logic without violating responsibility boundaries
- keep branch, join, and loop-carry fact production in shared
  prepare/prealloc code; this step only reorganizes x86-side consumption
- reject cleanup that merely renames or relocates matcher growth without
  reducing emitter-local semantic recovery

Completion check:

- `prepared_module_emit.cpp` has a narrower orchestration role, and the
  extracted logic lives primarily in existing x86 codegen `.cpp` files unless a
  new file was strictly necessary

## Step 5: Validate The Route

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
