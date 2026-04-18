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

- replace emitter-local reconstruction of branch meaning with prepared
  branch-condition lookups
- drive join and loop-carried handling from prepared transfer data
- delete or simplify matcher logic that only existed to re-derive control-flow
  semantics
- keep target-specific decisions limited to x86 legality and spelling

Completion check:

- x86 branch/join emission for the covered cases consults prepared control-flow
  data instead of recovering those semantics from CFG shape

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
