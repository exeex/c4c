Status: Active
Source Idea Path: ideas/open/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build Route And Boundary Tables

# Current Packet

## Just Finished

Step 2: Build Route And Boundary Tables completed the route/boundary evidence
extraction needed to draft
`docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` without
re-reading every source file.

Per-idea selected-consumer facts for ideas 182-189:

| Idea | Selected consumer | Route view/facade | Prepared fallback/oracle retained | Target-policy boundary | Proof scope | Residual prepared readers |
| --- | --- | --- | --- | --- | --- | --- |
| 182 Route 4 | AArch64 dispatch-publication reader `current_block_entry_publication_register(...)`. | Narrow Route 4 publication-identity boundary over current/block-entry publication facts. | Prepared current-block and block-entry publication helpers stayed public as fallback/oracle surfaces. | Value homes, storage availability, move planning, publication records, and block-order emission stay prepared/AArch64-owned. | 3-test focused subset: dispatch publication, block-entry publications, prepared lookup helper; 3/3 passed. | Other AArch64 value-publication paths, prepared printer/debug consumers, x86 wrapper/publication surfaces, edge-publication lookup users, and oracle tests still justify public prepared publication helpers. |
| 183 Route 5 | AArch64 current-block join-source reader behind `build_current_block_join_prepared_query_routing(...)`. | Indexed Route 5 current-block join-source view. | Prepared current-block join-source facts plus edge-publication/source-producer and related prepared lookup surfaces remain fallback/oracle. | Parallel-copy scheduling, execution placement, source/destination homes, move bundles, branch spelling, and final edge-copy records stay prepared or target-owned. | 3 focused backend tests with regression guard; covered normal predecessor, missing predecessor, no-source, memory-source, absent route, and fail-closed route-index behavior. | Edge-copy and join parallel-copy consumers beyond the selected reader, move-bundle users, prepared edge-publication helpers, and oracle tests remain. |
| 184 Route 1 | Publication-source consumer `value_publication_may_read_register_index(...)`. | Route 1 publication producer view for complete same-block source-producer facts. | Prepared same-block producer, integer-constant, scalar operand, value-publication, and related oracle/fallback surfaces remain. | Value homes, target registers, storage encodings, move generation, rematerialization spelling, and machine-record policy stay prepared/AArch64-owned. | Focused proof covered route-first behavior, fallback/oracle visibility, no/missing/future producer, recursive operand dependency without prepared source-producer indexes, and unrelated-register rejection. | Other scalar materialization, producer/constant, publication, ALU, memory/call operand, printer, and test consumers still read prepared producer/constant/value-publication facts. |
| 185 Route 2 | AArch64 scalar ALU control-publication `select.result` path in `lower_scalar_select_publication(...)`. | Local Route 2 adapter for select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | Prepared select-chain materialization and direct-global dependency helpers remain public fallback/oracle surfaces. | Target materialization sequence, storage/home choice, memory operand formation, call/publication policy, and final record spelling stay outside Route 2. | 4 focused backend tests with regression guard; 4/4 passed. | Publication, call, memory, ALU, FP, producer, edge-copy, printer/test, and remaining select-chain/direct-global consumers still require prepared helpers or oracle comparisons. |
| 186 Route 3 | AArch64 indirect-callee stored-value source consumer. | Narrow Route 3 stored-source / memory semantic source view. | Prepared memory/access, global-load, same-block global-load, and load-local source helpers remain public fallbacks/oracles. | Address base kind, offsets, frame/stack objects, TLS/relocation facts, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and memory operand records stay prepared/AArch64-owned or target-owned. | Canonical four-test proof passed 4/4; supervisor backend validation passed 180/180. | Direct memory/addressing, globals, load-local, FP/global-load materialization, store/publication writeback, printer/oracle, and target-addressing consumers remain prepared-owned. Idea 190 proves this boundary is a blocker rule. |
| 187 Route 6 | Direct-global select-chain call-argument materialization consumer. | Route 6 semantic dependency/call-use source facts read first. | Prepared publication-source routing helper plus call-plan, argument/result plan, value-home, move-bundle, and call-boundary effect helpers remain fallback/oracle surfaces. | ABI register/stack placement, variadic FPR counts, clobbers/preserve sets, byval lanes, outgoing stack sizing, late-publication mechanics, helper resources, carrier protocols, and call record spelling stay target-owned. | 4 focused backend tests before/after with regression guard; supervisor broader backend validation passed `^backend_`. | Other call argument/result role classes, publication-source classes, recursive operand fallback, ABI/helper/call-record consumers, and x86/riscv call wrappers remain. |
| 188 Route 7 | `aarch64::codegen::lower_materialized_compare_condition_branch(...)`. | Validated Route 7 materialized-condition provenance. | Prepared comparison helpers, scalar producer/select-chain fallbacks, Route 7 facade validation, fused-compare oracles, and emitted-condition fallback remain. | Branch spelling, fused legality, condition-code selection, hazards, emitted-register state, final branch/compare record ordering, ALU result storage, and return-chain operand recovery stay outside BIR. | Reviewer/supervisor accepted one-consumer migration with fallback and invalid-reference behavior preserved. | Other fused-compare, scalar comparison, branch operand, ALU, return-chain-adjacent, printer/debug, and oracle consumers still block comparison helper contraction. |
| 189 Cross-target | x86 `ConsumedPlans` call-boundary selector. | Reused AArch64-proven Route 6 call-use source view when Route 6 and prepared call-plan source ids agree. | Prepared fallback retained for absent or mismatched facts; x86 consumed prepared wrappers and route-debug surfaces remain. | x86 ABI placement, frame layout, register selection, wrapper decisions, instruction spelling, and target emission remain x86-owned. | `backend_prepared_lookup_helper` default-path coverage plus fresh `^backend_` regression guard. | x86 reuse is Route 6-only and scalar named `i32 ArgumentValue`-only; riscv has no imported reuse; x86/riscv target-local wrappers and emission policy remain prepared/target-owned. |

Route coverage and readiness outline:

| Area | Step 2 readiness fact |
| --- | --- |
| Route 1 producer/constant | Two selected-consumer rungs exist across earlier Route 1 thinning and idea 184, but route-wide migration is not proven. Prepared producer, integer constant, scalar operand, value publication, target home/register/storage, and oracle surfaces remain. |
| Route 2 select-chain/direct-global | Selected scalar publication path moved in idea 185. Prepared select-chain/direct-global helpers stay public for publication, call, memory, ALU, FP, producer, edge-copy, and diagnostics/test consumers. |
| Route 3 memory/source | Selected semantic stored-source reader moved in idea 186, but idea 190 makes the audit rule explicit: Route 3 semantic identity must not replace prepared target addressing or AArch64 materialization policy. |
| Route 4 publication | Selected dispatch-publication reader moved in idea 182 after earlier block-entry/current-block/facade work. Prepared publication helpers still serve residual production, printer/debug, x86 wrapper, edge-publication, and oracle uses. |
| Route 5 edge/join-source | Selected current-block join-source reader moved in idea 183. Prepared edge-publication, join-source, move-bundle, and parallel-copy policy surfaces remain. |
| Route 6 call-use source | Selected AArch64 direct-global call-argument class moved in idea 187 and one x86 wrapper reused that interface in idea 189. Most call result/argument classes and ABI/helper policy remain prepared/target-owned. |
| Route 7 comparison/condition | Selected materialized-condition branch consumer moved in idea 188. Prepared comparison/fused-compare helpers and target branch/ALU policy remain. |
| Route-index facade | Coverage remains intentionally narrow: selected Route 4/Route 7 validation only. It is not a universal route aggregate or `PreparedFunctionLookups` replacement. |

Remaining prepared/public surfaces to preserve in the audit:
- `PreparedFunctionLookups` remains an aggregate/pass-context and oracle bundle,
  not a BIR-owned schema. It projects call, address, move, value-home,
  publication, select-chain/source-producer, memory, comparison, and
  return-chain lookup groups used by residual production, printer/debug,
  target-wrapper, and oracle-test consumers.
- Prepared printer/debug/route-debug/oracle-test surfaces remain public
  validation consumers until replacement diagnostics exist for each migrated
  route family.
- Prepared value-home, storage, register, move-bundle, edge-publication,
  call-plan, address-materialization, memory-access, comparison, and
  return-chain helpers remain fallback/oracle or target-policy carriers.
- Wide-value carriers, runtime helper protocols, variadic entry, intrinsics,
  inline asm, atomics, dynamic stack, frame helpers, special target entries,
  and final machine/emit records are non-migration surfaces for this Phase D
  route ladder.

Target-policy boundary table for the durable audit:
- Calls: ABI placement, variadic counts, clobbers/preserves, byval lanes,
  outgoing stack, late publication, helper/carrier protocols, and call record
  spelling remain target-owned.
- Memory: address bases, offsets, stack/frame/TLS/relocation facts, pointer
  materialization, base-plus-offset legality, volatility/address spaces, and
  final memory operands remain prepared/AArch64-owned or target-owned.
- Publication/materialization: value homes, storage, registers, move planning,
  rematerialization spelling, publication records, block order, and emitted
  machine records remain prepared/AArch64-owned.
- Edge/control flow: parallel-copy scheduling, execution sites,
  source/destination homes, move bundles, branch spelling, and final edge-copy
  records remain prepared or target-owned.
- Comparison/ALU: fused legality, condition-code selection, branch spelling,
  hazards, emitted-register state, ALU result storage, and final record order
  remain target-owned.
- Cross-target wrappers: x86/riscv frame, register allocation, ABI, formatting,
  wrapper decisions, and emission remain target-local.

Cross-target and return-chain status:
- Cross-target reuse is proven only for x86 `ConsumedPlans` consuming the
  AArch64-proven Route 6 call-use source view when route and prepared source
  ids agree. There is no route-wide x86 migration and no riscv reuse proof in
  ideas 182-189.
- Return-chain must be imported as its own completed owner/schema line from
  `docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` and ideas
  176-180. The accepted return-chain route owns only target-neutral identity:
  function/block/instruction position, chain value, terminal return value, and
  optional next operand value. AArch64 still owns value homes, return ABI
  register choice, prepared-register conversion, alias detection, scratch
  selection, scalar register view, final ALU records, and emission order.
- The audit must not fold return-chain into Route 1 producer identity, Route 7
  comparison provenance, predecessor rescans, name matching, or a generic
  route-index facade.

Retirement blockers for `PreparedFunctionLookups` / `PreparedBirModule`:
- Ideas 182-189 prove selected consumer boundaries, not route-wide prepared API
  contraction.
- `PreparedFunctionLookups` cannot retire until residual production,
  printer/debug, target-wrapper, and oracle-test users are mapped by field and
  either migrated to route views or explicitly retained as target-policy
  owners.
- `PreparedBirModule` retirement analysis remains blocked until a
  field-by-field map names which facts become BIR annotations, which remain
  transient pass context, which remain target/prepared policy, and which need
  compatibility adapters. Draft 155 is not ready to open from selected-consumer
  evidence alone.
- Idea 190 is a hard readiness rule: semantic route facts can be correct while
  broader AArch64 behavior regresses if prepared fallback/target policy is
  bypassed. The durable audit should treat this as a blocker pattern, not only
  as a repaired test failure.
- Likely prerequisite follow-up areas: residual route-view consumer migrations
  by route family, Route 3 policy-boundary hardening, prepared printer/debug
  replacement planning, cross-target reuse beyond the single Route 6 x86
  boundary, `PreparedFunctionLookups` field ownership/readiness audit,
  return-chain import/naming clarification, and true Phase D vs Phase E
  lifecycle naming cleanup.

## Suggested Next

Execute Step 3 from `plan.md`: draft
`docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` using the
Step 2 tables above, with explicit readiness conclusions and follow-up idea
requirements.

## Watchouts

- This is analysis-only; do not edit implementation files or start draft 155.
- Treat ideas 182-189 as Phase D follow-up slices, not true Phase E retirement.
- Preserve prepared fallback/oracle and target-policy boundaries in the audit;
  do not convert selected-consumer proof into route-wide contraction language.
- Reflect idea 190's Route 3 finding as a readiness rule or blocker: semantic
  memory/source identity does not replace prepared target-addressing or
  materialization fallback.
- Import return-chain as its own completed owner/schema and contraction line,
  not as generic Route 1/Route 7/aggregate evidence.
- Do not cite the duplicate `177_aarch64_selected_f64_global_readback_dispatch_debt.md`
  as a return-chain follow-up.
- Draft 155 remains closed/draft until the audit can name residual prepared
  surfaces, target-policy owners, compatibility needs, and prerequisite open
  ideas.

## Proof

Docs-only/lifecycle-only packet; no build or CTest run required by the
delegated proof. Verified by updating `todo.md` with the Step 2 route and
boundary table facts needed for Step 3 drafting. No new `test_after.log` was
produced for this documentation/source-evidence synthesis packet.
