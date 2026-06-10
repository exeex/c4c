Status: Active
Source Idea Path: ideas/open/166_bir_annotation_lookup_index_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Route Index Surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for
`ideas/open/166_bir_annotation_lookup_index_schema.md`.

## Suggested Next

Execute Step 2: add the first narrow Route index record-reference contract and
validation proof without changing production consumers. Start with Route 7
comparison/condition indexes because they already have function-backed and
block-backed rebuild paths, explicit negative statuses, and a migrated BIR
helper (`find_materialized_condition_producer_identity`) that can prove the
contract without introducing a new MIR API.

Delegated Step 2 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log`

Also run `git diff --check`.

## Route Index Inventory

- Route 1 producer/materialization:
  `Route1ProducerIndex` is block-scoped and stores rebuilt
  `Route1ProducerRecord` snapshots. `Route1SameBlockProducerQuery` adds the
  before-instruction boundary. Lookup keys are block identity, produced value
  name/type/kind, and `before_instruction_index`; reverse scan preserves the
  nearest same-block producer behavior. Public lookup results may reference a
  record plus producer instruction/value pointers. Negative/fail-closed cases:
  no index/block, no producer before boundary, duplicate/non-unique value
  identity by scan order, non-materializable producer, and integer-constant
  recursion miss.
- Route 2 select-chain:
  `Route2SelectChainValueIndex` is block-scoped and stores rebuilt
  `Route2SelectChainValueRecord` snapshots. `Route2SelectChainValueQuery`
  carries the before boundary. Lookup keys are block identity plus normalized
  root value name/type/kind and boundary. MIR callers explicitly normalize a
  non-void request type instead of silently using `root_value->type`.
  Negative/fail-closed cases: no root producer, root after boundary,
  non-select root, explicit no direct-global dependency, mismatched root type,
  and missing direct-global load.
- Route 3 memory/access:
  `Route3MemoryAccessIndex` is block-scoped and stores rebuilt
  `Route3MemoryAccessRecord` snapshots. Direct lookup keys are block,
  instruction index, and `Route3MemoryAccessNodeKind`; same-block lookup keys
  add root value identity and `before_instruction_index`. Value-link records
  are separate construction helpers, not an index vector today. Negative cases:
  wrong node kind, no source, before-boundary miss, type mismatch,
  address-space/volatile-sensitive exclusion, non-global/non-local source, and
  same-slot invalidating store.
- Route 4 publication availability:
  `Route4PublicationAvailabilityIndex` is function-scoped and groups rebuilt
  current-block records, block-entry records, and value-link records. Current
  block lookup keys are function, block label/id, value name/id/type, and
  before boundary. Block-entry lookup keys are successor block label/id and
  destination value name/id/type. Negative cases: missing block/value,
  missing publication, alternate source, unavailable, no-match, and
  before-boundary miss. Current-block MIR lookup is migrated; block-entry is
  still oracle-covered and unswitched.
- Route 5 edge/join source:
  `Route5EdgeJoinSourceIndex` is function-scoped and groups rebuilt CFG edge
  publication records, current-block join-source records, and value-link
  records. Edge lookup keys are predecessor block label/id, successor block
  label/id, and destination value name/id/type; it must continue scanning
  across same successor/destination candidates until the requested predecessor
  is found, then report explicit `NoSource` only after exhaustion. Join lookup
  keys are successor block, destination value, and source value identity.
  Negative cases: no-source, memory-source, missing predecessor/successor/
  destination/publication/source value/source producer/source memory access,
  incomplete memory source, wrong edge, wrong destination/type, and no-match.
- Route 6 call-use source:
  `Route6CallUseSourceIndex` is function-scoped and groups rebuilt argument
  source, argument producer/materialization, direct-global, publication/memory
  source, result, and result-lane records. Lookup keys are block label/id, call
  instruction index, callee link/name, argument index, result value identity,
  and result-lane value identity. Negative cases: missing/wrong call, missing
  argument/result/source relationship/source value/source producer/direct
  global/memory source/publication source, ABI-bound excluded, duplicate
  relationship, duplicate result lane, and no-match. Duplicate result-lane
  records are keyed by lane identity and must not expose dangling lane pointers
  for negative records.
- Route 7 comparison/condition:
  `Route7ComparisonConditionIndex` can be function-scoped or block-scoped and
  groups rebuilt comparison instruction records, operand records, and branch
  condition records. Lookup keys are block label/id, instruction index,
  condition value identity, operand role/value identity, before-instruction
  boundary, and terminator true/false labels. Negative cases: missing block,
  missing instruction, wrong instruction, non-comparison, missing condition
  value, missing operand producer, duplicate producer, absent provenance, and
  no-match. The block-backed builder is important because the migrated
  materialized-condition helper must preserve caller block pointer identity.

## Record Reference Contract Targets

- Existing indexes are rebuildable snapshots of typed Route records; none yet
  has an explicit route-neutral typed reference contract that proves an index
  entry still corresponds to the current BIR owner and relationship key.
- Step 2 should add a narrow contract around Route 7 first: typed references
  from `Route7ComparisonConditionIndex` entries back to the route-specific
  record category, owner scope (`Function` or `Block`), block label/id,
  instruction or terminator key, value identity, operand role, and status.
  The contract should validate that lookup answers come from the rebuilt Route
  7 records and fail closed for stale owner/block, wrong role, wrong
  instruction, duplicate producer, absent provenance, and no-match.
- The contract must not copy `PreparedFusedCompareOperandProducer`,
  `PreparedFusedCompareOperandProducerFacts`, or
  `PreparedMaterializedConditionProducer`; those remain oracle inputs in tests.
- If a private aggregate facade is needed later, it should remain a
  non-durable implementation detail such as a stack-local route index bundle.
  It may hold typed route indexes/references but not semantic payload copies or
  target/prealloc policy fields, and it must not become a BIR-owned
  `PreparedFunctionLookups` clone.

## Payload Boundaries

- Durable semantic payloads stay on Route 1 through Route 7 typed annotation
  records. Indexes should carry owner/key/reference metadata and, where legacy
  vectors still contain rebuilt records, validation must prove those snapshots
  are rebuildable from current BIR annotation records.
- Keep target policy out of index payloads: ABI/register names, stack slots,
  outgoing stack sizing, frame/layout facts, addressing-mode legality,
  relocation/TLS spelling, dynamic-stack/helper plans, parallel-copy order,
  move scheduling, final instruction records/order, branch emission policy,
  and storage homes remain owned by prepared/prealloc/target code.
- Preserve explicit status records where a BIR owner/key exists. Absence alone
  should not stand in for unavailable, no-source, missing-source, duplicate,
  wrong-relationship, stale-reference, or no-match outcomes.

## Coverage Targets

- Positive coverage should tie index lookup answers to typed Route records for
  the selected Step 2 Route 7 target: fused operand facts, materialized
  condition producer identity, and branch-condition provenance.
- Negative coverage should include stale/wrong block or owner, wrong operand
  role, wrong instruction index, missing producer, non-comparison,
  duplicate-producer, absent provenance, before-boundary miss, and no-match.
- Existing route tests already cover Route 1 through Route 6 success and
  fail-closed behavior; future packets should avoid weakening those oracles
  while adding the common reference/validation contract route by route.

## Watchouts

- Keep typed Route 1 through Route 7 annotation records as the semantic source
  of truth.
- Do not create a durable BIR-owned `PreparedFunctionLookups` clone.
- Do not add ABI, layout, register allocation, move scheduling, call plan,
  memory-addressing, frame, dynamic-stack, helper, final-emission,
  storage-home, or target lowering policy data to function-local indexes.
- Do not duplicate semantic payload fields in indexes when typed annotations
  already own those payloads.
- Preserve explicit divergence, stale-reference, duplicate-key, missing-record,
  wrong-relationship, and no-match cases where applicable.

## Proof

Inventory-only packet. No build or test proof required.

Validation run after editing: `git diff --check`.
