# Backend Regex Failure Family Inventory Runbook

Status: Active
Source Idea: ideas/open/295_backend_regex_failure_family_inventory.md
Activated: 2026-05-19

## Purpose

Use the main-build backend regex surface as an umbrella inventory, then split
the next focused semantic owner before any implementation work starts.

## Goal

Classify the post-298 backend-regex residual failures well enough to create one
focused repair idea, or record why no focused owner is ready.

## Core Rule

Do not implement fixes in this umbrella plan. This plan exists to classify,
split, and switch lifecycle state.

## Read First

- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`
- `ideas/closed/297_lir_to_bir_local_memory_admission.md`
- `ideas/closed/298_lir_to_bir_global_pointer_aggregate_projection.md`
- `test_before.log`, when available, as the accepted focused close proof from
  idea 298

## Current Targets

- Machine-printer residuals after ideas 296 through 298.
- Runtime nonzero or mismatch buckets that still need generated assembly or
  narrower probes.
- Standalone timeout `00220`, only as a quarantine or hang-specific split.

## Non-Goals

- Do not edit implementation files.
- Do not change test expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 298 from failing counts alone.
- Do not treat the full backend regex as one monolithic repair owner.
- Do not run broad runtime scans without timeout and stale-process cleanup.

## Working Model

- The latest durable umbrella state says idea 298 is closed and idea 295 is
  parked with residual machine-printer, runtime, and timeout buckets.
- `ctest -R backend` is an imprecise selector. Classify failures before
  deciding an implementation owner.
- A focused owner needs generated-code, diagnostic, or proof evidence showing a
  shared semantic failure family.

## Execution Rules

- Preserve routine classification findings in `todo.md`.
- Update this plan only when the classification route changes.
- Update the source idea only for durable deactivation, closure, or a new split
  decision.
- When a focused owner is split, create a new `ideas/open/*.md` with concrete
  reviewer reject signals, then switch lifecycle state to that owner.

## Ordered Steps

### Step 1: Reconstruct post-298 residual inventory

Goal: establish the current residual failure set after closed idea 298.

Actions:

- Inspect the accepted post-298 proof/log context provided by the supervisor,
  especially `test_before.log` when it contains the focused close proof.
- If the supervisor delegates a fresh inventory command, capture the backend
  regex result from `/workspaces/c4c/build` with timeout and stale-process
  cleanup appropriate for runtime tests.
- Separate residuals into machine-printer, runtime nonzero/mismatch, timeout,
  and any local backend/unit failures.
- Record the current classification in `todo.md`.

Completion check:

- `todo.md` lists the residual buckets and states which bucket is the best
  candidate for focused ownership, or why no bucket is ready.

### Step 2: Identify the next semantic owner

Goal: choose one focused repair family from the residual inventory.

Actions:

- Prefer a bucket with multiple nearby failures sharing the same generated-code
  or diagnostic symptom.
- For runtime nonzero or mismatch buckets, require generated assembly or a
  narrower probe before declaring the owner.
- For machine-printer residuals, group by instruction/form semantics rather
  than by testcase filename.
- Treat timeout `00220` as a separate hang-specific owner or quarantine unless
  evidence proves it belongs to another semantic family.

Completion check:

- A focused owner candidate has a named semantic capability, included cases,
  out-of-scope cases, and proof scope.

### Step 3: Split or park

Goal: convert the classification decision into lifecycle state.

Actions:

- If a focused owner is ready, create a new `ideas/open/*.md` with goal,
  scope, proof expectations, and concrete reviewer reject signals.
- Add a durable deactivation note to idea 295 describing the split decision,
  proof/log basis, and remaining buckets.
- Switch lifecycle state from this umbrella plan to the new focused owner
  before implementation begins.
- If no owner is ready, keep idea 295 active only long enough to record the
  blocker in `todo.md`, then report the missing evidence needed.

Completion check:

- Either lifecycle state is switched to a focused owner, or `todo.md` explains
  why no activation/split is possible yet.
