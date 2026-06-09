Status: Active
Source Idea Path: ideas/open/130_aarch64_dispatch_family_post_contract_layout_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Follow-Up Ideas Or No-New-Idea Notes

# Current Packet

## Just Finished

Completed `plan.md` Step 4: Draft Follow-Up Ideas Or No-New-Idea Notes.

Step 4 synthesized the Step 3 fold-back and shared-contract-gap candidates into
bounded future-work draft notes. This packet did not create `ideas/open/` files;
the drafts below are ready for the Step 5 closure material or a plan-owner
lifecycle decision.

### Bounded Follow-Up Draft Notes

#### Draft A: Privatize Dispatch Edge-Copy Helper Surface

- Finding type: `fold-back`.
- Candidate files: `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`,
  `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch.cpp`, and CMake metadata only if the
  follow-up removes a translation unit.
- Missing or excess boundary: `EdgeProducerContext`,
  `edge_value_publication_may_read_register_index`,
  `emit_edge_load_local_to_register`, and
  `emit_edge_value_publication_to_register` are public header declarations even
  though Step 2 found them used only inside `dispatch_edge_copies.cpp` or by
  nearby dispatch-family flow. `should_emit_block_entry_edge_copy_move` and
  `lower_predecessor_select_parallel_copy_sources` are dispatch-family hooks
  called from `dispatch.cpp`, not broad public codegen hooks.
- Expected behavior preservation: no edge-copy, predecessor select parallel
  copy, or block-entry publication behavior changes; any move should only
  narrow header exposure or fold implementation into a private dispatch-family
  owner.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- Reject signals: moving code only to reduce file count, changing emitted edge
  copy instruction ordering, weakening edge/publication expectations, or hiding
  new cross-file dependencies behind forward declarations.

#### Draft B: Trim Or Rehome Thin Dispatch Lookup Helpers

- Finding type: `fold-back` plus possible `shared-contract-gap`.
- Candidate files: `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`,
  `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`, natural callers such
  as `calls.cpp`, `globals.cpp`, `memory.cpp`, `memory_store_retargeting.cpp`,
  `comparison.cpp`, and `alu.cpp`, and CMake metadata only if the follow-up
  removes the translation unit.
- Missing or excess boundary: `make_named_prepared_result_register` and
  `emitted_scalar_value_available` have real external callers and should remain
  query hooks unless a broader query owner replaces them. The no-direct-caller
  declarations `is_scalar_call_argument_producer_opcode`,
  `find_same_block_scalar_producer`, and `has_same_block_load_local_producer`
  are stale public surface or same-block producer rediscovery candidates.
- Expected behavior preservation: keep prepared-result register selection and
  call-lowering availability semantics unchanged; any removal must prove that
  unused declarations are not an ABI-like internal contract.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- Reject signals: deleting the externally used lookup hooks, replacing prepared
  lookup with BIR-name rescans, or treating the small file size as sufficient
  justification without reducing public surface.

#### Draft C: Extract Shared Prepared Fact Query Surface For Publication And Producer Lookups

- Finding type: `shared-contract-gap`.
- Candidate files: AArch64 query users in
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
  `dispatch_lookup.cpp`, `dispatch_edge_copies.cpp`,
  `dispatch_publication.cpp`, `dispatch_value_materialization.cpp`, plus the
  existing shared prepared/prealloc query owners under
  `src/backend/mir/prealloc/` and `src/backend/mir/prepare*` selected by the
  implementation owner.
- Missing shared fact: AArch64 still locally stitches together prepared
  value-home, edge-publication source-producer, current-block entry
  publication, and same-block materialization relationships in several
  dispatch-family files. A follow-up should provide one shared prepared-query
  contract so target code asks for facts instead of rediscovering relationships.
- Expected behavior preservation: AArch64 keeps register names, scratch choices,
  hazard policy, relocation operands, and final instruction spelling local; only
  target-neutral fact lookup shape moves shared.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure` because shared
  prepared/prealloc query code can affect more than AArch64 dispatch.
- Reject signals: moving target-local emission into shared code, reintroducing
  predecessor rescans or BIR-name matching, duplicating the current AArch64
  wrapper under a generic name, or weakening publication/current-block/select
  expectations.

#### Draft D: Share Select-Chain And Same-Block Materialization Dependency Queries

- Finding type: `shared-contract-gap`.
- Candidate files: `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`,
  `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`, and the
  shared MIR/prealloc query owner selected by implementation.
- Missing shared fact: Step 3 found select-chain direct-global dependency and
  same-block scalar/constant materialization checks still queried locally around
  producer and value-publication materialization. A follow-up should expose
  target-neutral dependency facts instead of keeping target-local recursive
  rediscovery helpers such as `select_chain_contains_direct_global_load` as
  public AArch64 dispatch surface.
- Expected behavior preservation: keep AArch64 materialization emission and
  register-read hazard handling local; only the fact query should become shared.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Reject signals: named-case select-chain shortcuts, direct-global special cases
  that do not generalize to the prepared fact model, or any rewrite that proves
  only one current failing case.

#### Draft E: Clarify Current-Block Entry Publication Fact Query

- Finding type: `shared-contract-gap`.
- Candidate files: `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`,
  `dispatch_publication.cpp`, `dispatch_producers.cpp`,
  `dispatch_value_materialization.cpp`, and shared prepared/prealloc query
  files selected by implementation.
- Missing shared fact: `collect_current_block_entry_publications`,
  `value_has_current_block_entry_publication`, and
  `current_block_entry_publication_register` are AArch64 wrappers over prepared
  current-block/value-home facts. A follow-up should decide whether the
  relationship belongs in a shared prepared query while retaining AArch64
  register-view conversion locally.
- Expected behavior preservation: no change to publication register recording,
  branch-fusion hooks, or AArch64 operand/register spelling.
- Proof route: `cmake --build --preset default`, then
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Reject signals: broad publication rewrites, moving register-view selection out
  of AArch64, or proposing a generic query that does not replace an actual
  target-local rediscovery site.

### Explicit No-New-Idea Rationales For Retained Public Hooks

- `dispatch.hpp`: keep `make_block_lowering_context` and
  `dispatch_prepared_block` as public orchestration hooks because
  `traversal.cpp` calls them as the top-level prepared-block lowering boundary.
  `InstructionDispatchResult` remains part of that dispatch API even without
  independent external use. No new idea is warranted unless traversal ownership
  changes.
- `dispatch_lookup.hpp`: keep `make_named_prepared_result_register` and
  `emitted_scalar_value_available` as public query hooks because globals,
  memory retargeting, comparison, ALU, memory, and calls consume them. Draft B
  covers only stale/no-direct-caller helpers and possible rehoming under a
  better query owner.
- `dispatch_producers.hpp`: keep
  `find_prepared_same_block_select_producer`,
  `prepared_publication_source_producer_for_value`,
  `prepared_source_producer_instruction`, `producer_instruction_index`, and
  `value_publication_may_read_register_index` as public hooks because select
  materialization, fp materialization, comparison hook tables, ALU, and
  value-materialization hazards consume them. Drafts C and D cover shared fact
  shape; they do not justify removing these hooks now.
- `dispatch_producers.hpp` dispatch-only routing helpers:
  `prepared_query_current_block_join_parallel_copy_source`,
  `build_current_block_join_prepared_query_routing`,
  `current_block_join_prepared_query_incoming_expression`,
  `current_block_join_prepared_query_source`,
  `block_entry_move_clobbers_current_join_publication`, and
  `prepared_value_home_reads_register_index` are retained only as local
  organization until a shared prepared-query follow-up replaces the underlying
  fact relationship. No standalone cleanup idea is warranted for the names
  alone.
- `dispatch_publication.hpp`: keep scalar/register/publication utilities
  including `integer_bit_width`, `power_of_two_shift`, `relocation_operand`,
  `scalar_view_for_type`, `gp_register_name`, `scalar_integer_width_bits`,
  `scalar_gp_register_view`, `scalar_fp_register_view`,
  `immediate_integer_bits`, `is_byval_formal_value_name`,
  `prepared_value_home_for_value`, `value_has_current_block_entry_publication`,
  `current_block_entry_publication_register`,
  `record_current_block_entry_publication_registers`, and
  `lower_missing_conditional_branch_condition_publication` because Step 2 found
  broad external callers across prepared value-home materialization, globals,
  select materialization, variadic, ALU, casts, memory, calls, comparison hooks,
  and dispatch-family code. Draft E covers only the target-neutral
  current-block fact relationship.
- `dispatch_value_materialization.hpp`: keep
  `emit_value_publication_to_register` as a public materialization hook because
  fp materialization, select materialization, comparison hooks, ALU, casts,
  memory, calls, edge copies, and publication code consume it. No fold-back idea
  is warranted for this translation unit; future work should target shared
  fact queries, not the AArch64 emission hook.
- `dispatch_publication.cpp` and `dispatch_value_materialization.cpp`: retain
  separate translation units on current evidence because they combine broad
  public hook surfaces with substantial target-local register/operand emission
  logic. Physical file count can shrink only through Draft A or Draft B if
  public surface is actually narrowed; no file-count-only cleanup should be
  created.

## Suggested Next

Execute Step 5 from `plan.md`: assemble the final audit closure material in
`todo.md`, carrying forward the ownership table, call-site map, Step 4 no-new-
idea rationales, bounded follow-up draft list, and file-count-shrink conclusion
for supervisor or plan-owner review.

## Watchouts

- The drafts above are not new source ideas yet; this packet intentionally did
  not create `ideas/open/*.md`.
- Draft A and Draft B are the only physical file-count shrink candidates. Drafts
  C, D, and E are contract/query-shape candidates and should reject target-local
  emission movement.
- Retained hooks with broad external callers should not be folded back just
  because the reference ARM backend has fewer dispatch-specific files.
- Any implementation follow-up touching shared prepared/prealloc query code
  needs broader `^backend_` proof, not only a narrow AArch64 dispatch smoke.

## Proof

Analysis-only packet. Re-read `todo.md` Step 3 findings, the Step 2 call-site
map from git history, the active source idea, and the public dispatch-family
headers. Ran focused `rg`/`sed` checks over dispatch-family and nearby AArch64
codegen files to confirm public hook consumers, dispatch-only edge-copy surface,
and prepared/current-block lookup sites.

No build/test run required because no implementation code was edited.
Captured the concise proof summary in `test_after.log`.
