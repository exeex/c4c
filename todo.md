# Current Packet

Status: Active
Source Idea Path: ideas/open/718_prepared_routing_root_dependency_classification_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Bind each probe to one owned backend seam

## Just Finished

- Step 3 bound each green focused probe to one prepared owner/query seam:
  - `current_block_direct_root_classification_probe`: owner-side rule is that a
    publication root is authoritative only under its prepared `Source` role;
    querying the same stable `(routed_value_id, routed_value_name)` as
    `IncomingExpression` is a role mismatch. Input is
    `PreparedFunctionLookups::current_block_join_routing_facts` plus
    `(successor_label, routed_value_id, routed_value_name, role)`; output is
    `PreparedCurrentBlockJoinRoutingConsumption`. All applicable facts must
    agree on complete destination id/name, source id/name, and non-unknown
    semantic origin; duplicate semantic edges, negative facts, invalid fields,
    or disagreement fail closed. Single implementation surface:
    `query_prepared_current_block_join_routing_consumption` in
    `src/backend/prealloc/publication_plans.cpp`.
  - `current_block_composed_dependency_authority_probe`: owner-side rule is
    that a producer dependency receives its own prepared
    `IncomingExpression` stable key and remains independently queryable when
    the enclosing direct root is rejected for that role. Inputs and output are
    the same stable-key consumption query above, with the dependency's exact
    id/name and `IncomingExpression` role. Id/name must agree, and all parallel
    applicable edge facts must agree on destination, preserved direct source,
    and semantic origin; wrong keys, missing/negative facts, duplicates, or
    parallel disagreement fail closed. Single implementation surface:
    `query_prepared_current_block_join_routing_consumption` in
    `src/backend/prealloc/publication_plans.cpp`.
  - `current_block_memory_source_authority_probe`: owner-side rule is that a
    memory-backed routed operand is authorized by the complete prepared edge
    publication identity, not by stack-home membership alone. Inputs are
    `(PreparedEdgePublication, consumer bir::Inst, operand bir::Value)`; output
    is `PreparedCurrentBlockRoutedOperandAuthority` naming the preserved source
    identity and authoritative value. Publication availability, consumer
    operand membership, exact source value/kind, source id/name, producer
    result, and source-home id/name/kind must agree; a home-only source or any
    id/kind/home conflict fails closed. Single implementation surface:
    `query_prepared_current_block_routed_operand_authority` in
    `src/backend/prealloc/publication_plans.cpp`.
  - `current_block_short_circuit_dependency_probe`: owner-side rule is that a
    direct prepared incoming expression may compose only the transitive
    operands of unique supported Binary/Cast/Select producers in the current
    block; each dependency is published under its own stable id/name while
    retaining the direct edge's destination, source, role, and semantic
    origin. Inputs are `PreparedCurrentBlockJoinParallelCopySourceQueryInputs`
    (names, locations/homes, edge publications, control flow, named source
    evidence, block, successor); output is
    `PreparedCurrentBlockJoinParallelCopySourceFacts`, especially
    `incoming_expression_value_{ids,names}` and `routing_facts`. The edge
    publication, complete Phi/JoinTransfer origin, freshness/evidence, unique
    producer, supported instruction shape, resolvable dependency id, and
    non-duplicate composed fact must all agree; missing or ambiguous producers
    stop closure so the orphan and its transitive leaf fail closed. Single
    implementation surface: `prepare_current_block_join_parallel_copy_source_facts`
    in `src/backend/prealloc/publication_plans.cpp`.

## Suggested Next

- Execute Step 4 by implementing the narrow generic direct-root role
  classification at the stable-key owner query, preserving the composed
  dependency, memory, and short-circuit contracts unchanged.

## Watchouts

- Direct-root rejection and dependency preservation are separate query
  outcomes even though both are owned by the same stable-key consumption
  function; Step 4 must not filter all facts that preserve the direct source.
- Stable authority requires the `(PreparedValueId, ValueNameId, role)` tuple,
  plus agreement of every applicable edge fact. Successor membership, storage
  identity, attachment, Route 5 payload, names, or fixture values alone grant
  no authority.
- The short-circuit closure must keep `%short.selected` and `%rhs.add` as
  distinct producers and must not admit `%rhs.leaf` when the add producer is
  absent.

## Proof

- No code proof was required for Step 3. Inspected all four registered probe
  contracts and used AST-backed definition/caller queries to bind their owner
  seams in `publication_plans.cpp`; no implementation, test, or proof-log file
  was changed.
