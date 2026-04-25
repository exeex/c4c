# BIR Memory Intrinsic Family Extraction Runbook

Status: Active
Source Idea: ideas/open/05_bir-memory-intrinsic-family-extraction.md

## Purpose

Extract the memcpy/memset runtime memory intrinsic family from
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` into
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp` without changing lowering
behavior or public/private declaration shape.

Goal: make memory intrinsic implementation placement semantic while preserving
the existing `BirFunctionLowerer` member-function API and BIR output.

## Core Rule

This is a behavior-preserving file extraction. Do not rewrite expectations,
change diagnostics, redesign memory lowering, add headers, or convert the moved
functions away from `BirFunctionLowerer` members.

## Read First

- `ideas/open/05_bir-memory-intrinsic-family-extraction.md`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`, only if moved code
  currently depends on helper declarations there

## Current Targets

Move this family out of `local_slots.cpp`:

- `try_lower_immediate_local_memset`
- `try_lower_immediate_local_memcpy`
- `lower_memory_memcpy_inst`
- `lower_memory_memset_inst`
- `try_lower_direct_memory_intrinsic_call`

Move helper structs only when they are exclusively used by the moved memcpy or
memset lowering family. Keep local-slot-only helpers in `local_slots.cpp`.

## Non-Goals

- Do not add new `.hpp` files.
- Do not move declarations out of `src/backend/bir/lir_to_bir/lowering.hpp`.
- Do not introduce `MemoryLoweringState`.
- Do not split load/store or GEP behavior.
- Do not move state ownership out of `BirFunctionLowerer`.
- Do not add testcase-shaped special cases or expectation rewrites.

## Working Model

`lowering.hpp` remains the complete private index for
`BirFunctionLowerer` member functions. The new implementation file should
include `../lowering.hpp`, and should include `memory_helpers.hpp` only if the
moved code needs it after extraction.

## Execution Rules

- Preserve exact member definitions and helper behavior while moving code.
- Prefer moving existing code over copying and reimplementing logic.
- Keep include changes minimal and local to the affected `.cpp` files.
- Treat any needed CMake/source-list update as part of the extraction step,
  not as a semantic backend change.
- Prove each code-changing step with a fresh build or narrower test command.

## Ordered Steps

### Step 1: Map the Intrinsic Family Boundary

Goal: identify the exact declarations, definitions, helper structs, anonymous
helpers, includes, and build registration needed for a clean extraction.

Primary target:
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`

Actions:

- Inspect the five target member definitions and their direct helper
  dependencies.
- Classify helper structs and local utilities as intrinsic-only or local-slot
  shared.
- Inspect how memory `.cpp` files are registered in the build.
- Identify the narrow tests that cover memcpy, memset, and direct runtime
  memory intrinsic lowering.

Completion check:

- The executor can name the exact code blocks to move, the helpers that stay,
  any required include/build-file edits, and the proof command to run after the
  move.

### Step 2: Extract `memory/intrinsics.cpp`

Goal: move the memory intrinsic family into its own implementation file without
changing behavior or declarations.

Primary target:
`src/backend/bir/lir_to_bir/memory/intrinsics.cpp`

Actions:

- Create `src/backend/bir/lir_to_bir/memory/intrinsics.cpp`.
- Move only the target family and intrinsic-only helper structs from
  `local_slots.cpp`.
- Keep declarations in `src/backend/bir/lir_to_bir/lowering.hpp`.
- Add only the includes required by the moved definitions.
- Register the new `.cpp` in the existing build source list if needed.
- Leave local-slot-only helpers and local slot lowering behavior in
  `local_slots.cpp`.

Completion check:

- `intrinsics.cpp` owns the five target definitions.
- `local_slots.cpp` no longer contains those definitions.
- No new headers exist.
- The moved definitions remain `BirFunctionLowerer::...` members.
- `c4c_codegen` builds.

### Step 3: Prove No Behavior Change

Goal: validate that the extraction did not alter BIR lowering behavior.

Primary target:
the relevant BIR/LIR-to-BIR tests covering memcpy, memset, and runtime memory
intrinsic lowering.

Actions:

- Run the narrow proof identified in Step 1 after the extraction.
- If the narrow proof exposes a compile or linkage issue, fix the extraction
  boundaries without changing testcase expectations.
- If the blast radius looks wider than file placement and build registration,
  escalate for supervisor/reviewer route scrutiny instead of broadening this
  idea.

Completion check:

- `c4c_codegen` builds.
- Relevant memcpy/memset/BIR memory lowering tests pass.
- No expectation rewrites are part of the accepted diff.

### Step 4: Final Acceptance Check

Goal: confirm the active idea's acceptance criteria are met and the slice is
ready for supervisor validation and commit.

Actions:

- Recheck that `local_slots.cpp` does not contain the moved intrinsic family.
- Recheck that `lowering.hpp` still declares the member functions explicitly.
- Recheck that no new `.hpp` files were added.
- Recheck the diff for semantic lowering changes or testcase overfit.
- Leave final broader validation choice to the supervisor.

Completion check:

- The extraction satisfies the source idea acceptance criteria.
- `todo.md` records the latest proof command and result.
- The supervisor can decide whether to run broader validation or close the
  idea.
