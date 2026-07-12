# Current Packet

Status: Active
Source Idea Path: ideas/open/710_rv64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit RV64 executable route authority

## Just Finished

- Completed Plan Step 1's RV64 executable-route audit from
  `prepared_edge_publication_emit.cpp` through its edge-publication and object
  intent callers. The scoped dependencies classify as follows:
  - Executable compatibility authority: the `route5_edge != nullptr` guard in
    `consume_prepared_backed_move_intent` rejects a prepared `LoadLocal`
    memory-source publication when `route5_edge_source_agrees` is false.
    That boolean recursively reads Route 5 status/edge/value/producer identity
    and Route 3 load/result/address/base identity through
    `route5_edge_source_agrees_with_prepared_publication`,
    `route3_source_memory_agrees_with_prepared_publication`,
    `route3_base_kind_agrees_with_prepared_source_memory`, and the
    `route5_source_kind_from_prepared` vocabulary adapter.
  - Compatibility fallback: non-memory Route 5 absence or disagreement does
    not gate emission; the already-prepared register/immediate publication is
    consumed. This is observable legacy fallback behavior, not authority that
    Step 2 must preserve as a route read.
  - Diagnostic-only: `attach_route5_edge_agreement` copies
    `route5_edge_status`, `route5_edge_source_agrees`, and
    `route3_source_memory_agrees` into `EdgePublicationMoveIntent`; outside the
    one memory-source guard above they are read only by structured object dump
    text and focused oracle assertions. Object-emission callers pass the Route
    5 row into the same adapter but do not independently branch on those
    fields.
  - Dead residue: no additional executable Route 3/5/agreement read was found
    in the scoped emitter, direct object-intent construction, or intent
    consumers.
- Existing typed prepared replacement is complete:
  - Publication owner/identity: `PreparedFunctionLookups::edge_publications`
    queried by `find_unique_indexed_prepared_edge_publication` with exact
    predecessor label, successor label, and destination value id, followed by
    `PreparedEdgePublicationLookupStatus::Available` (missing or ambiguous keys
    already return no publication).
  - Source identity: publication source value id/name/kind/type, producer
    kind plus producer block/instruction index and typed producer pointer,
    `source_home`/`source_home_kind`, and for memory sources the prepared base
    kind and frame-slot/symbol/pointer identity plus address-space, volatility,
    byte offset, size, alignment, layout/range, and materialization policy.
  - Move/publication identity: non-null `move` with `Move` op, source/destination
    value ids, authority and destination kind/storage/placement; publication
    also carries join transfer, edge transfer, parallel-copy bundle/step/site,
    move bundle, carrier kind, phase, and execution block identity.
  - Freshness identity: `source_memory_access_status == Available` and the
    publication-owned `source_memory_access` pointer must equal the unique
    prepared memory access indexed by source result value id; the selected
    pointer-base value id and prepared register home are then consumed. This is
    the existing fail-closed replacement for the Route 3/5 memory agreement.
- Focused proof surfaces are CTest
  `backend_riscv_prepared_edge_publication`: positive helpers
  `check_register_to_register_move_uses_shared_lookup`,
  `check_immediate_to_register_move_uses_shared_lookup`, and the dynamic
  memory-source portion of
  `check_route5_route3_oracle_rows_preserve_prepared_riscv_fallback`; negative
  helpers `check_stack_source_fail_closed_forms` and the missing, duplicate,
  stale, mismatched, and same-consumer drift cases in the agreement audit.
  Object-facing coverage is CTest `backend_riscv_object_emission` for prepared
  stack/FPR/large-offset intents and structured publication dumps.

## Suggested Next

- Execute Plan Step 2 in `prepared_edge_publication_emit.cpp` and `emit.hpp`:
  remove the Route 5 parameter/adapter attachment and the LoadLocal agreement
  guard; consume only the unique available prepared publication, exact move
  identities, source/destination homes, and unique publication-owned memory
  access described above; remove the three route-labelled intent fields and
  update focused assertions/dump spelling without weakening fail-closed cases.

## Watchouts

- Preserve the unique prepared memory-access pointer equality check: it is the
  freshness proof and must not be replaced by a target-local Route 3 lookup or
  a value-name/address reconstruction.
- Route 5 mismatch currently rejects only LoadLocal memory sources; deleting
  the guard must remain safe because prepared publication freshness already
  rejects stale, ambiguous, mismatched, missing, and incomplete source rows.
- Route-labelled dump fields and their oracle assertions are diagnostic
  residue, not semantic contracts. Keep supported/unsupported expectations
  unchanged while moving proof to prepared identities.

## Proof

- `git diff --check` (Step 1 audit proof): passed. No `test_after.log` was
  requested for this audit-only packet; implementation/build/test proof remains
  for Step 2.
