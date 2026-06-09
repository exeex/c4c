Status: Active
Source Idea Path: ideas/open/145_current_block_join_fact_routing_split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Current Join-copy Fact and Routing Uses

# Current Packet

## Just Finished

Step 1 inspection completed for current-block join-copy fact/routing surfaces.
No implementation files were edited.

### Current Symbols and Classification

- `PreparedCurrentBlockJoinParallelCopySourceStatus`
  - Declaration/inline name helper: `src/backend/prealloc/prepared_lookups.hpp:28`,
    `src/backend/prealloc/prepared_lookups.hpp:38`.
  - Definition: inline in the header.
  - Direct consumers: source query status in
    `src/backend/prealloc/prepared_lookups.cpp:2717-2736`,
    routing status in `src/backend/prealloc/prepared_lookups.cpp:2945-2949`,
    AArch64 wrapper checks in
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:342-345`,
    and helper tests in
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1704-1707`,
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1766-1769`,
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1785-1795`.
  - Classification: reusable source fact/status.
  - Candidate owner: shared prealloc, likely beside the current-block join source
    query in `publication_plans.hpp/.cpp` or a small shared current-block join
    fact header if `publication_plans` would grow too broad.
  - Boundary: status currently includes query-input availability, not target
    routing state; safe to keep shared.

- `PreparedCurrentBlockJoinParallelCopySourceFact`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:97`.
  - Definition/population: `src/backend/prealloc/prepared_lookups.cpp:2779-2871`.
  - Direct consumers: source query tests inspect fact fields at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1722-1755`;
    AArch64 currently consumes only the aggregate source id vectors, not the per
    fact struct directly.
  - Classification: mostly reusable source fact.
  - Candidate owner: shared prealloc, close to `PreparedEdgeCopySourceFacts` in
    `publication_plans.hpp/.cpp`.
  - Boundary: fields `destination_is_source_value`, `source_is_source_value`,
    `source_shares_destination_register`, and `source_home_is_stack` are facts
    derived from out-of-SSA moves and value homes, but their current naming is
    shaped by AArch64 emission skipping. Preserve the raw facts; consider
    renaming/adapting target-facing policy in AArch64.

- `PreparedCurrentBlockJoinParallelCopySourceFacts`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:123`.
  - Definition/population: `src/backend/prealloc/prepared_lookups.cpp:2712-2937`.
  - Direct consumers: local AArch64 wrapper in
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:79-122`,
    source predicate in
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:335-350`,
    and helper tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1685-1720`.
  - Classification: reusable source fact aggregate.
  - Candidate owner: shared prealloc, likely `publication_plans.hpp/.cpp`
    because it composes `PreparedEdgeCopySourceFacts` via
    `prepare_block_entry_parallel_copy_edge_source_facts`.
  - Boundary: `incoming_expression_value_ids/names` and `source_value_ids/names`
    are reusable identity sets, but AArch64 uses them as emission-routing sets.
    Keep the sets shared; build target-local routing from them.

- `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:133`.
  - Definition: plain input struct.
  - Direct consumers: shared query functions in
    `src/backend/prealloc/prepared_lookups.cpp:2713`,
    `src/backend/prealloc/prepared_lookups.cpp:2941`, AArch64 construction at
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:107-121` and
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:381-396`, tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1686-1694`,
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1757-1765`,
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1785-1791`.
  - Classification: reusable query wiring.
  - Candidate owner: move with the shared source query.
  - Boundary: includes `block` so the query can compute current-block expression
    closure; that is still source fact work, not target routing.

- `PreparedCurrentBlockJoinParallelCopyInstructionRouting`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:143`.
  - Definition/population:
    `src/backend/prealloc/prepared_lookups.cpp:2940-2970`.
  - Direct consumers: AArch64 routing wrapper in
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:352-400`,
    helper tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1756-1784`.
  - Classification: target-local routing convenience.
  - Candidate owner: AArch64 `dispatch_producers.hpp/.cpp`, as a target-local
    adapter over shared source facts.
  - Boundary: depends on shared identity sets and shared
    `prepared_instruction_result_value_id`, but the vector<bool> answers are
    instruction-dispatch decisions for AArch64 skipping/lowering.

- `prepare_current_block_join_parallel_copy_source_facts`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:262`.
  - Definition: `src/backend/prealloc/prepared_lookups.cpp:2712`.
  - Direct consumers: `prepare_current_block_join_parallel_copy_instruction_routing`
    in `src/backend/prealloc/prepared_lookups.cpp:2944`, AArch64 local wrapper in
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:107`, source
    predicate in `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:342`,
    and tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1686`,
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1785`.
  - Classification: reusable source fact producer.
  - Candidate owner: shared prealloc, likely `publication_plans.cpp`.
  - Boundary: currently in `prepared_lookups` because it needs value-home lookup
    helpers and instruction-result/name helpers. Moving may require either
    moving those helper predicates or passing precomputed lookup/fact inputs.

- `prepare_current_block_join_parallel_copy_instruction_routing`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:266`.
  - Definition: `src/backend/prealloc/prepared_lookups.cpp:2940`.
  - Direct consumers: AArch64
    `build_current_block_join_prepared_query_routing` at
    `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:381-400`, tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:1757-1784`.
  - Classification: target-local routing convenience.
  - Candidate owner: AArch64 `dispatch_producers.cpp`; keep only shared source
    fact query exported from prealloc.
  - Boundary: can be reimplemented target-locally by calling the shared source
    fact query and mapping instruction result ids in AArch64.

- `prepared_value_homes_share_register_name`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:185`.
  - Definition: `src/backend/prealloc/prepared_lookups.cpp:1725`.
  - Direct consumers: out-of-SSA helper at
    `src/backend/prealloc/prepared_lookups.cpp:1756`, tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:84-97`.
  - Classification: reusable value-home fact predicate.
  - Candidate owner: shared prealloc value-home/publication utility, not
    AArch64.
  - Boundary: compares register spellings as prepared facts only; it does not
    choose target instructions.

- `prepared_out_of_ssa_parallel_copy_register_destination_matches_value`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:189`.
  - Definition: `src/backend/prealloc/prepared_lookups.cpp:1735`.
  - Direct consumers: current-block source fact construction at
    `src/backend/prealloc/prepared_lookups.cpp:2847-2849`,
    source-shares predicate at
    `src/backend/prealloc/prepared_lookups.cpp:1751-1752`, tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:133-159`.
  - Classification: reusable out-of-SSA/value-home fact predicate.
  - Candidate owner: shared prealloc, near move authority/value-home helpers.
  - Boundary: name mentions register destination because the fact is about a
    prepared register destination, not AArch64 routing.

- `prepared_out_of_ssa_parallel_copy_source_shares_destination_register`
  - Declaration: `src/backend/prealloc/prepared_lookups.hpp:194`.
  - Definition: `src/backend/prealloc/prepared_lookups.cpp:1746`.
  - Direct consumers: current-block source fact construction at
    `src/backend/prealloc/prepared_lookups.cpp:2856-2860`, tests at
    `tests/backend/bir/backend_prepared_lookup_helper_test.cpp:141-174`.
  - Classification: reusable out-of-SSA/value-home fact predicate.
  - Candidate owner: shared prealloc, with the register-destination predicate.
  - Boundary: shared-register detection is reusable; deciding to skip or emit a
    particular AArch64 instruction remains target-local.

### AArch64 Dispatch Consumers

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:79-122` builds the
  shared source-fact query inputs from `BlockLoweringContext`; reusable input
  construction can remain as an AArch64 adapter, while the callee should become
  the shared fact API.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:335-350`
  (`prepared_query_current_block_join_parallel_copy_source`) checks whether an
  instruction result is in the shared `source_value_ids` set. Classification:
  target-local routing predicate over reusable facts.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp:352-400`
  (`build_current_block_join_prepared_query_routing`) converts shared fact sets
  into per-instruction `incoming_expressions` and `sources` vectors.
  Classification: target-local routing convenience.
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp:49-69` declares
  `CurrentBlockJoinPreparedQueryRouting`,
  `build_current_block_join_prepared_query_routing`,
  `current_block_join_prepared_query_incoming_expression`, and
  `current_block_join_prepared_query_source`. Classification: target-local
  routing declarations.
- `src/backend/mir/aarch64/codegen/dispatch.cpp:511-512` builds routing once per
  block, then `src/backend/mir/aarch64/codegen/dispatch.cpp:720-723` skips
  incoming expression instructions without stack homes and
  `src/backend/mir/aarch64/codegen/dispatch.cpp:829-832` skips source
  instructions without stack homes. Classification: AArch64 dispatch consumers.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp:161-169`
  uses `prepared_query_current_block_join_parallel_copy_source` to materialize a
  prepared value home to a register. Classification: AArch64 routing/materializer
  consumer of reusable source facts.
- Nearby AArch64 edge-copy dispatch consumers use shared
  `prepare_block_entry_parallel_copy_edge_source_facts` at
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp:1278-1289` and
  should continue depending on `publication_plans` facts rather than the
  current-block routing adapter.

### Existing Shared Fact Surface

- `PreparedEdgeCopySourceFactsStatus`,
  `PreparedEdgeCopySourceFacts`, and
  `prepare_block_entry_parallel_copy_edge_source_facts` already live in
  `src/backend/prealloc/publication_plans.hpp:86-145`,
  `src/backend/prealloc/publication_plans.hpp:304-347`,
  `src/backend/prealloc/publication_plans.hpp:492-497`, with implementation in
  `src/backend/prealloc/publication_plans.cpp:963-1023`.
- Current-block source facts are built by iterating block-entry
  `OutOfSsaParallelCopy` move bundles and composing those existing shared
  edge-copy source facts at
  `src/backend/prealloc/prepared_lookups.cpp:2771-2807`.
- This makes `publication_plans` the strongest candidate owner for reusable
  current-block join-copy source facts; only the per-instruction routing vectors
  should move into AArch64 dispatch.

### Ambiguous Boundaries

- `source_is_source_value` currently treats same-register and stack-source
  cases as source-value identities. The raw inputs are reusable facts, but the
  combined boolean is close to AArch64 emission policy and should be reviewed
  during Step 4 before exposing it as a durable shared predicate name.
- Moving `prepare_current_block_join_parallel_copy_source_facts` out of
  `prepared_lookups.cpp` may require moving or rehoming helper access to
  `existing_prepared_value_name_id`, `prepared_instruction_result_value_ref`,
  `find_indexed_prepared_value_home`, and
  `find_indexed_prepared_value_id`.
- AST caller queries are translation-unit scoped: they confirmed internal
  prepared-lookup call relationships, while cross-file AArch64 consumers were
  identified with targeted `rg` and file reads.

## Suggested Next

Proceed to Step 2 with a narrow code packet: move or rename only the reusable
current-block join-copy source status/fact/query surface toward shared prealloc
ownership, leaving `PreparedCurrentBlockJoinParallelCopyInstructionRouting` and
per-instruction vector routing in AArch64 dispatch.

## Watchouts

- Do not treat target routing booleans as shared prealloc policy.
- Do not delete reusable join-copy source facts.
- Do not prove only one dispatch case while leaving nearby join-copy cases
  unexamined.
- Keep `PreparedEdgeCopySourceFacts` and current-block join facts aligned; the
  current query is already a wrapper over shared edge-copy source facts.
- Do not move AArch64 skip/emit decisions into `publication_plans`.

## Proof

Inspection-only analysis slice. No build or tests were run, and no
`test_after.log` was required by the delegated proof contract.
