Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Consolidate Retained Bridges and Final Proof

# Current Packet

## Just Finished

Step 6 - Retire BIR and Backend Raw Symbol Fallbacks is sufficiently exhausted
for this runbook after the latest BIR/backend bridge slices.

Completed Step 6 packets:
- `1b640c06b Fence BIR initializer raw symbol parser`
- `272c82a2a Fence BIR raw symbol compatibility maps`

Those packets covered initializer raw symbol parsing plus the retained
BIR/LIR-to-BIR raw function/global symbol tables. Present `LinkNameId`
metadata remains semantic authority and fails closed on misses instead of
falling through raw spelling. Retained raw symbol paths are now documented as
raw-import/no-id producer compatibility boundaries, with verifier coverage for
present-id mismatch/miss closure and raw-import/no-id compatibility.

Prepared/x86 route-local value, block, slot, prepared, and generated-name
identity was deliberately left to idea 169, unless raw spelling acts as
source/link-visible bridge authority.

## Suggested Next

Begin Step 7 by consolidating the retained compatibility bridges across the
touched parser, sema/consteval, HIR, LIR, BIR, and backend domains.

Next packet:
- Re-run focused searches for rendered, legacy, compatibility, fallback,
  no-metadata, invalid-id, raw symbol, and final-output scan vocabulary in the
  touched domains.
- Record retained bridges with owner, domain, caller class, limitation, and
  removal condition.
- Verify route-local leftovers remain assigned to idea 169 and guard work
  remains assigned to idea 170.
- Propose the supervisor-approved final proof subset for closure readiness.

## Watchouts

- Do not re-open route-local value, block, slot, prepared, or generated-name
  cleanup under idea 168 unless raw spelling is source/link-visible bridge
  authority; those local identity families belong to idea 169.
- Do not turn Step 7 into a new bridge-retirement implementation bucket. It is
  for consolidation, final proof selection, closure readiness, and exact
  follow-up blockers.
- Raw-import/no-id compatibility remains valid when no `LinkNameId` metadata is
  present; present `LinkNameId` metadata must remain authoritative and fail
  closed on miss.
- The latest accepted full-suite candidate after Step 6 is 3135/3135 at
  `272c82a2a`.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Step 6 latest focused proof:
`cmake --build build --target backend_lir_to_bir_notes_test && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$'`

Result: passed in `test_after.log`.

Accepted full-suite candidate after relevant Step 6 commits: 3135/3135 at
`272c82a2a`.

Additional check: `git diff --check` passed.
