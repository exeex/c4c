# Post Full-Green Direction Audit Runbook

Status: Active
Source Idea: ideas/open/294_post_full_green_direction_audit.md

## Purpose

Choose the next direction after the 286-293 cleanup and repair sequence reached
full-suite green.

## Goal

Produce a direction matrix and, only if justified, tightly scoped follow-up
ideas.

## Core Rule

This is an audit-only runbook. Do not edit implementation files. Do not open
cleanup work unless it has a concrete owner surface, proof route, and reject
signals.

## Read First

- `ideas/closed/286_aarch64_00204_stdarg_semantic_bir_local_memory_admission.md`
- `ideas/closed/287_post_286_prepared_boundary_interface_cleanup.md`
- `ideas/closed/288_extract_aapcs64_variadic_hfa_lane_expansion_helper.md`
- `ideas/closed/289_structured_opaque_pointer_byte_range_provenance.md`
- `ideas/closed/290_gate_quarantined_opaque_compatibility_memory_accesses.md`
- `ideas/closed/291_retire_rendered_call_arg_abi_suffix_fallback.md`
- `ideas/closed/292_reopen_286_288_match_load_local_memory_admission.md`
- `ideas/closed/293_backend_remaining_aarch64_load_local_memory_failures.md`
- `test_baseline.log`

## Current Scope

- Lifecycle state after full-suite green.
- Closure-note leftovers from ideas 286 through 293.
- Baseline hook/candidate state.
- Follow-up work classification only.

## Non-Goals

- No implementation edits.
- No prepared/prealloc public-surface retirement without a new dedicated source
  idea.
- No broad cleanup queue.
- No expectation rewrites.

## Working Model

The safest next move is to classify work into stop/defer, monitor, open, or
execute. A green suite is evidence that the current boundary is healthy; it is
not evidence that every retained policy surface should be deleted.

## Execution Rules

- Preserve source intent in this runbook and `todo.md`.
- Write durable conclusions into the source idea only at closure.
- If a follow-up is needed, create one under `ideas/open/` with reviewer reject
  signals.
- If no follow-up is needed, close this idea with a clear no-new-work note.

## Step 1: Inventory Closure Leftovers

Goal: identify whether ideas 286 through 293 left any explicit unresolved
follow-up.

Actions:

- Read each closure note from 286 through 293.
- Record every leftover, deferral, generated idea, or retained-policy warning.
- Mark whether each item is already closed, intentionally retained, monitor-only,
  or still open.

Completion check:

- `todo.md` lists each closure-note leftover and its classification.

## Step 2: Check Baseline And Lifecycle State

Goal: confirm current proof and hook-managed baseline state.

Actions:

- Inspect `test_baseline.log`.
- Check whether `test_baseline.new.log` exists.
- Run `scripts/plan_review_state.py show`.
- Confirm whether `ideas/open/` contains only this audit idea.

Completion check:

- `todo.md` records baseline commit, pass count, pending-candidate state, and
  lifecycle state.

## Step 3: Build Direction Matrix

Goal: decide what should happen next.

Actions:

- Classify each candidate as `stop/defer`, `monitor`, `open-idea`, or
  `execute-now`.
- For `open-idea` candidates, require a concrete owner surface and proof route.
- For `execute-now`, require a bounded no-code or code packet that fits this
  source idea. Most implementation work should become a separate idea instead.

Completion check:

- `todo.md` contains a direction matrix with reasons.

## Step 4: Generate Follow-Ups Or Close

Goal: make the lifecycle state match the audit result.

Actions:

- If follow-ups are warranted, create the smallest set of `ideas/open/*.md`
  files with acceptance criteria and reviewer reject signals.
- If no follow-up is warranted, do not create any ideas.
- Close this source idea with a concise completion note.

Completion check:

- This idea is closed with either generated follow-up paths or a no-new-work
  conclusion.
