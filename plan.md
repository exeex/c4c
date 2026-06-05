# AArch64 00216/00204 Post-Closure Regression Runbook

Status: Active
Source Idea: ideas/open/112_aarch64_00216_00204_post_closure_regression.md

## Purpose

Repair the current AArch64 full-suite regression where
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c` segfault after idea 110 was closed
baseline-clean.

## Goal

Find the semantic regression introduced after commit `23213dbe4`, repair it
without weakening expectations, and prove the two failing tests plus the
idea-110 recovered target set stay clean.

## Core Rule

Do not special-case `00216.c`, `00204.c`, their expected output shape, or test
names. Treat any filename-shaped or expectation-weakening change as route
drift.

## Read First

- `ideas/open/112_aarch64_00216_00204_post_closure_regression.md`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- stack-layout prepared frame-slot materialization changes in
  `23213dbe4..HEAD`
- prepared-printer/store-source visibility changes in `23213dbe4..HEAD` only
  when they affect prepared-module construction

## Current Targets

- Failing tests:
  - `c_testsuite_aarch64_backend_src_00216_c`
  - `c_testsuite_aarch64_backend_src_00204_c`
- Regression window: `23213dbe4..HEAD`
- Suspect shared areas:
  - call aggregate address planning
  - variadic aggregate handling
  - frame-address materialization
  - publication-plan authority and store-source visibility

## Non-Goals

- Do not reopen the full idea-110 failure family.
- Do not do broad BIR/prealloc residue cleanup unrelated to the two segfaults.
- Do not touch x86 or RISC-V behavior unless needed to keep shared APIs
  coherent.
- Do not make printer-only formatting changes unless they prove a real
  construction-time side effect.
- Do not weaken, skip, or reclassify either C testsuite case.

## Working Model

The two failures are post-closure runtime segfaults with no stdout. The route
should first prove the current failure shape, then identify whether the shared
root is in call-planning frame-address materialization, publication authority,
or prepared-module construction visibility. The accepted fix must repair the
underlying lowering or data-flow rule, not just move names or expectations.

## Execution Rules

- Keep each implementation packet narrow and tied to one runbook step.
- Prefer semantic lowering and data-flow repairs over compatibility fallbacks.
- If a compatibility path is necessary, document and bound why it is safe.
- Preserve source idea intent; routine execution notes belong in `todo.md`.
- For code-changing steps, use build proof before claiming acceptance.
- Escalate to broader AArch64 or full-suite validation before closure.

## Step 1: Reproduce And Capture Failure Shape

Goal: Confirm the current two-test regression at `HEAD` and capture the
observable segfault shape.

Primary target:
- AArch64 C testsuite runtime tests `00216` and `00204`

Actions:
- Run:
  ```sh
  ctest --test-dir build -j1 --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00(216|204)_c)$'
  ```
- Record whether each test segfaults, exits differently, or emits any stdout.
- If needed, run targeted debug commands to determine whether the fault occurs
  before or after generated-program entry.

Completion check:
- `todo.md` records the fresh failure evidence and any debug artifact needed
  to start root-cause work.

## Step 2: Compare Against The Baseline-Clean Closure Point

Goal: Establish the relevant behavioral delta from idea 110's clean closure
point to `HEAD`.

Primary target:
- Change range `23213dbe4..HEAD`

Actions:
- Inspect commits touching call planning, publication planning, stack-layout
  prepared frame-slot materialization, and prepared-printer/store-source
  visibility.
- Compare generated/prepared artifacts for the two tests when that helps
  identify a shared missing materialization or authority fact.
- Keep printer-only differences out of scope unless they affect prepared
  module construction.

Completion check:
- The likely regression source is narrowed to a concrete subsystem, commit, or
  semantic rule, with enough evidence to guide a code-changing packet.

## Step 3: Repair The Shared Semantic Defect

Goal: Fix the underlying AArch64 backend lowering/data-flow defect responsible
for one or both segfaults.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- related stack-layout or prepared-module construction code only as needed

Actions:
- Implement the smallest semantic repair that restores the missing frame
  address, aggregate address, variadic aggregate, publication authority, or
  store-source fact.
- Do not add named-case matching for `00216`, `00204`, or expected output.
- Do not hide the old failure behind an unbounded legacy name fallback.
- Build after the change before running runtime proof.

Completion check:
- The code builds, and the two-test focused command passes or produces a
  strictly narrower remaining failure explained in `todo.md`.

## Step 4: Protect The Idea-110 Recovered Target Set

Goal: Prove the fix does not regress the previously recovered AArch64 targets
named by the source idea.

Primary target:
- Idea-110 recovered target subset

Actions:
- Run:
  ```sh
  ctest --test-dir build -j1 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00(172|180|216|220|204)_c)$'
  ```
- Investigate any failure before claiming the slice complete.

Completion check:
- The recovered target subset passes, or `todo.md` records a blocker that is
  not accepted as progress.

## Step 5: Broader Validation And Closure Notes

Goal: Prepare the active idea for supervisor-side closure review.

Primary target:
- Broader AArch64 or full-suite baseline validation chosen by the supervisor

Actions:
- Run or request the broader validation needed for closure.
- Identify the exact regression source in notes.
- State whether `00216` and `00204` shared the same root cause.
- Leave closure to the plan owner only after source-idea completion and
  regression guard requirements are satisfied.

Completion check:
- No unexplained new broader failures remain, and `todo.md` has concise
  closure-ready notes for the supervisor and reviewer.
