# AArch64 Dispatch Publication Owner Relocation Runbook

Status: Active
Source Idea: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md

## Purpose

Relocate remaining target-local publication helpers from the synthetic AArch64
dispatch publication files into the narrow owner files that actually consume
them, while preserving shared prepared-publication authority.

## Goal

Make publication helper ownership match the emitted record or instruction
family without changing diagnostics, machine-record behavior, or prepared fact
authority.

## Core Rule

Move owner-local emission support to the owner that emits the record; keep
shared prepared publication facts in shared prealloc/BIR surfaces and keep true
cross-owner orchestration in `dispatch.cpp`.

## Read First

- `ideas/open/80_aarch64_dispatch_publication_owner_relocation.md`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- Narrow candidate owners as justified by classification:
  `memory.cpp`, `globals.cpp`, `alu.cpp`, `comparison.cpp`, `calls.cpp`, and
  `dispatch.cpp`.

## Current Scope

- Classify every remaining helper in the dispatch publication,
  dispatch value-materialization, and dispatch producer surfaces.
- Relocate pure target-local helpers into narrower AArch64 owner files when
  ownership is clear.
- Preserve prepared-publication and prepared-materialization facts instead of
  recreating them locally.
- Keep behavior-preserving changes small enough for focused build and backend
  test proof after each relocation packet.

## Non-Goals

- Do not invent new shared prepared-publication authority unless a concrete
  missing fact is proven.
- Do not mix edge-copy relocation into this plan.
- Do not rewrite instruction records or machine printer spelling.
- Do not shrink code by weakening tests, deleting diagnostics, or adding
  named-case shortcuts.
- Do not move AArch64 target-local register spelling or record construction
  into shared prealloc/BIR code.

## Working Model

- Helpers that emit or assemble memory publication records belong near memory.
- Helpers that emit global publication records belong near globals.
- Helpers that emit scalar, compare, fixed-formal store, or call-boundary
  records belong near the matching owner when the dependency direction remains
  clean.
- Helpers that only coordinate multiple owners or maintain dispatch route
  ordering may remain in `dispatch.cpp`.
- Shared prepared facts remain shared; owner files consume those facts directly.

## Execution Rules

- Start each implementation packet from the classification table, not from a
  line-count target.
- Keep one owner family per packet unless the move is mechanically inseparable.
- Preserve public header boundaries; delete declarations only after all call
  sites move.
- Keep diagnostics and machine records byte-for-byte equivalent unless an
  intentional formatting or message change is explicitly justified in `todo.md`.
- Run build proof after each relocation packet. Add the focused backend CTest
  subset chosen by the supervisor for the affected publication family.
- Escalate to regression guard before closure if multiple destination owners
  are touched.

## Ordered Steps

### Step 1: Build the helper classification table

Goal: produce a helper-by-helper ownership map before moving code.

Primary targets:

- `dispatch_publication.*`
- `dispatch_value_materialization.*`
- `dispatch_producers.*`
- Candidate destination owners listed in `Read First`

Actions:

- Inventory remaining non-trivial helpers in the three source surfaces.
- Classify each helper as memory, globals, scalar/ALU, comparison, calls,
  current-block join, dispatch orchestration, or still-shared prepared
  authority.
- Record the classification and proposed first relocation packet in `todo.md`.
- Identify the focused build and backend proof command the executor should run
  for the first relocation packet.

Completion check:

- `todo.md` contains a classification summary sufficient for the supervisor to
  delegate a bounded relocation packet.
- No implementation files are changed in this step unless the supervisor
  explicitly delegates the first move in the same packet.

### Step 2: Relocate one narrow owner-local publication family

Goal: move the first clearly classified helper family into its natural AArch64
owner.

Actions:

- Move the helper implementation and any private support helpers into the
  destination owner selected by Step 1.
- Update headers and call sites only as needed for the move.
- Keep prepared-publication facts consumed from existing shared authority.
- Remove dead declarations from the old dispatch publication surface.

Completion check:

- The affected owner builds.
- Focused backend proof for the moved publication family passes.
- `todo.md` records the files changed, proof command, and any remaining helper
  family queued next.

### Step 3: Continue owner-by-owner relocation

Goal: drain the remaining helper families without mixing unrelated owners in a
single packet.

Actions:

- Repeat the Step 2 pattern for memory, globals, scalar/ALU, comparison, calls,
  fixed-formal store, current-block join, or dispatch-orchestration helpers as
  classification justifies.
- Leave helpers in `dispatch.cpp` only when their role is genuinely
  cross-owner orchestration.
- Stop and request plan review if a helper needs new shared prepared authority
  or if ownership would require broad dispatch restructuring.

Completion check:

- Each moved family has focused build and backend proof.
- The old dispatch publication surfaces no longer expose owner-local helpers
  that have a clearer destination owner.
- `todo.md` names any helper intentionally left in dispatch and why.

### Step 4: Contract obsolete dispatch publication surfaces

Goal: remove or thin synthetic dispatch publication files once their helpers
have moved.

Actions:

- Delete empty implementation or header surfaces only if no remaining include
  or build target needs them.
- Otherwise reduce them to true dispatch-route declarations.
- Update build files only when a file is actually deleted or a target source
  list changes.

Completion check:

- The project builds after any file-list update.
- No stale declarations, includes, or source-list entries remain.
- The remaining dispatch publication surface has a clear ownership purpose.

### Step 5: Validate closure readiness

Goal: prove the source idea is satisfied, not merely that one packet passed.

Actions:

- Run the supervisor-chosen backend CTest subset covering AArch64 dispatch
  publication, same-block publication, fused-compare publication,
  fixed-formal store publication, and current-block join publication.
- Run regression guard before closure if multiple destination owners were
  changed.
- Review the final diff for source-idea reject signals.

Completion check:

- Focused publication tests pass.
- Required regression guard passes when applicable.
- No helper is moved while re-deriving prepared publication authority locally.
- No edge-copy, printer, instruction-record, or unrelated call/memory cleanup
  was pulled into this route.
