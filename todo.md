Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Source Metadata Planner Surface

# Current Packet

## Just Finished

Step 1 (`Locate the Source Metadata Planner Surface`) inventoried the
store-source publication planner surface for the source-value/source-producer
metadata packet without implementation changes.

Selected helper surface:
- `populate_store_source_publication_plans(...)` in
  `src/backend/prealloc/publication_plans.cpp` is the publication planner
  collection point. It derives the store source value, prepared destination
  memory access, prepared source home, same-block source producer,
  recovered-source fact, byval pointer-source fact, direct-global
  select-chain fact, duplicate-publication flag, destination frame slot/object,
  then calls `plan_prepared_store_source_publication(...)`.
- `plan_prepared_store_source_publication(...)` is the smallest write surface
  for `PreparedStoreSourcePublicationPlan` fields. It currently copies
  `source_value`, `source_value_id`, `source_value_name`, `source_home*`,
  storage encoding, source-producer kind/block/instruction/payload pointers,
  recovered/direct-global flags, pointer-base home fields, pending state, and
  duplicate state into the public plan.
- The Step 2 agreement boundary should be a local helper near
  `plan_prepared_store_source_publication(...)` that gates only the
  source-value/source-producer metadata publication: source value id/name,
  source home, producer kind, producer block label, producer instruction index,
  producer payload pointer, and produced value identity must agree before
  producer metadata is copied into the plan.
- Existing source-producer lookup agreement lives below
  `find_prepared_select_chain_source_producer(...)`, but the store-source plan
  still needs its own fail-closed tuple check because the plan helper accepts a
  raw `PreparedEdgePublicationSourceProducer*` input and publishes public
  metadata fields.

## Suggested Next

Execute Step 2 in:
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp` only if a small test-visible
  status/seam is unavoidable
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Concrete helper-test fixture area: add a focused verifier in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` near the completed
store-source fixtures
`verify_direct_global_select_chain_dependency_query()`,
`verify_prepared_same_block_scalar_source_facts()`, and
`verify_byval_pointer_source_classification_requires_prepared_agreement()`,
then invoke it from the same main-test sequence before
`verify_bir_block_entry_publication_identity_lookup()`.

## Watchouts

- Keep this runbook limited to the source-value/source-producer metadata
  candidate.
- Do not revisit completed recovered-source, byval pointer-source, or
  direct-global select-chain packets.
- Leave these policy surfaces out of Step 2 scope: status, intent,
  destination access, source-home discovery, source storage encoding,
  recovered-source policy, byval pointer-source policy, direct-global
  select-chain policy, pointer-base home policy, pending-publication policy,
  duplicate-publication policy, target lowering, storage encoding policy,
  output text, diagnostics, helper status names, baselines, expectations, and
  public prepared aggregate compatibility.
- Preserve existing false, empty, unavailable, or prepared-only behavior for
  missing source value/home/name, incomplete producer payloads, stale ids,
  wrong producer kind, block/instruction/value-name drift, route/BIR-only
  identity, pending publications, duplicate publications, recovered/direct
  global disagreement, pointer-base disagreement, and storage-policy
  rejection.
- Do not rewrite output expectations, diagnostics, helper statuses, baselines,
  or target output to claim progress.

## Proof

Inventory-only Step 1 packet; no build required by delegation. Ran
`git diff --check`.
