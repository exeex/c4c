Status: Active
Source Idea Path: ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify Each Candidate Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Classify Each Candidate Boundary", by replacing
the provisional E4 candidate states in
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
with exact allowed source-idea classifications.

The E4 document now classifies:

- x86 Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility as
  `ready to draft one wrapper convergence implementation idea`;
- x86 compare-join stack-home handoff as `retained target-local policy`;
- Route 6 consumed scalar `i32` call-argument source facts and prepared
  selected-value-chain metadata as `retained prepared fallback or
  compatibility adapter`;
- x86 Route 5/Route 7-adjacent joined-branch and handoff rows as
  `needs another named AArch64 or route semantic proof first`;
- riscv prepared edge-publication emission and wrapper output as
  `blocked for riscv because no matching AArch64-proven semantic boundary
  exists`;
- other wrapper-adjacent E0/E1/E2/E3 prerequisites as `no-action`.

Each row records the consumed proven fact or explicit blocker, retained
target-local authority, prepared fallback or compatibility adapter status,
diagnostic/oracle and expected-string authority, baseline/proof authority, and
short reject-signal reasoning.

Correction before supervisor acceptance: refreshed the E4 retained-baseline
context from current `test_baseline.log`. Step 3 now uses the accepted
full-suite baseline `3428/3428` at commit
`8cebab4beba219e6a8cdef998bc970c8658ce28b` (`Draft E4 wrapper readiness
skeleton`) instead of the stale pre-review baseline commit.

## Suggested Next

Execute Step 4 from `plan.md`: prepare the accepted follow-up source idea for
the single ready x86 Route 6 scalar `i32` argument-source route-debug row and
`ConsumedPlans` compatibility boundary.

## Watchouts

- Keep x86 repair evidence separate from AArch64/route-native facts; x86
  `ConsumedPlans` and handoff repairs do not imply riscv readiness or
  route-wide x86 migration.
- Step 4 should open only the one ready x86 Route 6 route-debug /
  `ConsumedPlans` compatibility idea. The x86 compare-join handoff, Route 6
  source facts, and selected-value-chain metadata are retained authority or
  adapter evidence, not standalone wrapper convergence ideas.
- riscv remains blocked unless a matching AArch64-proven semantic boundary
  plus riscv formatting/emission/output proof is named.
- Do not weaken expected strings, supported-path status, helper names, wrapper
  output, helper-oracle behavior, or baselines as proof.
- Broad route-wide wrapper migration, aggregate `PreparedFunctionLookups` or
  `PreparedBirModule` contraction, draft 155, and E5 prepared aggregate
  retirement remain deferred and out of scope.

## Proof

Analysis/docs-only packet. Delegated proof:

- inspected current `test_baseline.log`
- `git diff -- docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md todo.md`
- `git diff --check -- docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md todo.md`
- `git status --short`

No build or CTest required because only the E4 markdown document and
`todo.md` were edited. No new `test_after.log` was written for this
docs-only packet.
