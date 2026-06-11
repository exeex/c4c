# Phase D Follow-Up Pre-Phase-E Readiness Audit

Status: Step 3 audit artifact.

Source idea: `ideas/open/191_phase_d_followup_closure_pre_phase_e_readiness_audit.md`

## Conclusion

Ideas 182-189 are Phase D follow-up implementation slices with Phase E-like
filenames. They prove selected route-view consumer boundaries and one
cross-target interface reuse point. They are not the true Phase E
`PreparedBirModule` retirement plan, and they do not justify prepared API
deletion, route-wide migration claims, or route-wide
`PreparedFunctionLookups`/`PreparedBirModule` contraction.

Draft 155, `ideas/draft/155_phase_e_prepared_bir_module_retirement_plan.md`, is
not ready to open immediately. Its required analysis asks for a field-by-field
retirement map, remaining blockers, compatibility adapters, printer/dump
migration, final consumer-switch follow-ups, and criteria for when demoting or
removing `PreparedBirModule` is safe. The Phase D follow-up evidence does not
yet supply that map. It identifies the prerequisite work that must precede
opening the true Phase E retirement analysis.

## Route Coverage

| Idea | Route/family | Selected consumer proven | BIR route view or facade | What remains unproven |
| --- | --- | --- | --- | --- |
| 182 | Route 4 publication | AArch64 dispatch-publication reader `current_block_entry_publication_register(...)`. | Narrow Route 4 publication-identity boundary over current/block-entry publication facts. | Route-wide publication migration, prepared publication helper contraction, x86/riscv publication-wrapper replacement, and printer/oracle replacement. |
| 183 | Route 5 edge/join-source | AArch64 current-block join-source reader behind `build_current_block_join_prepared_query_routing(...)`. | Indexed Route 5 current-block join-source view. | Edge-copy and join parallel-copy consumer families beyond the selected reader, move-bundle policy replacement, and prepared edge-publication helper contraction. |
| 184 | Route 1 producer/constant | Publication-source consumer `value_publication_may_read_register_index(...)`. | Route 1 publication producer view for complete same-block source-producer facts. | Broad scalar materialization, producer/constant, ALU, memory/call operand, publication, printer, and oracle consumers. |
| 185 | Route 2 select-chain/direct-global | AArch64 scalar ALU control-publication `select.result` path in `lower_scalar_select_publication(...)`. | Local Route 2 adapter for select-root identity, root instruction index, scalar eligibility, and direct-global dependency presence. | Remaining select-chain/direct-global publication, call, memory, ALU, FP, producer, edge-copy, printer/test, and oracle consumers. |
| 186 | Route 3 memory/source | AArch64 indirect-callee stored-value source consumer. | Narrow Route 3 stored-source / memory semantic source view. | Direct memory/addressing, globals, load-local, FP/global-load materialization, store/publication writeback, printer/oracle, and target-addressing policy consumers. |
| 187 | Route 6 call-use source | Direct-global select-chain call-argument materialization consumer. | Route 6 semantic dependency/call-use source facts read first. | Other call argument/result role classes, publication-source classes, recursive operand fallback, ABI/helper/call-record consumers, and x86/riscv wrappers. |
| 188 | Route 7 comparison/condition | `aarch64::codegen::lower_materialized_compare_condition_branch(...)`. | Validated Route 7 materialized-condition provenance. | Other fused-compare, scalar comparison, branch operand, ALU, return-chain-adjacent, printer/debug, and oracle consumers. |
| 189 | Cross-target reuse | x86 `ConsumedPlans` call-boundary selector. | Reuse of the AArch64-proven Route 6 call-use source view when route and prepared call-plan source ids agree. | Any route-wide x86 migration, any riscv reuse, and target-local frame/register/ABI/wrapper/emission policy migration. |

## Prepared Fallback And Oracle Surfaces

| Family | Public prepared surfaces to preserve | Reason they remain public |
| --- | --- | --- |
| Aggregate pass context | `PreparedFunctionLookups` and related lookup groups. | Still projects call, address, move, value-home, publication, select-chain/source-producer, memory, comparison, and return-chain facts used by residual production, printer/debug, target-wrapper, and oracle-test consumers. |
| Route 1 producer/constant | Same-block producer, integer-constant, scalar operand, and value-publication helpers. | Needed by unmigrated scalar materialization, producer/constant, publication, ALU, memory/call operand, printer, and test consumers. |
| Route 2 select-chain/direct-global | Prepared select-chain materialization and direct-global dependency helpers. | Needed as fallback/oracle for publication, call, memory, ALU, FP, producer, edge-copy, and diagnostic/test paths. |
| Route 3 memory/source | Prepared memory/access, global-load, same-block global-load, and load-local source helpers. | Needed where target-addressing and materialization policy still belongs to prepared/AArch64 code or target code. |
| Route 4 publication | Current-block and block-entry publication helpers, edge-publication lookup surfaces, x86 wrappers, and route-oracle tests. | Needed by residual publication paths and diagnostics while only selected publication readers have moved. |
| Route 5 edge/join-source | Edge-publication, source-producer, join-source, and move-bundle helpers. | Needed by residual parallel-copy and edge-copy consumers and equivalence tests. |
| Route 6 call-use source | Call-plan, argument/result plan, publication-source routing, value-home, move-bundle, and call-boundary effect helpers. | Needed by unmigrated call role classes and by ABI/helper/call-record policy. |
| Route 7 comparison/condition | Comparison helpers, scalar producer/select-chain fallbacks, Route 7 facade validation, fused-compare oracles, and emitted-condition fallback. | Needed by residual comparison/branch/ALU and return-chain-adjacent consumers. |
| Diagnostics and tests | Prepared printer, debug output, route-debug summaries, and prepared/route oracle tests. | These are validation consumers. They must remain until route-native diagnostics and equivalent oracle coverage exist. |

## Target-Policy Boundaries

| Area | Boundary that must remain prepared-owned or target-owned |
| --- | --- |
| Calls | ABI register/stack placement, variadic FPR counts, clobbers/preserves, byval lanes, outgoing stack sizing, late publication, helper/carrier protocols, and call record spelling. |
| Memory | Address bases, offsets, stack/frame objects, TLS/relocation facts, pointer materialization, base-plus-offset legality, volatility/address spaces, and final memory operands. |
| Publication/materialization | Value homes, storage encodings, target registers, move planning, rematerialization spelling, publication records, block order, and emitted machine records. |
| Edge/control flow | Parallel-copy scheduling, execution sites, source/destination homes, move bundles, branch spelling, and final edge-copy records. |
| Comparison/ALU | Fused legality, condition-code selection, branch spelling, hazards, emitted-register state, ALU result storage, and final branch/compare record order. |
| Cross-target wrappers | x86/riscv frame layout, register allocation, ABI, formatting, wrapper decisions, instruction selection, and emission. |
| Non-migration surfaces | Wide-value carriers, runtime helper protocols, variadic entry, intrinsics, inline asm, atomics, dynamic stack, frame helpers, special target entries, and final machine/emit records. |

Idea 190 makes the Route 3 boundary a hard readiness rule: semantic
memory/source identity can be correct while broader AArch64 behavior regresses
if prepared fallback or target-addressing policy is bypassed. Future route work
must prove that route-first reads preserve prepared fallback and target policy;
it must not treat narrow oracle success as permission to replace prepared
target-addressing behavior.

## Cross-Target Reuse

| Target | Current status | Readiness conclusion |
| --- | --- | --- |
| x86 | Idea 189 reused the AArch64-proven Route 6 call-use source view in `ConsumedPlans` only when Route 6 and prepared call-plan source ids agree. Prepared fallback remains for absent or mismatched facts. | This proves one interface reuse boundary, not route-wide x86 migration. x86 ABI, frame, register, wrapper, formatting, and emission policy remain target-local. |
| riscv | No imported reuse proof exists in ideas 182-189. | Any riscv migration must wait for an AArch64-proven semantic route view and then prove riscv wrapper consumption without inventing riscv-only BIR adapters first. |

## Return-Chain Import

Return-chain is importable as its own completed owner/schema line from
`docs/bir_prealloc_fusion/return_chain_owner_schema_decision.md` and the closed
return-chain follow-ups 176-180. It must remain distinct from Route 1 producer
identity, Route 7 comparison provenance, predecessor rescans, name matching,
and a generic route-index facade.

The accepted return-chain route owns only target-neutral identity:

- function, block, and instruction position;
- the named chain value produced by the scalar binary instruction;
- the terminal return value reached by the same-block chain;
- the optional next operand value of the immediate next binary when named;
- absent or invalid status for unavailable or conflicting answers.

AArch64 still owns value homes, return ABI register choice, before-return ABI
move lookup, prepared-register conversion, alias detection, scalar register
view, scratch selection, final ALU records, and emission order. Existing
prepared return-chain helpers should remain public migration oracles until the
BIR-owned return-chain route and migrated consumers have equivalent positive,
negative, ambiguous, and conflict coverage.

## Retirement Blockers

| Blocker | Why it blocks draft 155 opening |
| --- | --- |
| Selected-consumer scope | Ideas 182-189 each prove a bounded reader or interface boundary. They do not prove full route-family migration or residual reader exhaustion. |
| `PreparedFunctionLookups` field ownership | There is no current field-by-field map naming which lookup groups become BIR annotations, which remain transient pass context, which remain target/prepared policy, and which require compatibility adapters. |
| `PreparedBirModule` field ownership | Draft 155 requires inspection of construction, mutation, consumers, printer/dump tooling, and compatibility strategy before any retirement map can be written. This audit does not perform that implementation-field map. |
| Fallback/oracle consumers | Prepared printer/debug/route-debug/oracle-test consumers still need public prepared surfaces until replacement diagnostics and equivalence coverage exist. |
| Target-policy separation | Calls, memory, publication/materialization, edge/control-flow, comparison/ALU, and target-wrapper policy remain outside BIR ownership. The audit must not move them into route views by implication. |
| Cross-target breadth | Only one x86 Route 6 reuse point is proven; riscv has no imported reuse proof. |
| Return-chain import | Return-chain can be imported only as its own owner/schema route, not as evidence that Routes 1 or 7 cover return-chain behavior. |
| Route 3 boundary hardening | Idea 190 shows that route-first semantic facts need explicit prepared fallback and target-policy preservation rules before broader contraction. |

## Follow-Up Idea Requirements

The next lifecycle step should create separate open ideas for these
prerequisites before opening draft 155:

| Requirement | Scope the idea should own | Reject signals |
| --- | --- | --- |
| Residual route-view consumer migration map | For each route family 1-7, enumerate remaining production, printer/debug, target-wrapper, and oracle consumers and define the next selected-consumer migrations. | Claiming route-wide completion from one migrated reader; weakening prepared oracle tests; mixing unrelated route families into one catch-all implementation. |
| Route 3 prepared-policy boundary hardening | Turn idea 190's lesson into explicit route-first/fallback rules and tests for memory/source identity versus prepared target-addressing and materialization policy. | Treating the 190 regression as only a fixed testcase; moving address-materialization or target-addressing policy into BIR route facts. |
| Prepared printer/debug/oracle replacement planning | Define route-native diagnostics and oracle strategy needed before public prepared printer/debug/oracle surfaces can contract. | Removing prepared diagnostics before replacements exist; using production-only green tests as proof of diagnostic readiness. |
| Cross-target route-view reuse beyond x86 Route 6 | Identify x86 and riscv wrapper boundaries that can consume already-proven AArch64 route views while preserving target-local policy. | Inventing target-only BIR adapters; treating one x86 scalar call-use reuse as broad cross-target readiness. |
| `PreparedFunctionLookups` ownership/readiness audit | Build a field-by-field lookup-group map: BIR annotation, transient pass context, target/prepared policy, compatibility adapter, or retained oracle. | Renaming the aggregate without reducing coupling; deleting lookup groups with residual production or oracle readers. |
| Return-chain import and naming clarification | Import the completed return-chain owner/schema result as a distinct route and define how future retirement analysis cites it. | Folding return-chain into Route 1, Route 7, predecessor rescans, name matching, or a generic route-index facade. |
| Phase D versus Phase E lifecycle naming cleanup | Clarify that ideas 182-189 are Phase D follow-ups despite their filenames and that draft 155 remains the future true Phase E retirement plan. | Cosmetic renames that obscure history; opening draft 155 before the prerequisite maps exist. |

## Draft 155 Readiness

Draft 155 should remain a draft. It can become ready to open only after the
follow-up ideas above have produced enough durable evidence to answer its
required questions:

- which `PreparedBirModule` and `PreparedFunctionLookups` fields are durable
  semantic state after Phases A-D;
- which fields can become BIR annotations;
- which fields can become private pass context;
- which MIR/codegen, printer/debug, wrapper, and oracle consumers still require
  prepared wrappers;
- what compatibility adapters are required during retirement;
- what final proof strategy and deletion/demotion criteria make the endgame
  safe.

Until those prerequisites exist, the accepted readiness state is: Phase D
follow-up selected-consumer migrations are useful evidence for future Phase E,
but the project is not ready to open or execute the `PreparedBirModule`
retirement plan.
