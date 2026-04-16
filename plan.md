# Prealloc Cpp Rs Convergence And Activation

Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md

## Purpose

Converge the active C++ prealloc pipeline toward the retained Rust reference
implementations so that `stack_layout`, `liveness`, and `regalloc` are both:

- semantically close to their Rust references
- demonstrably active inside the c4c backend pipeline

## Goal

Land the prealloc migration in ordered phases:

1. `stack_layout`
2. `liveness`
3. `regalloc`
4. explicit acceptance and runtime proof

## Core Rule

Do not accept structural similarity without active behavior.

A phase is only considered complete when:

- its C++ implementation materially corresponds to the Rust reference
- the migrated logic is actually exercised in the c4c pipeline
- tests prove the phase changes real prepared/backend behavior

## Read First

- `ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/stack_layout/*.rs`
- `src/backend/prealloc/liveness.rs`

## Current Targets

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/stack_layout/*.cpp`
- `src/backend/prealloc/liveness.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `tests/backend/*prealloc*`
- retained Rust references under `src/backend/prealloc/*.rs`

## Non-Goals

- deleting retained Rust reference files before acceptance comparison is done
- accepting placeholder or no-op phase behavior because builds are green
- proving a migration with one brittle text-shape testcase
- letting object-layer and value-layer identities drift together again

## Working Model

### Layer separation

- `stack_layout` is object/slot oriented
- `liveness` is value oriented
- `regalloc` consumes value-oriented liveness and optional links back to stack
  objects / frame slots

### Contract discipline

- phase input/output contracts are canonical in `src/backend/prealloc/prealloc.hpp`
- object-layer metadata must not be reused as value identity
- value-layer records may link to stack objects, but must not depend on them for
  identity

### Acceptance model

- each phase needs both semantic convergence and runtime proof
- acceptance must include explicit `.cpp` vs `.rs` comparison
- intentional host-framework deviations must be documented as bounded
  divergences, not left implicit

## Execution Rules

- prefer semantic parity work over placeholder scaffolding
- when a contract is missing, fix the public prepared contract before growing
  more implementation around the wrong shape
- do not let `liveness` or `regalloc` use `PreparedStackObject.source_name` as
  value identity
- before treating the migration as ready for autonomous packet execution, make
  these three roads explicit and real:
  - phase contracts are stable and documented in `prealloc.hpp`
  - `regalloc.cpp` is active and non-stub
  - acceptance scaffolding exists for comparison plus runtime proof
- tests should prefer prepared-data or semantic assertions over brittle IR text
  matching where possible
- use `build -> focused backend test -> broader backend checkpoint` as the
  validation ladder for nontrivial phase changes

## Ordered Steps

### Step 1: Lock The Prealloc Phase Contracts

Goal:
Make `prealloc.hpp` the stable execution contract for object-layer stack
layout, value-layer liveness, and regalloc consumption.

Actions:
- audit prepared structs for object/value identity drift
- remove fields that encode the wrong phase boundary
- add fields required for Rust-like liveness and regalloc data flow
- ensure each field is justified by an active consumer or imminent consumer

Completion Check:
- `prealloc.hpp` cleanly separates object-layer and value-layer data
- `stack_layout`, `liveness`, and `regalloc` can be described without ambiguous
  identity rules

### Step 2: Converge `stack_layout`

Goal:
Make the C++ `stack_layout` path materially closer to the retained Rust module
set and remove blocking provisional behavior.

Primary Target:
- `src/backend/prealloc/stack_layout.cpp`
- `src/backend/prealloc/stack_layout/*.cpp`

Actions:
- compare each C++ helper to the corresponding Rust helper
- close major semantic gaps in object discovery, coalescing, and slot
  assignment responsibilities
- keep `.rs` files as references until acceptance comparison is complete
- add focused tests showing stack-layout output is active in prepared data

Completion Check:
- the active C++ stack-layout route matches the Rust phase boundaries well
  enough to drive later phases without placeholder assumptions
- tests prove prepared stack-layout output changes under real function shapes

### Step 3: Converge `liveness`

Goal:
Make `liveness.cpp` follow the Rust analysis model closely enough for real
regalloc consumption.

Primary Target:
- `src/backend/prealloc/liveness.cpp`

Actions:
- keep liveness value-oriented
- implement Rust-like program-point numbering, CFG-aware backward dataflow,
  interval construction, call-point handling, and loop-depth handling
- improve phi behavior so predecessor-edge uses are modeled correctly within the
  current BIR shape
- add focused tests that prove liveness runs on BIR values rather than only on
  stack-layout objects

Completion Check:
- `liveness.cpp` publishes regalloc-usable value data close to the Rust
  reference
- focused tests prove the active path is reached and semantically relevant

### Step 4: Activate And Converge `regalloc`

Goal:
Replace the current stub with an active C++ regalloc path that consumes the
prepared liveness contract.

Primary Target:
- `src/backend/prealloc/regalloc.cpp`

Actions:
- define the minimum active allocator behavior needed to stop being a stub
- consume value intervals, call barriers, loop weighting, and stack links from
  prepared liveness / stack layout
- compare the C++ allocator helpers and outputs to the Rust reference intent
- add tests proving regalloc is active in the prealloc pipeline

Completion Check:
- `regalloc.cpp` is no longer a stub
- regalloc materially consumes prepared liveness output
- tests prove the allocator path is active in c4c

### Step 5: Acceptance And Comparison

Goal:
Finish the migration with explicit one-to-one comparison and runtime proof.

Actions:
- compare each active C++ phase file to its Rust reference partner
- verify helper coverage and major semantic behavior one-to-one
- document bounded intentional divergences where host-framework adaptation is
  required
- run focused migration tests plus a broader backend checkpoint

Completion Check:
- each `.cpp` / `.rs` pair has explicit comparison coverage
- runtime tests prove the migrated phases are active
- the remaining differences are bounded, intentional, and documented

## Validation

### Focused Proof

- `cmake --build /workspaces/c4c/build --target c4c_backend -j4`
- phase-specific backend tests, including prepared-data tests under
  `tests/backend/`

### Broader Checkpoint

- `ctest --test-dir /workspaces/c4c/build --output-on-failure -L 'backend'`

## Done Condition

This runbook is complete only when:

- `stack_layout`, `liveness`, and `regalloc` all have active non-placeholder
  C++ implementations
- each phase has explicit C++ vs Rust comparison coverage
- tests prove the migrated behavior runs and matters inside c4c
