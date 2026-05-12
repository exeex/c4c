# Byval Copy Layout Structured Boundary Runbook

Status: Active
Source Idea: ideas/open/179_byval_copy_layout_structured_boundary.md

## Purpose

Make one bounded byval copy/materialization route consume structured aggregate
layout identity instead of rendered aggregate spelling.

## Goal

Thread structured owner/layout facts into a selected byval copy path while
preserving existing ABI classification behavior and final output spelling.

## Core Rule

Metadata-rich byval copy lowering must not treat equal rendered aggregate
spelling as sufficient layout authority.

## Read First

- `ideas/open/179_byval_copy_layout_structured_boundary.md`
- Prior closed aggregate layout and ABI work from ideas 173 and 174 when the
  selected route depends on their helpers or metadata shape.
- Existing byval call-argument, parameter materialization, prealloc, and BIR
  local-slot lowering tests before choosing the first packet.

## Current Scope

- Pick one byval copy/materialization family.
- Prefer a route with existing structured ABI/layout metadata nearby.
- Use size, alignment, field, owner, or layout identity facts from structured
  metadata for generated metadata-rich inputs.
- Keep no-metadata compatibility explicit and fenced.
- Add focused tests that expose stale or same-spelled aggregate layout
  collisions in the selected copy route.

## Non-Goals

- Do not rework all aggregate memcpy or memset routes.
- Do not reclassify call ABI decisions already covered by idea 174.
- Do not replace every rendered local slot, temp, or debug name.
- Do not weaken byval or backend route expectations.
- Do not add testcase-shaped matching for one named fixture.

## Working Model

- ABI classification and copy/materialization are adjacent but separate.
- The selected copy route should inherit or request structured aggregate layout
  facts before lowering bytes, fields, or slots.
- Rendered type text may remain display, diagnostic, ABI spelling, or explicit
  legacy fixture compatibility, but not normal semantic authority for
  metadata-rich generated inputs.
- Missing or mismatched structured metadata should fail closed or produce a
  clear unsupported metadata-gap result.

## Execution Rules

- Keep each implementation packet tied to one concrete byval copy route.
- Preserve existing final output spelling unless a test already requires a
  semantic output change.
- When adding tests, prove both structured success and stale/missing metadata
  behavior where possible.
- Prefer semantic layout plumbing over parser or printer changes.
- For code-changing packets, run a fresh build or compile proof plus targeted
  byval/backend route coverage chosen by the supervisor.

## Step 1: Inventory and Select Byval Copy Route

Goal: identify the bounded byval copy/materialization path to repair first.

Primary targets:

- Byval parameter copy into BIR local slots.
- Aggregate copy before prealloc.
- Any adjacent route where idea 174 structured ABI facts already reach the
  call boundary.

Actions:

- Inspect current byval lowering and materialization code paths.
- Identify where aggregate layout facts are consumed or reconstructed.
- Record whether the route currently trusts rendered aggregate/type spelling.
- Pick one route with a narrow proof surface and visible structured metadata
  entry point.
- Leave broader routes for later steps or follow-up ideas.

Completion check:

- The executor can name the selected route, the files/functions involved, the
  current spelling-based authority point if present, and the narrow validation
  command to prove changes in that route.

## Step 2: Thread Structured Layout Facts Into the Selected Route

Goal: make the selected byval copy route use structured layout identity for
metadata-rich inputs.

Actions:

- Reuse existing aggregate owner/layout metadata from prior structured layout
  and ABI work when available.
- Pass the structured facts to the copy/materialization logic before it chooses
  size, alignment, fields, slots, or byte ranges.
- Keep rendered names and type spellings for output and diagnostics only.
- Add an explicit compatibility branch only for no-metadata legacy inputs.

Completion check:

- Metadata-rich generated inputs in the selected route no longer derive copy
  layout from rendered spelling first.
- Legacy no-metadata behavior remains named, fenced, and covered by existing or
  focused tests.

## Step 3: Fail Closed on Missing or Mismatched Structured Metadata

Goal: prevent stale or same-spelled aggregate layout collisions from silently
lowering through text fallback.

Actions:

- Detect missing structured layout facts for metadata-rich byval copy inputs.
- Detect mismatches between structured owner/layout facts and the selected copy
  operation when the route has enough information to compare them.
- Emit a clear unsupported metadata-gap result or fail closed through the
  repo's existing diagnostic/error style.
- Avoid reparsing rendered aggregate text to recover semantic layout.

Completion check:

- A stale or mismatched metadata-rich byval copy case is rejected or fenced
  instead of silently using rendered type text as authority.

## Step 4: Add Focused Byval Copy Coverage

Goal: prove the structured byval copy boundary with tests that would fail under
rendered-spelling authority.

Actions:

- Add a success test for the selected structured byval copy route.
- Add a stale, same-spelled, missing, or mismatched layout case that exercises
  the fail-closed boundary.
- Keep assertions tied to copy/materialization behavior, not only final return
  values or printed temp names.
- Preserve supported-path expectations; do not downgrade tests to unsupported
  without explicit supervisor approval.

Completion check:

- Tests demonstrate that equal rendered spelling is insufficient for
  metadata-rich byval copy layout in the selected route.

## Step 5: Validate and Hand Off

Goal: provide enough proof for the supervisor to accept or route-review the
slice.

Actions:

- Run the supervisor-delegated build or compile proof exactly.
- Run targeted byval/backend route coverage.
- Escalate to broader backend coverage if the selected route touches shared
  aggregate copy, ABI preparation, or layout helpers used outside byval.
- Update `todo.md` with the current packet result and proof log path.

Completion check:

- Fresh proof exists for the selected route, and any remaining legacy fallback
  or metadata gap is documented in `todo.md` without mutating the source idea.
