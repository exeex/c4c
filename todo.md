Status: Active
Source Idea Path: ideas/open/192_residual_route_view_consumer_migration_map.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Prior Route Evidence

# Current Packet

## Just Finished

Step 1 inventory complete for `plan.md` Step 1:

| Route | Source idea | Selected reader already migrated | Route-view or facade used | Retained prepared fallback/oracle surfaces | Target-policy boundary and proof scope |
| --- | --- | --- | --- | --- | --- |
| Route 1 producer/constant | idea 184 | Publication-source consumer `value_publication_may_read_register_index(...)`. | Route 1 publication producer view for complete same-block source-producer facts. | Prepared same-block producer, integer-constant, scalar operand, value-publication, target register, home, storage, move, and machine-record policy surfaces remain public/fallback/oracle guardrails. | Does not move value homes, storage encodings, register choice, rematerialization spelling, emitted machine records, or move generation into BIR. Proof covered selected route-first behavior, prepared fallback/oracle visibility, no/missing/future producer cases, recursive operand register dependencies without prepared source-producer indexes, and unrelated-register rejection. |
| Route 2 select-chain/direct-global | idea 185 | AArch64 scalar ALU control-publication `select.result` path in `lower_scalar_select_publication(...)`. | Local Route 2 adapter for select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | Prepared select-chain/direct-global helpers remain public fallback and oracle surfaces for publication, call, memory, ALU, FP, producer, edge-copy, printer/test, and diagnostic consumers. | Does not move target materialization sequence choice, storage/home selection, memory operand formation, call-publication policy, or final record spelling into BIR. Proof covered prepared lookup helper and AArch64 scalar ALU/record subsets. |
| Route 3 memory/source | idea 186 | AArch64 indirect-callee stored-value source consumer. | Narrow Route 3 stored-source / memory semantic source view. | Prepared memory/access, global-load, same-block global-load, and load-local source helpers remain public oracles/fallbacks. | Does not move address base kind, offsets, stack/frame objects, TLS/global relocation data, pointer materialization, base-plus-offset legality, volatile/address-space behavior, or final memory operand records into BIR. Proof covered the selected semantic source route and broader backend validation, but idea 190 proves future Route 3 work must preserve prepared fallback and AArch64 target-addressing policy explicitly. |
| Route 4 publication | idea 182 | AArch64 dispatch-publication reader `current_block_entry_publication_register(...)`. | Narrow Route 4 publication-identity boundary over current/block-entry publication facts. | Prepared current-block and block-entry publication helpers, edge-publication lookup surfaces, prepared printer/debug consumers, x86 wrappers, scalar publication planning, and oracle tests remain public. | Does not move value homes, storage availability, move planning, publication record construction, or block-order emission into BIR. Proof covered AArch64 instruction dispatch, prealloc block-entry publication, and prepared lookup helper tests; rejected a regressive full-suite candidate instead of accepting weaker baseline evidence. |
| Route 5 edge/join-source | idea 183 | AArch64 current-block join-source reader behind `build_current_block_join_prepared_query_routing(...)`. | Indexed Route 5 current-block join-source view. | Prepared current-block join-source facts, edge-publication helpers, edge-publication source-producer helpers, and move-bundle helpers remain fallback/oracle surfaces. | Does not move parallel-copy scheduling, out-of-SSA placement, source/destination homes, move-bundle selection, branch spelling, or final edge-copy records into BIR. Proof covered normal predecessor routing, missing predecessor, no-source fallback, load-local stack/memory-source routing, absent-route prepared fallback, and route-index fail-closed behavior. |
| Route 6 call-use source | idea 187 | Direct-global select-chain call-argument materialization consumer. | Route 6 semantic dependency/call-use source facts read first. | Prepared call-plan, argument/result plan, publication-routing, call-argument source-producer, value-home, move-bundle, and call-boundary effect helpers remain fallback/oracle surfaces. | Does not move ABI register/stack placement, variadic FPR counts, clobber/preserve sets, byval lanes, outgoing stack sizing, late-publication mechanics, helper resources, carrier protocols, or call record spelling into BIR. Proof covered dispatch, prepared lookup helper, selected indirect call, and call-boundary owner subsets plus broader backend validation. |
| Route 7 comparison/condition | idea 188 | `aarch64::codegen::lower_materialized_compare_condition_branch(...)`. | Validated Route 7 materialized-condition provenance. | Prepared comparison helpers, scalar producer/select-chain fallbacks, Route 7 facade validation, fused-compare oracle tests, and emitted-condition fallback remain visible. | Does not move branch spelling, fused legality, condition-code selection, hazard handling, emitted-register state, final branch/compare record ordering, ALU result storage, or return-chain operand recovery into BIR. Proof accepted one comparison/branch provenance consumer only; route-wide helper contraction remains out of scope. |

Idea 189 is cross-target evidence only: x86 `ConsumedPlans` reused the
AArch64-proven Route 6 call-use source view at one call-boundary selector when
Route 6 and prepared call-plan source ids agree. Prepared fallback remains for
absent or mismatched facts, riscv has no imported reuse proof, and x86/riscv
ABI, frame, register, wrapper, formatting, instruction-selection, and emission
policy remain target-local.

Missing or ambiguous material recorded for future steps: source docs do not
claim residual reader exhaustion for any route; draft 155 still requires a
field-by-field `PreparedBirModule` and `PreparedFunctionLookups` retirement
map; printer/debug/route-debug/oracle-test consumers must remain represented;
return-chain is separate owner/schema evidence, not Route 1 or Route 7 proof;
and Route 3 requires an explicit prepared-policy boundary after idea 190.

## Suggested Next

Execute Step 2 from `plan.md`: locate residual prepared consumers for Routes
1-7 and classify them as production lowering, printer/debug, target-wrapper,
oracle/test, retained target/prepared policy, or unknown.

## Watchouts

- This is analysis-only; do not edit implementation files while inventorying.
- Treat ideas 182-188 as selected-consumer evidence, not route-wide completion.
- Treat idea 189 as one x86 Route 6 interface-reuse proof only, not evidence
  that every target wrapper can reuse every route view.
- Preserve prepared fallback/oracle surfaces while classifying residual
  consumers; printer/debug, oracle, target-wrapper, and retained policy
  consumers are first-class blockers.
- Keep target policy, ABI policy, address-materialization policy, final
  emission policy, and return-chain outside Routes 1-7 unless a separate owner
  says otherwise.
- Do not weaken prepared oracle, diagnostic, route-debug, wrapper, baseline, or
  c_testsuite expectations.
- Keep source idea edits unnecessary unless durable intent changes.

## Proof

Docs/lifecycle-only packet; no build required. Verification was source reading
of `ideas/open/192_residual_route_view_consumer_migration_map.md`,
`docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`,
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`, closed ideas
181-190, and draft 155; proof note recorded in `test_after.log`.
