# BIR Global Initializer Family Extraction Runbook

Status: Active
Source Idea: ideas/open/10_bir-global-initializer-family-extraction.md

## Purpose

Reduce `src/backend/bir/lir_to_bir/globals.cpp` pressure by extracting the
global initializer parsing/lowering family into an implementation-only file
without changing global lowering behavior.

## Goal

Create `src/backend/bir/lir_to_bir/global_initializers.cpp` as the owner of
global initializer helpers while keeping `globals.cpp` responsible for global
entry behavior and global address metadata behavior.

## Core Rule

This is a mechanical ownership extraction. Preserve semantics, do not rewrite
test expectations, do not add new headers, and keep `lowering.hpp` as the
complete private declaration index for `lir_to_bir_detail`.

## Read First

- `ideas/open/10_bir-global-initializer-family-extraction.md`
- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- the build registration for files under `src/backend/bir/lir_to_bir/`

## Current Targets

- Add `src/backend/bir/lir_to_bir/global_initializers.cpp`.
- Move initializer parsing/lowering helpers out of `globals.cpp`.
- Keep existing declarations reachable through `lowering.hpp`.
- Update build wiring only as needed for the new `.cpp` file.

## Non-Goals

- Do not redesign global initializer semantics.
- Do not split all global lowering in this pass.
- Do not move `lower_string_constant_global` unless it is proven required for
  the initializer extraction.
- Do not move `lower_minimal_global` unless only a tiny wrapper is unavoidable.
- Do not move `resolve_known_global_address`.
- Do not introduce a global-lowering state owner.
- Do not add `global_initializers.hpp`, `globals.hpp`, or any other new header.
- Do not convert initializer helpers into whole-lowerer free functions when
  they are already argument-driven.

## Working Model

`globals.cpp` should remain focused on global entry behavior, known global
address metadata, string constant global lowering, and minimal global lowering.
`global_initializers.cpp` should own initializer parsing and lowering helpers,
including LLVM byte strings, scalar initializers, integer array initializers,
aggregate initializers, global symbol/address initializers, and GEP
initializers.

## Execution Rules

- Keep each move behavior-preserving and compile after each coherent packet.
- Prefer moving helper groups intact over rewriting their internals.
- Keep helper structs/functions with the initializer routines when they are
  used only by those routines.
- Leave global type parsing in `globals.cpp` unless a parser is exclusively
  tied to moved initializer helpers.
- Treat expectation rewrites or testcase-specific shortcuts as route drift.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different canonical proof artifact.

## Ordered Steps

### Step 1: Map Initializer Ownership and Move Boundary

Goal: identify the exact helper family to move and the helpers that must stay
in `globals.cpp`.

Primary target: `src/backend/bir/lir_to_bir/globals.cpp`

Actions:

- Inspect the current helper declarations and call graph around:
  - `lower_llvm_byte_string_initializer`
  - `parse_global_symbol_initializer`
  - `parse_global_gep_initializer`
  - `parse_global_address_initializer`
  - `lower_global_initializer`
  - `lower_integer_array_initializer`
  - `lower_aggregate_initializer`
- Identify small helper structs/functions used only by those initializer
  routines.
- Confirm that `lower_minimal_global`, `lower_string_constant_global`, and
  `resolve_known_global_address` can remain in `globals.cpp`.
- Note any global type parser that is exclusively tied to moved initializer
  helpers before deciding whether to move it.

Completion check:

- The executor can name the move set, the stay set, and any dependency that
  needs a declaration in `lowering.hpp`.

### Step 2: Add Implementation File and Build Wiring

Goal: create the implementation-only extraction target and make it part of the
existing backend build.

Primary targets:

- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- the build registration that lists lir-to-bir implementation files

Actions:

- Add `global_initializers.cpp` under `src/backend/bir/lir_to_bir/`.
- Include the same private headers needed by the moved initializer helpers.
- Register the new file in the existing build source list.
- Do not add any new `.hpp` file.

Completion check:

- `global_initializers.cpp` exists, is compiled by the build, and no new header
  file was introduced.

### Step 3: Move Initializer Parsing Helpers

Goal: move global symbol, address, and GEP initializer parsing into
`global_initializers.cpp` without changing their behavior.

Primary targets:

- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Move `parse_global_symbol_initializer`.
- Move `parse_global_gep_initializer`.
- Move `parse_global_address_initializer`.
- Move helper structs/functions used only by these parsers.
- Keep declarations in `lowering.hpp` when cross-file linkage requires them.
- Keep argument-driven helper shape where it already exists.

Completion check:

- The parser helpers are no longer implemented in `globals.cpp`.
- `globals.cpp` still compiles against the moved helpers through the private
  declaration index.

### Step 4: Move Initializer Lowering Helpers

Goal: move scalar, byte string, integer array, and aggregate initializer
lowering into `global_initializers.cpp`.

Primary targets:

- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`

Actions:

- Move `lower_llvm_byte_string_initializer`.
- Move `lower_global_initializer`.
- Move `lower_integer_array_initializer`.
- Move `lower_aggregate_initializer`.
- Move helper structs/functions used only by these lowerers.
- Leave global entry behavior in `globals.cpp`.

Completion check:

- `global_initializers.cpp` owns the global initializer lowering family.
- `globals.cpp` still owns global entry behavior and global address metadata
  behavior.

### Step 5: Prove No Behavior Change

Goal: validate the extraction without expectation rewrites.

Primary target: existing build and relevant BIR/LIR-to-BIR global tests

Actions:

- Build `c4c_codegen`.
- Run relevant global initializer coverage for scalar globals,
  string/byte initializers, aggregate initializers, pointer/global address
  initializers, and GEP initializers.
- Escalate to a broader backend or full CTest run if the move touches shared
  lowering contracts or multiple narrow packets land before broader proof.

Completion check:

- `c4c_codegen` builds.
- Relevant global initializer tests pass.
- No expectation rewrites were made.
- No new headers were added.
