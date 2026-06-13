Status: Active
Source Idea Path: ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build the E4 Evidence Map

# Current Packet

## Just Finished

Completed `plan.md` Step 1, "Build the E4 Evidence Map", by reading the
required E0/E1/E2/E3 inputs, E1/E2 helper closures, E3 follow-up closures
`227` through `236`, the residual route-view and prepared ownership maps,
canonical baseline/test logs, and `scripts/plan_review_state.py show`.

Required-input availability:

- Present: every required prior-phase idea/doc listed in `plan.md`, including
  closed ideas `217`, `218`, `219`, `220`, `223`, `224`, `225`, `226`, and
  `227` through `236`, plus the residual route-view, prepared diagnostics, and
  prepared lookup ownership maps.
- Expected absent at Step 1: the E4 durable output
  `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`;
  that is the Step 2 target, not a Step 1 blocker.
- Baseline/log context: `test_baseline.log` records full-suite baseline
  `3428/3428` passing at commit `47557552bc77c8c06dca01b9a81e2b696e21f90b`.
  `test_after.log` records the later 15-test prepared/prealloc/x86 close scope
  passing, including `backend_x86_handoff_boundary`,
  `backend_x86_route_debug`, and `backend_prepared_lookup_helper`.
- Hook state: `scripts/plan_review_state.py show` reports no pending code or
  baseline review reminder. Its stored step pointer still names an older
  Step 4 (`Validate And Prepare Acceptance Notes`) while canonical `todo.md`
  correctly names Step 1; treat the hook pointer as stale local state, not E4
  evidence.

Evidence map by candidate family:

| Candidate family | Available evidence | E4 implication |
| --- | --- | --- |
| x86 Route 6 scalar `i32` argument-source route-debug row and `ConsumedPlans` compatibility | E0/E3 and closed idea `232` prove one x86 route-debug row can use Route 6 scalar source agreement while retaining `ConsumedPlans`, x86 wrapper behavior, ABI/call wrapper policy, prepared call printer/debug, helper-oracle families, public fallback, and expected strings. | Candidate for a one-boundary x86 wrapper/debug classification, but not broad x86 call-wrapper readiness. |
| x86 compare-join stack-home handoff | Closed idea `234` repaired the x86 compare-join stack-backed parameter-home handoff by following authoritative prepared stack-home state through compare-join entry and return. The proof stayed guarded by `backend_x86_route_debug` and `backend_prepared_lookup_helper`. | x86-local handoff repair evidence. It does not by itself prove shared BIR/route ownership or riscv readiness. |
| Route 6 consumed scalar `i32` call-argument source facts | Closed idea `235` repaired named scalar `i32` call-argument source publication from prepared call-plan authority through `ConsumedPlans`, with fail-closed behavior for absent/mismatched/missing-name/non-Route-6/non-i32 cases. | x86/Route 6 compatibility evidence for one consumed-call-source boundary; ABI/call policy and prepared call-plan authority remain retained. |
| Prepared selected-value-chain metadata | Closed idea `236` repaired prepared compare-join selected-value-chain metadata for pointer-backed same-module global return contexts by preserving the true global-root selected-value chain. | Prepared metadata repair evidence that unblocks handoff validation; it is not route-native wrapper convergence proof. |
| x86 joined-branch and handoff rows adjacent to Route 5/Route 7 | E0/residual map says Route 5 edge/join-source and Route 7 comparison facts are agreement-gated semantic evidence; E3 closed row-scoped Route 5 helper-oracle (`231`) and Route 7 materialized-condition helper-oracle (`233`) follow-ups while retaining branch policy, move/parallel-copy policy, wrappers, printer/debug rows, expected strings, and prepared fallback. | Needs exact one-boundary classification. Adjacent x86 wrapper rows cannot inherit AArch64/route evidence unless the wrapper input consumes the same proven semantic boundary and preserves target-local output policy. |
| riscv prepared edge-publication emission and wrapper output | E0, residual-route, E2, and prepared lookup maps name riscv edge-publication/wrapper output as retained prepared/target-owned no-change evidence only. Route 4/5 publication identity can be a semantic candidate, but riscv has no imported route-view migration proof. | Blocked for riscv unless an AArch64-proven semantic route boundary plus riscv-specific formatting/emission/output proof is named. |
| Other wrapper-adjacent E0/E1/E2/E3 prerequisites | E1 proved only selected AArch64 BIR branch-target identity and Route 7 fused-compare provenance helpers. E2 moved only those two selected helper families toward private pass context. E3 accepted row-scoped diagnostic/oracle candidates but explicitly retained wrapper authority and delegated cross-target wrapper prerequisites to E4. | Use these as positive semantic or diagnostic context only where the E4 row names the exact consumed boundary; they do not authorize aggregate lookup, wrapper-family, draft 155, or E5 work. |

AArch64/route-native facts available to E4:

- BIR structured successor/branch-target identity for the selected
  `find_prepared_control_flow_branch_target_labels(...)` helper under
  prepared agreement.
- Route 7 fused-compare operand-producer comparison provenance for the
  selected AArch64 comparison-lowering boundary under prepared agreement.
- Route 3 memory/source identity, Route 4 publication attribution, Route 5
  current-block join-source metadata, Route 6 scalar call-use source identity,
  and Route 7 materialized-condition/comparison evidence are available only as
  selected semantic or row-scoped facts with prepared fallback retained.

x86-only or repair-local evidence:

- `ConsumedPlans` compatibility for one x86 Route 6 scalar `i32`
  argument-source route-debug row.
- x86 compare-join stack-home handoff repair.
- Route 6 consumed scalar `i32` call-argument source publication through
  prepared call-plan authority for named scalar arguments.
- Prepared selected-value-chain metadata repair for pointer-backed
  same-module global return-context ownership.

Target-local authority surfaces that must remain retained:

- ABI, frame layout, register allocation, value-home/storage policy, stack
  homes, move scheduling, out-of-SSA/parallel-copy mechanics, call wrapper
  policy, helper/carrier protocols, branch spelling, comparison suffix/fused
  legality, instruction selection, final records, formatting, emission order,
  and wrapper output.
- Prepared fallback/oracle behavior for absent, invalid, duplicate/conflict,
  ambiguous, mismatch, unsupported, policy-sensitive, prepared-only, and
  non-agreement cases.
- `PreparedFunctionLookups`, `PreparedBirModule`, aggregate lookup
  construction, public prepared helper compatibility, `ConsumedPlans`, route
  debug output, prepared printer/debug rows, helper-oracle names/status
  labels, expected strings, supported-path contracts, baselines, and
  c_testsuite behavior.

Current blockers or ambiguities:

- No required prior-phase input file is missing.
- No ambiguous ownership authority blocks Step 1, but riscv readiness is
  blocked by lack of matching AArch64-proven semantic boundary plus riscv
  formatting/emission proof.
- The only local-state ambiguity is the stale hook-state step pointer noted
  above; canonical `todo.md` remains authoritative for the active Step 1
  packet.

## Suggested Next

Execute Step 2 from `plan.md`: create or refresh
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
with the E4 framing, retained-authority sections, and a candidate table seeded
from the Step 1 evidence map. Keep classifications provisional until Step 3.

## Watchouts

- Keep x86 repair evidence separate from AArch64/route-native facts; x86
  `ConsumedPlans` and handoff repairs do not imply riscv readiness or
  route-wide x86 migration.
- Treat baseline and green test logs as guardrails only, not replacement or
  wrapper-ownership proof.
- Do not move target-local ABI, frame, register, move, call, branch,
  instruction-selection, formatting, or emission policy into BIR/route
  authority.
- Do not weaken expected strings, supported-path status, helper names, wrapper
  output, helper-oracle behavior, or baselines as proof.

## Proof

Analysis-only packet. Inspected the required markdown inputs, baseline/test
logs, and `scripts/plan_review_state.py show`; no build or CTest required
because only `todo.md` was edited.
