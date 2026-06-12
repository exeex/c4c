# Phase E3 Prepared Diagnostic/Oracle Replacement Readiness

Status: Step 3 durable readiness document drafted.

Source idea:
`ideas/open/226_phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`

## Scope

This document is the durable Phase E3 analysis surface for prepared
diagnostic, printer/debug, route-debug, helper-oracle, and expected-string row
replacement readiness. E3 is analysis-only. It classifies candidate rows and
tightly scoped row families after E1 and E2 proved selected semantic identity
and private pass-context boundaries; it does not implement row replacement,
refresh baselines, rewrite expected strings, downgrade supported paths, delete
prepared fallback/oracle behavior, move target policy into route/BIR
diagnostics, open draft 155, open E5, or claim broad prepared diagnostic or
oracle retirement.

The source evidence for this document is the committed Step 1 inventory and
Step 2 candidate classification in `todo.md`. Those steps read the required
Phase E0, E1, E2, Route 3 through Route 7, D2, diagnostic/oracle replacement,
accepted baseline, and local hook-state inputs and found no missing required
analysis inputs.

## Readiness Rule

A candidate is ready for a later implementation idea only when it names one
diagnostic/oracle/string row or one tightly scoped row family, the positive
BIR/route fact that explains the row, the prepared fallback/oracle authority
that remains for every non-agreement path, and the proof matrix required
before output or oracle ownership can change.

Green production or backend tests are guardrails only. Baseline refreshes,
expected-string rewrites, helper renames, unsupported downgrades, timeout
masking, or section-label matching are not E3 replacement evidence.

## Candidate Readiness Table

| Candidate row or row family | E3 classification | Positive BIR/route fact | Retained authority and deferral decision |
| --- | --- | --- | --- |
| `find_prepared_control_flow_branch_target_labels(...)` helper-oracle branch-target label row for the selected AArch64 conditional branch identity consumer | Ready to draft one implementation idea. | E1 proved agreement-gated BIR structured successor label identity, and E2 moved the selected read behind `BranchTargetIdentityPassContext` / `read_agreeing_bir_branch_target_labels(...)`. | Prepared remains authoritative for public helper fallback, absent context, invalid ids, mismatch/conflict, non-conditional BIR, non-agreement, branch spelling, edge-copy scheduling, wrapper output, printer/debug text, helper-oracle statuses, expected strings, and aggregate APIs. |
| Branch-target printer/debug, wrapper, branch-control, edge-copy, or emitted-output rows adjacent to the selected helper | Retained target/prepared policy or emitted-output authority. | The same BIR successor identity may explain a diagnostic agreement, but it does not own output spelling or scheduling. | Branch emission, branch spelling, edge-copy scheduling, wrapper output, printer/debug rows, and expected strings stay prepared/target-owned. Future work must name one diagnostic row and avoid output-policy movement. |
| `find_prepared_fused_compare_operand_producer_facts(...)` helper-oracle positive operand-producer row for the selected AArch64 comparison lowering consumer | Ready to draft one implementation idea. | E1 proved Route 7/prepared agreement for the selected comparison provenance identity read, and E2 extracted the private Route 7 agreement reader for the selected fused-compare operand producer fact. | Prepared remains authoritative for absent route, invalid reference, duplicate/conflict, mismatch, unfused/non-agreement, prepared-only cases, helper-oracle names/statuses, expected strings, printer/debug, wrappers, aggregate route views, and prepared aggregate ownership. |
| Fused-compare branch-control and machine-printer rows whose positive text is explained only by the selected Route 7 comparison provenance fact | Proof harness only. | Route 7 comparison provenance can corroborate the selected operand-producer identity used by AArch64 comparison lowering. | Branch target choice, suffix mapping, fused legality, hazard checks, final assembler rows, machine-printer formatting, wrapper output, expected strings, and prepared fallback remain target/prepared-owned. Implementation defers until a later idea names one exact diagnostic row without target-policy movement. |
| Route 3 memory/source stored-value helper-oracle success row | Ready to draft one implementation idea. | Route 3 memory/source agreement can explain one stored-value helper-oracle success row. | Prepared diagnostics remain authoritative for non-memory negatives, alias/address ambiguity, mismatch, and fallback. Address formation, relocation, materialization, addressing legality, final operands, target wrappers, row text, and expected strings remain retained. |
| Route 3 prepared addressing printer row | Retained target/prepared policy or emitted-output authority. | Route 3 can provide memory access/source identity agreement, but the inventory does not prove route ownership of final addressing text. | Address formation, relocation, materialization, addressing legality, final operands, target-addressing fallback, target wrappers, printer text, and expected strings remain prepared/target-owned. |
| Route 4 `block_entry_publication` available-register printer/debug row | Ready to draft one implementation idea. | Route 4 attribution can reproduce one available-register block-entry publication prepared value-location printer row under prepared agreement and fail-closed fallback. | Prepared publication mechanics, block order/output policy, wrapper behavior, CLI dump and broad printer/debug sections, fallback, row text, x86/riscv wrapper output, and expected strings remain retained. |
| Route 5 current-block join-source helper-oracle row | Ready to draft one implementation idea. | Route 5 evidence is diagnostic metadata for one `PreparedCurrentBlockJoinParallelCopySourceFact` current-block join-source helper-oracle success row after agreement with prepared edge/join semantics. | Prepared behavior remains authoritative for absent, invalid, duplicate/conflicting, memory-source, mismatch, unsupported, branch/parallel-copy, prepared printer, wrapper, and expected-string paths. The adjacent prepared printer row is retained/deferred unless a future separate idea names it exactly. |
| Route 6 x86 scalar `i32` argument-source route-debug row | Ready to draft one implementation idea. | Route 6 x86 scalar argument-source agreement can support one route-debug row while keeping `ConsumedPlans` compatibility. | `ConsumedPlans`, x86 wrapper behavior, ABI/call wrapper policy, prepared call printer/debug, direct-call/helper-oracle families, public fallback, expected strings, and broad x86 call wrapper migration remain retained. |
| Route 7 materialized-condition helper-oracle row in `verify_prepared_bir_comparison_condition_producer_equivalence` | Ready to draft one implementation idea. | Route 7-native comparison evidence already augmented one materialized-condition helper-oracle row. | Prepared oracle assertion, branch policy, branch-control output, wrappers, final assembler behavior, expected strings, helper-oracle statuses, and broad printer/debug or route-index migration remain retained. |
| Route 7 fused-compare branch-control or machine-printer rows beyond the selected helper-oracle/materialized-condition row | Proof harness only. | Route 7 provenance can help verify comparison identity and materialized-condition agreement. | Branch-control output, suffix mapping, fused legality, hazards, final assembler rows, wrappers, printer/debug strings, expected strings, and target policy remain retained. Later work must name one diagnostic row and prove it is not output-policy movement. |
| Mixed Route 1/2/5/6/7 source-producer helper, printer, or oracle rows lacking a completed row-specific route closure | Retained prepared oracle/fallback authority. | No row-specific positive BIR/route fact is proven beyond the named branch-target, fused-compare, and Route 3 through Route 7 closures. | Prepared fallback/oracle, diagnostic/string authority, public compatibility APIs, wrappers, expected strings, and aggregate lookups remain retained until a future source idea proves one concrete row plus fallback boundary. |
| AArch64 lookup threading or private pass-context plumbing around retained lookup groups | Proof harness only. | E2 proved private pass-context readiness for selected helper families, not ownership of lookup-threading output. | Lookup groups, aggregate lookup construction, public prepared helper compatibility, wrappers, diagnostics, expected strings, and target policy remain retained. E2/private-pass-context follow-up work owns API contraction. |
| Cross-target wrapper output and wrapper compatibility rows | Cross-target wrapper prerequisite owned by E4. | No E3-positive route/BIR fact owns cross-target wrapper formatting or compatibility. | x86/riscv/AArch64 wrapper behavior, target ABI/layout/call policy, expected output, compatibility adapters, and emitted strings remain retained. E4 owns wrapper prerequisite analysis and migration. |
| Baseline files, expected strings, supported/unsupported markers, and helper-oracle names/status labels | Blocked by expected-string, baseline, or unsupported-path authority. | Green backend/full-suite logs and string-authority guardrails are non-regression context only, not row ownership evidence. | Baselines, expected strings, helper names/status labels, supported-path contracts, timeout behavior, and oracle strength remain authoritative unless a later implementation idea proves semantic behavior without baseline refresh, unsupported downgrade, or expectation rewrite. |
| `PreparedFunctionLookups`, `PreparedBirModule`, aggregate prepared lookup construction, aggregate route views, route facades, Route 8, draft 155, E5, and broad prepared diagnostic/oracle retirement | No-action. | No row-scoped positive fact in the Step 1 inventory transfers aggregate or retirement authority. | Aggregate prepared APIs, route facades, target policy, compatibility, fallback/oracle, diagnostics, and source-idea phase ownership remain retained. Route 8, draft 155, and E5 remain unopened unless separate lifecycle work reopens them. |

## Accepted Follow-Up Implementation Candidates

E3 accepts the following candidates for later source-idea drafting. Step 3 does
not create those ideas and does not start implementation.

| Accepted candidate | Scoped row or row family | Positive owner | Required proof before ownership can change |
| --- | --- | --- | --- |
| Branch-target helper-oracle row for `find_prepared_control_flow_branch_target_labels(...)` | One selected helper-oracle branch-target label row family around the private `BranchTargetIdentityPassContext` reader. | BIR structured successor identity under prepared agreement. | Positive agreement, absent context, invalid id, duplicate/conflict where applicable, mismatch, non-conditional BIR, non-agreement fallback, unchanged public helper fallback, unchanged printer/debug, wrapper, helper-oracle status/name/string, expected-string, and nearby same-feature behavior. |
| Fused-compare operand-producer helper-oracle row for `find_prepared_fused_compare_operand_producer_facts(...)` | One selected helper-oracle operand-producer row family around the private Route 7 agreement reader. | Route 7 comparison provenance under prepared agreement for the selected AArch64 comparison lowering consumer. | Positive agreement, absent route, invalid reference, duplicate/conflict, mismatch, unfused/non-agreement fallback, prepared-only fallback, unchanged branch-control and machine-printer output, wrappers, helper-oracle status/name/string, expected-string, and nearby same-feature behavior. |
| Route 3 memory/source stored-value helper-oracle success row | One memory/source helper-oracle success row. | Route 3 memory/source agreement. | Positive stored-value row, non-memory negatives, alias/address ambiguity, invalid or absent route evidence, mismatch, prepared fallback, unchanged address formation/materialization/legality/final operands, wrappers, helper-oracle strings, and expected strings. |
| Route 4 block-entry publication available-register printer/debug row | One `block_entry_publication` available-register printer/debug row. | Route 4 publication attribution under prepared agreement. | Positive available-register row, absent route evidence, wrong-reference, mismatch, duplicate-reference fallback, prepared publication mechanics retained, wrapper no-change proof, unchanged row text, printer/debug sections, CLI dump scope, and expected strings. |
| Route 5 current-block join-source helper-oracle row | One current-block join-source helper-oracle row. | Route 5 current-block join-source metadata after prepared edge/join agreement. | Positive helper-oracle row, absent, invalid, duplicate/conflicting, memory-source, mismatch, unsupported, branch/parallel-copy fallback, prepared-printer retention, wrapper no-change, unchanged helper-oracle strings, expected strings, and no edge-publication, move-bundle, or prepared-printer migration. |
| Route 6 x86 scalar `i32` argument-source route-debug row | One x86 scalar argument-source route-debug row. | Route 6 scalar argument-source agreement while retaining `ConsumedPlans`. | Positive row, absent, invalid, duplicate/conflict, mismatch, compatibility with `ConsumedPlans`, unchanged ABI/call wrapper behavior, prepared call printer/debug, direct-call/helper-oracle families, wrappers, and expected strings. |
| Route 7 materialized-condition helper-oracle row | One materialized-condition helper-oracle row in `verify_prepared_bir_comparison_condition_producer_equivalence`. | Route 7 comparison evidence under prepared agreement. | Positive materialized-condition row, absent-route, invalid-reference, duplicate/conflict, mismatch, unfused fallback, prepared fallback, unchanged oracle assertion strength, branch-control output, wrappers, final assembler behavior, helper-oracle strings, and expected strings. |

No accepted E3 follow-up may use a baseline refresh, expected-string rewrite,
helper rename, unsupported downgrade, timeout mask, section relabel, or broad
prepared diagnostic/oracle deletion as progress. Each later idea must be
one-row or tightly scoped row-family work and must preserve prepared authority
for all non-agreement paths.

## Retained Authority Decisions

Prepared fallback and oracle authority remain retained for absent evidence,
invalid references, duplicate/conflict, ambiguous facts, mismatch,
unsupported-path, prepared-only, policy-sensitive, and non-agreement cases.
E3 readiness can augment or explain a positive row only when the fallback
matrix remains intact.

Target policy remains retained for ABI, layout, registers, stack, address
formation, relocation, materialization, move scheduling, branch spelling,
suffix mapping, fused legality, hazard checks, call/helper protocols, final
records, final assembler rows, and emitted output.

Wrapper authority remains retained for x86, riscv, and AArch64 compatibility
output. Cross-target wrapper convergence is an E4 prerequisite, not an E3 row
replacement.

Expected-string and baseline authority remain retained. Exact row text,
helper-oracle names/status labels, supported-path contracts, timeout behavior,
baseline files, CLI dump snippets, printer/debug output, route-debug output,
and wrapper expected strings are guardrails, not replacement evidence.

Aggregate prepared authority remains retained for `PreparedFunctionLookups`,
`PreparedBirModule`, aggregate prepared lookup construction, aggregate route
views, route facades, public compatibility APIs, prepared fallback/oracle
surfaces, and pass-context delivery.

## Deferrals And No-Action Decisions

E1 owns future semantic helper contraction when a candidate still needs a
named semantic reader or route/BIR identity proof before diagnostic/oracle work
can be considered.

E2 owns public prepared API/private pass-context contraction and lookup
threading. E3 may cite E2 private-reader evidence as proof harness context but
does not privatize public APIs.

E4 owns cross-target wrapper prerequisites and wrapper migration. E3 keeps
wrapper behavior byte-stable.

Route 8 remains separate for return-chain owner/schema work. No current E3
candidate opens Route 8 or treats return-chain facts as part of Route 1
through Route 7 diagnostic readiness.

E5 and draft 155 remain unopened. E3 does not open broad
`PreparedBirModule`, `PreparedFunctionLookups`, aggregate prepared-retirement,
or prepared rewrite work.

No action is taken on broad prepared printer/debug replacement, broad CLI dump
replacement, broad route-debug replacement, broad helper-oracle replacement,
baseline refreshes, expected-string rewrites, helper renames, unsupported
downgrades, timeout masking, aggregate route-view migration, or target-policy
movement.

## Proof Requirements For Later Implementation Ideas

Every accepted E3 follow-up idea must require fresh proof for:

- the positive route/BIR-native row explanation;
- absent evidence and prepared fallback;
- invalid references or ids;
- duplicate, conflict, or ambiguous evidence;
- mismatch and non-agreement;
- unsupported or prepared-only cases where applicable;
- wrapper behavior;
- printer/debug or route-debug behavior;
- helper-oracle names, status labels, strings, and assertion strength;
- expected-string and baseline stability;
- nearby same-feature coverage showing the change is not testcase-shaped.

For target-sensitive rows, the proof must also show no movement of ABI,
layout, address, move, branch, call, wrapper, final assembler, or emitted
output policy into route/BIR diagnostic ownership.

## Unopened Scope

Draft 155, E5, broad prepared diagnostic/oracle replacement, broad prepared
printer/debug replacement, broad helper-oracle retirement, broad route-debug
replacement, aggregate `PreparedBirModule` or `PreparedFunctionLookups`
retirement, and baseline/expected-string authority changes remain unopened by
Phase E3.
