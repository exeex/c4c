# AArch64 C-Testsuite Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md

## Purpose

Use the current AArch64 backend c-testsuite failures as an umbrella inventory,
then split exactly one focused semantic repair idea before implementation work
continues.

## Goal

Refresh the post-293 AArch64 failure map, classify remaining backend/runtime
families, and select the next focused owner or document why no owner is ready.

## Core Rule

This runbook is inventory-only. Do not implement compiler/backend repairs under
this umbrella idea.

## Read First

- `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`
- `ideas/closed/293_aarch64_side_effect_control_value_publication_authority.md`
- Latest focused proof artifacts if still present:
  - `test_before.log`
  - `test_after.log`
  - `/tmp/c4c_aarch64_side_effect_step4_broader_sample.log`

## Current Targets

- Remaining AArch64 backend c-testsuite failures after closed focused ideas
  285-293.
- Known unresolved buckets from 293 close: closed-owner overlap
  (`src/00159.c`, `src/00168.c`, `src/00193.c`) and pointer/address/string-heavy
  work (`src/00217.c`) from the broader side-effect sample.
- Previously deferred buckets from the umbrella inventory: timeout/hang,
  printer/admission, floating/conversion/string-only, pointer/aggregate address
  authority, and broad aggregate/function-pointer behavior.
- Closed owners must stay closed unless fresh generated-code evidence
  contradicts their closure.

## Non-Goals

- Do not touch implementation files, tests, CTest registration, runner
  behavior, expected outputs, allowlists, unsupported classifications, timeout
  policy, or build/test infrastructure.
- Do not treat timeout/hang cases as ordinary runtime mismatches.
- Do not create a named-testcase repair idea without a broader semantic owner.
- Do not reopen closed AArch64 owners from residual failure counts alone.
- Do not keep this umbrella active for implementation once a coherent focused
  owner is identified.

## Working Model

The focused side-effect/control-value owner is closed. The next inventory pass
should classify the remaining failure surface from current evidence, separate
closed-owner overlap from genuinely new semantic ownership, and split the next
tractable focused idea before coding starts.

## Execution Rules

- Use timeout-protected broad scans and check for stale generated-runtime
  processes afterward.
- Prefer current broad evidence if it already reflects the post-293 tree;
  otherwise rerun the AArch64 backend label with an explicit timeout.
- Inspect representatives by semantic family, not by filename count alone.
- Separate frontend-owned failures from backend/runtime-owned failures.
- Write current inventory observations and rejected buckets to `todo.md` while
  investigating.
- If a coherent focused owner emerges, create a new `ideas/open/*.md` source
  idea with concrete reviewer reject signals and switch lifecycle state to it.

## Steps

### Step 1: Refresh Post-293 Inventory

Goal: establish the current AArch64 backend failure map after side-effect
control-value publication authority closed.

Primary target: timeout-protected AArch64 backend c-testsuite scan.

Actions:

- Prefer existing post-293 broad artifacts only if they are current for
  `HEAD`.
- Otherwise run:
  `ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`
- Record pass/fail totals and classify failures into frontend, runtime
  nonzero, runtime mismatch, timeout, and compile/printer/admission buckets.
- Check that no generated-runtime process remains after the scan.
- Preserve the side-effect Step 4 broader-sample separation for `00159`,
  `00168`, `00193`, and `00217` until fresh evidence proves a different owner.

Completion check:

- `todo.md` records current totals, representative cases, log paths,
  stale-process cleanup result, and any buckets that remain closed-owner
  overlap rather than new owner candidates.

### Step 2: Select the Next Semantic Owner

Goal: choose the smallest remaining backend/runtime family that can become a
focused repair idea.

Primary target: representatives from non-frontend, non-timeout buckets unless
fresh evidence shows a timeout-specific owner is now safest.

Actions:

- Inspect generated output and source shape for representative failures.
- Compare candidates against closed owners 285-293 to avoid reopening closed
  work without contradictory evidence.
- Prefer a semantic family that can be proven with a small starter subset and
  broader nearby sampling.
- Defer buckets that are primarily frontend, environment-sensitive timeout,
  or compile-stage printer/admission work unless they are clearly the next
  safest owner.
- Record why the selected family is not just a named c-testsuite case and why
  rejected candidates are deferred.

Completion check:

- `todo.md` names the selected owner, starter representatives, rejected or
  deferred buckets, closed-owner overlap rationale, and the proof shape expected
  for the focused idea.

### Step 3: Split or Hold Inventory

Goal: leave lifecycle state consistent after the inventory decision.

Primary target: `ideas/open/` plus active lifecycle files.

Actions:

- If a focused owner is ready, create a new `ideas/open/*.md` file with intent,
  scope, acceptance criteria, and concrete reviewer reject signals.
- Switch active lifecycle state from this umbrella inventory to the focused
  idea using a new `plan.md` and reset `todo.md`.
- If no owner is ready, keep this umbrella active only with a narrow next
  classification step or record the exact ambiguity that blocks selection.

Completion check:

- There is at most one active plan.
- `plan.md` and `todo.md`, if present, point to the same `ideas/open/*.md`.
- This umbrella is not used for implementation work.
