# Prepared Module Store-Source Metadata Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate the remaining idea 260 candidate:
`store_source_publications` source-value/source-producer metadata.

## Goal

Allow store-source publication planning to use source value and
route/prepared producer identity only behind complete prepared agreement.

## Core Rule

Do not move store-source publication policy, storage policy, pending policy,
duplicate handling, pointer-base authority, target lowering, output text, or
prepared aggregate compatibility into BIR. BIR or route facts may only confirm
source-value and source-producer metadata when prepared planner facts remain
authoritative and every missing, mismatch, duplicate, route-only, BIR-only, or
policy-sensitive row fails closed.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/names.cpp`
- `src/backend/prealloc/value_locations.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Scope

- Candidate: `store_source_publications` source-value/source-producer metadata
  packet.
- Primary publication surface:
  `populate_store_source_publication_plans(...)`,
  `plan_prepared_store_source_publication(...)`, and nearby helpers that fill
  `PreparedStoreSourcePublicationPlan`.
- Primary metadata fields:
  `source_value`, `source_value_id`, `source_value_name`,
  `source_producer_kind`, `source_producer_block_label`,
  `source_producer_instruction_index`, source memory-access status,
  recovered-source flags, direct-global select-chain flags, pointer-base homes,
  pending-publication state, and duplicate-publication state.
- Primary compatibility surface:
  public prepared `store_source_publications` aggregate behavior and existing
  helper/status names.

## Non-Goals

- Do not revisit the completed recovered-source identity, byval pointer-source
  classification, or direct-global select-chain dependency packets.
- Do not delete, privatize, wrap, or broadly retire
  `PreparedBirModule::store_source_publications` or other prepared aggregate
  fields.
- Do not rewrite printer/debug text, route-debug text, target output,
  diagnostics, helper statuses, baselines, or expectations to claim progress.
- Do not alter call argument lowering, target lowering, stack/frame layout,
  storage encoding policy, pointer-base policy, pending store-global policy, or
  duplicate-publication policy outside the selected metadata agreement row.
- Do not replace prepared planner authority with route-only or BIR-only facts.

## Working Model

- Prepared publication planning remains the policy owner for status, intent,
  destination access, source home, storage encoding, recovered-source flags,
  direct-global flags, pointer-base homes, pending policy, and duplicate policy.
- Route or BIR identity may only confirm that the source value and prepared
  source producer describe the same same-block value under the prepared name
  and location facts already available to the planner.
- Missing prepared names, missing source homes, stale value ids, route/BIR
  drift, duplicate source values, duplicate producer rows, incomplete producer
  payloads, unavailable destination access, storage-policy rejections,
  recovered/direct-global disagreement, pointer-base disagreement, pending
  policy, and duplicate policy must keep existing false, empty, unavailable, or
  prepared-only results.

## Execution Rules

- Keep each implementation packet focused on publication-planner metadata and
  its direct fail-closed proof.
- Prefer local agreement checks over broad API movement.
- Preserve public prepared aggregate compatibility and existing prepared lookup
  maps.
- Add focused fail-closed proof before treating the candidate as complete.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Steps

### Step 1: Locate the Source Metadata Planner Surface

Goal: identify the exact helper and field writes that publish source value and
source-producer metadata for store-source publication plans.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Inspect `populate_store_source_publication_plans(...)` and
  `plan_prepared_store_source_publication(...)`.
- Inspect helpers that derive source value id/name, source home, source
  producer, memory-access status, recovered-source flags, direct-global flags,
  pointer-base home fields, pending-publication state, and duplicate state.
- Identify the smallest agreement boundary that can validate source
  value/source-producer metadata without taking over unrelated policy.
- Identify focused helper-test fixtures that already cover store-source
  publication planning and nearby completed candidates.

Completion check:
- `todo.md` records the selected helper surface, owned files for Step 2, and
  policy surfaces explicitly left out of scope.

### Step 2: Implement the Prepared Metadata Agreement Boundary

Goal: make source-value/source-producer metadata publication require complete
prepared agreement while preserving prepared planner policy.

Primary target:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp` only if a small test-visible
  seam is needed
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Add or reuse a local helper that verifies the prepared source value id/name,
  source home, source producer kind, source producer block label, instruction
  index, and same-block value identity agree before metadata is published as
  route/BIR-confirmed.
- Keep status, intent, destination access, source storage encoding,
  recovered-source flags, direct-global flags, pointer-base homes, pending
  policy, and duplicate policy in the prepared planner.
- Preserve current false, empty, unavailable, or prepared-only results for
  missing source value, missing source home, stale ids, invalid names,
  incomplete producer payloads, wrong producer kind, block mismatch, instruction
  mismatch, value-name drift, route/BIR-only identity, BIR/prepared mismatch,
  pending publication, duplicate publication, and storage-policy rejection.

Completion check:
- Focused helper tests pass with the delegated narrow command.
- No output baselines, status names, diagnostics, or expectation contracts are
  weakened.

### Step 3: Add Nearby Fail-Closed Rows

Goal: prove the metadata candidate is not a named-case shortcut.

Primary target:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Cover the positive prepared agreement path.
- Cover fail-closed rows for missing source value, missing source home, stale
  prepared value id, missing or invalid source value name, duplicate or
  conflicting source values, missing source producer, wrong producer kind,
  missing producer payload, block mismatch, instruction-index mismatch,
  producer-after-consumer ordering, prepared/BIR value-name drift, recovered
  source disagreement, direct-global disagreement, pointer-base home
  disagreement, pending-publication policy, duplicate-publication policy, and
  storage-policy rejection.
- Keep public prepared compatibility assertions observable.

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
- `todo.md` states whether the source-value/source-producer metadata candidate
  is ready for plan-owner close review of the whole source idea.
