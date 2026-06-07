Status: Active
Source Idea Path: ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Focused Follow-Up Ideas

# Current Packet

## Just Finished

Step 4 - Draft Focused Follow-Up Ideas completed as an analysis-only lifecycle
packet.

Created follow-up ideas:

- `ideas/open/116_aarch64_dispatch_prepared_producer_contract_surface.md` for
  the concrete dispatch-family `move-forward`/`contract-needed` residue in
  `dispatch_edge_copies.cpp` and `dispatch_producers.cpp`. This is justified
  by Step 3 evidence around edge-publication producer contexts, current-block
  join/source routing, select-chain producer wrappers, and publication
  register clobber/read checks still being shaped in AArch64 dispatch after
  prior prepared-authority repairs.
- `ideas/open/117_aarch64_comparison_fused_compare_publication_contract.md`
  for the bounded `comparison.cpp` `move-forward`/`contract-needed` residue.
  This is justified by Step 2 evidence around
  `find_prepared_materialized_compare_join_targets`,
  `find_prepared_fused_compare_operand_producer_facts`,
  `collect_prepared_current_block_entry_publications`, and
  `prepared_current_block_entry_publication_register`.

No-new-idea decisions from Step 4:

- No new `calls.cpp` idea: old helper split cleanup is already covered by idea
  35, outgoing stack argument area authority is covered by idea 114, and Step 2
  did not name a concrete call-boundary dump/test consumer strong enough for a
  new contract-visibility idea.
- No new `memory.cpp` idea: old store-source helper cleanup is covered by
  ideas 34, 39, and 39a, and Step 2 found prepared store-local/global
  publication consumers rather than a concrete new shared-authority slice.
- No new `alu.cpp` idea: Step 2 found target-local scalar opcode, immediate,
  scratch, and publication emission responsibilities; scalar-publication
  visibility was not concrete enough to draft without a named cleanup
  consumer.
- No new `f128.cpp` or `i128_ops.cpp` idea: Step 2 found specialized
  representation/runtime-helper ABI machinery, not a concrete shared-authority
  migration or phoenix route.
- No new `machine_printer.cpp` or `instruction.cpp` idea: Step 2 found
  target-local spelling, printing, record-schema, and validation surfaces; no
  bounded cleanup or contract proof route was identified.
- No new `dispatch.cpp` idea: Step 3 found central prepared-block orchestration
  and hook wiring, with contract-consumer evidence only.
- No new `dispatch_value_materialization.cpp` idea: Step 3 found target
  materialization glue that already delegates same-block/select-chain lookup to
  `prepare` helpers; no rediscovery cleanup was justified.
- No phoenix-candidate idea: Step 2 and Step 3 found identifiable owner
  domains rather than a tangled replacement target.
- No fold-back idea: Step 3 found no current dispatch helper family that should
  mechanically merge back into an owner before shared contracts are clarified.

Step 3 - Classify Dispatch-Family Residue completed as an analysis-only
classification packet.

Retained Step 1 audit baseline:

| Area | Files | Total lines | Notes |
| --- | ---: | ---: | --- |
| `src/backend/mir/aarch64/codegen` | 74 | 51871 | Current C++/header AArch64 codegen owner surface after ideas 34-39, 113, and 114. |
| `ref/claudes-c-compiler/src/backend/arm/codegen` | 20 | 7856 | Reference Rust layout is smaller and centered on `emit.rs`; use it as an owner-boundary signal, not a line-count target. |

Largest AArch64 owners from Step 1: `calls` 8665 lines, `memory` 5485,
`alu` 4486, `instruction` 4033, `i128_ops` 2895, `comparison` 2731,
`f128` 2559, and `machine_printer` 2201. Other large-tail context was
`cast_ops` 1757, `globals` 1582, and `variadic` 1580.

Dispatch size notes for Step 3: `dispatch_edge_copies` 1391 lines,
`dispatch` 1082, `dispatch_value_materialization` 762,
`dispatch_producers` 642, `dispatch_publication` 435, and
`dispatch_lookup` 226. Prior dispatch work says Step 3 should reject local
rediscovery of prepared/publication facts through predecessor scans, BIR-name
matching, or same-block producer matching.

Reference comparison retained from Step 1: reference ARM's largest owner is
`emit.rs` at 1999 lines, then `peephole.rs` at 1223. Feature files are much
smaller than AArch64's large owners: `calls.rs` 263, `memory.rs` 252,
`alu.rs` 229, `comparison.rs` 75, `f128.rs` 334, and `i128_ops.rs` 322.

Prior-work and stale/covered map retained from Step 1:

- Idea 20 is stale as a current inventory but remains the non-duplication
  guard: separate shared authority migration from mechanical fold-back.
- Ideas 34, 39, and 39a covered the old memory store-source gap and folded the
  deleted `memory_store_sources.*` helper family back into `memory.cpp`.
- Idea 35 covered old calls helper file fold-back; idea 36 covered dispatch
  diagnostics/publication-common fold-back; idea 37 covered comparison branch
  fusion/prologue helper fold-back; idea 38 covered compatibility projection.
- Idea 113 recorded no new idea for aggregate source selection, byval
  transport, aggregate `va_arg` home preservation, and related aggregate
  call-boundary details except for idea 114.
- Idea 114 made outgoing stack argument area an explicit prepared call fact, so
  stale notes about AArch64 deriving call-level stack reservation from
  per-argument destinations are covered.

Large-owner classification table, based on AST-backed function ownership
queries with narrow text/reference context:

| Owner | Classification | Evidence | Follow-up disposition |
| --- | --- | --- | --- |
| `calls.cpp` | `keep-local`, `contract-needed` | The largest clusters consume prepared call facts instead of deriving the old reservation facts: `find_prepared_call_plan`/`require_prepared_call_plan`, `outgoing_stack_argument_bytes`, `make_outgoing_stack_base_instruction`, and `make_outgoing_stack_adjustment_instruction`. The rest is target call-boundary emission and ABI mechanics: `make_call_boundary_move_instruction`, `lower_before_call_move`, `lower_after_call_move`, `materialize_indirect_call_callee_to_prepared_register`, `record_call_result_source_register`, and `publish_stack_preserved_call_values`. | No new fold-back idea for old calls helper files; idea 35 already covered that. No new outgoing-stack-area idea; idea 114 made that a prepared fact. Candidate to draft later only if Step 4 wants contract visibility around call-boundary move/republication dumps, because the owner still has many prepared-consumer validation paths. |
| `memory.cpp` | `keep-local`, `contract-needed` | Current bulk is AArch64 memory instruction construction, addressing, effects, and prepared-publication consumption: `make_memory_record_from_prepared_access`, `make_prepared_memory_operand_record`, `lower_memory_instruction`, `lower_f128_transport_instruction`, `lower_i128_transport_instruction`, `prepared_store_local_access`, `prepared_store_source_producer`, `lower_store_local_value_publication`, and `lower_store_global_value_publication_from_plan`. Store-source logic now appears as prepared-plan consumers, not the deleted `memory_store_sources.*` split. | No new store-source fold-back idea; ideas 34, 39, and 39a cover that prior gap. Candidate to draft later as contract-needed only if reviewer wants dump/tests for prepared store-local/global publication coverage before cleanup. |
| `alu.cpp` | `keep-local`, `contract-needed` | The file is dominated by AArch64 scalar opcode selection, immediate encodability, scratch selection, and publication emission: `scalar_alu_operation_accepts_immediate`, `scalar_alu_opcode_semantic_facts`, `scalar_alu_stack_publication_lines`, `find_prepared_scalar_value_home`, `make_prepared_scalar_operand`, `make_scalar_alu_instruction_record`, `make_prepared_scalar_alu_record`, `lower_scalar_mul_with_distinct_rhs_scratch`, and `lower_scalar_instruction`. | No reference-layout parity idea; reference ARM keeps this smaller because it centralizes emission in `emit.rs`. Possible follow-up should be contract visibility for scalar-publication/prepared-home facts, not a mechanical split. |
| `comparison.cpp` | `keep-local`, `move-forward`, `contract-needed` | Target-local branch and compare emission is clear in `make_prepared_conditional_branch_record`, `lower_prepared_branch_terminator`, `lower_prepared_i128_compare_instruction`, `lower_fused_compare_branch_from_emitted_cast`, and `lower_conditional_branch_from_emitted_condition`. Shared-authority residue is more plausible here than in the other owners because the file still locates prepared/current-block publication facts with `find_prepared_materialized_compare_join_targets`, `find_prepared_fused_compare_operand_producer_facts`, `collect_prepared_current_block_entry_publications`, and `prepared_current_block_entry_publication_register`. | No new branch-fusion fold-back idea; idea 37 already folded the old helper. Candidate to draft later: a bounded move-forward/contract idea for fused-compare prepared publication facts if Step 4 decides this should be shared instead of comparison-local lookup. |
| `f128.cpp` | `keep-local`, `contract-needed` | The owner is specialized f128 transport/runtime-helper ABI machinery: `prepared_f128_full_width_carrier_facts`, `prepared_f128_memory_backed_carrier_facts`, `append_f128_helper_move_line`, `validate_f128_transport_instruction`, `validate_f128_runtime_helper_boundary_instruction`, `make_prepared_f128_carrier_transport_record`, `make_f128_helper_operand_record`, `make_prepared_f128_runtime_helper_boundary_record`, and `lower_f128_runtime_helper_instruction`. | No new aggregate/HFA call-boundary idea; idea 113 already found no current follow-up there. Candidate only for dump/test visibility around f128 runtime-helper boundary contracts before any cleanup. |
| `i128_ops.cpp` | `keep-local`, `contract-needed` | The file owns i128 pair/lane transport, shift/compare semantics, and runtime helper boundary records: `validate_i128_div_rem_helper_marshaling_plan`, `append_i128_helper_move_line`, `make_prepared_i128_shift_record`, `make_prepared_i128_compare_record`, `make_i128_helper_carrier_lane_adapter`, `make_prepared_i128_runtime_helper_boundary_record`, `i128_*_selection_status`, and `lower_i128_pair_operation_instruction`. | No phoenix or fold-back idea justified from this pass; the bulk is specialized representation and ABI. Candidate only for contract visibility around i128 runtime helper marshaling and lane binding if Step 4 wants proof before cleanup. |
| `machine_printer.cpp` | `keep-local` | This is target spelling/printing and final assembly line materialization: `register_name_with_view`, `print_memory`, `print_atomic_memory`, `print_frame`, `print_scalar`, `print_call_boundary_move`, `print_call`, `print_call_boundary_abi_binding`, and `print_machine_instruction_line_payloads`. The reference ARM comparison also points to centralized emit/printing rather than shared semantic authority. | No new idea from Step 2. Printing remains AArch64-local unless a later contract idea requires extra dump visibility. |
| `instruction.cpp` | `keep-local`, `contract-needed` | The file is the target machine-record schema and validation/spelling layer: `machine_opcode_spelling`, `operand_kind_spelling`, `record_surface_kind_spelling`, `machine_node_selection_status_spelling`, `is_target_mir_record_surface`, `machine_opcode_from_scalar_instruction`, `branch_selection_status`, `scalar_selection_status`, `make_*_operand`, `make_*_instruction`, and `make_unsupported_machine_instruction`. | No mechanical cleanup idea from this pass. Candidate only if Step 4 decides machine-record selection-status/dump visibility is a prerequisite contract for later owner cleanup. |

Follow-up idea candidates to consider in Step 4:

- `comparison.cpp`: prepared fused-compare/current-block publication facts may
  be a bounded `move-forward` plus `contract-needed` idea, with reject signals
  against BIR-name matching or same-block producer rediscovery.
- `calls.cpp`: call-boundary move/republication dump or test visibility may be
  a bounded `contract-needed` idea, but not an outgoing-stack-area repair.
- `memory.cpp`, `alu.cpp`, `f128.cpp`, `i128_ops.cpp`, and `instruction.cpp`:
  only draft contract-visibility ideas if Step 4 can name concrete dumps/tests
  and cleanup consumers; otherwise record no new idea.

No phoenix-candidate classification was justified for any Step 2 owner in this
pass. The files are large, but the observed clusters have identifiable target
ABI, emission, record-schema, or prepared-contract consumer responsibilities
rather than one tangled rewrite target.

Dispatch-family residue classification table, based on AST-backed function
ownership queries with narrow function-body reads:

| Dispatch owner | Classification | Evidence | Step 4 disposition |
| --- | --- | --- | --- |
| `dispatch.cpp` | `keep-local`, `contract-needed` | The file is the central prepared-block orchestration loop and target-family router: `dispatch_prepared_block`, `make_block_lowering_context`, `classify_instruction`, `make_dispatch_branch_fusion_hooks`, `lower_store_local_with_address_materialization`, and `lower_scalar_with_address_materialization`. It also tracks local emission state with `before_return_publication_already_emitted`, `record_before_return_publication`, and `instruction_result_has_stack_home`. The notable residue is hook wiring from dispatch into prepared-publication consumers, not a standalone shared-fact owner. | No fold-back or phoenix idea. Step 4 may cite dispatch as a consumer when drafting contract visibility around prepared publication/materialization routing, but should not draft a `dispatch.cpp` cleanup idea by itself. |
| `dispatch_edge_copies.cpp` | `move-forward`, `contract-needed` | This file still owns edge-copy producer-context reconstruction and publication/source matching: `prepared_edge_publication_producer_block_context`, `prepared_edge_publication_producer_context`, `prepared_edge_source_producer_context`, `prepared_edge_named_source_producer_context`, `prepared_publication_source_memory_access`, `prepared_publication_source_register`, `edge_value_publication_may_read_register_index`, and `lower_predecessor_select_parallel_copy_sources`. The callee query for `lower_predecessor_select_parallel_copy_sources` shows it now consumes `prepare::prepare_block_entry_parallel_copy_edge_source_facts`, `prepare::prepared_edge_publication_matches_parallel_copy_move_source`, and `prepare::find_prepared_move_bundle`, but the surrounding file still maps those prepared facts back to BIR producer contexts and AArch64 edge-copy emission. | Strong Step 4 follow-up candidate: draft a bounded prepared edge-publication/parallel-copy contract idea, with explicit reject signals against predecessor rescans and BIR-name matching. This is a `move-forward`/`contract-needed` candidate, not a mechanical fold-back. |
| `dispatch_value_materialization.cpp` | `keep-local`, `contract-needed` | The public owner is `emit_value_publication_to_register`; it emits AArch64 register materialization from immediate, prepared home, current-block entry publication, same-block scalar producer, prepared memory/global access, select-chain materialization, and FP/scalar helper paths. The local helper evidence is `prepared_same_block_scalar_producer`, `prepared_scalar_select_chain_materialization`, and `prepared_same_block_integer_constant`, which call `prepare::find_prepared_same_block_scalar_producer`, `prepare::find_prepared_scalar_select_chain_materialization`, and `prepare::evaluate_prepared_same_block_integer_constant` instead of hand-scanning producers. | No move-forward idea unless Step 4 wants dump/test visibility for value-publication materialization contracts. Keep local as target materialization glue; do not draft a same-block rediscovery cleanup because the current path delegates lookup to `prepare`. |
| `dispatch_producers.cpp` | `move-forward`, `contract-needed` | This is the strongest dispatch-family shared-fact residue. It builds and queries current-block join/source routing with `prepare_current_block_join_parallel_copy_source_facts`, `prepared_query_current_block_join_parallel_copy_source`, `build_current_block_join_prepared_query_routing`, `current_block_join_prepared_query_incoming_expression`, and `current_block_join_prepared_query_source`; it also owns same-block and select-chain producer wrappers: `find_prepared_same_block_select_producer`, `prepared_publication_source_producer_for_value`, `prepared_source_producer_instruction`, `prepared_select_chain_contains_direct_global_load`, `select_chain_contains_direct_global_load`, `block_entry_move_clobbers_current_join_publication`, and `value_publication_may_read_register_index`. The residue is not raw local scans everywhere, but AArch64 still owns prepared-query routing shape and fallback decisions. | Strong Step 4 follow-up candidate, possibly paired with `dispatch_edge_copies.cpp`: draft a prepared producer/join-routing contract idea that moves or exposes current-block join/publication facts before more dispatch cleanup. No phoenix candidate; the file has a coherent domain but the domain likely belongs closer to prepared lookup authority. |

Dispatch Step 3 conclusion:

- No `fold-back` classification is justified for the four dispatch owners in
  this pass. The old helper-split cleanup work is not the current residue.
- No `phoenix-candidate` classification is justified. The files have coherent
  dispatch/materialization/edge-copy roles rather than one tangled replacement
  target.
- Step 4 should focus on one concrete follow-up idea around prepared
  edge-publication/current-block producer and join-routing contracts, anchored
  in `dispatch_edge_copies.cpp` plus `dispatch_producers.cpp`.
- `dispatch.cpp` and `dispatch_value_materialization.cpp` should be recorded as
  contract consumers/supporting evidence, not primary cleanup targets, unless
  Step 4 can name a concrete dump/test consumer.

## Suggested Next

Step 5 - Prepare Closure Summary. Summarize the final owner classification
table, dispatch-family residue map, no-new-idea decisions, and new follow-up
ideas 116 and 117 with their evidence. Confirm this audit did not change
implementation, test, or build metadata files, and preserve the full-suite
baseline note from the source idea.

## Watchouts

- This active idea is analysis-only.
- Do not edit `src/backend/mir/aarch64/codegen`, tests, or build metadata.
- Do not treat line-count reduction as the success metric.
- Do not duplicate closed idea 20 follow-ups without a concrete current gap.
- Current verification found no live references to the old standalone helper
  files from ideas 35-39 except the expected module-private
  `derive_compatibility_projection` helper inside `module_compile.cpp`.
- For `calls.cpp`, account for idea 114 before classifying outgoing stack
  reservation/addressing as shared-authority residue.
- For `memory.cpp`, do not treat the old `memory_store_sources.*` critique as
  current evidence; those files have been deleted and folded back.
- For Step 4 dispatch follow-up drafting, prior closure notes reject
  AArch64-local rediscovery of prepared/publication facts through predecessor
  scans, BIR name matching, or same-block producer matching.
- Step 4 created idea 116 for dispatch prepared producer contract surface and
  idea 117 for comparison fused-compare publication contract.
- Step 4 deliberately did not create new ideas for calls, memory, alu, f128,
  i128, machine printer, instruction, dispatch.cpp alone,
  dispatch_value_materialization.cpp alone, fold-back cleanup, or phoenix
  rebuild because the audit evidence did not justify concrete non-duplicative
  follow-up slices.
- Step 3 found no new idea for `dispatch.cpp` alone and no new idea for
  `dispatch_value_materialization.cpp` alone; both are mostly target-local
  orchestration/materialization consumers of prepared facts.
- Step 3 found the strongest dispatch-family follow-up in
  `dispatch_edge_copies.cpp` plus `dispatch_producers.cpp`, where edge
  publication producer contexts, current-block join routing, select-chain
  producer queries, and register-clobber/read checks still look like prepared
  fact ownership leaking into AArch64 dispatch.
- Step 2 found the strongest possible move-forward candidate in
  `comparison.cpp`, where fused compare/current-block publication lookup still
  looks more like shared prepared fact ownership than pure target spelling.
- Do not draft new ideas for old calls helper splits, outgoing stack area,
  memory store-source helpers, branch-fusion file folding, aggregate source
  selection, byval transport, or aggregate `va_arg` preservation unless Step 3
  or Step 4 finds concrete current evidence beyond the closed-work map.

## Proof

No build/test proof required by the delegated packet because this slice was
analysis-only. Step 4 created only new idea files under `ideas/open/` and
updated `todo.md`; no implementation, test, or build metadata files were
changed, and no `test_after.log` was produced for this packet.

Step 3 verification used read-only `c4c-clang-tool-ccdb`
`function-signatures` and `function-callees` queries for `dispatch.cpp`,
`dispatch_edge_copies.cpp`, `dispatch_value_materialization.cpp`, and
`dispatch_producers.cpp`, plus narrow text reads around the relevant helpers.
