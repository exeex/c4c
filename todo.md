Status: Active
Source Idea Path: ideas/open/141_prepared_lookups_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Draft closure payload and follow-up ideas

# Current Packet

## Just Finished

Step 5 closure payload drafted for the prepared-lookups residual owner audit.
The payload consolidates the Step 1 inventory, Step 2 definition map, Step 3
consumer map, and Step 4 classifications into close-ready audit evidence and
concrete follow-up idea drafts. No implementation files, tests, `plan.md`, or
source idea files were edited.

### Close-Ready Audit Payload

#### Residual Ownership Table

| Residual group | Classification | Close note |
| --- | --- | --- |
| Stack-layout id lookup helpers | `move-to-domain-owner` | `find_frame_slot_by_id` and `find_stack_object_by_id` are simple `PreparedStackLayout` scans. Move them to stack-layout ownership instead of keeping them in the prepared lookup facade. |
| Move-bundle index facts and maps | `move-to-domain-owner` | `PreparedMoveBundleLookups` indexes `PreparedValueLocationFunction::move_bundles`, with ABI move maps and after-call result-lane bindings. Move the lookup surface to value-location/move-bundle ownership while keeping aggregate field projection. |
| Return-chain index maps | `keep-core-aggregate` | `PreparedReturnChainLookups` has semantic return-chain construction, but no concrete narrow owner was identified. Keep it in `prepared_lookups.*` until a real return-chain owner exists. |
| Value-home index maps | `move-to-domain-owner` | `PreparedValueHomeLookups` is a reverse index over `PreparedValueLocationFunction::value_homes`. Move type, builder, and indexed accessors to value-location ownership. |
| Current-block join source status naming | `split-fact-from-routing` | Status belongs with reusable join-copy source facts, not with AArch64 instruction-routing convenience. |
| Current-block entry publication status naming | `move-to-domain-owner` | Status is query-result state over value homes and block-entry publication collection. Move with entry-publication/value-location ownership. |
| Same-block producer query facts | `move-to-domain-owner` | Same-block scalar producer and materialization query structs are semantic source-producer/materialization facts, not aggregate construction. Move to a same-block/source-producer owner. |
| Call-argument materialization facts | `move-to-domain-owner` | Call-argument materialization is call-specific policy over same-block source producers. Move public API toward call ownership with a dependency on the same-block materialization owner. |
| Fused-compare and condition producer facts | `move-to-domain-owner` | These are comparison/materialized-condition facts over same-block producers and integer constants. Move only after naming or creating a concrete comparison owner. |
| Same-block load-local stored-value facts | `move-to-domain-owner` | The query crosses same-block source producers, memory-access lookups, addressing, and stack layout. Move toward addressing/memory ownership with source-producer dependency. |
| Current-block join parallel-copy source facts | `split-fact-from-routing` | Reusable join-copy facts should be separated from target-facing source/destination routing flags and instruction-value sets. |
| Current-block entry publication query result | `move-to-domain-owner` | Query inputs/result resolve value ids/homes and block-entry publication collection. Move with value-location/publication ownership. |
| Current-block join instruction routing convenience | `split-fact-from-routing` | This helper turns reusable fact value-id sets into current-block per-instruction routing booleans. Localize or move separately from reusable facts. |
| Function-level lookup aggregate | `keep-core-aggregate` | Keep `PreparedFunctionLookups` and `make_prepared_function_lookups` as the central one-pass aggregate and field-projection boundary. |
| Key helper functions | `move-to-domain-owner` | Move move-bundle key builders with move-bundle lookup ownership. Keep return-chain key builders with the retained return-chain aggregate until a concrete return-chain owner exists. |
| Lookup preparation helpers | `move-to-domain-owner` | Move value-home, move-bundle, edge-publication, and source-producer builders to their domain owners where visible. Keep return-chain builder with the retained core return-chain surface. |
| Value-home and out-of-SSA helper predicates | `split-fact-from-routing` | Register-sharing and parallel-copy predicates support reusable facts and routing. Split fact predicates from routing use under value-location/control-flow ownership. |
| Indexed move-bundle query helpers | `move-to-domain-owner` | Move indexed bundle, before-call, after-call, and before-return ABI lookup helpers with `PreparedMoveBundleLookups`. |
| Return-chain query helpers | `keep-core-aggregate` | Keep terminal/next operand helpers with `PreparedReturnChainLookups` because no concrete non-facade owner was proven. |
| Publication source-producer lookup helper | `move-to-domain-owner` | Declaration remains in `prepared_lookups.hpp`, but definition already lives in `select_chain_lookups.cpp`. Move declaration to source-producer/select-chain ownership. |
| Current-block publication consumption query | `move-to-domain-owner` | Query validates source-producer lookup, same-block ordering, instruction identity, and result-name match. Move with same-block/source-producer materialization ownership. |
| Same-block scalar and integer-constant query helpers | `move-to-domain-owner` | Move scalar-producer and integer-constant evaluation helpers to same-block/source-producer materialization ownership. |
| Stack-source publication helpers absent from `prepared_lookups.hpp` | `no-new-idea` | Same-width and aggregate stack-source publication helpers already live in `publication_plans.hpp/.cpp`; no residual prepared-lookup move is needed. |

#### Definition-Location Map

| Residual group | Definition and construction evidence |
| --- | --- |
| Stack-layout id lookup helpers | Defined as scans in `prepared_lookups.cpp`; used by prepared memory/address helpers. Narrow owner: `src/backend/prealloc/stack_layout/stack_layout.hpp`. |
| Move-bundle index facts and maps | Built by `make_prepared_move_bundle_lookups` from `PreparedValueLocationFunction::move_bundles`; after-call result-lane bindings are published from call results plus value homes; wired by `make_prepared_function_lookups`. |
| Return-chain index maps | Built by `make_prepared_return_chain_lookups`, scanning return blocks for scalar binary chains reaching return terminators; wired by `make_prepared_function_lookups`. |
| Value-home index maps | Built by `make_prepared_value_home_lookups` from `PreparedValueLocationFunction::value_homes`; reused by memory access indexing, edge publications, move-bundle lane binding, entry publication, and join-copy facts. |
| Current-block join source status naming | Declared in `prepared_lookups.hpp`; status assigned during join parallel-copy source fact preparation and propagated into instruction routing. |
| Current-block entry publication status naming | Declared in `prepared_lookups.hpp`; status assigned by `find_prepared_current_block_entry_publication` overloads. |
| Same-block producer query facts | Structs declared in `prepared_lookups.hpp`; public wrappers call private source-producer and same-block order validation helpers. |
| Call-argument materialization facts | Opcode predicate and materialization query are defined in `prepared_lookups.cpp` and delegate through same-block scalar producer lookup. |
| Fused-compare and condition producer facts | Defined in `prepared_lookups.cpp`; fused compare accepts immediates, then uses same-block scalar producer and integer-constant evaluation. |
| Same-block load-local stored-value facts | Defined in `prepared_lookups.cpp`; resolves same-block load-local producers, addressing facts, prior stores, and stack-layout ranges. |
| Current-block join parallel-copy source facts | Defined in `prepared_lookups.cpp`; combines edge publication/source/home facts with routing flags and instruction-value sets. |
| Current-block entry publication query result | Defined in `prepared_lookups.cpp`; value-id overload uses indexed homes plus `collect_prepared_block_entry_publications`, while BIR-value overload resolves name/id before delegation. |
| Current-block join instruction routing convenience | Defined in `prepared_lookups.cpp`; calls source-fact preparation, then marks instruction result booleans for the current block walk. |
| Function-level lookup aggregate | `make_prepared_function_lookups` wires call plans, address materializations, memory accesses, move bundles, return chains, value homes, edge publications, and source producers in one pass. |
| Key helper functions | Move-bundle and return-chain key builders are defined in `prepared_lookups.cpp` as hash-key construction helpers. |
| Lookup preparation helpers | Move-bundle, return-chain, value-home, and edge-publication builders are in `prepared_lookups.cpp`; source-producer builder is declared in `prepared_lookups.hpp` but defined in `select_chain_lookups.cpp`. |
| Value-home and out-of-SSA helper predicates | Defined in `prepared_lookups.cpp`; used by join-copy source facts and routing. |
| Indexed move-bundle query helpers | Defined in `prepared_lookups.cpp`; access move-bundle maps with value-location fallback scans. |
| Return-chain query helpers | Defined in `prepared_lookups.cpp`; read maps built by `make_prepared_return_chain_lookups`. |
| Publication source-producer lookup helper | Declared in `prepared_lookups.hpp`; defined in `select_chain_lookups.cpp`. |
| Current-block publication consumption query | Defined in `prepared_lookups.cpp`; validates source-producer lookup, same-block position, instruction identity, and result name. |
| Same-block scalar and integer-constant query helpers | Defined in `prepared_lookups.cpp`; scalar overloads are public wrappers and integer-constant evaluation uses a private recursive evaluator. |
| Stack-source publication helpers absent from `prepared_lookups.hpp` | Helpers are declared in `publication_plans.hpp` and defined in `publication_plans.cpp`; `prepared_lookups.cpp` only consumes aggregate stack-source authority while building edge publications. |

#### Consumer Map

| Consumer area | Residual API pressure |
| --- | --- |
| AArch64 dispatch edge copies | Uses `find_indexed_prepared_edge_publication_source_producer` while already including `publication_plans.hpp`, so source-producer lookup declarations should move to a narrower owner. |
| AArch64 dispatch producers | Uses value-home indexes, same-block scalar producer queries, source-producer lookup helpers, join-copy source facts, and join instruction routing. This is the clearest split-fact-from-routing consumer. |
| AArch64 dispatch publication | Uses current-block entry publication query inputs/result/status. Pressure points to value-location/publication ownership. |
| AArch64 value materialization and FP materialization | Use same-block scalar producer and integer-constant materialization helpers; already include or are adjacent to select-chain/source-producer ownership. |
| AArch64 comparison | Uses fused-compare/materialized-condition facts and stack-layout id lookup helpers. Comparison facts need a comparison owner; stack id helpers need stack-layout ownership. |
| AArch64 ALU | Uses before-return ABI move lookup, return-chain lookups, indexed value homes, same-block scalar producer queries, and fallback aggregate construction. Supports both aggregate retention and declaration moves. |
| AArch64 calls | Uses indexed value homes, indexed move bundle and before-call move lookups, stack id helpers, call-argument materialization, publication consumption, source-producer lookup, and same-block load-local stored-value source. Pressure spans calls, value locations, stack layout, addressing, and same-block materialization. |
| AArch64 memory and retargeting | Uses before-return ABI move lookup, indexed value homes, source-producer lookup helper, aggregate fields, typed stack-source publication results, and stack id helpers. Stack-source publication ownership is already correct under `publication_plans.hpp`. |
| AArch64 module/traversal and cross-target RISC-V/x86 | Use `PreparedFunctionLookups` and `make_prepared_function_lookups`, then project aggregate fields. Strong evidence to keep a central one-pass aggregate. |
| Prealloc formal publications and decoded home storage | Use `PreparedValueHomeLookups` and indexed value-home/id helpers. Strong include-cleanup pressure toward `value_locations.hpp`. |
| Prealloc select-chain, publication plans, and prepared printer | Define or call source-producer lookup helper outside `prepared_lookups.cpp`. Strong owner pressure toward `select_chain_lookups.hpp` and publication/source-producer declarations. |
| Prealloc call plans and module aggregation | Some broad includes are aggregate/module convenience only. Do not treat these includes alone as owner-move evidence. |

#### Explicit No-New-Idea Notes

- Return-chain maps and query helpers should remain under `prepared_lookups.*`
  for now. The audit found return-chain semantics, but no concrete owner file
  comparable to `calls.hpp`, `value_locations.hpp`, `publication_plans.hpp`, or
  `select_chain_lookups.hpp`. A vague "move return-chain lookups" idea would be
  rejected.
- `PreparedFunctionLookups` and `make_prepared_function_lookups` should remain
  the central one-pass aggregate and field-projection boundary. AArch64,
  RISC-V, x86, and traversal all show aggregate-level pressure. Follow-ups
  should shrink declarations around the aggregate, not delete the aggregate or
  replace it with independent rescans.
- Same-width and aggregate stack-source publication helpers already have
  publication-plan ownership in `publication_plans.hpp/.cpp`. No residual
  prepared-lookup follow-up is needed for them.
- Broad includes through module aggregation are not sufficient evidence for an
  owner move. Only direct residual groups with clearer owner files should become
  follow-up ideas.

#### Long-Term `PreparedFunctionLookups` Recommendation

Keep `PreparedFunctionLookups` as a central prepared-function aggregate that
coordinates one construction pass and exposes field projections for targets and
prealloc helpers. Move domain lookup structs, key builders, accessors, semantic
query declarations, and status enums out of `prepared_lookups.hpp` only when a
clear owner exists. Do not replace reusable prepared facts with AArch64-local
BIR rescans, predecessor rescans, or name matching.

### Follow-Up Idea Drafts

#### Draft 1: Move value-home and move-bundle lookup ownership to value locations

Residual groups:

- `PreparedValueHomeLookups`, value-home builder, and indexed value-home/id helpers.
- `PreparedMoveBundleLookups`, move-bundle key helpers, move-bundle builder,
  indexed move-bundle helpers, before-call/before-return ABI move helpers, and
  after-call result-lane binding lookup helpers.
- Current-block entry publication status/query only if the implementation
  remains directly tied to `PreparedBlockEntryPublication` collection.

Proposed owner files:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/value_locations.cpp` if a new implementation file is
  needed; otherwise keep implementation movement within existing prealloc
  source ownership chosen by the plan.
- `src/backend/prealloc/prepared_lookups.hpp/.cpp` only for aggregate wiring
  and field projection.

Out-of-scope boundaries:

- Do not delete `PreparedFunctionLookups`.
- Do not redesign move-bundle semantics, call plans, return handling, or
  value-home construction.
- Do not move target-specific AArch64 emission or ABI register spelling into
  prealloc value-location code.
- Do not move return-chain lookups in this draft.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Escalate to full `ctest --test-dir build -j --output-on-failure` only if
  shared prepared semantics change rather than declaration/include ownership.

Reviewer reject signals:

- New owner is just a renamed prepared-lookup facade.
- Consumers are forced into local rescans instead of using reusable lookup
  facts.
- The aggregate builder is removed or multiple targets lose the one-pass
  preparation path.
- Return-chain helpers are swept into the move without a concrete owner.

#### Draft 2: Move stack-layout id lookup helpers to stack-layout ownership

Residual group:

- `find_frame_slot_by_id`
- `find_stack_object_by_id`

Proposed owner files:

- `src/backend/prealloc/stack_layout/stack_layout.hpp`
- Existing stack-layout implementation file if present, otherwise the narrowest
  stack-layout source file selected by the plan.

Out-of-scope boundaries:

- Do not rewrite stack layout construction.
- Do not change frame-address, addressing, memory retargeting, or comparison
  semantics.
- Do not move AArch64 frame-address wrapper policy into prealloc.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Reviewer reject signals:

- Helper movement causes broader prepared lookup dependency churn without
  reducing the facade.
- AArch64 wrapper behavior changes instead of only moving ownership.
- Stack id lookup becomes target-specific.

#### Draft 3: Move source-producer and same-block materialization APIs to a narrow owner

Residual groups:

- Same-block producer query facts.
- Publication source-producer lookup helper.
- Current-block publication consumption query.
- Same-block scalar and integer-constant query helpers.

Proposed owner files:

- `src/backend/prealloc/select_chain_lookups.hpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp` only for source-producer fact
  structs that already belong to publication planning.

Out-of-scope boundaries:

- Do not reopen completed select-chain ownership from idea 137 except for this
  concrete residual declaration dependency.
- Do not move AArch64 materialization, register allocation, scratch, hazard, or
  final emission policy into prealloc.
- Do not replace prepared source-producer facts with local target scans.
- Keep call-argument, comparison, and load-local specialized policies as
  separate follow-up boundaries unless the plan explicitly stages them after the
  shared same-block owner exists.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Full `ctest --test-dir build -j --output-on-failure` if source-producer
  semantics change.

Reviewer reject signals:

- The new header hides the old facade without clarifying source-producer or
  same-block ownership.
- AArch64 consumers are rewritten around local rediscovery.
- The implementation expands into call, comparison, or memory policy before the
  shared same-block API boundary is stable.

#### Draft 4: Split current-block join parallel-copy facts from instruction routing

Residual groups:

- Current-block join source status naming.
- Current-block join parallel-copy source facts.
- Current-block join instruction routing convenience.
- Value-home and out-of-SSA helper predicates used by join-copy facts/routing.

Proposed owner files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/control_flow.hpp` for parallel-copy bundle traversal
  helpers if declarations need to move there.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` or a local AArch64
  dispatch header for target-facing instruction routing convenience.

Out-of-scope boundaries:

- Do not move AArch64 register spelling, scratch policy, hazard policy, or final
  instruction emission into shared prealloc code.
- Do not delete reusable join-copy source facts.
- Do not reopen edge-publication facade splitting from idea 140 except for the
  explicit current-block join-copy residual surface.
- Do not fold unrelated return-chain or stack-source publication work into this
  split.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Escalate to full CTest if reusable publication/control-flow facts change.

Reviewer reject signals:

- Reusable facts and target routing remain mixed under a new name.
- AArch64 routing booleans become shared prealloc policy.
- The route deletes facts that RISC-V/x86 could reuse.
- The proof only shows one dispatch case while nearby join-copy cases are
  unexamined.

#### Draft 5: Move call-argument materialization policy to call ownership

Residual groups:

- `PreparedCallArgumentSourceProducerMaterialization`
- `prepared_call_argument_binary_producer_opcode_is_materializable`
- `find_prepared_call_argument_source_producer_materialization`

Proposed owner files:

- `src/backend/prealloc/calls.hpp`
- Existing call-plan implementation file selected by the plan, with dependency
  on the same-block/source-producer materialization owner from Draft 3.

Out-of-scope boundaries:

- Do not move general same-block scalar producer helpers into call ownership.
- Do not change call-plan construction or ABI lowering.
- Do not move AArch64 call emission, register spelling, or scratch policy into
  prealloc.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Reviewer reject signals:

- Call ownership duplicates same-block materialization logic instead of
  depending on the shared owner.
- The route turns call-argument lookup into an AArch64-only shortcut.
- ABI move-bundle work is mixed in without a clear staged boundary.

#### Draft 6: Create or name a comparison owner for fused-compare and condition facts

Residual groups:

- `PreparedFusedCompareOperandProducer`
- `PreparedFusedCompareOperandProducerFacts`
- `PreparedMaterializedConditionProducer`
- Fused-compare operand producer and materialized-condition producer lookup
  helpers.

Proposed owner files:

- A new narrow prealloc comparison owner, such as
  `src/backend/prealloc/comparison.hpp` and matching implementation file, or a
  specifically named control-flow comparison owner if the plan chooses to extend
  `src/backend/prealloc/control_flow.hpp`.
- Dependency on the same-block/source-producer materialization owner from
  Draft 3.

Out-of-scope boundaries:

- Do not move AArch64 compare instruction selection or condition-code emission
  into prealloc.
- Do not use `control_flow.hpp` as a dumping ground without naming the
  comparison boundary.
- Do not change branch-condition semantics.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Full CTest if shared branch/comparison semantics change.

Reviewer reject signals:

- Follow-up fails to name a concrete owner and only says "clean comparison
  lookups".
- The owner move depends on line count rather than semantic comparison facts.
- Target-local compare lowering leaks into shared prealloc.

#### Draft 7: Move same-block load-local stored-value source lookup toward addressing/memory ownership

Residual group:

- `PreparedSameBlockLoadLocalStoredValueSource`
- `find_prepared_same_block_load_local_stored_value_source`

Proposed owner files:

- `src/backend/prealloc/addressing.hpp`
- Existing addressing implementation file selected by the plan.
- Dependency on stack-layout ownership from Draft 2 and same-block
  source-producer ownership from Draft 3.

Out-of-scope boundaries:

- Do not rewrite prepared memory-access construction.
- Do not move AArch64 load/store emission, register view, scratch, or extension
  handling into prealloc.
- Do not special-case the current AArch64 call consumer.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Full CTest if shared memory-access semantics change.

Reviewer reject signals:

- The move duplicates source-producer or stack-layout scans instead of using
  prepared facts.
- The implementation is shaped around only the AArch64 call-site case.
- Addressing ownership is used to absorb unrelated same-block materialization
  APIs.

#### Draft 8: Clean residual include pressure after owner moves

Residual groups:

- Broad residual `prepared_lookups.hpp` includes in AArch64 and prealloc files
  that only need moved value-home, move-bundle, stack-layout, source-producer,
  publication, or same-block materialization declarations.

Proposed owner files:

- Consumer files identified by the Step 3 map after the owner-move drafts land:
  AArch64 `dispatch_producers.cpp`, `dispatch_publication.cpp`,
  `dispatch_value_materialization.cpp`, `comparison.cpp`, `alu.cpp`,
  `memory.cpp`, `memory_store_retargeting.cpp`, `frame_slot_address.cpp`,
  `fp_value_materialization.cpp`, plus prealloc `formal_publications.*`,
  `decoded_home_storage.*`, `select_chain_lookups.cpp`,
  `publication_plans.cpp`, and `prepared_printer/select_chains.cpp` as needed.

Out-of-scope boundaries:

- Do not perform include cleanup before declarations actually have narrower
  owner homes.
- Do not remove `prepared_lookups.hpp` where a consumer still names
  `PreparedFunctionLookups` or `make_prepared_function_lookups`.
- Do not change behavior to satisfy include cleanup.

Proof route:

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Reviewer reject signals:

- Cleanup hides the old facade through a different broad umbrella header.
- Consumers lose required aggregate declarations.
- The patch changes lowering behavior while claiming to be include-only.

## Suggested Next

Supervisor should hand this close-ready payload to plan-owner for lifecycle
closure of idea 141 and, if desired, durable creation of the concrete follow-up
ideas above. Suggested durable ordering is Draft 2 or Draft 1 first for low
semantic risk, Draft 3 before Drafts 5-7 because call/comparison/memory
specializations depend on same-block/source-producer ownership, and Draft 8
only after relevant owner moves have landed.

## Watchouts

- Return-chain work is intentionally not a follow-up draft. The audit did not
  prove a concrete owner file.
- Stack-source publication helper work is intentionally not a follow-up draft.
  Ownership is already in `publication_plans.hpp/.cpp`.
- Follow-up drafts are payload only; no files under `ideas/open/` were created
  or edited.
- Keep `PreparedFunctionLookups` central even when declarations around it move.

## Proof

Inspection-only analysis slice. Used current `todo.md`, `plan.md`, the source
idea, and prior committed `todo.md` states:

- `70b9e2162` for the Step 1 inventory.
- `77dc273bf` for the Step 2 definition and construction map.
- `40134c67b` for the Step 3 consumer map.
- `b411c3588` for the Step 4 classification.

No build or test command was run, and no `test_after.log` was required by the
delegated packet.
