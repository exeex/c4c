Status: Active
Source Idea Path: ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Matrix Usability

# Current Packet

## Just Finished

Step 4 - Validate Matrix Usability completed as an analysis-only packet. The
matrix below is usable as the future reviewer checklist required by the source
idea: every row identifies a public prepared compatibility surface and pairs it
with either route/BIR agreement requirements or explicit fail-closed behavior.
The common rows remain the shared rejection reference for prepared-only,
route/BIR-only, mismatch, duplicate/conflict, unsupported, fallback,
expected-string, and policy-sensitive behavior. The family rows add the exact
prepared surface and route/BIR agreement gates a future reviewer should check.

| Common row | Prepared surface | Required agreement or rejection condition | Reviewer reject signal |
| --- | --- | --- | --- |
| Public status/name preservation | Public helper/oracle names for prepared decoded home storage, formal publications, edge-copy/source facts, aggregate stack source authority, typed stack source publication, scalar publication, store-source publication, block-entry/current-block-entry publication, call-boundary classification, route statuses, target move intents, and wrapper kinds. | Any new route/BIR evidence must preserve the exact public spelling emitted by prepared status helpers and prepared printers, including `PreparedCallWrapperKind` names and target intent statuses. A change that only renames helpers, aliases enum names, or moves the spelling behind a classification wrapper is not proof. | Reject helper renames, status-string churn, renamed wrapper kinds, changed `--dump-prepared-bir` spelling, or claims that classification-only parity proves compatibility while public output names changed. |
| Fallback preservation | `PreparedFunctionLookups`, target `ConsumedPlans::prepared_lookups`, prepared call plans, prepared addressing/memory access readers, prepared publication readers, source producers, names, and value homes. | When route/BIR evidence is absent, stale, mismatched, incomplete, duplicate, or policy-insufficient, target lowering and prepared printer/debug output must continue using the prepared fallback path and preserve the previous output. Agreement may annotate or gate use, but disagreement must not transfer authority away from prepared facts. | Reject removal of prepared lookup reads, unconditional route-first lowering, fallback deletion, route/BIR-only output replacing prepared dump rows, or named-case shortcuts that only preserve one known fallback test. |
| Unsupported fail-closed behavior | Target intent status surfaces for x86 and RISC-V edge publications and any prepared reader that reports unsupported source, destination, publication, memory, call, or wrapper conditions. | Unsupported route/BIR/prepared combinations must fail closed into an explicit unsupported or missing status while preserving prepared fallback behavior where the previous contract did so. Unsupported must not be silently treated as available, and unsupported rows must not weaken expected contracts. | Reject expectation rewrites that downgrade supported behavior to unsupported, changes that turn unsupported into available without semantic proof, or classification-only changes that hide unsupported reasons behind a generic success path. |
| Expected-string stability | Prepared printer rows, route-debug rows, prepared dump CLI output, wrapper strings, grouped authority summaries, and target-policy-sensitive expected asm snippets. | Expected strings may change only when the prepared compatibility contract intentionally changes through source intent; route agreement rows must prove old strings remain stable for non-agreeing evidence. Tests must validate the observable string surface, not only internal enum classification. | Reject baseline churn, weaker substring-only assertions that drop compatibility text, helper-renamed output, or expectation rewrites used as proof instead of preserving the public surface. |
| Missing or invalid evidence | Missing shared lookups, missing publications, missing source/destination homes, missing names, invalid route indexes, absent route rows, malformed route references, and missing prepared evidence. | Missing or invalid route/BIR evidence must reject route authority and keep prepared fallback/output stable. Missing prepared evidence must produce the existing explicit missing status rather than inventing route-only authority. | Reject accepting nameless or invalid route facts, treating absence as agreement, fabricating raw-string names, or proving only by adding a classification that does not preserve prepared behavior. |
| Duplicate or conflict evidence | Duplicate route rows, duplicate/conflicting publications, conflicting names, conflicting source producers, conflicting call/memory facts, and ambiguous prepared-or-route ownership. | Duplicate or conflicting evidence must fail closed by rejecting route authority unless all identity, owner, key, value, and policy fields agree with the prepared surface. Ambiguity is not a reason to select an arbitrary route row. | Reject first-match or last-match shortcuts, testcase-shaped duplicate filters, conflict resolution by named case, or a pass condition that ignores one conflicting row. |
| Mismatch evidence | Wrong owner, wrong key, wrong successor/predecessor, wrong instruction, stale symbol/link-name ids, wrong value/home, wrong callsite/block index, and route/prepared name mismatches. | Route/BIR evidence may participate only when it matches the prepared identity and policy fields required by the family row. Any mismatch must preserve the prepared path or produce the existing explicit missing/invalid/mismatch status. | Reject broad equality shortcuts, raw-string name fallback authority, matching on display text alone, or changes that make a mismatched route fact drive target output. |
| Prepared-only preservation | Prepared-only surfaces including `--dump-prepared-bir`, prepared printer/debug rows, call wrapper output, prepared memory/GOT/address materialization behavior, and prepared target fallback asm. | If no agreeing route/BIR fact exists, prepared-only behavior remains authoritative and observable. Route/BIR rows may not be required for existing prepared output to appear. | Reject route/BIR-only replacement of prepared dumps, requiring route facts to print existing prepared rows, removing prepared-only target paths, or claiming compatibility through route coverage while prepared-only tests regress. |
| Route/BIR-only rejection | Route 1/4/5/6 availability/status rows, route index validation, route-debug annotations, and any BIR fact that overlaps a prepared call, memory, publication, source, name, or control-flow surface. | Route/BIR-only facts are rejected as proof unless they agree with the prepared identity, owner, key, status, and target policy required by the matching row. Route/BIR facts can gate diagnostics or validation; they cannot silently replace prepared authority. | Reject classification-only changes, route-only positive assertions, helper renames presented as proof, or named-case shortcuts that accept a specific route row without a general agreement check. |
| Policy-sensitive output stability | x86 call asm fallback, RISC-V `mv`/`lw`/`sw` edge moves, AArch64 prepared memory/GOT/address materialization, and target wrapper or publication behavior controlled by prepared policy. | Target output must stay stable when route/BIR facts are absent, stale, mismatched, incomplete, unsupported, or insufficient for the target policy. Route evidence can drive output only after proving policy-compatible agreement with prepared facts. | Reject output changes justified only by route availability, policy-insensitive lowering, expectation rewrites for target asm, or named-case materialization shortcuts that do not generalize across nearby policy cases. |

### Calls

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Call-plan identity | `PreparedCallPlans`, `PreparedCallPlanLookups`, `PreparedCallPlan`, `PreparedCallArgumentPlan`, and `PreparedCallResultPlan`. | Route 6 or BIR call-use evidence may feed one selected source read only when call instruction identity, argument index or result lane, source value id/name, and availability status agree with the prepared call plan. Missing, wrong-call, duplicate, no-match, ABI-bound, or route-only facts reuse the Step 2 common missing, duplicate/conflict, mismatch, unsupported, prepared-only, and fallback rows. | Reject whole `call_plans` substitution, route-only call-source acceptance, first-match duplicate handling, ABI-bound promotion to available, or removal of prepared call-plan fallback/oracle rows. |
| Wrapper and target policy | `PreparedCallWrapperKind`, direct/same-module wrapper rows, helper/carrier protocols, variadic FPR counts, byval lanes, outgoing stack sizing, clobber/preserve facts, and final call records. | Route/BIR agreement must not claim ownership of wrapper kind or ABI/layout policy. Any future wrapper packet must prove byte-stable wrapper output for absent, stale, mismatched, duplicate/conflict, unsupported, and policy-insufficient route facts before route evidence affects emitted calls. | Reject wrapper-output baseline rewrites, broad x86/riscv wrapper migration from one Route 6 scalar-source proof, or moving ABI/frame/register policy into BIR. |
| Route-debug and `ConsumedPlans` | x86 `ConsumedPlans::prepared_lookups`, `find_consumed_call_plan`, `find_consumed_call_argument_plan`, `find_consumed_call_result_plan`, and the x86 Route 6 scalar source route-debug row. | `ConsumedPlans` may consume Route 6 only at the proven scalar call-source boundary after route/prepared source agreement. Absent, unavailable, wrong-call, duplicate, mismatched source-name/value, ABI-bound, or non-scalar evidence preserves prepared selection and route-debug spelling. | Reject deleting `prepared_lookups`, treating the scalar helper as broad `ConsumedPlans` migration, weakening route-debug names, or accepting one named x86 case without adjacent fallback/mismatch coverage. |
| Call-boundary helper statuses | `PreparedCallBoundaryMoveClassificationStatus` and `PreparedPriorPreservedValueLookupStatus`. | Helper/oracle statuses must retain public spelling. Route/BIR facts can annotate one semantic source only after matching the prepared call-boundary value, home, move, and owner fields; unsupported move or missing preserved-value cases fail closed through existing statuses. | Reject status renames, classification-only parity, or route facts driving call-boundary moves while prepared move/home or preserved-value authority disagrees. |

### Memory And Source-Memory

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Memory lookup identity | `PreparedMemoryAccessLookups`, `PreparedAddressMaterializationLookups`, `PreparedSameBlockLoadLocalStoredValueSource`, and prepared addressing/GOT/materialization records. | Route 3 or BIR memory/source facts may replace one selected semantic read only when block/instruction identity, source value, address base, loaded/stored value, and node kind agree with prepared memory access. Missing, invalid, stale, duplicate, mismatch, route-only, or incomplete memory facts keep prepared memory/address materialization output stable through the common rows. | Reject route-first memory lowering, direct-global/load-local shortcuts that skip prepared fallback, stale instruction matching, or AArch64 expected-asm rewrites used as proof. |
| Source-memory publication status | `PreparedEdgePublicationSourceMemoryAccessStatus` with `Available`, `MissingPreparedMemoryAccess`, `IncompletePreparedMemoryAccess`, and unavailable states. | A route/BIR memory source can mark a publication source only when the prepared source producer is `LoadLocal` and the prepared memory access is complete. Non-`LoadLocal`, missing, incomplete, mismatched, route-only, or duplicate memory evidence must preserve the existing source-memory status. | Reject treating any route memory fact as materializable, collapsing missing vs incomplete statuses, or accepting memory-source publication without the prepared `LoadLocal` dependency. |
| Target memory policy | Prepared AArch64 memory/GOT/address materialization, store/load operand records, and target-specific memory output snippets. | Route/BIR agreement covers semantic source identity only; addressing mode choice, immediate spelling, register/stack policy, and materialization sequence remain prepared/target policy unless a later source idea transfers ownership. Unsupported, fallback, and policy-sensitive cases reuse the Step 2 output-stability row. | Reject policy-insensitive memory output changes, route-only address materialization, or named-case materialization shortcuts that do not cover adjacent load/store/GOT forms. |

### Edge Publications

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Publication identity | `PreparedEdgePublicationLookups`, `PreparedEdgePublicationLookupStatus`, `PreparedBlockEntryPublicationStatus`, and `PreparedCurrentBlockEntryPublicationStatus`. | Route 4 or Route 5 publication evidence must match predecessor/successor or block label, destination value/home, source value, publication key, instruction relationship, and availability status before attribution or lowering can use route data. Missing publication, missing labels, wrong destination, stale/wrong instruction, duplicate/conflict, route-only, or no-match evidence keeps prepared publication rows and fallback behavior. | Reject route-only publication records, first-match duplicate records, wrong-reference attribution, public status spelling churn, or contraction of prepared publication helpers from one selected reader. |
| Printer/debug and dump rows | `--dump-prepared-bir`, prepared value-location rows, `--- prepared-block-entry-publications ---`, `block_entry_publication ... status=available ...`, and Route 4 attributed printer rows. | Agreeing route evidence may add only the proven attribution while preserving byte-stable prepared row text. Missing-PHI, wrong-successor, wrong-destination, wrong-instruction, duplicate, route/BIR-only, and prepared-only cases must keep the existing prepared row without attribution or keep the existing absence status. | Reject baseline churn, substring-only assertions that drop publication text, or printer replacement that does not prove every listed fallback/mismatch/duplicate case. |
| Edge output policy | `PreparedTypedStackSourcePublicationStatus`, `PreparedAggregateStackSourceAuthorityStatus`, target edge-copy records, x86/riscv publication wrapper paths, and RISC-V `mv`/`lw`/`sw` emission. | Route 4/5 can own selected semantic identity only. Stack-source extension, destination bank/register view, move/home/storage policy, block-order emission, immediate spelling, wrapper formatting, and final edge-copy records require prepared/target agreement and remain fail-closed on unsupported or policy-insufficient facts. | Reject moving edge-copy mechanics into route facts, weakening typed stack-source statuses, or changing target publication output because a route publication identity is available. |

### Source Producers

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Source-producer lookup identity | `PreparedEdgePublicationSourceProducerLookups`, `PreparedEdgePublicationSourceProducer`, and `PreparedEdgePublicationSourceProducerKind` values `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, `SelectMaterialization`, `Immediate`, and `Unknown`. | Route 1, Route 2, Route 5, Route 6, Route 7, or BIR producer evidence can replace one selected producer read only when block, instruction, produced value/name, producer kind, source value, direct-global/select-chain dependency, and materializability agree with the prepared source producer. Missing producer, unknown producer, duplicate/conflict, wrong owner/key, mismatch, route-only, or unsupported facts preserve prepared producer fallback/status. | Reject aggregate source-producer lookup deletion, matching on display text alone, accepting route-only materialization, or adding named-case producer shortcuts for one source kind. |
| Dynamic `LoadLocal` dependencies | Prepared `LoadLocal` source producers, `PreparedSameBlockLoadLocalStoredValueSource`, source-memory access fields, and Route 3/Route 6 memory-source bridges. | A dynamic `LoadLocal` producer is accepted only when route/BIR producer identity and prepared memory-source identity both agree. Missing source-memory, incomplete memory access, nonmaterializable source, stale instruction, duplicate, or mismatch cases fail closed to prepared fallback and existing statuses. | Reject treating `LoadLocal` as a generic scalar producer, skipping source-memory agreement, or route-only call/publication materialization when prepared memory data is incomplete. |
| Producer printer/oracle rows | Prepared source-producer helper/oracle tests, prepared printer `select_chains` and publication-source rows, route-debug producer rows, and wrapper-adjacent source paths. | Any route-native producer diagnostic must be row-scoped and byte-stable. Absent, invalid, ambiguous/conflict, mismatch, prepared-only, route/BIR-only, unsupported, and policy-sensitive cases reuse common rows and must preserve prepared printer/debug/oracle output. | Reject helper renames, route-debug name churn, printer replacement without fallback cases, or source-producer proof that only covers one expected string. |

### Names

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Name-table identity | `PreparedNameTables`, `ValueNameId`, `BlockLabelId`, symbol/link-name ids, generated labels, and prepared printer name rendering. | Route/BIR evidence must match the prepared id and owner relationship, not just rendered text. Missing names, invalid ids, stale symbol/link-name ids, raw-string-only matches, duplicate/conflicting names, or route-only names preserve the prepared name table output or produce the existing explicit missing/invalid status. | Reject raw-string fallback authority, display-text-only matching, fabricated route names, or status-preserving claims that rename public prepared output. |
| Cross-family name gates | Names used by calls, memory, publications, source producers, control-flow edges, and store-source rows. | A family row may accept route evidence only after all referenced prepared names for owner, block, successor/predecessor, value, instruction, callsite, and symbol agree. Name mismatch is a hard route/BIR rejection and must not be hidden by later semantic agreement. | Reject route facts that drive output after name mismatch, broad equality helpers that ignore owner/key fields, or duplicate-name conflict resolution by first/last match. |

### Control Flow

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Control-flow authority | `PreparedControlFlowFunction`, `PreparedControlFlowBlock`, `PreparedBranchTargetLabels`, `PreparedBranchCondition`, and prepared branch records. | Route/BIR control-flow evidence must match block label, terminator instruction, successor/predecessor relationship, branch condition value, and true/false targets before it can affect a selected branch/join read. Missing target, unsupported predicate, missing condition home, mismatched prepared targets, route-only, duplicate/conflict, or no-match evidence fails closed through existing branch diagnostics/statuses. | Reject route-only branch spelling, target-label swaps, unchanged old failure modes with renamed statuses, or baseline rewrites for unsupported branch cases. |
| Join and edge movement | `PreparedAuthoritativeBranchJoinTransfer`, `PreparedAuthoritativeBranchParallelCopyBundles`, `PreparedAuthoritativeJoinBranchSources`, `PreparedShortCircuitBranchPlan`, move bundles, and current-block join-source helpers. | Route 5 join-source evidence may answer one semantic source question only after predecessor/successor, source/destination value, current-block join key, move-bundle relationship, and no-source/memory-source status agree with prepared facts. Duplicate/conflict, mismatch, unsupported move, route/BIR-only, and prepared-only cases preserve prepared parallel-copy scheduling and branch output. | Reject moving move ordering, execution site, cycle temporary, scratch, branch spelling, or final edge-copy records into Route 5; reject one-reader join proof used as broad control-flow migration. |
| Comparison/control provenance | Prepared materialized compare join branches, fused-compare branch facts, Route 7 comparison provenance where present, and route-debug/printer rows. | Route 7 or BIR comparison facts must match comparison instruction, operands, condition value, producer provenance, and branch targets. Missing operand producer, duplicate producer, non-comparison, absent provenance, route-only, or mismatch cases keep prepared compare/branch fallback and output. | Reject comparison-route availability as branch-policy proof, helper renames, or diagnostic rows that cover only the positive fused case. |

### Store-Source Publications

| Family row | Prepared surface | Future route/BIR agreement or fail-closed gate | Reviewer reject signal |
| --- | --- | --- | --- |
| Store-source publication plans | `PreparedStoreSourcePublicationPlans`, `PreparedStoreSourcePublicationPlan`, `PreparedStoreSourcePublicationRecord`, `PreparedStoreSourcePublicationStatus`, `PreparedRecoveredStoreSourcePublication`, and `--- prepared-store-source-publications ---` printer rows. | Route 1/2/3/4/6 or BIR source/publication evidence may feed one store-source row only when stored value, store instruction, source producer, memory/source identity, publication destination, direct-global/select-chain dependency, and status agree with the prepared plan. Missing, invalid, duplicate/conflict, mismatch, unsupported, route/BIR-only, or prepared-only facts preserve prepared printer rows, helper statuses, and publication fallback. | Reject whole `store_source_publications` replacement, route-only stored-source authority, status-name weakening, or expected-string rewrites for store-source rows. |
| Store-source output and policy | Prepared scalar load-local source producers, fixed-formal store-source publications, store-global publication candidates, AArch64 store/load output, and publication/materialization policy. | Route/BIR agreement covers source identity only; store materialization, fixed-formal policy, destination publication construction, ordering, and target output remain prepared/target-owned. Unsupported or policy-sensitive facts must fail closed without changing output or weakening contracts. | Reject moving publication/materialization mechanics into route facts, policy-insensitive store output changes, or named-case handling for one direct-global/load-local store. |

### Usability Validation Summary

- Source idea acceptance criteria are represented directly: the matrix is a
  concrete rejection checklist, each row names the authoritative public
  prepared compatibility surface, and no row authorizes field deletion,
  privatization, helper/status renames, or broad prepared retirement.
- Route/BIR agreement gates are explicit for calls, memory/source-memory, edge
  publications, source producers, names, control flow, and store-source
  publications. Where route/BIR evidence is absent, stale, invalid,
  duplicate/conflicting, mismatched, unsupported, route-only, or
  policy-insufficient, the row requires prepared fallback/output preservation
  or an existing explicit fail-closed status.
- Reviewer reject signals cover the source idea's required negative cases:
  expectation weakening, old-failure retention through renamed surfaces, field
  deletion or privatization, broad prepared retirement, helper/status renames,
  classification-only preservation, and testcase-shaped shortcuts.
- The matrix is scoped as compatibility-retention proof only. It does not
  implement adapters, demotions, semantic ownership transfer, target-policy
  migration into BIR, baseline rewrites, or expected-string refreshes.

### Open Proof Gaps

- Several rows name status families and printer/debug surfaces rather than
  citing every exact enum value, CLI row, and expected string from code/tests.
  Future implementation packets should add focused citations or row-specific
  tests before claiming a compatibility surface is fully proven.
- Target-policy-sensitive rows identify x86, RISC-V, and AArch64 output
  classes, but do not enumerate every adjacent asm baseline. Future target
  packets must prove nearby policy cases, not only the named positive case.
- Route/BIR agreement requirements are intentionally strict where ownership
  transfer is out of scope. A later source idea is required before relaxing any
  prepared fallback, public-field, helper/status spelling, or target-policy
  requirement.

### Reviewer Checklist

- Reject expectation weakening, unsupported downgrades, weaker fallback
  behavior, or baseline rewrites without explicit source-idea approval.
- Reject retaining the exact old failure mode while changing only the
  compatibility surface name.
- Reject public field deletion, API privatization, broad prepared retirement,
  or removal of prepared fallback readers/helpers.
- Reject helper renames, status renames, wrapper kind renames, route-debug name
  churn, and prepared printer/debug spelling churn as compatibility proof.
- Reject classification-only preservation claims that do not preserve the
  observable prepared surface.
- Reject testcase-shaped shortcuts, named-case materialization, first-match or
  last-match duplicate handling, and proof that covers only one expected string
  or one baseline.
- Reject route/BIR-only authority unless identity, owner, key, value, status,
  name, and target-policy fields agree with the prepared surface required by
  the matching row.

## Suggested Next

Supervisor should decide whether the runbook is exhausted and whether to call
the plan owner for close, deactivate, or split handling.

## Watchouts

The matrix intentionally records open proof gaps without weakening requirements.
Future packets should treat those gaps as citation/test obligations, not as
permission to relax public prepared compatibility contracts.

## Proof

Analysis/documentation-only packet. No build, implementation tests, or
`test_after.log` were required by the delegated proof contract. Required
formatting proof: `git diff --check -- todo.md`.
