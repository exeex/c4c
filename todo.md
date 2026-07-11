# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Bind each probe to one owned backend capability

## Just Finished

- Completed plan Step 5 by binding all four focused probes to narrow,
  target-independent backend owners and implementation-ready semantic rules:
  - **Direct publication identity** — owner:
    `src/backend/prealloc/publication_plans.cpp`,
    `query_prepared_current_block_routed_operand_authority` (public contract in
    `publication_plans.hpp`). Rule: an operand is directly authoritative only
    when it is an actual operand of the consumer and exactly equals the
    available publication's preserved `source_value`; for a named source, its
    kind, `source_value_id`, `source_value_name`, and `source_home` must be
    present and mutually consistent. Return the preserved publication source
    as both identity and authority. A dependency, rewritten source, missing
    identity component, or conflicting home/name is `Mismatched`, never an
    alternate identity.
  - **Transitive producer closure** — owner:
    `src/backend/prealloc/publication_plans.cpp`,
    `prepare_current_block_join_parallel_copy_source_facts`, at the production
    of `incoming_expression_value_ids/names` from prepared source facts (with
    producer identity supplied by the prepared BIR block/evidence boundary).
    Rule: start at each validated direct incoming-expression source and follow
    named data operands of its unique prepared producer recursively; add each
    `(PreparedValueId, ValueNameId)` once in deterministic producer/operand
    order. A visited set terminates cycles. Missing producer evidence stops
    that branch before adding an unproved dependency; zero or multiple
    mutually valid producers, conflicting id/name pairs, invalid names/ids, or
    incomplete evidence fail that branch closed rather than selecting by
    instruction order. Immediates terminate without adding a named member.
  - **No-policy/zero-fact behavior** — owner:
    `src/backend/prealloc/publication_plans.cpp`,
    `query_prepared_current_block_join_policy`, consuming the
    `PreparedCurrentBlockJoinRoutingPolicy` state defined in
    `publication_plans.hpp`. Rule: null policy is `AbsentPolicy`; an attached
    policy without `owner_facts` is `AbsentOwner`; an attached owner whose
    stable-key query yields no available applicable fact (including empty,
    missing, mismatched, incomplete, conflicting, or ambiguous facts) is
    `AttachedOwnerWithoutApplicableFacts`; only an available, internally
    consistent consumption is `AuthoritativeFacts`. The first three states
    carry no consumption and grant no routing authority; owner presence or an
    empty vector is never authority.
  - **Stable-key owner-fact composition** — production owner:
    `src/backend/prealloc/publication_plans.cpp`, the `append_routing_fact`
    boundary inside `prepare_current_block_join_parallel_copy_source_facts`;
    function-level aggregation owner:
    `src/backend/prealloc/prepared_lookups.cpp`,
    `make_prepared_function_lookups`; validation/lookup owner:
    `query_prepared_current_block_join_routing_consumption`. Rule: publish a
    routing fact only from an available prepared parallel-copy source fact
    with a valid publication/move, selected freshness for non-immediates, and
    a valid routed home. Its stable lookup key is exactly
    `(successor_label, routed_value_id, routed_value_name, role)`; all matching
    facts must also agree on predecessor, destination id/name, source id/name,
    and semantic origin. Missing or negative facts fail closed; duplicate
    semantic edges and conflicting invariants are `Ambiguous`. Aggregate only
    these upstream facts into `PreparedFunctionLookups`; consumers may query
    the key but may not reconstruct facts.
- Explicitly rejected Route 5 attachment, target/AArch64 scans, publication
  source rewriting, and integration-vector or named-operand matching as owners
  or implementations of any rule.

## Suggested Next

- Execute plan Step 6 as one implementation packet at
  `query_prepared_current_block_routed_operand_authority`, making the direct
  publication-source identity probe green while preserving the exact source
  identity and all fail-closed conflicts.

## Watchouts

- Keep the four owners separate during implementation: direct identity must
  not absorb dependency traversal, closure must not select the first producer,
  policy state must not turn absence/zero facts into authority, and stable-key
  consumers must not reconstruct upstream facts.
- `prepare_current_block_join_parallel_copy_source_facts` still exposes the
  legacy `attach_route5_current_block_join_source_if_agrees` callee in its AST
  dependency set. Do not extend or use that route for Steps 6-9.
- Keep `backend_aarch64_current_block_join_routing` integration-only and do not
  weaken the three intentionally red focused contracts.

## Proof

- Analysis-only packet as delegated: no build or tests were run, and
  `test_before.log` / `test_after.log` were not modified.
- Ownership was traced with `c4c-clang-tool-ccdb` symbol and callee queries for
  `prepare_current_block_join_parallel_copy_source_facts`,
  `make_prepared_function_lookups`, and
  `query_prepared_current_block_join_routing_consumption`, followed by bounded
  inspection of their definitions and the four focused probes.
