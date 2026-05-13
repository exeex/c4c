Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Lowerer Local, Param, Static, Label, And Signature Mirrors

# Current Packet

## Just Finished

Step 4 packet completed for lowerer local/param rendered-name compatibility.
Value lookup, generic type inference, and function-pointer return inference now
require explicit `rendered_compat_*_text_ids` for valid source `TextId`
fallbacks; name-only compatibility remains limited to no-metadata references.
Focused tests cover stale rendered-name rejection and retained explicit
`TextId` compatibility for locals and params.

## Suggested Next

Continue Step 4 with the remaining lowerer compatibility mirrors: static/global
bridges, local const binding maps, labels, and any function prototype or
signature mirrors that still allow stale rendered names to override complete
source metadata.

## Watchouts

- The new helper treats a valid source `TextId` as complete enough to reject
  name-only rendered compatibility; callers must insert the explicit
  `rendered_compat_*_text_ids` entry when a valid-`TextId` generated bridge is
  intentional.
- Static/global and local const bridges already have nearby focused tests; keep
  the next packet semantic rather than expectation-only.

## Proof

Passed: `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_hir_lookup_tests$"' > test_after.log 2>&1`

Proof log: `test_after.log`
