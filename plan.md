# Prealloc Coordinator And Fact Publishers Decomposition Runbook

Status: Active
Source Idea: ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md

## Purpose

Split `src/backend/prealloc/prealloc.cpp` into a small preparation coordinator
plus focused prepared-fact publisher implementation files.

Goal: make `BirPreAlloc::run()` and `publish_contract_plans()` read as phase
coordination while family-specific prepared fact construction lives in files
named for the facts they publish.

Core Rule: this is a behavior-preserving ownership decomposition. Do not change
prepared output, phase order, stack layout, regalloc behavior, ABI semantics,
runtime helper semantics, snapshots, or test expectations.

## Read First

- `ideas/open/273_prealloc_coordinator_and_fact_publishers_decomposition.md`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/module.hpp`
- Existing focused schema headers under `src/backend/prealloc/`
- `CMakeLists.txt` and any nearby backend/prealloc source registration

## Current Targets

- Keep `BirPreAlloc::run()` and high-level publication order in
  `src/backend/prealloc/prealloc.cpp`.
- Extract prepared fact publishers into focused implementation files under
  `src/backend/prealloc/`.
- Add narrow private helper headers only when multiple extracted files
  genuinely need shared private state.
- Preserve the public prealloc API and focused schema header boundaries created
  by the schema-header decomposition.

## Non-Goals

- Do not split `src/backend/prealloc/regalloc.cpp`.
- Do not split `src/backend/prealloc/prepared_printer.cpp`.
- Do not redesign `PreparedBirModule` fields or schema ownership.
- Do not move shared prealloc logic into target-specific AArch64 directories.
- Do not add target capability work while decomposing ownership.
- Do not rewrite tests or prepared dumps to accept changed output.

## Working Model

- `prealloc.cpp` should remain the coordinator and own phase sequencing,
  construction of `BirPreAlloc`, and final module preparation entry points.
- Each extracted file should own one prepared fact family or a tightly coupled
  group named in the source idea.
- Shared helper code should move only when it is required by an extracted
  family boundary and should remain narrow enough that it does not become a
  replacement monolith.
- Prefer small compile-proven packets. After each extraction, run build plus the
  supervisor-selected backend/prealloc subset and keep `test_after.log` as the
  canonical proof log.

## Execution Rules

- Move code mechanically first; make semantic edits only when required to keep
  the same behavior after relocation.
- Preserve publication order in `publish_contract_plans()` unless a later plan
  review explicitly approves a route change.
- Keep `prealloc.cpp` readable after every slice; do not leave large orphaned
  helper clusters behind when their owning family has moved.
- Update build registration in the same slice that adds a new `.cpp` file.
- Run `git diff --check` after code-changing slices.
- Escalate to reviewer scrutiny if a slice changes expected dumps, weakens a
  test, introduces broad helper ownership, or makes the coordinator harder to
  reason about.

## Steps

### Step 1: Inventory current publisher families and helper dependencies

Goal: map the existing `prealloc.cpp` regions into extraction families and
identify dependency edges before moving code.

Primary target: `src/backend/prealloc/prealloc.cpp`.

Concrete actions:

- Inspect the current helper/function clusters in `prealloc.cpp`.
- Group clusters by prepared fact family: frame plan, dynamic stack, storage
  plans, call plans, variadic plans, special carriers, runtime helpers,
  intrinsics, inline asm, atomics, addressing, and remaining coordinator code.
- Note helper dependencies that are shared across more than one family.
- Identify the first low-risk extraction family whose dependencies are mostly
  local.

Completion check:

- `todo.md` records the selected first extraction family, shared-helper
  watchouts, and the intended proof command.
- No implementation files need to change for this inventory-only step unless
  the supervisor delegates an extraction packet immediately.

### Step 2: Extract low-risk standalone fact publishers

Goal: move isolated or mostly self-contained fact publishers out of
`prealloc.cpp` without changing output.

Primary targets may include:

- `src/backend/prealloc/frame_plan.cpp`
- `src/backend/prealloc/dynamic_stack.cpp`
- `src/backend/prealloc/addressing.cpp`
- Any narrow private helper header justified by shared use

Concrete actions:

- Move one family per executor packet unless two families are inseparable.
- Add declarations in the narrowest existing or new private prealloc header.
- Keep `prealloc.cpp` calling the same publisher functions in the same order.
- Register new implementation files in the build.

Completion check:

- The extracted family compiles from its new file.
- Backend/prealloc proof selected by the supervisor passes.
- `git diff --check` passes.
- `prealloc.cpp` no longer owns the moved family implementation.

### Step 3: Extract storage, call, and variadic publishers

Goal: move ABI-sensitive prepared fact publishers into focused implementation
owners while preserving exact semantics.

Primary targets may include:

- `src/backend/prealloc/storage_plans.cpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/variadic_plans.cpp`

Concrete actions:

- Preserve direct/indirect callee resolution behavior and memory-return logic.
- Preserve call clobber/preserved-value construction and variadic entry facts.
- Use narrow shared helpers only for genuinely common ABI facts.
- Keep storage encoding and value-home dependencies explicit through focused
  schema headers.

Completion check:

- Existing backend tests and prepared dump behavior remain unchanged.
- The coordinator keeps only phase calls and no family-specific ABI logic for
  moved publishers.
- Reviewer reject signals from the source idea are still avoided.

### Step 4: Extract special carriers and runtime helper publishers

Goal: move i128/f128 carriers, atomics, intrinsics, inline asm, and runtime
helper fact publication into focused implementation files.

Primary targets may include:

- `src/backend/prealloc/special_carriers.cpp`
- `src/backend/prealloc/runtime_helpers.cpp`
- `src/backend/prealloc/intrinsics.cpp`
- `src/backend/prealloc/inline_asm.cpp`
- `src/backend/prealloc/atomics.cpp`

Concrete actions:

- Preserve i128/f128 lane, carrier, marshaling, and boundary-policy facts.
- Preserve atomic, intrinsic, and inline-asm validation and missing-fact
  reporting.
- Avoid target-local shortcuts in shared prealloc files.
- Split large helper clusters only along clear prepared fact ownership
  boundaries.

Completion check:

- Backend/prealloc proof passes with unchanged prepared output.
- Runtime helper and carrier ownership is clear from filenames.
- `prealloc.cpp` does not retain validation/build logic for moved carriers.

### Step 5: Reduce `prealloc.cpp` to coordinator ownership

Goal: finish the decomposition so `prealloc.cpp` owns orchestration rather than
family-specific fact construction.

Primary target: `src/backend/prealloc/prealloc.cpp`.

Concrete actions:

- Remove leftover moved-family helpers from `prealloc.cpp`.
- Keep `BirPreAlloc::run()`, `publish_contract_plans()`, notes, and public
  preparation entry points in the coordinator.
- Verify helper headers remain narrow and do not duplicate a monolithic
  implementation surface.
- Confirm future prepared facts have an obvious owner file.

Completion check:

- `prealloc.cpp` reads as the preparation coordinator.
- Major fact-publisher families listed in the source idea have named
  implementation owners.
- No family-specific logic remains in the coordinator except phase sequencing
  and shared error/note plumbing.

### Step 6: Final validation and lifecycle closure check

Goal: prove the decomposition is behavior-preserving and ready for source idea
closure.

Concrete actions:

- Run the supervisor-selected broader validation, at minimum a fresh build plus
  backend/prealloc tests.
- Use regression guard when the supervisor treats the route as a milestone.
- Review the final diff for output changes, broad helper drift, or hidden
  semantic edits.
- If the source idea completion criteria are satisfied, ask the plan owner to
  close the lifecycle state.

Completion check:

- Validation passes.
- Reviewer or supervisor finds no testcase-overfit or ownership-drift issues.
- The source idea can be closed or the remaining scope is recorded in
  `todo.md` for the next packet.
