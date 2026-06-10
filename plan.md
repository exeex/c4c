# AArch64 Selected F64 Global Readback Dispatch Debt

Status: Active
Source Idea: ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md

## Purpose

Turn the ambient `backend_aarch64_instruction_dispatch` blocker into a bounded
repair route for the selected f64 global readback that should feed a call ABI
move.

## Goal

Make the selected f64 global readback path feed the expected call ABI move in
`backend_aarch64_instruction_dispatch` while preserving existing call ABI and
layout ownership.

## Core Rule

Repair source selection or call ABI move routing for the selected f64 global
readback. Do not weaken the dispatch expectation, special-case the test text,
or move ABI/layout policy into Route 5 or generic BIR records.

## Read First

- `ideas/open/177_aarch64_selected_f64_global_readback_dispatch_debt.md`
- `ideas/open/171_route5_current_block_join_source_migration.md`
- AArch64 instruction dispatch test and its selected f64 global readback case
- AArch64 call ABI move selection and source-selection code
- Prepared call plan or semantic source helpers touched by the failing path

## Scope

- Selected f64 global readback path in AArch64 instruction dispatch.
- Call ABI move selection for the failing readback case.
- Any semantic source or ABI handoff required for that readback to feed the
  call move correctly.
- Narrow diagnostic or regression coverage that helps prove the same behavior
  without replacing the monolithic dispatch acceptance check.

## Non-Goals

- Do not change Route 5 current-block join-source identity or prepared helper
  contraction.
- Do not weaken, delete, downgrade, or mark unsupported the failing monolithic
  dispatch expectation.
- Do not perform broad call lowering rewrites unrelated to the selected f64
  readback failure.
- Do not move ABI placement, layout, or call-plan policy into Route 5 or a
  generic BIR record.
- Do not claim success from a named-test shortcut or emitted-assembly string
  patch that leaves the underlying source-selection failure intact.

## Working Model

- Route 5 narrow validation is already green; this runbook owns only the
  remaining ambient dispatch debt.
- The failing dispatch assertion is the close-level signal that a selected f64
  global readback is not connected to the call ABI move as expected.
- ABI/layout ownership should remain with the prepared call plan or existing
  target-local call lowering surfaces.
- A correct fix should be visible through the monolithic
  `backend_aarch64_instruction_dispatch` CTest and should not require weakening
  neighboring expectations.

## Execution Rules

- Start by reproducing the exact failure and localizing which readback, source
  record, and call move no longer line up.
- Keep any added helper or assertion general enough to describe the semantic
  handoff, not the test function name.
- Preserve existing call ABI/layout boundaries. If a repair seems to require a
  broad call-plan migration, stop and report that the idea needs lifecycle
  review.
- For code-changing steps, build `backend_aarch64_instruction_dispatch_test`
  before running CTest.
- Use the monolithic dispatch CTest as the acceptance proof. Narrower proof may
  be added for diagnosis or regression coverage, but it does not replace the
  monolithic check.
- Escalate to broader backend validation if public call, source-selection, or
  AArch64 dispatch interfaces change.

## Ordered Steps

### Step 1: Reproduce And Localize Selected F64 Readback Failure

Goal: Confirm the current failure and identify the source-selection and call
ABI move records involved in the selected f64 global readback path.

Primary targets:
- `backend_aarch64_instruction_dispatch_test`
- `backend_aarch64_instruction_dispatch`
- AArch64 instruction dispatch test case containing the selected f64 global
  readback call ABI move expectation
- Call source-selection and ABI move routing helpers reached by that case

Actions:
- Build the focused dispatch test binary.
- Run the monolithic dispatch CTest and confirm the failure message:
  `expected selected f64 global readback to feed call ABI move`.
- Inspect the failing test setup and the actual selected source/move chain.
- Trace whether the break is caused by missing semantic source selection,
  incorrect ABI move routing, an ownership mismatch, or stale test coverage.
- Record the concrete files, helper names, and proof command for the executor
  handoff in `todo.md`.

Completion check:
- `todo.md` names the localized failure path, the suspected owner of the
  repair, and the exact focused build and CTest commands to rerun.

### Step 2: Select The Minimal Semantic Repair

Goal: Decide the smallest source-selection or ABI handoff repair that restores
the selected f64 readback to the call move without changing ABI/layout
ownership.

Primary targets:
- AArch64 source-selection helpers for global readback values
- Call ABI move selection and prepared call plan accessors used by the failing
  path
- Existing semantic source or prepared lookup APIs already responsible for the
  selected readback

Actions:
- Compare the selected f64 path with nearby passing global readback or call
  argument paths.
- Identify whether an existing semantic source fact should be threaded through,
  an existing call ABI move lookup should be consulted differently, or a stale
  handoff should be removed.
- Keep the repair at the current owner layer. Do not invent a new route,
  aggregate, or facade unless the source idea is returned for lifecycle review.
- If additional narrow coverage is useful, make it assert the semantic
  readback-to-call-move relationship rather than the exact test name.

Completion check:
- The chosen repair is documented in `todo.md` with a clear owner boundary and
  no need for broad call lowering or Route 5 changes.

### Step 3: Implement The Readback To Call-Move Repair

Goal: Make the selected f64 global readback feed the expected call ABI move
through the existing source-selection or call ABI routing model.

Primary targets:
- The localized AArch64 source-selection or call ABI routing files from Step 1
- Focused dispatch or helper tests when needed

Actions:
- Apply the minimal repair selected in Step 2.
- Preserve ABI register/stack placement, layout records, and call plan
  ownership.
- Avoid testcase-shaped matching on the dispatch test name, exact emitted text,
  or a single hard-coded global.
- Keep any fallback behavior fail-closed and explain remaining unsupported
  cases in `todo.md` if encountered.

Completion check:
- The focused dispatch test target builds, and the selected f64 global
  readback path now reaches the expected call ABI move without expectation
  downgrades.

### Step 4: Prove And Handoff

Goal: Produce acceptance proof for the ambient dispatch debt and identify
whether broader validation is required.

Primary targets:
- `backend_aarch64_instruction_dispatch_test`
- `backend_aarch64_instruction_dispatch`
- Any narrow helper test added during Step 2 or Step 3

Actions:
- Run:

```bash
cmake --build build --target backend_aarch64_instruction_dispatch_test
ctest --test-dir build -R '^backend_aarch64_instruction_dispatch$' --output-on-failure
```

- If public call, source-selection, or AArch64 dispatch interfaces changed,
  ask the supervisor to run or delegate broader backend validation.
- Record proof results, remaining risks, and any follow-up ideas in `todo.md`.

Completion check:
- `backend_aarch64_instruction_dispatch` passes the selected f64 global
  readback call ABI move expectation, and `todo.md` contains the proof command
  and result.
