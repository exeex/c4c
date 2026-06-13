# Phase E4 Cross-Target Wrapper Convergence Readiness

Linked source idea:
`ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md`.

This document is the durable Phase E4 analysis surface for deciding which
exact x86 or riscv wrapper, route-debug, handoff, or wrapper-adjacent
compatibility boundary can safely consume an already-proven BIR, AArch64, or
route-native semantic fact. E4 is analysis-only until it opens a separate
one-boundary implementation idea. It does not migrate wrappers, rewrite
expected strings, refresh baselines as proof, retire prepared aggregate
surfaces, or claim broad cross-target wrapper convergence.

Step 3 assigns each candidate exactly one allowed readiness state from the
source idea. The table separates x86 readiness from riscv readiness and keeps
route-wide, aggregate, draft 155, and E5 conclusions out of scope.

## Candidate Readiness Table

| Candidate boundary | E4 classification | Consumed proven fact or explicit blocker | Retained target-local authority | Prepared fallback or compatibility adapter status | Diagnostics, expected strings, and baseline authority | Short reject-signal reasoning |
| --- | --- | --- | --- | --- | --- | --- |
| x86 Route 6 scalar `i32` argument-source route-debug row and `ConsumedPlans` compatibility | ready to draft one wrapper convergence implementation idea | Consumes the closed idea `232` fact: one x86 Route 6 scalar `i32` argument-source route-debug row can use Route 6 scalar source agreement while preserving `ConsumedPlans` compatibility. This is x86-only readiness. | ABI placement, call wrapper policy, route-debug row formatting, helper/carrier protocols, direct-call behavior, and wrapper output stay x86/prepared-owned. | `ConsumedPlans`, prepared call plans, public prepared call/debug fallback, and direct-call/helper-oracle families remain compatibility adapters for absent, invalid, duplicate/conflict, mismatch, non-agreement, and policy-sensitive paths. | Route-debug output, helper-oracle names/status labels, supported-path contracts, expected strings, and baseline behavior must be byte-stable. Follow-up proof must cover positive agreement plus absent, invalid, duplicate/conflict, mismatch, compatibility fallback, wrapper output, route-debug, helper-oracle, expected-string, and baseline-stability cases. | Reject if the follow-up treats the row as broad x86 call-wrapper readiness, weakens `ConsumedPlans`, rewrites expected strings, moves ABI/call policy into Route 6, or uses the x86 row as riscv readiness. |
| x86 compare-join stack-home handoff | retained target-local policy | Explicit blocker for E4 readiness: closed idea `234` repaired an x86 prepared-module handoff by following authoritative prepared stack-home state through compare-join entry and return. It consumes no shared BIR/AArch64 route fact for wrapper convergence. | Stack homes, value-home/storage policy, compare-join entry/return handoff behavior, move/parallel-copy policy, and x86 output stay target/prepared-owned. | Prepared stack-home and lookup-helper behavior remain the authority and fallback; there is no wrapper convergence adapter to draft from this row alone. | `backend_x86_handoff_boundary`, `backend_x86_route_debug`, and `backend_prepared_lookup_helper` are guardrail proofs only. Expected strings, helper behavior, supported-path status, and baseline logs are not ownership evidence. | Reject if this local repair is recast as shared BIR ownership, riscv readiness, wrapper migration, or permission to weaken the stack-backed parameter-home assertion. |
| Route 6 consumed scalar `i32` call-argument source facts | retained prepared fallback or compatibility adapter | Consumes the closed idea `235` fact: named scalar `i32` call-argument source publication can be recovered from prepared call-plan authority through `ConsumedPlans` with fail-closed behavior. The fact supports the x86 route-debug candidate above but is not itself a wrapper boundary. | ABI layout, outgoing stack/register placement, helper/carrier protocols, call records, direct-call behavior, and call-wrapper policy stay target/prepared-owned. | Prepared call plans and `ConsumedPlans` remain retained compatibility adapters for absent facts, mismatched prepared ids, missing names, non-Route-6 cases, non-`i32` arguments, and policy-sensitive paths. | AArch64/x86 output, route-debug rows, prepared call/debug output, helper-oracle behavior, expected strings, and current baseline behavior remain unchanged. Follow-up proof belongs to a named wrapper/debug boundary, not to wholesale call-plan ownership. | Reject if the source fact is used to delete call plans, migrate broad call wrappers, guess missing facts, weaken fail-closed behavior, or claim riscv support. |
| Prepared selected-value-chain metadata | retained prepared fallback or compatibility adapter | Explicit blocker for E4 readiness: closed idea `236` preserved true global-root selected-value-chain metadata for pointer-backed same-module global return contexts. It names prepared metadata correctness, not a route/BIR wrapper-consumable fact. | Return-context ownership, pointer/global-root interpretation, selected-value metadata publication, and any handoff behavior using those facts stay prepared-owned. | Prepared selected-value metadata remains fallback/compatibility authority until a later boundary names an exact route/BIR fact that consumes it safely. | Helper-oracle behavior, handoff assertions, expected strings, and the 15-test close-scope baseline are guardrails only. No E4 follow-up can use this row as proof of route-native wrapper ownership. | Reject if this metadata fix is treated as wrapper convergence, broad route ownership, baseline proof, or a reason to move return-context or pointer/global policy into BIR. |
| x86 joined-branch and handoff rows adjacent to Route 5 edge/join and Route 7 comparison evidence | needs another named AArch64 or route semantic proof first | Current evidence is adjacent, not exact. Route 5 edge/join-source identity and Route 7 comparison provenance exist under agreement; E3 closed Route 5 helper-oracle (`231`) and Route 7 materialized-condition helper-oracle (`233`) rows. No exact x86 joined-branch or handoff wrapper boundary has yet proven it can consume one of those facts while preserving target output policy. | Branch spelling, comparison suffix selection, fused legality, edge-copy placement, parallel-copy mechanics, move scheduling, wrapper formatting/output, and handoff output stay target/prepared-owned. | Prepared edge-publication, join-source, comparison, move-bundle, and helper-oracle fallbacks remain authoritative for absent, invalid, duplicate/conflict, mismatch, memory-source, unsupported, and non-agreement paths. | E3 oracle rows can be cited only as diagnostic context. x86 wrapper, branch-control, printer/debug rows, helper names/status labels, expected strings, supported-path contracts, and baselines remain byte-stable. Required proof must first name one exact x86 row plus the exact Route 5 or Route 7 fact it consumes. | Reject if a follow-up imports Route 5/7 evidence by proximity, moves branch or move policy into BIR, rewrites wrapper output, or proves only a known row while nearby same-feature rows remain unexamined. |
| riscv prepared edge-publication emission and wrapper output | blocked for riscv because no matching AArch64-proven semantic boundary exists | Explicit riscv blocker: E0, residual-route, E2, and lookup maps currently provide only retained prepared/target-owned no-change evidence for riscv edge-publication emission and wrapper output. Route 4/5 publication identity may be relevant semantic context, but no matching AArch64-proven wrapper boundary plus riscv formatting/emission/output proof exists. | riscv formatting, emission order, branch/copy spelling, wrapper output, prepared edge-publication lookup threading, instruction selection, and target emission policy stay riscv/prepared-owned. | Prepared edge-publication lookups and riscv prepared-module emission remain retained compatibility/fallback surfaces; no riscv-only BIR adapter is ready. | riscv expected strings, wrapper output, baseline behavior, and any publication helper-oracle rows remain authoritative. Any future riscv idea must first reuse a shared AArch64/route semantic boundary and then prove riscv output no-change. | Reject if the path invents a riscv-only route fact, treats x86 repairs as riscv readiness, uses no-change baselines as ownership proof, or weakens wrapper/expected output. |
| Other wrapper-adjacent E0/E1/E2/E3 prerequisites | no-action | Context only. E1 proved selected AArch64 BIR branch-target identity and Route 7 fused-compare provenance helpers; E2 moved selected helper families toward private pass context; E3 accepted row-scoped diagnostic/oracle candidates and retained wrapper authority. None of these names an additional exact E4 wrapper/debug/handoff boundary. | Aggregate lookup construction, public prepared compatibility, wrappers, diagnostics/oracles, expected strings, baselines, target-local ABI/layout/formatting/emission policy, and route facades remain outside E4 implementation authority. | Public prepared compatibility, `PreparedFunctionLookups`, `PreparedBirModule`, diagnostics/oracles, and prepared fallback remain retained until a separate source idea names one exact boundary. | Baselines and expected strings stay guardrails. E1/E2/E3 artifacts are evidence inputs only and do not open route-wide wrapper migration. | Reject if this context is used to open aggregate API contraction, draft 155, E5, broad diagnostic/oracle retirement, or broad cross-target wrapper migration. |

## Retained Target-Local Policy

E4 must keep these surfaces target-local unless a future source idea explicitly
proves a narrower ownership change:

- ABI, frame layout, register allocation, storage/value-home policy, stack
  homes, outgoing stack layout, clobbers, and helper/carrier protocols.
- Call wrapper policy, branch spelling, comparison suffix selection,
  fused-compare legality, instruction selection, final instruction records,
  formatting, emission order, and wrapper output.
- Move scheduling, out-of-SSA mechanics, parallel-copy placement, edge-copy
  execution site, scratch routing, and cycle-temporary policy.

Any implementation follow-up must consume one proven semantic fact while
preserving these target-local policies as output policy, not route/BIR
ownership.

## Retained Prepared Fallback

Prepared fallback remains required for absent, invalid, duplicate/conflict,
ambiguous, mismatched, unsupported, policy-sensitive, prepared-only, and
non-agreement cases. E4 may only propose a follow-up when the row can fail
closed back to the existing prepared or compatibility surface without changing
observable behavior.

Prepared `ConsumedPlans`, prepared call plans, prepared edge-publication
lookups, selected-value metadata, `PreparedFunctionLookups`, and
`PreparedBirModule` remain compatibility and fallback surfaces in this phase.

## Retained Diagnostics And Oracles

Diagnostic, printer/debug, route-debug, helper-oracle, and helper-status rows
must stay byte-stable unless a later implementation idea proves a single row
with explicit oracle parity. E3 owns diagnostic/oracle replacement decisions;
E4 may cite E3 evidence only when the wrapper or handoff candidate consumes the
same proven semantic boundary and keeps the diagnostic/oracle behavior intact.

## Retained Expected Strings

Expected strings, helper names, status labels, supported-path contracts, and
wrapper output are not proof artifacts to be weakened. E4 rejects any candidate
whose only route to readiness is an expectation rewrite, helper rename,
unsupported downgrade, timeout masking, wrapper-output relabeling, or
named-case-only shortcut.

## Retained Baseline Surfaces

The accepted baseline and current green logs are guardrails, not ownership
proof. The current baseline context is:

- `test_baseline.log` records the accepted full-suite baseline `3428/3428`
  passing at commit `8cebab4beba219e6a8cdef998bc970c8658ce28b`
  (`Draft E4 wrapper readiness skeleton`).
- `test_after.log` records the later 15-test prepared/prealloc/x86 close scope
  passing, including `backend_x86_handoff_boundary`,
  `backend_x86_route_debug`, and `backend_prepared_lookup_helper`.

Any follow-up implementation idea must name its own proof matrix and preserve
baseline behavior without using a baseline refresh as convergence evidence.

## Phase And Route Deferrals

### E1

E1 semantic duplicate triage is already complete for selected AArch64 BIR
branch-target identity and Route 7 fused-compare provenance helpers. E4 may
consume those facts only where the selected wrapper/debug/handoff row names
the exact boundary. E4 does not reopen E1 triage or broaden those semantic
facts into route-wide wrapper ownership.

### E2

E2 public/private prepared lookup API contraction is out of E4 scope. E4 does
not move aggregate `PreparedFunctionLookups`, `PreparedBirModule`, or public
prepared compatibility into private route ownership.

### E3

Prepared diagnostic/oracle replacement remains E3-owned. E4 can cite E3
row-scoped closures for Route 5 and Route 7 only as diagnostic/oracle context,
and only when wrapper output, helper-oracle behavior, expected strings, and
prepared fallback stay retained.

### Route 8

Route 8 return-chain work remains a citation or future visible diagnostic-row
matter. E4 does not use Route 8 as proof for wrapper migration, Route 1/7
ownership, return ABI/home/register policy, or broad lookup contraction.

### E5

E5 prepared aggregate retirement is deferred. E4 readiness for one boundary
does not retire prepared aggregates, prepared fallback, or compatibility
adapters.

### Draft 155

Draft 155 is not opened or advanced by this E4 skeleton. Any later draft-155
or aggregate-retirement work needs its own source idea and proof route.

### Broad Wrapper Migration

Broad x86, riscv, cross-target, call-wrapper, edge-wrapper, branch-wrapper, or
wrapper-family migration is rejected for this activation. E4 may only prepare
one-boundary follow-up ideas after Step 3 gives that exact boundary an allowed
ready classification.

## Step 3 Classification Closure

Step 3 classified every candidate row above with exactly one allowed source
idea state and recorded:

- the consumed proven BIR, AArch64, or route-native fact;
- retained target-local authority;
- retained prepared fallback or compatibility adapter status;
- diagnostic/oracle and expected-string status;
- baseline authority and required proof matrix;
- explicit x86 versus riscv readiness reasoning.

The only Step 4-ready candidate is the x86 Route 6 scalar `i32`
argument-source route-debug row and `ConsumedPlans` compatibility boundary.
Its follow-up must be scoped to that one x86 route-debug/debug-compatibility
boundary and must not claim broad x86 call-wrapper migration.

No riscv candidate is ready. riscv edge-publication emission and wrapper
output are blocked until a matching AArch64-proven semantic boundary and
riscv formatting/emission/output no-change proof exist.

Broad route-wide wrapper migration, aggregate `PreparedFunctionLookups` or
`PreparedBirModule` contraction, draft 155, and E5 prepared aggregate
retirement remain deferred and out of scope for this E4 activation.

## Closure-Ready Summary

The closure payload for E4 is this document:
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`.
It links back to the active source idea
`ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md` and
uses the candidate readiness table above as the authoritative Step 3 status
record.

The candidate table is closed for this analysis pass. It records exactly one
ready row: the x86 Route 6 scalar `i32` argument-source route-debug row and
`ConsumedPlans` compatibility boundary. Step 4 opened exactly one matching
follow-up idea:
`ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`.
No follow-up idea was opened for retained target-local policy rows, retained
prepared fallback or compatibility-adapter rows, blocked riscv rows, adjacent
proof rows, or no-action prerequisite context.

Retained-authority decisions remain unchanged at closure. Target ABI, frame,
register, value-home, formatting, emission, instruction-selection,
wrapper-output, branch/call policy, move scheduling, and helper/carrier
protocols stay target-local. Prepared `ConsumedPlans`, prepared call plans,
prepared edge-publication lookups, selected-value metadata,
`PreparedFunctionLookups`, `PreparedBirModule`, public prepared debug/call
fallbacks, diagnostics/oracles, helper names/status labels, supported-path
contracts, expected strings, and baseline behavior remain compatibility or
guardrail surfaces unless a later source idea proves a narrower boundary.

All non-ready rows are deferred or retained, not silently converted into
implementation work. riscv remains blocked until a matching AArch64-proven
semantic boundary and riscv formatting/emission/output no-change proof are
named. x86 joined-branch and handoff rows adjacent to Route 5 or Route 7 need
another exact named semantic proof first. E1, E2, E3, Route 8, aggregate lookup
contraction, and prepared aggregate retirement remain outside this E4
implementation authority.

Any implementation follow-up must carry its own proof matrix. The accepted x86
Route 6 follow-up must prove positive agreement plus absent, invalid,
duplicate/conflict, mismatch, compatibility fallback, wrapper output,
route-debug, helper-oracle, expected-string, and baseline-stability behavior
without weakening expected strings, supported-path status, helper names,
wrapper output, prepared fallback, or baseline authority.

Draft 155, E5 prepared aggregate retirement, and broad x86, riscv,
cross-target, call-wrapper, edge-wrapper, branch-wrapper, or wrapper-family
migration remain unopened by this E4 analysis payload.
