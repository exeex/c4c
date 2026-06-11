# 203 Phase D2 residual consumer switch plan

## Goal

Plan MIR/codegen, diagnostic, and cross-target consumers that should switch to
Phase B2 residual BIR schemas or retained target-local owners, while preserving
prepared fallback/oracle behavior until equivalence is proven.

This is Phase D2 of the BIR/prealloc thinning program. It is analysis-only and
must produce follow-up implementation ideas for residual consumer migration.

## Prerequisite

Do not activate this draft until Phase C2 has closed and produced:

- `docs/bir_prealloc_fusion/phase_c2_residual_cache_contraction.md`

If Phase C2 leaves all residual surfaces retained as target policy or
diagnostics, this draft should be rewritten around retained-owner adapters
rather than route-view migration.

## Shared Artifact Contract

This phase must read:

- Phase A2/B2/C2 durable artifacts;
- implementation closure notes for Phase B2 and C2 follow-ups;
- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`;
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`;
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`;
- current accepted baseline state and any baseline recovery closure notes.

This phase must write its durable analysis payload to:

- `docs/bir_prealloc_fusion/phase_d2_residual_consumer_switch_plan.md`

## Direction

Switch consumers one semantic or diagnostic boundary at a time. If a residual
surface is target policy or pass context, the plan should define retained-owner
adapters rather than forcing a BIR route-view migration.

## In Scope

- Residual MIR/codegen consumers of Phase B2 accepted route facts.
- Consumers of retained target-policy/pass-context surfaces that need clearer
  adapters before retirement analysis.
- Prepared printer/debug/dump/oracle consumers needing route-native or
  retained-owner replacement.
- x86/riscv wrapper consumers where an AArch64-proven route view or retained
  target-local adapter exists.
- Ordered migration ladder and follow-up ideas.

## Out Of Scope

- Direct implementation.
- Prepared API deletion or `PreparedBirModule` retirement.
- Broad aggregate replacement.
- Cross-target adapters without prior AArch64 or retained-owner proof.
- Weakening tests, diagnostics, baselines, or string-authority rules.

## Expected Output

The closure note must contain:

- a link to `docs/bir_prealloc_fusion/phase_d2_residual_consumer_switch_plan.md`;
- consumer dependency map for residual route/retained-owner surfaces;
- ordered migration ladder;
- adapter and fallback boundaries;
- follow-up implementation ideas for each safe consumer migration;
- explicit statement whether draft 155 is ready to open after D2, and what
  blockers remain.

## Reviewer Reject Signals

- Treating residual ownership classification as consumer migration proof.
- Migrating consumers before Phase B2 facts or retained-owner adapters exist.
- Reintroducing broad BIR scans, name matching, or testcase-shaped shortcuts.
- Moving target policy into BIR route views.
- Treating diagnostic/oracle replacement as optional.
- Opening or executing draft 155 before D2 states that its prerequisites are
  satisfied.
