# Prepared Module Store Source Byval Pointer-Source Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate one remaining idea 260 candidate: the
`store_source_publications` byval pointer-source classification packet.

## Goal

Allow byval formal pointer-source classification to use a pointer-value memory
access and formal-name fact only behind complete prepared agreement.

## Core Rule

Do not move store-source publication policy, addressing authority, formal-name
authority, target output, or prepared aggregate compatibility into BIR. Any
BIR or route fact may only confirm a byval pointer-source classification that
the prepared publication planner can still validate and fail closed.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepare_liveness_test.cpp`

## Current Scope

- Candidate: `store_source_publications` byval pointer-source classification
  packet.
- Primary prepared helper surface:
  `prepared_store_source_load_local_is_byval_formal_pointer_source(...)`.
- Primary local classification helper:
  `is_byval_formal_value_name(...)`.
- Primary prepared fact inputs:
  `PreparedAddressingFunction`,
  `PreparedEdgePublicationSourceProducer`, prepared memory-access rows, and
  prepared value-name tables.
- Primary consumer surface:
  store-source publication planning around
  `plan_prepared_fixed_formal_store_source_publication(...)` and nearby
  publication-plan consumers that read `byval_load_local_source`.

## Non-Goals

- Do not implement the direct-global select-chain dependency or
  source-value/source-producer metadata packets.
- Do not revisit the completed module, names, control-flow, or recovered-source
  packets.
- Do not delete, privatize, wrap, or broadly retire
  `PreparedBirModule::store_source_publications` or other prepared aggregate
  fields.
- Do not rewrite printer/debug text, route-debug text, target output,
  diagnostics, helper statuses, or expectations to claim progress.
- Do not alter broad publication policy, addressing policy, stack-home policy,
  formal publication policy, pointer-base homes, storage encoding, pending
  policy, duplicate handling, or target lowering outside this one classifier.

## Working Model

- Prepared addressing and prepared source-producer rows remain the
  compatibility authority.
- Prepared formal-name lookup remains authoritative for deciding whether the
  pointer value names a byval formal.
- A successful classification requires a load-local source producer, a valid
  block label, a prepared memory-access row, pointer-value addressing, an
  available pointer value name, allowed base-plus-offset addressing, and a
  matching byval formal name.
- Wrong producer kind, missing prepared rows, missing pointer names, non-byval
  formals, base-plus-offset rejection, prepared/BIR name mismatch, or
  addressing mismatch must return `false`.

## Execution Rules

- Keep each implementation packet focused on this classifier or its direct
  consumer boundary.
- Prefer local agreement helpers over broad API movement.
- Preserve public prepared fallback behavior and existing prepared lookup maps.
- Add focused fail-closed proof before treating the candidate as complete.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Steps

### Step 1: Locate the Byval Pointer-Source Helper Surface

Goal: confirm the exact prepared helper, inputs, and consumer sites involved
in byval pointer-source classification.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/bir/backend_prepare_liveness_test.cpp`

Actions:
- Inspect
  `prepared_store_source_load_local_is_byval_formal_pointer_source(...)`.
- Inspect `is_byval_formal_value_name(...)` and how prepared value-name ids are
  converted to raw names.
- Inspect the publication-plan callers that set or consume
  `byval_load_local_source`.
- Identify the smallest agreement boundary and focused helper-test area.

Completion check:
- `todo.md` records the selected helper surface, owned files for Step 2, and
  any surfaces explicitly left out of scope.

### Step 2: Implement the Prepared Agreement Boundary

Goal: make byval pointer-source classification accept only complete prepared
addressing, source-producer, and formal-name agreement.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp` only if a small test-visible
  seam is needed
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Add or reuse a local helper that verifies the prepared memory-access row and
  prepared formal-name fact agree on the pointer value.
- Require the prepared source producer to remain a load-local producer in a
  valid block.
- Preserve `false` for absent addressing, absent source producer, wrong
  producer kind, missing memory access, missing pointer-value name,
  base-plus-offset rejection, non-byval formal names, and mismatches.
- Keep the consumer using prepared publication planning as the policy owner.

Completion check:
- Focused helper tests pass with the delegated narrow command.
- No output baselines or expectation contracts are weakened.

### Step 3: Add Nearby Fail-Closed Rows

Goal: prove the byval pointer-source candidate is not a named-case shortcut.

Primary target:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Cover the positive prepared agreement path.
- Cover fail-closed rows for absent addressing, absent source producer,
  wrong-kind producer, invalid block label, missing prepared memory access,
  non-pointer-value addressing, missing pointer name, base-plus-offset
  rejection, non-byval formal, missing formal, and prepared/BIR name mismatch.
- Keep existing public prepared compatibility assertions observable.

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
- `todo.md` states whether the byval pointer-source classification candidate
  is ready for plan-owner retirement review while the source idea remains
  open.
