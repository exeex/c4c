# BIR Memory Header Vocabulary Extraction Runbook

Status: Active
Source Idea: ideas/open/01_bir-memory-header-vocabulary-extraction.md

## Purpose

Extract the fixed memory vocabulary headers for `src/backend/bir/lir_to_bir/memory/`
without changing BIR lowering behavior.

## Goal

Create exactly `memory_types.hpp` and `memory_helpers.hpp`, keep
`lowering.hpp` as the complete private lowerer index, and preserve current BIR
output, diagnostics, and testcase contracts.

## Core Rule

This is a behavior-preserving header/vocabulary extraction. Do not redesign
memory lowering semantics, split `coordinator.cpp`, introduce
`MemoryLoweringState`, or add testcase-shaped shortcuts.

## Read First

- `ideas/open/01_bir-memory-header-vocabulary-extraction.md`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/addressing.cpp`
- `src/backend/bir/lir_to_bir/memory/provenance.cpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp`

## Scope

- Add only:
  - `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
  - `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- Move memory data vocabulary and shared helper declarations into those headers.
- Let `lowering.hpp` include the new headers and remain the complete
  `BirFunctionLowerer` private index.
- Keep `BirFunctionLowerer` as the direct owner of memory lowering state.

## Non-Goals

- Do not create per-implementation headers such as `addressing.hpp`,
  `provenance.hpp`, `local_slots.hpp`, `coordinator.hpp`, or
  `memory_state.hpp`.
- Do not move implementation control flow out of `coordinator.cpp`.
- Do not introduce a separate memory state owner.
- Do not optimize, redesign, or intentionally change memory lowering behavior.
- Do not rewrite testcase expectations.

## Working Model

`lowering.hpp` remains the LLM-friendly full private index for the LIR-to-BIR
lowerer. The memory directory gains two local vocabulary headers:

- `memory/memory_types.hpp` for memory-specific data structs, hashes, and map
  aliases.
- `memory/memory_helpers.hpp` for shared pure helper declarations around layout
  walking, byte-offset projection, scalar subobjects, and related extent lookup.

The implementation files may include the new headers, but state ownership and
method declaration visibility stay centered in `BirFunctionLowerer`.

## Execution Rules

- Keep each step behavior-preserving.
- Compile after every code-changing step with
  `cmake --build --preset default --target c4c_codegen`.
- Run relevant BIR/LIR-to-BIR tests before close readiness.
- Do not add any `.hpp` file beyond the two allowed memory headers.
- Keep helper APIs explicit and argument-driven where possible.
- Leave genuinely stateful or caller-specific policy in the current
  implementation file.

## Ordered Steps

### Step 1: Create Memory Types Header

Goal: add `memory/memory_types.hpp` and move memory-specific data vocabulary out
of the lowerer surface where that does not change ownership or behavior.

Primary targets:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- memory implementation files that use the moved vocabulary

Actions:

- Add `memory_types.hpp` with the needed includes, namespace, and declarations.
- Move memory-specific address structs, hash structs, dynamic access structs,
  aggregate slot structs, and related aliases listed in the source idea.
- Keep names direct and readable; avoid deep namespace nesting.
- Include `memory_types.hpp` from `lowering.hpp`.
- Preserve compatibility aliases inside `BirFunctionLowerer` only where they
  reduce churn without hiding ownership.

Completion check:

- `c4c_codegen` builds.
- `lowering.hpp` still shows `BirFunctionLowerer` state ownership directly.
- No behavior or testcase expectation changes are made.

### Step 2: Move Memory Map Aliases

Goal: complete the memory data vocabulary extraction by moving memory map and
set aliases into `memory_types.hpp`.

Primary targets:

- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_types.hpp`
- memory implementation files that use memory maps or sets

Actions:

- Move the memory map aliases named in the source idea when they are part of
  the memory data vocabulary.
- Keep `BirFunctionLowerer` fields directly owned by the lowerer, using the new
  aliases from `memory_types.hpp`.
- Avoid moving non-memory or lowerer-wide aliases into the memory header.

Completion check:

- `c4c_codegen` builds.
- Memory map vocabulary is available from `memory_types.hpp`.
- No extra headers are introduced.

### Step 3: Create Memory Helpers Header

Goal: add `memory/memory_helpers.hpp` as the shared helper declaration surface
for pure memory layout/projection operations.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `addressing.cpp`, `provenance.cpp`, and `local_slots.cpp`

Actions:

- Add `memory_helpers.hpp` with declarations for shared helper vocabulary that
  is reused by multiple memory files.
- Include only the types needed for helper signatures.
- Prefer explicit arguments over access to mutable `BirFunctionLowerer` state.
- Keep state-mutating or caller-policy helpers in implementation files.

Completion check:

- `c4c_codegen` builds.
- `lowering.hpp` includes the helper header while remaining the complete
  private lowerer index.
- No implementation behavior changes are made.

### Step 4: Consolidate Shared Layout Projection Helpers

Goal: move or consolidate repeated pure helper logic behind
`memory_helpers.hpp` where semantics match.

Primary targets:

- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `addressing.cpp`
- `provenance.cpp`
- `local_slots.cpp`

Actions:

- Consolidate shared aggregate layout walking.
- Consolidate byte-offset projection through scalar, array, and struct layouts.
- Consolidate scalar leaf/subobject lookup where callers ask the same layout
  question.
- Consolidate byte-storage reinterpretation checks when semantics match.
- Consolidate repeated aggregate extent lookup and pointer-array
  length-at-offset lookup where traversal is equivalent.

Completion check:

- `c4c_codegen` builds.
- Shared helpers are reused by the relevant memory files.
- Caller-specific policy remains at the call sites.

### Step 5: Validate Header Budget and Behavior

Goal: prove the extraction satisfies the source idea without broadening scope.

Actions:

- Confirm exactly the two allowed new headers exist under
  `src/backend/bir/lir_to_bir/memory/`.
- Confirm no per-implementation memory headers were introduced.
- Confirm `lowering.hpp` remains the complete private index and direct state
  owner for `BirFunctionLowerer`.
- Run the relevant BIR/LIR-to-BIR test subset after the build proof.
- Inspect the diff for expectation rewrites or semantic control-flow changes.

Completion check:

- `c4c_codegen` builds.
- Relevant BIR/LIR-to-BIR tests pass.
- No testcase expectations are weakened or rewritten.
- The source idea acceptance criteria are satisfied.
