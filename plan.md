# X86 Prepared-Module Renderer De-Headerization Runbook

Status: Active
Source Idea: ideas/open/56_x86_prepared_module_renderer_deheaderization.md
Activated from: ideas/open/56_x86_prepared_module_renderer_deheaderization.md

## Purpose

Move the x86 prepared-module renderer implementation out of
`x86_codegen.hpp` so the header returns to being a shared contract surface
before the x86 capability-family route widens again.

## Goal

Land a behavior-preserving de-headerization of the prepared-module renderer,
with `emit.cpp` owning the main orchestration flow and the header reduced to
declarations, shared types, and contract helpers only.

## Core Rule

This runbook is refactor-only. Do not mix new x86 backend capability growth,
expectation changes, or testcase-lane claims into these packets.

## Read First

- [ideas/open/56_x86_prepared_module_renderer_deheaderization.md](/workspaces/c4c/ideas/open/56_x86_prepared_module_renderer_deheaderization.md)
- [review/x86_codegen_header_split_review.md](/workspaces/c4c/review/x86_codegen_header_split_review.md)
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/emit.cpp)
- [src/backend/mir/x86/codegen/memory.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/memory.cpp)
- [src/backend/mir/x86/codegen/calls.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/calls.cpp)
- [src/backend/mir/x86/codegen/comparison.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/comparison.cpp)
- [src/backend/mir/x86/codegen/globals.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/globals.cpp)

## Current Targets

- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/emit.cpp)
- existing x86 codegen owners under `src/backend/mir/x86/codegen/*.cpp`
- [tests/backend/backend_x86_handoff_boundary_test.cpp](/workspaces/c4c/tests/backend/backend_x86_handoff_boundary_test.cpp)
- [tests/backend/backend_lir_to_bir_notes_test.cpp](/workspaces/c4c/tests/backend/backend_lir_to_bir_notes_test.cpp)

## Non-Goals

- claiming x86 backend capability-family progress from this refactor
- broad AArch64 or non-x86 cleanup
- moving implementation by testcase lane name or arbitrary line-count slicing
- leaving `x86_codegen.hpp` as a thin wrapper around another monolithic inline
  blob
- reopening idea `55` instead of continuing through the x86 codegen owner
  surfaces

## Working Model

- `x86_codegen.hpp` should keep shared declarations, shared types, and public
  contract helpers used across sibling x86 translation units.
- `emit.cpp` should own the main `emit_prepared_module(...)` orchestration
  flow.
- Existing `.cpp` owners such as `memory.cpp`, `calls.cpp`, `comparison.cpp`,
  and `globals.cpp` should absorb extracted implementation when ownership is
  already clear.
- New support `.cpp` files are allowed only when an existing owner is not
  coherent.
- Each packet must read as move/extract work first, with behavior preserved and
  proof run afterward.

## Execution Rules

- Preserve the current public API shape while moving implementation.
- Prefer semantic owner files over temporary staging files.
- Keep the first packet centered on de-headerization, not deep sub-splitting.
- Validate every packet as `build -> narrow x86 handoff proof`, and broaden
  only if the moved implementation reaches beyond the handoff surface.
- If execution uncovers a separate initiative, stop and route it through
  lifecycle instead of stretching this refactor plan.

## Step 1. Re-Read The Header Contract

Goal: Confirm which pieces of `x86_codegen.hpp` are true shared contract and
which are renderer implementation.

Primary targets:
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/emit.cpp)
- [review/x86_codegen_header_split_review.md](/workspaces/c4c/review/x86_codegen_header_split_review.md)

Actions:
- identify the exact declaration surface that must remain in the header
- map the current `emit_prepared_module(...)` body to its natural `.cpp`
  owners
- choose the first extraction packet so owned files and proof are clear before
  edits begin

Completion check:
- the next packet names one explicit ownership move for the renderer body and
  leaves the public contract surface unchanged

## Step 2. Move The Prepared-Module Renderer Out Of The Header

Goal: Relocate the main renderer implementation into `.cpp` ownership without
changing behavior.

Primary targets:
- [src/backend/mir/x86/codegen/x86_codegen.hpp](/workspaces/c4c/src/backend/mir/x86/codegen/x86_codegen.hpp)
- [src/backend/mir/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/mir/x86/codegen/emit.cpp)
- any existing sibling x86 codegen owner that naturally absorbs part of the
  move

Actions:
- move the `emit_prepared_module(...)` implementation out of the header
- keep public declarations and shared types coherent in `x86_codegen.hpp`
- place moved implementation under `emit.cpp` and existing semantic owners
  where read-through already justifies them
- keep the packet free of capability-family behavior claims

Completion check:
- the header no longer owns the full renderer body, and the orchestration flow
  lives in `.cpp` ownership with unchanged API shape

## Step 3. Prove The Refactor Packet Narrowly

Goal: Show the de-headerization is behavior preserving on the x86 handoff
surface.

Actions:
- run `cmake --build --preset default`
- run `ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_lir_to_bir_notes)$'`
- capture the exact proving command and result in `todo.md`

Completion check:
- the build passes, the narrow x86 handoff proof passes, and `todo.md` records
  the packet as refactor-only progress

## Step 4. Check Remaining Ownership Pressure

Goal: Decide whether the renderer still has one obvious follow-on split or
whether the next lifecycle return should go back to the x86 umbrella idea.

Actions:
- compare the remaining header and `.cpp` ownership mix against idea `56`
- record the next honest extraction candidate in `todo.md`
- if the header is back to a contract role, leave a clear handoff for the next
  lifecycle checkpoint

Completion check:
- the next packet is described by owner surface, not by testcase lane or
  temporary nickname
