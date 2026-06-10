# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the prepared edge-source oracle

## Just Finished

Step 1 inspected the prepared edge-source oracle and classified the BIR semantic
boundary.

Inspected definitions/helpers/callers:

- `PreparedEdgePublicationSourceProducer` and
  `PreparedEdgePublicationSourceProducerLookups`: producer kind, producer block
  label, instruction index, and typed BIR instruction pointers for load-local,
  load-global, cast, binary, and select materialization.
- `PreparedEdgePublication` and `PreparedEdgePublicationLookups`: indexed by
  predecessor label, successor label, and destination value id; mixes semantic
  edge/source identity with destination/source homes, move resolution, parallel
  copy step, carrier, phase, and execution-site routing.
- `PreparedEdgeCopySourceFacts`: copies the publication source subset for
  edge-copy queries and validates missing publication, ambiguous publication,
  unavailable publication, edge mismatch, missing source value/home/producer,
  and missing/incomplete source memory access.
- `PreparedCurrentBlockJoinParallelCopySourceFact(s)`: collects current-block
  join facts, incoming expression closure, and source value identities, but also
  carries move, bundle, destination register, register-sharing, stack-source,
  and immediate-source convenience flags.
- `make_prepared_edge_publication_lookups`: builds publications from
  `join_transfers`, derives source value/name/id/kind, applies source producer
  facts, applies optional load-local source memory facts, then attaches homes,
  move bundles, coalescing/redundancy, step index/kind, cycle temp, execution
  site/block, and prepared move pointers.
- `find_indexed_prepared_edge_publication_source_producer`: consumed by
  publication construction plus AArch64 dispatch/call/memory consumers as the
  producer oracle keyed by source value name.
- `prepare_edge_copy_source_facts` and
  `prepare_block_entry_parallel_copy_edge_source_facts`: query the indexed edge
  publication tuple and optionally require exact block-entry parallel-copy move
  identity.
- `prepare_current_block_join_parallel_copy_source_facts`: walks block-entry
  out-of-SSA parallel-copy bundles for the successor, queries the edge-source
  oracle, and records incoming-expression/source closures plus routing flags.

BIR-owned semantic fields to model:

- Edge key: `predecessor_label`, `successor_label`, destination value/name/id
  identity.
- Source identity: `source_value`, optional `source_value_id`,
  `source_value_name`, `source_value_kind`.
- Producer identity: `source_producer_kind`,
  `source_producer_block_label`, `source_producer_instruction_index`, and the
  corresponding BIR producer node identity for load-local, load-global, cast,
  binary, or select materialization.
- Optional load-local memory-source identity as a semantic access link:
  availability/status, access pointer or equivalent BIR memory-access identity,
  result value name, base kind, local/global/pointer source identity,
  byte offset, size, align, address space, and volatile flag. The BIR surface
  should treat this as memory-access identity only, not target addressing policy.
- Current-block join source identity: incoming expression/source value ids and
  names, fact status, edge predecessor/successor/destination/source identities,
  and whether the source is an incoming expression or source value when this is
  describing the semantic value relationship.

Rejected prealloc/codegen routing fields:

- `source_home`, `destination_home`, home kinds, destination storage kind,
  destination register names, stack-source convenience, and register-sharing
  checks.
- `source_and_destination_same_value_id` when used for coalescing/redundancy,
  `matching_move_coalesced_by_assigned_storage`,
  `matching_move_redundant_by_assigned_storage`, `phase`, `carrier_kind`,
  parallel-copy step index/kind, cycle-temp source flag, bundle cycle flag,
  execution site/block label, `join_transfer`, `edge_transfer`,
  `parallel_copy_bundle`, `move_bundle`, and `move`.
- Memory routing policy fields such as base-plus-offset usability and
  address-materialization requirement should not become BIR ownership even
  though they are copied beside source-memory identity today.

Existing coverage found in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:

- Normal edge/source path: `verify_edge_publication_shared_source_and_parallel_copy_facts`
  checks named source publication, edge-copy source facts, exact move identity,
  tuple-key lookup, missing publication, ambiguous publication, unavailable
  publication, missing source value/home, immediate source, same-value,
  stack-source, cycle/temp, and move-edge mismatch paths.
- Join-source path: `verify_current_block_join_parallel_copy_source_query`
  checks named, immediate, stack-source, unsupported-move, missing lookup, source
  value identity, and incoming expression closure paths.
- Optional memory-source path: `verify_edge_publication_source_producer_facts`
  checks load-local producer identity and source-memory access fields, plus
  memory match/mismatch helpers and load/cast/binary/select producer kinds.
- No-source / unavailable paths: edge-source facts cover missing prepared
  lookups, missing publication, ambiguous publication, unavailable publication,
  missing source value/home, missing source producer via helper status, immediate
  sources, unknown producer, and publicationless source facts.

Missing narrow coverage to add before or with Step 2:

- Direct prepared-vs-BIR equivalence tests for the proposed CFG edge query do
  not exist yet; current tests prove prepared helper behavior only.
- No focused BIR-facing query coverage for no-source/unavailable-source edge
  answers keyed by predecessor/successor/destination.
- No focused BIR-facing query coverage proving optional load-local
  memory-source identity is included while base-plus-offset/materialization
  policy is excluded.
- No focused BIR-facing join-source equivalence coverage that separates
  incoming expression/source identity from destination register, stack-source,
  and register-sharing convenience flags.

## Suggested Next

Proposed Step 2 code-changing packet: add a target-neutral BIR CFG edge
publication/source identity query surface keyed by predecessor label,
successor label, and destination value/name. Populate only the semantic source,
producer, optional memory-access identity, and current-block join identity
fields listed above, keep the prepared oracle intact, and add narrow tests that
compare BIR answers with prepared answers for normal edge, no-source, optional
load-local memory-source, and current-block join-source paths without switching
MIR consumers.

## Watchouts

- Do not copy full `PreparedEdgePublication`, `PreparedEdgeCopySourceFacts`, or
  `PreparedCurrentBlockJoinParallelCopySourceFact` shapes into BIR; they contain
  homes, move routing, and target/prealloc policy.
- Keep parallel-copy scheduling, move execution, cycle temp routing,
  destination registers, storage-sharing checks, and prepared move records
  outside BIR.
- Treat prepared publication helpers as the oracle until BIR equivalence is
  proven with nearby positive and negative coverage.
- Do not weaken tests or add named-case shortcuts to claim progress.

## Proof

Inspection-only/todo-only packet. No tests were run, and `test_after.log` was
not created or modified per the delegated proof contract.
