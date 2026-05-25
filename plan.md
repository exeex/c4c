# Prepared Call Preservation Source Authority Runbook

Status: Active
Source Idea: ideas/open/06_prepared_call_preservation_source_authority.md

## Purpose

Move prior-preserved call argument source authority out of AArch64 emission and
into shared prepared call-plan facts.

## Goal

Prepared call lookups and call-plan records choose valid prior-preservation
sources; AArch64 call-boundary lowering consumes those choices instead of
rescanning local value homes or preservation records.

## Core Rule

Do not replace the current AArch64 scan with another target-local scan. The
source of truth must live in prepared planning or prepared lookup helpers, and
incomplete or ambiguous choices must be rejected before emission.

## Read First

- `ideas/open/06_prepared_call_preservation_source_authority.md`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

## Current Targets

- Prepared call-plan structures and lookup helpers for prior-preserved values.
- AArch64 call argument source selection in `calls_moves.cpp`.
- Same-block, cross-block, and no-valid-prior-source prepared lookup coverage.

## Non-Goals

- Do not change AAPCS64 preservation policy.
- Do not merge or retire calls files; that belongs to later ideas.
- Do not clean up dispatch publication or edge-copy behavior.
- Do not fix unrelated c_testsuite families while claiming progress here.
- Do not downgrade supported tests or weaken expectations.

## Working Model

- Prepared planning knows function-wide call order, block order, domination,
  and preservation routes.
- A prior-preservation source is valid only when the prepared facts can name a
  single stable preserved value for the current call argument.
- AArch64 lowering may translate a prepared source into target operands, but it
  must not decide which semantic source wins.

## Execution Rules

- Keep each implementation packet narrow enough for build proof plus a focused
  prepared-call or AArch64 call-boundary test.
- Prefer adding prepared lookup tests before removing the corresponding
  AArch64 fallback.
- Preserve indexed lookup behavior for same-block and cross-block cases.
- Add diagnostics or a stable rejection path for incomplete and ambiguous
  preservation choices before emission.
- Treat testcase-shaped matching, named c_testsuite shortcuts, and expectation
  weakening as route failures.

## Ordered Steps

### Step 1: Map Current Prior-Preservation Authority

Goal: identify every place that currently selects a prior-preserved source for
a later call argument.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- prepared call-plan definitions and lookup helpers under `src/backend/mir`

Actions:

- Inspect AArch64 helpers that call or wrap
  `find_latest_indexed_prior_preserved_value` and related stack-preserved
  lookup routines.
- Identify the prepared records already available on
  `PreparedCallArgumentPlan`, `PreparedCallPlan`, and preservation values.
- Record which source kinds are complete prepared facts and which still depend
  on target-local reconstruction.

Completion check:

- The executor can name the exact AArch64 selection helpers to retire or
  redirect and the prepared data needed to replace them.

### Step 2: Add Prepared Prior-Preservation Lookup Contract

Goal: define the shared prepared query surface for selecting a valid prior
preserved source.

Primary targets:

- prepared call-plan lookup APIs
- prepared call-plan data structures
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`

Actions:

- Add or tighten prepared lookup helpers for prior-preservation source
  queries.
- Cover same-block and cross-block prior preserved values with indexed lookup
  tests.
- Cover no-valid-prior-source or ambiguous-source behavior with an explicit
  rejection contract.

Completion check:

- Focused prepared-call tests prove the lookup helper returns one valid source
  or rejects the selection before any AArch64 emission path runs.

### Step 3: Publish Explicit Source Selection In Call Plans

Goal: make call-plan argument source selections carry the chosen preservation
source explicitly.

Primary targets:

- `PreparedCallArgumentPlan`
- `PreparedCallArgumentSourceSelection`
- preservation route records consumed by call arguments

Actions:

- Extend prepared source selections only as needed to reference the selected
  preservation source.
- Keep source identity stable across same-block and cross-block lookups.
- Preserve existing indexed lookup behavior for stack and callee-saved
  preservation routes.
- Reject incomplete prepared selections before AArch64 lowering consumes them.

Completion check:

- Prepared plans expose enough structured data for AArch64 to build source
  operands without rediscovering the semantic source.

### Step 4: Redirect AArch64 Emission To Prepared Authority

Goal: remove target-local prior-preservation source choice from AArch64 call
argument lowering.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`

Actions:

- Replace local scans for prior preserved values with the prepared source
  selection or prepared lookup result.
- Keep AArch64-only operand construction, register view handling, and
  instruction emission local.
- Remove or narrow fallback paths that rederive prior homes from raw value
  homes or broad preservation scans.
- Add diagnostics for missing prepared source authority instead of silently
  choosing a target-local fallback.

Completion check:

- AArch64 emission consumes complete prepared source facts and no longer owns
  the semantic selection of prior preserved call argument sources.

### Step 5: Validate Representative Call Paths

Goal: prove the authority move without relying on expectation rewrites.

Primary targets:

- prepared-call focused tests
- AArch64 call-boundary and instruction dispatch tests
- representative same-family c_testsuite cases chosen by the supervisor

Actions:

- Run build or compile proof after each code-changing packet.
- Run focused prepared-call tests for same-block, cross-block, and rejection
  paths.
- Run focused AArch64 call-boundary tests that exercise prepared
  prior-preservation source consumption.
- Escalate to the supervisor for broader regression guard when the prepared
  contract and AArch64 consumption path are both changed.

Completion check:

- Existing prior-preservation runtime cases remain green, focused lookup tests
  pass, and no unsupported markings or weaker contracts are introduced.
