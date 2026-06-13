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

The classifications below are provisional until Step 3 assigns each candidate
exactly one allowed readiness state.

## Candidate Readiness Table

| Candidate boundary | Provisional E4 classification | Evidence currently available | Retained authority before any follow-up |
| --- | --- | --- | --- |
| x86 Route 6 scalar `i32` argument-source route-debug row and `ConsumedPlans` compatibility | Provisional candidate for one x86 wrapper/debug boundary; not broad x86 call-wrapper readiness. | E0/E3 and closed idea `232` show one x86 route-debug row can consume Route 6 scalar source agreement while retaining `ConsumedPlans`, x86 wrapper behavior, ABI/call wrapper policy, prepared call printer/debug, helper-oracle families, public fallback, and expected strings. | ABI placement, call wrapper policy, prepared call-plan fallback, `ConsumedPlans` compatibility, route-debug output, helper-oracle names/status labels, expected strings, and supported-path contracts remain authoritative. |
| x86 compare-join stack-home handoff | Provisional x86-local handoff repair evidence; not shared BIR/route or riscv readiness. | Closed idea `234` repaired the x86 compare-join stack-backed parameter-home handoff by following authoritative prepared stack-home state through compare-join entry and return, with proof guarded by `backend_x86_route_debug` and `backend_prepared_lookup_helper`. | Stack homes, value-home/storage policy, compare-join handoff output, move/parallel-copy policy, prepared lookup helper behavior, and fallback behavior remain target/prepared-owned. |
| Route 6 consumed scalar `i32` call-argument source facts | Provisional candidate for one consumed-call-source compatibility boundary; not full call-plan or ABI ownership. | Closed idea `235` repaired named scalar `i32` call-argument source publication from prepared call-plan authority through `ConsumedPlans`, with fail-closed behavior for absent, mismatched, missing-name, non-Route-6, and non-`i32` cases. | Prepared call plans, ABI layout, helper/carrier protocols, outgoing stack and register policy, `ConsumedPlans`, fallback, and unchanged AArch64/x86 output remain retained. |
| Prepared selected-value-chain metadata | Provisional prepared metadata repair evidence; not route-native wrapper convergence proof. | Closed idea `236` preserved the true global-root selected-value chain for pointer-backed same-module global return contexts, unblocking handoff validation. | Prepared selected-value metadata, return-context ownership, pointer/global-root interpretation, helper-oracle behavior, fallback, and expected strings remain retained until a later boundary names a route/BIR fact it consumes. |
| x86 joined-branch and handoff rows adjacent to Route 5 edge/join and Route 7 comparison evidence | Needs exact one-boundary classification before any follow-up. | E0/residual-route evidence says Route 5 edge/join-source and Route 7 comparison facts are agreement-gated semantic evidence. E3 closed row-scoped Route 5 helper-oracle (`231`) and Route 7 materialized-condition helper-oracle (`233`) follow-ups while retaining branch policy, move/parallel-copy policy, wrappers, printer/debug rows, expected strings, and prepared fallback. | Branch spelling, comparison suffix/fused legality, edge-copy placement, parallel-copy mechanics, move scheduling, wrapper formatting/output, printer/debug rows, expected strings, and prepared fallback remain retained. |
| riscv prepared edge-publication emission and wrapper output | Provisional blocked for riscv unless Step 3 can name a matching AArch64-proven semantic boundary plus riscv formatting/emission/output proof. | E0, residual-route, E2, and prepared lookup maps describe riscv edge-publication/wrapper output as retained prepared/target-owned no-change evidence only. Route 4/5 publication identity may be semantic context, but there is no imported riscv route-view migration proof. | riscv formatting, emission order, branch/copy spelling, wrapper output, prepared edge-publication lookup authority, expected strings, and baseline behavior remain target/prepared-owned. |
| Other wrapper-adjacent E0/E1/E2/E3 prerequisites | Context only until a row names the exact consumed boundary. | E1 proved selected AArch64 BIR branch-target identity and Route 7 fused-compare provenance helpers. E2 moved only those selected helper families toward private pass context. E3 accepted row-scoped diagnostic/oracle candidates and retained wrapper authority. | Aggregate lookup construction, public prepared compatibility, wrappers, diagnostics/oracles, expected strings, baselines, and target-local policy remain outside E4 implementation authority. |

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

- `test_baseline.log` records full-suite baseline `3428/3428` passing at
  commit `47557552bc77c8c06dca01b9a81e2b696e21f90b`.
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

## Step 3 Classification Worklist

Step 3 must assign exactly one allowed classification to every candidate row
above and include:

- the consumed proven BIR, AArch64, or route-native fact;
- retained target-local authority;
- retained prepared fallback or compatibility adapter status;
- diagnostic/oracle and expected-string status;
- baseline authority and required proof matrix;
- explicit x86 versus riscv readiness reasoning.

Step 4 follow-up ideas remain provisional until Step 3 completes this
classification table.
