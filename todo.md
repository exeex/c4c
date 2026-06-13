Status: Active
Source Idea Path: ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Draft the E4 Readiness Document Skeleton

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Draft the E4 Readiness Document Skeleton", by
creating
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
from the Step 1 evidence map.

The new E4 document now contains:

- a framing section linked to source idea
  `ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md`;
- a candidate-by-candidate readiness table seeded from Step 1 evidence, with
  provisional/non-final classifications for x86 Route 6 route-debug,
  x86 compare-join handoff, Route 6 consumed scalar call-argument source
  facts, prepared selected-value-chain metadata, x86 Route 5/Route 7-adjacent
  joined-branch/handoff rows, riscv edge-publication/wrapper output, and other
  wrapper-adjacent E0/E1/E2/E3 prerequisites;
- retained-authority sections for target-local policy, prepared fallback,
  diagnostics/oracles, expected strings, and baseline surfaces;
- deferral sections for E1, E2, E3, Route 8, E5, draft 155, and broad wrapper
  migration;
- a Step 3 worklist that keeps implementation follow-up details provisional
  until each candidate receives exactly one allowed readiness classification.

## Suggested Next

Execute Step 3 from `plan.md`: classify each E4 candidate boundary in
`docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`
with exactly one allowed readiness classification, evidence, retained
authority, and reject-signal reasoning.

## Watchouts

- Classifications in the E4 document are explicitly provisional; Step 3 must
  assign exactly one allowed classification per candidate before Step 4 can
  open any implementation follow-up idea.
- Keep x86 repair evidence separate from AArch64/route-native facts; x86
  `ConsumedPlans` and handoff repairs do not imply riscv readiness or
  route-wide x86 migration.
- riscv remains blocked unless a matching AArch64-proven semantic boundary plus
  riscv formatting/emission/output proof is named.
- Do not weaken expected strings, supported-path status, helper names, wrapper
  output, helper-oracle behavior, or baselines as proof.

## Proof

Analysis/docs-only packet. Delegated proof:

- `git diff -- docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md todo.md`
- `git status --short`

No build or CTest required because only the E4 markdown document and
`todo.md` were edited. No new `test_after.log` was written for this
docs-only packet.
