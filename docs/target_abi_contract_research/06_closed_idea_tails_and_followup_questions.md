# 06. Closed Idea Tails And Follow-Up Questions

Question: Which recent closed ideas mention tails related to target ABI
contracts, value home freshness, publication, preservation, rematerialization,
or move-bundle authority, and what concrete follow-up questions do they leave?

## Short Answer

The closed tails point to the same three unresolved contract questions found in
the earlier research files:

1. prepared rows can be internally coherent while stale relative to the latest
   semantic producer;
2. publication and producer rematerialization are repaired in narrow slices, but
   there is no single shared freshness rule that tells every consumer when a
   producer, publication, preserved home, or move source wins;
3. move bundles now carry more authority for stack-destination fan-in, but the
   authority is still mostly destination-oriented and does not by itself prove
   source freshness or rematerialization precedence.

Current `ideas/open/` coverage contains only the active research idea,
`ideas/open/585_target_abi_contract_and_value_consumption_research.md`.
Several closed tails reference follow-up ideas that have since been closed or
are no longer present under `ideas/open/`, so they are evidence for this
research but not active implementation coverage.

## Closed Tails Reviewed

| Closed idea | Tail used as evidence | Classification | Currently covered by an open idea? |
| --- | --- | --- | --- |
| `ideas/closed/265_phase_f4_memory_accesses_unsupported_stale_fail_closed_proof_map.md` | The closure keeps `PreparedFunctionLookups::memory_accesses` as public compatibility because prepared-only, stale-publication, byte-offset drift, and cross-publication mismatch rows still lack same-consumer synthetic fail-closed proof. Stale rows remain blocked until old prepared facts are proven to reject wrong producer block, instruction index, source value, base kind, wrong edge, duplicate, and obsolete Route 5 owner across x86 and riscv consumers. | Value freshness; publication. | Yes, by the active research idea's freshness/publication question. No separate open implementation idea exists. |
| `ideas/closed/184_phase_e_route1_producer_constant_view_consumer_migration.md` | The selected publication-source consumer was migrated to prefer Route 1 same-block source-producer facts, but the closure explicitly preserves prepared fallback/oracle surfaces for incomplete, out-of-scope, no-producer, missing-producer, future-producer, recursive operand, and unrelated-register cases. It does not claim route-wide consumer migration or prepared API contraction. | Preservation/rematerialization; publication. | Yes, by the active research idea's value-consumption model. No separate open migration idea exists. |
| `ideas/closed/164_bir_call_use_source_annotation_schema.md` | Route 6 BIR call-use source records were accepted only for semantic source facts. ABI register/stack placement, outgoing stack sizing, variadic FPR count, preservation/clobber sets, byval lanes, scratch requirements, destination homes, helper protocols, aggregate layout, and ABI-bound publication-routing reads remain prepared/codegen-owned. | Target ABI policy; publication. | Yes, by the active research idea's target ABI contract boundary question. |
| `ideas/closed/171_route5_current_block_join_source_migration.md` | The selected Route 5 helper and consumer route proved edge-publication and join-source oracle equivalence, but no prepared current-block join-source helper surface was proven private enough to contract. The closure keeps additional prepared public seams for broader prepared aggregate/privacy work. | Publication. | Yes, by the active research idea's prepared-publication and ownership-split questions. |
| `ideas/closed/394_rv64_same_module_sret_callee_home_publication.md` | RV64 same-module callee `%ret.sret` home publication was repaired by publishing incoming `a0` into supported sret homes and fail-closing malformed homes. The important tail is the pre-repair shape: avoiding a bad load was not enough; the incoming ABI pointer had to be published into the callee home expected by prepared facts. | Target ABI policy; publication; value freshness. | Yes, by active research. No current open follow-up; the closure says no later observed boundary for this representative. |
| `ideas/closed/398_rv64_object_route_stack_frame_and_param_home_edges.md` | Fresh proof found explicit callee-saved save slots, frame facts, register-passed parameter homes, and admitted stack-passed formals. The remaining `va-arg-21.c` issue was producer-side missing `helper_operand_homes.va_start.destination_va_list_address` metadata, split at the time into idea 408. | Target ABI policy; publication. | Not currently covered by a separate open idea because only idea 585 is open. Covered as historical evidence by this research. |
| `ideas/closed/406_rv64_object_route_residual_local_memory_boundaries.md` | The close notes route later first owners away from local-memory repair: `20020225-2.c` to instruction-fragment lowering and `ieee/mul-subnormal-single-1.c` to stack-frame support. The idea also required newly discovered missing producer facts to be split instead of hidden in RV64 object emission. | Value freshness; preservation/rematerialization. | Yes, by active research's missing-producer and stale-home questions. No separate open producer idea exists now. |
| `ideas/closed/323_rv64_loop_carried_pointer_postincrement_publication.md` | The loop-carried pointer post-increment path was repaired, but `src/00143.c` still had a distinct Duff fallthrough residual: BIR stored `%t39` and `%t42` into pointer locals with no `bir.add ptr` producers, prepared homes had `source_producer=unknown`, and RV64 loaded uninitialized stack homes before publishing them. The closure routed that to idea 324 at the time. | Publication; preservation/rematerialization; value freshness. | Not currently covered by a separate open idea. Covered as evidence by active research. |
| `ideas/closed/462_rv64_preterminator_predecessor_edge_parallel_copy_materialization.md` | The closure rejects treating a predecessor-edge move as a plain copy because `%t46` is produced in the successor block after the predecessor branches. Prepared GPR homes for `%t46` and `%t50` do not prove `%t46` is available at the predecessor terminator. It routed producer rematerialization questions to idea 463 at the time. | Move-bundle authority; preservation/rematerialization; value freshness. | Not currently covered by a separate open idea. Covered as evidence by active research. |
| `ideas/closed/512_stack_passed_parameter_home_publication.md` | Explicit stack-passed parameter homes were published for ordinary C calls, and RV64 consumes only explicit `passed_on_stack` and frame-slot facts. Remaining non-goals include later stack-frame and generic stack-to-stack move failures, not parameter-home publication. | Target ABI policy; publication. | Yes, by active research's ABI contract sufficiency question. No current open implementation tail from this closure. |
| `ideas/closed/513_rv64_stack_to_stack_prepared_move_materialization.md` | Same-scalar stack-slot to stack-slot moves were accepted through explicit prepared source-home, destination-home, and storage facts, but malformed, missing-authority, unsupported-bank, and conversion-adjacent shapes remained fail-closed and were split to follow-up ideas. | Move-bundle authority; publication. | Yes, by active research. Follow-up implementation ideas from the closure are now closed. |
| `ideas/closed/514_rv64_register_source_stack_destination_move_bundles.md` | Same-width register-source stack-destination shapes became accepted, while conversion and multi-source representatives reject through precise owner diagnostics. Residual unsupported shapes remain fail-closed when source size, destination offset, bank, width, or broader multi-move scheduling authority is absent. | Move-bundle authority; publication. | Yes, by active research. No separate open tail remains. |
| `ideas/closed/516_rv64_multi_source_prepared_move_bundle_classification.md` | Ambiguous non-parallel multi-source stack-destination bundles now reject before RV64 object emission when they have `authority=none`, `move_count=2`, and `parallel_copy=no`. The closure requires producer/classifier ownership rather than RV64 choosing a source, dropping a move, or inferring ordering. | Move-bundle authority. | Yes, by active research. It is historical evidence for the later stack-destination authority contract. |
| `ideas/closed/579_rv64_prepared_stack_destination_move_bundle_authority.md` | The idea closed on a narrower fail-closed path: two register sources fanning into one stack slot had no producer ordering, mutual exclusion, or parallel-copy authority, so the route advanced only to `producer_authority_missing_for_register_fan_in_stack_destination`. | Move-bundle authority; value freshness. | Yes, by active research. Its direct follow-up, closed idea 584, repaired the narrow contract. |
| `ideas/closed/584_rv64_stack_destination_move_bundle_authority_contract.md` | The narrow authority taxonomy for stack-destination fan-in was implemented, including producer publication, RV64 consumption, and positive/negative coverage. The closure rejects accepting multi-source fan-in without visible authority kind and owner facts. | Move-bundle authority; publication. | Yes, by active research for architectural implications. No separate open implementation coverage is needed for the closed narrow repair. |
| `ideas/closed/583_rv64_pointer_arithmetic_result_publication.md` | RV64 learned to materialize supported pointer add/sub results and publish them into prepared destination register or stack homes. The retained representative then advanced to a runtime mismatch tracked separately at the time. This tail proves that materializing the address is insufficient unless the pointer result is published to the prepared home later consumers trust. | Publication; preservation/rematerialization; value freshness. | Yes, by active research. The named runtime follow-up is closed now. |
| `ideas/closed/584_rv64_20000819_runtime_mismatch_after_pointer_publication.md` | The downstream runtime abort was caused by a computed global-address call argument being treated as already available in `s1` and copied to `a0`. RV64 now materializes the computed global address through semantic relocation before the call. The closure says no separate follow-up is required. | Publication; target ABI policy. | Yes, by active research as evidence. No remaining open tail. |

## Related Closed Ideas With No Remaining Tail For This Question

These ideas were reviewed because they are nearby in the same recent RV64 or
prepared-authority lanes, but their closure notes do not leave a distinct live
target ABI/value-consumption tail beyond the categories already captured above.

| Closed idea | Closure disposition | Classification |
| --- | --- | --- |
| `ideas/closed/27_riscv_prepared_edge_publication_stack_destination_support.md` | Focused RISC-V `Register -> StackSlot` edge-publication consumption through shared `edge_publications` authority was implemented. Unsupported source-to-stack forms were preserved in idea 30 at the time, but no such open idea exists now. | Publication; historical only. |
| `ideas/closed/115_aarch64_codegen_layout_post_prealloc_contract_audit.md` | The audit created follow-up ideas 116 and 117 for AArch64 prepared producer, edge-publication, current-block join, select-chain producer, register clobber/read, fused-compare, and publication residue. Those are no longer open. The remaining value for this research is the classification boundary: shared authority should move forward, target emission stays local. | Target ABI policy; publication; historical only. |
| `ideas/closed/580_rv64_scalar_compare_publication.md` | Ordinary F32/F64 compare results are now published into prepared GPR homes. Later rows moved to `unsupported_call_abi` or `unsupported_move_bundle_target_shape`; those are not this compare-publication tail. | Publication; no live tail in this question. |
| `ideas/closed/581_rv64_ordinary_floating_cast_lowering.md` | Ordinary F32/F64 and selected integer-to-FP casts advanced past `unsupported_floating_cast` to a separate `unsupported_terminator_fragment` owner. | Unrelated to target ABI/value-consumption authority except as a non-overfit routing example. |
| `ideas/closed/582_rv64_va_start_stack_backed_destination.md` | Stack-backed `destination_va_list_address` homes are supported in RV64 `va_start` lowering. The representative now stops at an ordinary call-lowering instruction fragment outside the helper destination-address boundary. | Target ABI policy; no live tail in this question. |

## Coverage Assessment

The closures show that many narrow backend symptoms have been repaired without
creating a single shared contract for value-consumption authority. Current open
coverage is intentionally research-only:

| Tail family | Current state | Open coverage |
| --- | --- | --- |
| Stale prepared rows and public lookup compatibility | Still lacks a general same-consumer stale-row fail-closed matrix across target consumers. | Active research only. |
| Producer rematerialization versus preserved homes | Several routes preserve fallback/oracle behavior, but no closure makes rematerialization precedence a uniform prepared contract. | Active research only. |
| ABI-bound call/publication facts | BIR annotations intentionally exclude ABI/layout-bound placement and preservation details, leaving prepared/codegen as current owners. | Active research only. |
| Stack-passed and sret home publication | Specific ABI publication gaps were repaired and fail-closed malformed shapes remain guarded. | Historical evidence only unless future research creates an implementation idea. |
| Move-bundle authority | Stack-destination fan-in has a narrow authority taxonomy now, but broader source freshness and producer/rematerialization precedence remain outside the move-bundle contract. | Active research only. |

## Remaining Follow-Up Questions

1. Should prepared value homes, publication rows, preservation records, and
   move-bundle sources share one freshness authority record, or should each
   surface keep local proof fields with a common verifier?
2. What exact fact should make a producer rematerialization outrank
   `PriorPreservation`: producer block/instruction order, publication epoch,
   value id plus dominance, or an explicit supersedes relation?
3. Should old prepared public lookup surfaces such as `memory_accesses` remain
   compatibility APIs indefinitely, or should they be demoted only after a
   cross-target stale-row matrix proves same-consumer fail-closed behavior?
4. Where should ABI-bound publication-routing reads live after BIR Route 6
   semantic call-use facts: in prepared call plans, a target ABI policy layer,
   or target backends with stricter prepared authority requirements?
5. Does the stack-destination move-bundle authority taxonomy need to expand
   from destination fan-in legality to source freshness and producer ownership,
   or should those facts be linked from a separate value-consumption authority?
6. Should AArch64 and RV64 use the same completeness checks for
   prior-preserved stack and callee-saved sources, including required placement
   identity and freshness-source evidence?
7. What is the minimum stale-row proof set required before a target consumer may
   trust prepared fallback or oracle state as non-semantic compatibility rather
   than current authority?
8. When a closed idea routed a residual to a follow-up that is now closed or no
   longer open, should future implementation planning create one consolidated
   target ABI/value-consumption idea or several narrow ideas split by
   freshness, publication, preservation, and move-bundle authority?

## Answer

The relevant closed tails are already converging on an architectural boundary:
target backends should consume explicit prepared authority, while shared
prepared/prealloc contracts must say why a value source is fresh enough to
trust. Narrow repairs have handled stack-passed homes, same-module sret homes,
pointer-result publication, scalar compare publication, stack-backed
`va_start` destinations, and stack-destination fan-in authority. They have not
settled the cross-cutting question of how to rank old homes, preserved homes,
producer rematerialization, publication rows, and move-bundle sources under one
AArch64/RV64-compatible contract.
