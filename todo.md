Status: Active
Source Idea Path: ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Synthesize E1-E5 Readiness

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: added the durable
`PreparedFunctionLookups` group-by-group ownership classification to
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`.

The map classifies all seven lookup groups, separates route-fact duplicate
candidates from retained target/prepared policy, public fallback/oracle,
diagnostic/string authority, and cross-target compatibility, and names
realistic E2 public prepared API/private pass-context contraction candidates
without claiming aggregate deletion.

## Suggested Next

Execute Step 4: synthesize the `PreparedBirModule` and
`PreparedFunctionLookups` maps into E1-E5 readiness recommendations in
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`.

## Watchouts

- Draft 155 has been read as evidence only; do not activate, rewrite, or
  execute it as the active plan.
- Phase D and D2 evidence supports selected route readers and row-scoped
  diagnostics only; it does not support prepared API deletion, route-wide
  migration, aggregate `PreparedFunctionLookups` contraction, or
  `PreparedBirModule` retirement.
- Keep target ABI/layout/register/stack/addressing/move/emission policy out of
  target-neutral BIR. Step 2 explicitly leaves target-policy fields retained
  even when adjacent source identity subfacts are BIR-owned candidates.
- Preserve diagnostics, printer/debug, route-debug, helper-oracle, wrapper,
  baseline, and expected-string authority as retained surfaces or blockers.
- AArch64 has selected-reader proof; x86 has only limited Route 6
  `ConsumedPlans`/route-debug reuse; riscv has no imported route-view
  migration proof.
- Step 3 keeps `PreparedFunctionLookups` aggregate construction and delivery as
  compatibility/pass-context, not a deletion-ready owner boundary. E2
  candidates must name one lookup group plus one consumer or diagnostic row.

## Proof

`printf 'docs-only Step 3 PreparedFunctionLookups classification; no executable proof applies\n' > test_after.log`

Delegated proof is sufficient for this docs-only classification slice. Proof
log: `test_after.log`.
