# Prepared Module Store Source Recovered-Source Identity Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate one remaining idea 260 candidate: the
`store_source_publications` recovered-source identity packet.

## Goal

Allow recovered narrow-store source lookup to use same-block BIR load/store
identity only behind a complete prepared agreement boundary.

## Core Rule

Do not move store-source publication policy, addressing authority, target
output, or prepared aggregate compatibility into BIR. BIR identity may only
confirm a source fact that the prepared publication planner can still validate
and fail closed.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/query.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Scope

- Candidate: `store_source_publications` recovered-source identity packet.
- Primary prepared helper surface:
  `find_prepared_recovered_narrow_store_source_for_wide_local_load(...)` and
  nearby same-block load-local stored-value helpers.
- Primary BIR identity surface:
  `find_bir_same_block_load_local_stored_value_source_identity(...)`.
- Primary consumer surface:
  `prepared_recovered_narrow_store_source(...)` and
  `plan_store_local_source_publication(...)` in AArch64 memory lowering.

## Non-Goals

- Do not implement the byval pointer-source classification, direct-global
  select-chain dependency, or source-value/source-producer metadata packets.
- Do not delete, privatize, wrap, or broadly retire
  `PreparedBirModule::store_source_publications` or other prepared aggregate
  fields.
- Do not rewrite printer/debug text, route-debug text, target output,
  diagnostics, helper statuses, or expectations to claim progress.
- Do not alter broad AArch64 store-source publication policy, frame-slot
  policy, storage encoding, pointer-base homes, pending policy, or duplicate
  handling outside this recovered-source lookup.

## Working Model

- Prepared memory-access and source-producer rows remain the compatibility
  authority.
- The BIR same-block identity may only be used when the prepared load-local
  producer, stored value, store instruction, memory access, block label, and
  before-instruction boundary agree.
- Missing addressing, missing prepared lookup rows, mismatch, wrong memory
  kind, wrong source value, wrong destination, wrong lane offset, wrong width,
  or after-store ordering must return the current unavailable/null result.

## Execution Rules

- Keep each implementation packet focused on one helper or consumer boundary.
- Prefer local agreement helpers over broad API movement.
- Preserve public prepared fallback behavior and existing prepared lookup maps.
- Add focused fail-closed proof before treating the candidate as complete.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Steps

### Step 1: Locate the Recovered-Source Helper Surface

Goal: confirm the exact prepared and BIR helpers involved in recovered
narrow-store source lookup.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/query.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Inspect
  `find_prepared_recovered_narrow_store_source_for_wide_local_load(...)`.
- Inspect
  `find_prepared_same_block_load_local_stored_value_source(...)` and its
  prepared lookup dependencies.
- Inspect
  `find_bir_same_block_load_local_stored_value_source_identity(...)`.
- Identify the smallest agreement boundary and the focused helper-test area.

Completion check:
- `todo.md` records the selected helper surface, owned files for Step 2, and
  any surfaces explicitly left out of scope.

### Step 2: Implement the Prepared/BIR Agreement Boundary

Goal: make recovered-source lookup accept BIR same-block identity only when it
agrees with complete prepared source-publication facts.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp` only if a small test-visible
  seam is needed
- `src/backend/mir/aarch64/codegen/memory.cpp` only if the consumer boundary
  needs local integration
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Add or reuse a local helper that compares the BIR same-block stored-value
  identity against the prepared lookup result.
- Require prepared addressing and source-producer facts to remain complete.
- Preserve current null/unavailable behavior when BIR is absent, incomplete,
  mismatched, or outside the before-instruction boundary.
- Keep the consumer using prepared publication planning as the policy owner.

Completion check:
- Focused helper tests pass with the delegated narrow command.
- No output baselines or expectation contracts are weakened.

### Step 3: Add Nearby Fail-Closed Rows

Goal: prove the recovered-source candidate is not a named-case shortcut.

Primary target:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Cover the positive agreement path.
- Cover fail-closed rows for missing addressing, missing prepared source
  producer, wrong producer kind, missing load/store memory access, frame-slot
  or object mismatch, lane offset mismatch, store width mismatch, wrong stored
  value, after-store ordering, and prepared/BIR mismatch.
- Keep any existing public prepared compatibility assertions observable.

Completion check:
- Focused helper tests pass.
- `git diff --check` passes.

### Step 4: Broader Backend Validation

Goal: establish close-readiness for this one-candidate runbook.

Primary target:
- validation only

Actions:
- Run the supervisor-delegated broader backend proof, expected default:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Preserve output in `test_after.log`.
- Record proof and any residual risk in `todo.md`.

Completion check:
- Broader backend subset is green.
- `todo.md` states whether the recovered-source identity candidate is ready
  for plan-owner retirement review while the source idea remains open.
