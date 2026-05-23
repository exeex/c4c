# AArch64 Scalar Cast Stack-Homed Register Source Publication Runbook

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md

## Purpose

Refresh the parked scalar-cast source-publication owner and determine whether
there is any current in-scope executable work before closure is attempted.

## Goal

Confirm that AArch64 selected scalar casts fed by stack-homed values still
publish structured register source operands after prepared consumer
stack-to-register moves.

## Core Rule

Do not claim progress through expectation changes, unsupported classification,
runner behavior, proof-log behavior, or any `00143`-specific shortcut.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- AArch64 selected scalar cast lowering and printer paths
- Prepared consumer stack-to-register move publication paths
- Focused backend scalar-cast coverage around selected/prepared scalar cast
  records

## Current Targets

- `backend_aarch64_scalar_cast_records`
- `backend_aarch64_prepared_scalar_cast_records`
- `c_testsuite_aarch64_backend_src_00143_c`

## Non-Goals

- Do not reopen closed idea 338 or mutate `ideas/closed/`.
- Do not broaden into local storage/writeback sizing, compare lowering,
  frame-layout sizing, runtime value correctness, runner policy, CTest
  registration, or c-testsuite expectation work.
- Do not hard-code `00143`, value ids, block names, one register, one
  diagnostic string, or one selected instruction sequence.

## Working Model

The source idea is parked because earlier execution repaired the scalar-cast
source-publication boundary and later refresh proof found no live in-scope
failure. The active runbook should first re-check that boundary. If a current
in-scope scalar-cast publication failure reappears, execution may localize and
repair it. If the focused proof remains green or the representative reaches a
different first bad fact, the supervisor should route to lifecycle closure or
another owner rather than expanding this plan.

## Execution Rules

- Keep source-idea edits unnecessary unless durable source intent changes.
- Use `todo.md` for packet state and proof notes.
- Prefer focused backend proof before any broader validation.
- Treat any expectation, unsupported, timeout, runner, or proof-log edit as
  out of scope for this runbook.
- Escalate to a separate idea if the refreshed first bad fact belongs to a
  different compiler/backend owner.

## Step 1: Refresh Scalar-Cast Publication Proof

Goal: confirm whether the scalar-cast stack-homed register source-publication
boundary is still live.

Concrete actions:

- Build the default preset.
- Run the focused scalar-cast publication subset:

  ```sh
  cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|c_testsuite_aarch64_backend_src_00143_c)$'
  ```

- If any target fails, inspect the first bad fact and determine whether it is
  the structured register-source publication diagnostic covered by this idea.
- Record the proof result and first-bad-fact classification in `todo.md`.

Completion check:

- The focused proof either passes, or a failing target is classified as
  in-scope scalar-cast source-publication work or out-of-scope follow-up work.

## Step 2: Repair Only If The In-Scope Boundary Reappears

Goal: fix a current scalar-cast source publication failure only if Step 1
proves the old boundary is live again.

Primary target:

- The selected-node publication, normalization, or handoff path that should
  expose the prepared consumer stack-to-register source as a structured scalar
  cast source operand.

Concrete actions:

- Localize where the prepared consumer stack-to-register move is available and
  where the selected scalar cast loses the register source.
- Repair the general publication or handoff rule for stack-homed scalar cast
  inputs.
- Add or update focused backend coverage for the general stack-homed
  scalar-cast shape.
- Re-run the Step 1 focused proof.

Completion check:

- The representative no longer fails with the structured scalar cast register
  source diagnostic, and focused coverage proves the general publication rule.

## Step 3: Lifecycle Handoff

Goal: hand the refreshed state back to the supervisor with a clear close,
switch, or execution recommendation.

Concrete actions:

- If Step 1 passes with no in-scope failure, mark the current packet complete
  in `todo.md` and recommend lifecycle close consideration.
- If Step 2 repairs a reappeared in-scope failure, record the proof and any new
  first bad fact in `todo.md`.
- If refresh exposes a different owner, record the classification in
  `todo.md` and recommend split or switch rather than widening this runbook.

Completion check:

- `todo.md` contains the latest proof, first-bad-fact classification, and a
  concrete supervisor next action.
