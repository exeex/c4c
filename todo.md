# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete or Isolate Compatibility Bridge Helpers

## Just Finished

Lifecycle closed idea 148 after the HIR static-member carrier authority
decomposition met its acceptance criteria and reactivated idea 147 at Step 4.
Commit `1a206d419` is the carrier-policy repair handoff;
`review/148_step4_precommit_followup.md` accepted the route, and
`test_after.log` records standalone `frontend_hir_lookup_tests` plus broader
CTest 357/357 passing.

## Suggested Next

Next packet: resume idea 147 Step 4 by deleting or isolating the remaining
rendered qualified-name compatibility bridge helpers now that HIR static-member
carrier policy no longer depends on rendered owner suffix recovery.

## Watchouts

- Keep idea 148's boundary: rendered `expr->name` must not provide
  static-member owner authority.
- Generated member payload may be used only after structured owner authority
  exists.
- Do not reintroduce rendered `A::B::member` suffix owner recovery while
  deleting bridge helpers.
- Idea 147 is the active bridge-removal source; idea 148 is closed as the
  decomposition handoff.

## Proof

Lifecycle-only close/switch. Close accepted after `c4c-regression-guard`
compared `test_before.log` and `test_after.log` for the matching 357-test
scope in non-decreasing mode: 357/357 before, 357/357 after, no new failures.
