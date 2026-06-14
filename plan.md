# Prepared Module Direct-Global Select-Chain Dependency Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate one remaining idea 260 candidate: the
`store_source_publications` direct-global select-chain dependency packet.

## Goal

Allow select/direct-global dependency lookup to use same-block producer
identity only behind complete prepared agreement.

## Core Rule

Do not move store-source publication policy, call argument policy, target
output, or prepared aggregate compatibility into BIR. BIR or route facts may
only confirm a direct-global select-chain dependency when the prepared
source-producer lookup, block identity, and instruction cutoff agree and all
missing or mismatch rows fail closed.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/publication_plans.hpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Scope

- Candidate: `store_source_publications` direct-global select-chain dependency
  packet.
- Primary prepared helper surface:
  `find_prepared_direct_global_select_chain_dependency(...)` and
  `find_prepared_store_source_direct_global_select_chain_dependency(...)`.
- Primary producer lookup surface:
  `find_prepared_select_chain_source_producer(...)`,
  `prepared_select_chain_contains_direct_global_load(...)`, and
  `PreparedEdgePublicationSourceProducerLookups`.
- Primary consumers:
  `plan_call_argument_direct_global_select_chain_dependency(...)` and store
  source publication planning around
  `direct_global_select_chain_source`,
  `direct_global_select_chain_root_is_select`, and
  `direct_global_select_chain_root_instruction_index`.

## Non-Goals

- Do not implement the source-value/source-producer metadata packet.
- Do not revisit the completed module, names, control-flow, recovered-source,
  or byval pointer-source packets.
- Do not delete, privatize, wrap, or broadly retire
  `PreparedBirModule::store_source_publications` or other prepared aggregate
  fields.
- Do not rewrite printer/debug text, route-debug text, target output,
  diagnostics, helper statuses, baselines, or expectations to claim progress.
- Do not alter broad publication policy, storage encoding, pending policy,
  duplicate handling, pointer-base homes, target lowering, or call argument
  lowering outside this one dependency helper row.

## Working Model

- Prepared source-producer lookups remain the compatibility authority.
- A dependency is available only when the root value has an agreed same-block
  prepared source producer before the consumer instruction.
- The select-chain walk may report a direct-global dependency only when it can
  prove that the chain contains a direct global load and can record the root
  producer's instruction index.
- Missing source-producer lookups, invalid block labels, null blocks, unnamed
  values, absent prepared names, producer/value-name drift, same-block cutoff
  failures, after-store or after-call producers, non-direct-global roots, and
  incomplete select-chain producers must keep dependency fields false or empty.

## Execution Rules

- Keep each implementation packet focused on this dependency helper or its
  direct consumer boundary.
- Prefer local agreement checks over broad API movement.
- Preserve public prepared fallback behavior and existing prepared lookup
  maps.
- Add focused fail-closed proof before treating the candidate as complete.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates a different artifact.

## Steps

### Step 1: Locate the Direct-Global Select-Chain Helper Surface

Goal: confirm the exact prepared helper, producer lookup, and consumer sites
involved in direct-global select-chain dependency classification.

Primary target:
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Inspect `find_prepared_direct_global_select_chain_dependency(...)`.
- Inspect `find_prepared_store_source_direct_global_select_chain_dependency(...)`.
- Inspect `find_prepared_select_chain_source_producer(...)` and the recursive
  direct-global scan in `prepared_select_chain_contains_direct_global_load(...)`.
- Inspect the call-plan and store-source publication consumers that publish
  direct-global select-chain dependency fields.
- Identify the smallest agreement boundary and focused helper-test area.

Completion check:
- `todo.md` records the selected helper surface, owned files for Step 2, and
  any surfaces explicitly left out of scope.

### Step 2: Implement the Prepared Agreement Boundary

Goal: make direct-global select-chain dependency lookup accept only complete
prepared source-producer, block, and before-instruction agreement.

Primary target:
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.hpp` only if a small test-visible
  seam is needed
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Add or reuse a local helper that verifies the prepared source-producer row
  agrees with the queried BIR value, block label, block, and
  before-instruction cutoff.
- Require the root producer to be before the consumer and to have a complete
  instruction index for the dependency result.
- Preserve false or empty dependency fields for absent source-producer lookup,
  invalid block identity, missing prepared names, producer/value-name drift,
  incomplete producer payloads, non-direct-global chains, and after-consumer
  producers.
- Keep call-plan and store-source consumers using prepared planning as the
  policy owner.

Completion check:
- Focused helper tests pass with the delegated narrow command.
- No output baselines or expectation contracts are weakened.

### Step 3: Add Nearby Fail-Closed Rows

Goal: prove the direct-global select-chain dependency candidate is not a
named-case shortcut.

Primary target:
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Cover the positive prepared agreement path.
- Cover fail-closed rows for absent source-producer lookup, invalid or
  conflicting block identity, missing prepared value name, wrong producer
  kind, root not select when select-specific fields are expected, missing
  producer payload, non-direct-global select chains, producer after the
  consumer cutoff, duplicate or conflicting producers, and prepared/BIR
  value-name mismatch.
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
- `todo.md` states whether the direct-global select-chain dependency candidate
  is ready for plan-owner retirement review while the source idea remains
  open.
