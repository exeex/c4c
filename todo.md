Status: Active
Source Idea Path: ideas/open/217_phase_e0_prepared_bir_merge_readiness_and_ownership_boundary_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Synthesize E1-E5 Readiness

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: synthesized the `PreparedBirModule` field map
and `PreparedFunctionLookups` group map into E1-E5 readiness recommendations in
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`.

The synthesis adds a target-end-state table, E1 semantic duplicate candidates,
E2 public prepared API/private pass-context candidates, realistic
code-size-reduction helper/API families with required proof, no-reduction
surfaces, E3 diagnostic/oracle prerequisites, E4 cross-target wrapper blockers,
and exact criteria for when draft 155 / E5 can be rewritten or opened. It keeps
the recommendations conditional and does not claim aggregate
`PreparedBirModule` or `PreparedFunctionLookups` deletion is ready.

## Suggested Next

Execute Step 5: validate
`docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md` against the
active source idea's expected outputs and reject signals, then record the final
docs-only proof.

## Watchouts

- Step 5 should verify that selected-reader proof is not treated as broad
  retirement proof and that no aggregate `PreparedBirModule` or
  `PreparedFunctionLookups` deletion is claimed.
- Keep target ABI/layout/register/stack/addressing/move/helper/emission policy
  out of target-neutral BIR, including target-policy portions of mixed fields
  and lookup groups.
- Preserve prepared fallback, diagnostics, printer/debug, route-debug,
  helper-oracle, wrapper, baseline, and expected-string authority as retained
  surfaces or explicit prerequisites.
- Draft 155 remains evidence only. The Step 4 criteria require field and lookup
  ownership, E1-E4 proof, retained fallback/oracle, cross-target wrapper proof,
  and concrete code-deletion targets before draft 155 / E5 can be rewritten or
  opened.
- E2 candidates must stay one lookup group plus one consumer or diagnostic row;
  facade moves, container reshuffles, and wrapper renames are not code-size
  reduction.

## Proof

`printf 'docs-only Step 4 E1-E5 readiness synthesis; no executable proof applies\n' > test_after.log`

Delegated proof is sufficient for this docs-only synthesis slice. Proof log:
`test_after.log`.
