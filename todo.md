Status: Active
Source Idea Path: ideas/open/115_aarch64_codegen_layout_post_prealloc_contract_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Large Owner Families

# Current Packet

## Just Finished

Step 2 - Classify Large Owner Families completed as an analysis-only
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

## Suggested Next

Delegate Step 3: classify the dispatch-family residue in `dispatch.cpp`,
`dispatch_edge_copies.cpp`, `dispatch_value_materialization.cpp`, and
`dispatch_producers.cpp`. Focus on whether current dispatch helpers still
rediscover prepared/publication facts locally or are now target-local
materialization/edge-copy consumers.

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
- For dispatch-family classification, prior closure notes reject AArch64-local
  rediscovery of prepared/publication facts through predecessor scans, BIR name
  matching, or same-block producer matching.
- Step 2 found the strongest possible move-forward candidate in
  `comparison.cpp`, where fused compare/current-block publication lookup still
  looks more like shared prepared fact ownership than pure target spelling.
- Do not draft new ideas for old calls helper splits, outgoing stack area,
  memory store-source helpers, branch-fusion file folding, aggregate source
  selection, byval transport, or aggregate `va_arg` preservation unless Step 3
  or Step 4 finds concrete current evidence beyond the closed-work map.

## Proof

No build/test proof required by the delegated packet because this slice was
analysis-only. Verification used read-only `c4c-clang-tool-ccdb`
`list-symbols` and `function-signatures` queries plus narrow text/reference
context; no `test_after.log` was produced for this packet.
