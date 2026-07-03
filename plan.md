# Prepared Move-Bundle Ambiguous Multi-Source Stack Destination Runbook

Status: Active
Source Idea: ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md

## Purpose

Repair the prepared move-bundle classifier boundary that rejects the
`ambiguous_non_parallel_multi_source_stack_destination` shape reproduced by
`src/20001026-1.c`.

Goal: make the prepared layer publish coherent authority for this
multi-source stack-destination shape, or split it into explicit supported
moves, without pretending this is RV64 integer div/rem lowering progress.

Core Rule: do not claim integer div/rem or RV64 object-route progress until the
prepared classifier boundary has been crossed and any later failure is
separately classified.

## Read First

- ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md
- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/object-route.log`
- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/dump-bir.txt`
- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/dump-prepared-bir.txt`
- Existing prepared move-bundle classifier tests and helpers
- Existing RV64 gcc_torture object-runner convention for focused reruns

## Current Scope

- Identify the concrete prepared move bundle that produces the ambiguous
  non-parallel multi-source stack-destination diagnostic.
- Add focused prepared-layer coverage for that boundary.
- Repair the prepared classifier to publish coherent authority or split the
  shape into supported moves.
- Re-run `src/20001026-1.c` through the RV64 gcc_torture object route and
  classify any later failure separately.

## Non-Goals

- Do not implement RV64 integer division or remainder instruction lowering.
- Do not repair generic `unsupported_instruction_fragment` rows from 549.
- Do not change runtime comparison, expected output, unsupported markers, or
  allowlists.
- Do not broad-rewrite prepared move-bundle classification outside the
  ambiguous multi-source stack-destination shape.
- Do not special-case `src/20001026-1.c` by name.

## Working Model

The current row was selected from an `integer_div_rem` family map, but the
reproduced first owner is prepared move-bundle classification. The first useful
implementation result is crossing that prepared boundary with a general rule
and focused coverage. If crossing the boundary reveals a later RV64 object
failure, record it as the next owner instead of folding it into this repair.

## Execution Rules

- Keep routine packet state in `todo.md`.
- Preserve the source idea unless durable intent changes.
- Prefer semantic prepared authority over testcase-shaped matching.
- Keep proof tied to both focused prepared tests and the representative
  `src/20001026-1.c` object-route rerun.
- Escalate to broader backend validation once shared prepared classifier
  behavior changes.

## Ordered Steps

### Step 1: Pin The Prepared Classifier Boundary

Goal: identify the exact prepared move-bundle shape, producer context, and
diagnostic path behind the current rejection.

Primary target: existing Step 2 evidence and focused `src/20001026-1.c` dumps.

Actions:

- Re-run or inspect `src/20001026-1.c` with the RV64 object runner and
  prepared dump commands.
- Locate the function, block, move bundle, source homes, destination home, and
  reason fields involved in
  `ambiguous_non_parallel_multi_source_stack_destination`.
- Identify the prepared classifier function or helper that emits the rejection.
- Record a minimal evidence packet under `build/agent_state` and summarize it
  in `todo.md`.

Completion check:

- `todo.md` names the concrete prepared move-bundle shape and the rejecting
  helper or diagnostic path.
- The evidence distinguishes prepared classifier failure from integer div/rem
  lowering.
- No implementation files, tests, expectations, or unsupported markers were
  changed.

### Step 2: Add Focused Prepared Contract Coverage

Goal: encode the ambiguous multi-source stack-destination boundary in focused
prepared-layer tests before changing behavior.

Primary target: prepared move-bundle classifier tests and helper fixtures.

Actions:

- Add or extend focused tests that reproduce the ambiguous non-parallel
  multi-source stack-destination shape without depending on the full torture
  testcase.
- Assert the current fail-closed behavior first, then define the intended
  coherent authority or supported split contract.
- Keep test fixtures general enough to reject named-case shortcuts.

Completion check:

- Focused tests fail for the missing contract before the repair or otherwise
  document the current fail-closed diagnostic.
- Test names and assertions describe the prepared move-bundle shape, not
  `src/20001026-1.c`.

### Step 3: Repair Prepared Move-Bundle Classification

Goal: teach the prepared classifier to handle the multi-source
stack-destination shape coherently.

Primary target: prepared move-bundle classifier implementation and the focused
tests from Step 2.

Actions:

- Implement the narrow prepared classifier rule or supported split identified
  in Steps 1 and 2.
- Preserve existing fail-closed behavior for unrelated ambiguous move bundles.
- Keep diagnostics precise if a nearby shape remains unsupported.
- Run the focused prepared tests and build proof selected by the supervisor.

Completion check:

- Focused prepared tests pass.
- The repair is not keyed to `src/20001026-1.c` or to a narrow source filename.
- Unrelated prepared move-bundle rejection behavior remains covered.

### Step 4: Representative RV64 Rerun And Owner Disposition

Goal: prove the representative crosses the prepared classifier boundary and
classify any next failure.

Primary target: RV64 gcc_torture object rerun for `src/20001026-1.c`.

Actions:

- Re-run `src/20001026-1.c` through the RV64 gcc_torture object route.
- Confirm the old prepared classifier diagnostic no longer appears, or record
  the exact blocker if it still does.
- If a later RV64 object-route failure appears, classify it separately and do
  not claim this idea fixed that later owner.
- Run the supervisor-selected broader validation for a prepared classifier
  change.

Completion check:

- `todo.md` records the old-vs-new representative result and log paths.
- The source idea's acceptance criteria can be evaluated from focused tests and
  representative rerun evidence.
- No runtime comparison, expected output, unsupported marker, or allowlist
  behavior was weakened.
