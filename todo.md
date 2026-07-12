# Current Packet

Status: Active
Source Idea Path: ideas/open/708_x86_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory x86 route dependencies and bind migration packets

## Just Finished

- Completed Step 1's bounded x86 semantic inventory. Targeted AST caller
  queries plus a route-vocabulary scan found three executable dependency
  clusters, all in `x86.hpp` or `module/module.cpp`:
  - **Direct-call/scalar (Step 2):** `ConsumedPlans` constructs and retains a
    `Route6CallUseSourceIndex`; `find_consumed_scalar_i32_call_argument_source_authority`
    gates the prepared call argument through a Route 6 record and returns its
    source spelling; its executable consumers are
    `append_prepared_direct_extern_call_return_function` and
    `append_prepared_symbol_call_local_i32_function`. Replace this with the
    indexed `PreparedCallPlan`/`PreparedCallArgumentPlan` identity
    (`source_value_id`), the named BIR call argument spelling/type, and the
    prepared value home plus before-call move/ABI destination. Missing call or
    argument plans/homes/moves, absent or ambiguous named identity, incomplete
    placement, or ID/type/cursor mismatch must reject; the raw argument name is
    not a fallback authority. Positive surface:
    `backend_x86_handoff_boundary_direct_extern_call_test` and the scalar
    same-module cases in `backend_x86_handoff_boundary_multi_defined_call_test`;
    fail-closed surface: that suite's missing before/after-call bundle, load
    home, and local-frame-access mutations, tightened with a named/source-ID
    mismatch case.
  - **Memory (Step 3):** the three declarations/definitions of
    `render_agreed_route3_load_local_statement_memory_operand` ultimately call
    `find_agreed_route3_load_local_source_memory_access`, construct a Route 3
    index, compare Route 3 memory metadata, and in the statement lane admit a
    prepared-addressing compatibility fallback. AST callers cover compare-join
    return arms, same-module call-local loads, local-slot compare loads, guard
    prefixes/blocks, short-circuit loads, and local-slot returns. Replace the
    Route 3 record with the idea-706 named
    `find_bir_same_block_load_local_source_identity` /
    `BirMemoryAccessIdentity`, then require the existing prepared memory access,
    frame-slot/stack-object, and prepared source-publication fields to agree on
    block/instruction cursor, result ID/name/type, slot, address space, offset,
    width, alignment, and volatility. Unavailable/ambiguous named identity,
    missing or incomplete addressing/frame/source-memory authority, stale
    cursor, or any metadata mismatch rejects; the compatibility access cannot
    authorize emission. Positive surface: selected-loadlocal cases in
    `backend_x86_handoff_boundary_joined_branch_test` plus
    `backend_x86_handoff_boundary_local_slot_guard_lane_test`; fail-closed
    surface: missing/incomplete source-memory and carrier-only loadlocal cases,
    with slot/cursor/width mismatch retained.
  - **Edge publication and joined control (Steps 3 then 4):**
    `append_prepared_compare_join_parallel_copy` calls
    `edge_publication_move_allowed_by_route5_agreement_or_compatibility`, which
    rebuilds a Route 5 edge/join index and compares source value, producer,
    source-memory, and edge key; its compatibility branch can allow emission
    without Route 5 agreement. Replace the entire gate with
    `PreparedMirFunctionView::current_block_direct_edge_publication_sources`
    as already consumed by `consume_edge_publication_move_intent`, backed by
    the prepared edge publication, freshness authority, source/destination
    homes, move bundle, parallel-copy step, and prepared join/branch labels.
    Query states `MissingFunctionView`, `MissingBlock`, `MissingLookups`,
    `MissingValueLocations`, `MissingEdgePublicationLookups`, and
    `MissingSuccessorLabel`, and row states `MissingPublication`, missing/
    ambiguous/invalid freshness, unsupported source/destination home, move, or
    source, all fail closed when a publication move is required. Duplicate
    destination matches and edge/value/producer/cursor/home/move mismatches
    likewise reject. Positive surface:
    `backend_x86_handoff_boundary_compare_branch_test` predecessor and
    successor-entry parallel-copy cases and joined-branch scalar/local/global
    publication shapes; fail-closed surface: missing join transfer/parallel
    copy/publication/source-memory and Route-5 mismatch mutations (renamed to
    producer-contract language as implementation is migrated).
- The remaining route hits in `debug/debug.cpp`, `codegen/route_debug.hpp`, and
  summary/trace wrappers in `x86.hpp` only format diagnostics; they do not
  select emission and remain idea-712 work. `prepared_call_preservation_route_name`
  in emission error text and comments containing “route”/“fallback” are labels,
  not executable route authority. No direct Route 1/2/4/7/8 dependency was
  found under semantic x86 sources.

## Suggested Next

- Execute Step 2 as one bounded direct-call/scalar packet: remove Route 6 state
  and source-record helpers from `x86.hpp`, bind both AST-confirmed call
  consumers to prepared argument ID/home/before-call move authority, and add
  one positive direct-extern plus one same-module positive and named/source-ID
  mismatch fail-closed proof.

## Watchouts

- The existing prepared call argument plan publishes `source_value_id` but no
  target-local source spelling; obtain spelling only by validating the named
  BIR argument against prepared name/value identity. If that cannot be made
  unique with existing common/prepared contracts, report a producer gap rather
  than recreating Route 6 under another helper name.
- The Route 3 and Route 5 clusters share source-memory comparison code. Step 3
  should migrate the common named memory identity first, then publication, so
  Step 4 only consumes prepared branch/join/control facts.
- Debug-only route labels belong to idea 712. Keep AArch64, RV64, BIR
  quarantine, and broad fixture renaming out of these packets.

## Proof

- Inventory-only packet: no build or tests were requested or run.
- Completeness validation used targeted `rg` over every `*.cpp`/`*.hpp` under
  `src/backend/mir/x86/`, declaration/definition inspection in `x86.hpp` and
  `module/module.cpp`, and `c4c-clang-tool[-ccdb]` function-signature and direct
  caller queries. No `test_after.log` was produced because the delegated proof
  explicitly required no build or test execution.
