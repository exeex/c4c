# AArch64 Peephole Shard Implementation Redistribution Runbook

Status: Active
Source Idea: ideas/open/269_aarch64_peephole_markdown_shard_implementation_redistribution.md

## Purpose

Activate the AArch64 `peephole.md` shard cleanup as executable work. The route
must reconcile the markdown-only peephole surface into compiled ownership files
without reviving the old text-first optimizer or changing emitted assembly.

## Goal

Create `src/backend/mir/aarch64/codegen/peephole.cpp` and
`src/backend/mir/aarch64/codegen/peephole.hpp` as the current peephole ownership
boundary, wire that boundary into the build if needed, then delete
`src/backend/mir/aarch64/codegen/peephole.md`.

## Core Rule

This is an ownership redistribution, not an optimization project. Preserve
current AArch64 output unless an already-supported behavior is moved without
semantic change.

## Read First

- `ideas/open/269_aarch64_peephole_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/README.md`
- `src/backend/mir/aarch64/codegen/peephole.md`
- `src/backend/CMakeLists.txt`
- Nearby codegen owners such as `asm_emitter`, `machine_printer`, and
  `module_compile` only as needed to find the current output path.

## Current Targets

- `src/backend/mir/aarch64/codegen/peephole.cpp`
- `src/backend/mir/aarch64/codegen/peephole.hpp`
- `src/backend/mir/aarch64/codegen/peephole.md`
- `src/backend/CMakeLists.txt`, if the new translation unit must be compiled
- Existing AArch64 output path owners only where a behavior-preserving boundary
  hook is required.

## Non-Goals

- Do not reintroduce the legacy raw assembly text optimizer wholesale.
- Do not add new peephole optimizations.
- Do not change MIR instruction selection, register allocation, frame layout,
  printer spelling, encoder behavior, or object emission.
- Do not use peephole matching to repair a narrow testcase.
- Do not redistribute `prologue.md`, `variadic.md`, or other markdown shards.

## Working Model

The live route is prepared BIR to compiled AArch64 MIR nodes to assembly text
through `asm_emitter` and the shared MIR printer. The legacy `peephole.md`
describes a historical post-text optimizer and should be treated as a design
archive to reconcile into an explicit compiled boundary. If there is no active
peephole pass, the compiled owner should expose a narrow no-op or deferred
boundary rather than scattering the decision across broader emitters.

## Execution Rules

- Keep ownership decisions inside `peephole.{hpp,cpp}` where possible.
- Keep `asm_emitter`, `machine_printer`, and `backend.cpp` from becoming hidden
  peephole owners.
- Prefer a minimal no-op/deferred API if the current backend has no supported
  peephole behavior.
- Preserve assembly output and existing test contracts.
- Delete `peephole.md` only after the compiled owner records the relevant
  current boundary.
- Run fresh build or focused compile proof for each code-changing packet.
- Use broader AArch64 backend proof before accepting deletion if any integration
  point touches the assembly output path.

## Step 1: Establish The Current Peephole Boundary

Goal: determine whether the current AArch64 route has live peephole behavior,
no peephole behavior, or an explicit deferred need.

Primary target: `src/backend/mir/aarch64/codegen/peephole.md`

Actions:

- Inspect the live output path through `module_compile`, `asm_emitter`, and
  `machine_printer`.
- Search for any existing peephole helper or post-output cleanup in compiled
  code.
- Decide whether `peephole.{hpp,cpp}` should expose a no-op pass, a deferred
  marker, or behavior-preserving ownership for already-live cleanup.
- Do not change emitted assembly in this step.

Completion check:

- The next implementation step has a concrete API shape for
  `peephole.{hpp,cpp}` and a clear answer for whether integration is no-op,
  deferred, or behavior-preserving movement.

## Step 2: Add The Compiled Peephole Owner

Goal: create the compiled peephole owner and make the build recognize it.

Primary targets:

- `src/backend/mir/aarch64/codegen/peephole.hpp`
- `src/backend/mir/aarch64/codegen/peephole.cpp`
- `src/backend/CMakeLists.txt`

Actions:

- Add `peephole.hpp` and `peephole.cpp` using existing namespace and codegen
  owner conventions.
- If there is no active pass, make the API visibly no-op or deferred and
  document the boundary in code comments only where needed.
- Add the new translation unit to `src/backend/CMakeLists.txt` if required.
- Avoid broad includes or dependencies that imply text rewriting support that
  does not exist.

Completion check:

- The project builds or at least compiles the backend target with the new files.
- No assembly output behavior is intentionally changed.

## Step 3: Route Or Record Integration

Goal: ensure the compiled boundary is not orphaned if the current output path
needs an explicit hook, while keeping behavior unchanged.

Primary targets:

- `src/backend/mir/aarch64/codegen/asm_emitter.*`
- `src/backend/mir/aarch64/codegen/machine_printer.*`
- `src/backend/mir/aarch64/codegen/module_compile.*`
- `src/backend/mir/aarch64/codegen/peephole.*`

Actions:

- If the chosen boundary is a no-op pass that belongs in the assembly output
  path, route through it at the narrowest existing assembly-text boundary.
- If integration is not warranted, keep the deferred/no-op contract explicit in
  `peephole.{hpp,cpp}` and do not touch broader owners.
- Verify no peephole behavior is hidden in printer or emitter code.

Completion check:

- The ownership boundary is reviewable from compiled code.
- Any integration point is behavior-preserving and narrower than the broad
  backend driver.

## Step 4: Delete The Markdown Shard

Goal: complete the redistribution by removing the markdown-only shard.

Primary target: `src/backend/mir/aarch64/codegen/peephole.md`

Actions:

- Delete `peephole.md` after the compiled owner preserves the needed boundary.
- Confirm no live documentation or build reference still points at the deleted
  shard as an active artifact.
- Do not delete unrelated markdown shards.

Completion check:

- `peephole.md` is gone.
- `peephole.cpp` and `peephole.hpp` are the discoverable owner boundary.

## Step 5: Focused Proof And Acceptance Check

Goal: prove the redistribution did not alter behavior or create an overfit
route.

Actions:

- Run a fresh build or compile command covering the backend after code changes.
- Run focused AArch64 backend proof chosen by the supervisor.
- If output-path integration changed, compare relevant generated assembly or
  run the matching regression subset before acceptance.
- Check the diff for reviewer reject signals from the source idea.

Completion check:

- Build or compile proof is green.
- Focused backend proof is green or any failure is documented as pre-existing
  by matching before/after logs.
- No tests or expectations were weakened.
