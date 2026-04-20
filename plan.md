# Stack Frame And Addressing Consumption For Prepared X86

Status: Active
Source Idea: ideas/open/61_stack_frame_and_addressing_consumption.md
Activated from: ideas/closed/60_prepared_value_location_consumption.md

## Purpose

Turn idea 61 into an execution runbook that makes prepared frame and address
facts authoritative before x86 emission.

## Goal

Make shared prepare ownership publish canonical frame layout and memory-access
addressing facts so x86 consumes prepared frame/address data instead of
rebuilding local-slot offsets and provenance locally.

## Core Rule

Do not grow new x86-local local-slot layout rebuilders or testcase-shaped
address matchers. Frame and addressing meaning must be produced in shared
prepare code and consumed by x86 as data.

## Read First

- `ideas/open/61_stack_frame_and_addressing_consumption.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

## Scope

- define consumer-oriented prepared frame and addressing data
- produce frame-size, frame-slot, and memory-access facts in shared prepare
- teach x86 to consume those facts for prologue/epilogue and load/store routes
- prove the route with build plus narrow backend/x86 coverage, then broaden as
  needed

## Non-Goals

- reactivating idea 58 control-flow ownership work
- reopening closed idea 60 value-home or move-bundle work
- activating idea 59 generic scalar instruction selection
- extending prepared ownership to direct `CallInst` pointer-argument symbol
  materialization that is not already represented as prepared memory/address
  data
- introducing new x86 matcher families whose practical scope is one testcase

## Working Model

- shared prepare owns frame size, frame-slot identity, and address provenance
- prepared addressing data is keyed by function, block, and instruction
- the current idea covers frame/layout and memory-address consumers, not every
  direct symbol spelling that might appear in x86 call-lane setup
- x86 may reject unsupported operand forms, but it must not rediscover local
  slot offsets or memory provenance when prepared data exists
- prior idea 61 work already moved direct frame-slot and same-module global
  guard/helper consumers toward prepared addressing; reactivation resumes the
  remaining Step 3.2 string-backed and residual direct-symbol memory-access
  lanes

## Execution Rules

- prefer prepared addressing lookup helpers over x86-local slot-name analysis
- keep packet boundaries small enough for `todo.md` tracking
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for each code slice
- escalate to broader validation when shared prepare helpers or x86 memory
  consumers change across multiple families

## Step 1: Lock The Prepared Addressing Contract

Goal: define the data and lookup surface that make frame layout and address
meaning explicit to x86.

Primary targets:

- `src/backend/prealloc/prealloc.hpp`

Actions:

- add or refine prepared frame/addressing enums and structs for frame-slot,
  global-symbol, pointer-value, and string-constant access bases
- expose function-level addressing records that can answer frame size,
  alignment, and per-instruction access questions
- add consumer lookup helpers for prepared addressing by function, block, and
  instruction identity
- keep names flexible, but preserve the source-idea semantics and invariants

Completion check:

- the header exposes a consumer-oriented prepared addressing contract that can
  answer frame and memory-access questions without emitter-local slot rebuilds

## Step 2: Produce Frame And Addressing Facts In Shared Prepare

Goal: populate the prepared addressing contract from shared prepare ownership.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- related shared prepare helpers if extraction is needed

Actions:

- build prepared frame-size and alignment facts from the canonical stack layout
- classify direct frame-slot, global-symbol, string-constant, and
  pointer-indirect memory accesses into prepared addressing records
- do not widen this producer step into new call-argument symbol payloads unless
  the linked source idea is revised to cover that ownership explicitly
- keep address provenance ownership in shared prepare rather than x86-local
  helper paths
- ensure the producer path records enough information for later consumer
  lookups by function, block label, and instruction index

Completion check:

- prepared modules carry frame and addressing facts that cover ordinary
  prologue/epilogue and load/store consumption without x86-local slot analysis

## Step 3: Consume Prepared Addressing In X86

Goal: remove x86 dependence on private frame-slot and address reconstruction
for the covered routes.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- execute this step through the ordered substeps below rather than treating all
  frame/address consumption as one undifferentiated packet stream
- keep target-specific decisions limited to x86 legality and spelling
- do not widen this step into idea 60 value-home work, idea 59 instruction
  selection, or unrelated control-flow work
- delete or simplify x86-local layout/address helpers only when the prepared
  consumer path makes the emitter-local reconstruction unnecessary

Completion check:

- Step 3.1 through Step 3.3 are all complete
- the covered x86 prologue/epilogue and load/store paths consult prepared
  frame/address data instead of rebuilding slot layout or provenance locally

### Step 3.1: Frame Layout Consumer Migration

Goal: move prologue/epilogue and direct frame-size consumers onto prepared
frame data.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- add or refine one shared frame-layout lookup/helper path in the x86 prepared
  consumer
- route covered prologue/epilogue emission through prepared frame size and
  alignment facts instead of x86-local layout recomputation
- keep this substep focused on frame-layout consumption, not memory-access
  breadth

Completion check:

- the covered x86 frame-layout paths consume prepared frame facts as the
  authoritative source of stack size and alignment

### Step 3.2: Direct Frame And Symbol Access Consumption

Goal: move direct frame-slot, global-symbol, and string-constant access lanes
onto prepared addressing lookups.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- focused backend/x86 proof coverage that matches the changed route

Actions:

- add one shared prepared-address lookup/helper path in the x86 consumer
- begin consuming canonical prepared addressing for direct frame-slot loads and
  stores, plus symbol-backed accesses where the same contract applies
- finish the remaining string-backed and residual direct-symbol consumer lanes
  that were still open when idea 61 was deactivated, but only when those lanes
  already correspond to prepared memory/address ownership
- treat bounded call-lane pointer-argument materialization that still inspects
  raw `@name` spellings as out of scope for this step unless a separate
  lifecycle change expands shared prepare ownership for that family
- cover adjacent bounded local/global memory cases through prepared addressing
  ownership rather than local slot-name or byte-offset reconstruction

Completion check:

- the covered x86 direct frame and symbol-backed memory-access paths consume
  prepared addressing data without emitter-local slot analysis

### Step 3.3: Pointer-Indirect And Residual Address Cleanup

Goal: finish the remaining prepared-address consumer paths without widening
scope beyond frame/address consumption.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- focused backend/x86 proof coverage that matches the changed route

Actions:

- finish pointer-value-based addressing and residual base-plus-offset consumer
  paths through prepared addressing lookups
- remove or isolate any remaining x86-local local-slot root/suffix rebuilders
  once prepared addressing covers the matched family
- stop the step when the remaining x86 ownership is target legality rather
  than semantic address recovery

Completion check:

- the remaining covered pointer-indirect and residual address lanes consult
  prepared frame/address data instead of local slot or provenance recovery,
  and Step 3 can be treated as exhausted

## Step 4: Validate The Route

Goal: prove the prepared frame/address boundary without relying on one named
testcase.

Actions:

- require a fresh build for every accepted slice
- choose the narrowest proving test that exercises the changed memory/address
  family
- broaden validation when shared prepare changes affect multiple backend
  buckets or when several narrow-only packets have landed
- reject slices whose main effect is expectation weakening or local matcher
  growth instead of prepared-address consumption

Completion check:

- accepted slices have fresh proof logs and validation proportional to the
  frame/addressing blast radius
