Status: Active
Source Idea Path: ideas/open/16_bir_edge_value_flow_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define Shared Edge Publication Facts

# Current Packet

## Just Finished

Completed the Step 2 computed-source prepared fact slice.

Discovery found a bounded target-neutral path: `make_prepared_function_lookups`
already has both the prepared module and control-flow edge publications, so the
source-producer scan can stay in shared prealloc instead of target lowering.
Extended `PreparedEdgePublication` with source producer kind, producer block
label, producer instruction index, and typed BIR producer pointers for
load-local, cast, binary, and select-style materialization sources. Existing
direct edge-publication lookup callers keep the old no-module behavior; the
module-backed prepared function lookup now publishes the computed-source facts.
Added focused shared coverage in `backend_prepared_lookup_helper` for all four
producer families.

## Suggested Next

Move to Step 3 with a narrow AArch64 consumption packet that reads
`PreparedEdgePublication::source_producer_kind` for the covered load-local,
cast, binary, and select-source cases and fails closed when a required prepared
producer fact is missing.

## Watchouts

- Computed producer facts are available only through module-backed lookup
  construction (`make_prepared_function_lookups` or the new overload taking
  `PreparedBirModule`). Direct `PreparedNameTables` calls preserve prior
  behavior and leave named producer kind as `unknown`.
- Duplicate BIR result names intentionally suppress the producer fact instead
  of guessing; this keeps the record fail-closed for non-unique SSA spelling.
- `matching_move_redundant_by_assigned_storage` is currently true for explicit
  coalesced moves or same source/destination value id; it does not infer
  physical register spelling equivalence.

## Proof

`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_prealloc_block_entry_publications|backend_prepare_authoritative_join_ownership|backend_prepared_lookup_helper|backend_publication_plan_record)$"'`
passed. Proof log: `test_after.log`.
