Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Lowerer Local, Param, Static, Label, And Signature Mirrors

# Current Packet

## Just Finished

Step 3 is completed and committed. The consteval replay and pending identity
name fences now have focused coverage proving stale rendered registry names do
not redirect complete structured identity.

## Suggested Next

Begin Step 4: inspect lowerer function-context compatibility maps, including
rendered local, param, static-global, local const binding, label, and
function-pointer signature mirrors. Select one lowerer rendered fallback
surface, fence complete structured metadata misses so they fail closed, and
preserve only route-local generated names or explicitly documented no-metadata
legacy bridges.

## Watchouts

- Do not let stale rendered lowerer names override metadata-rich local, param,
  static, label, local const, or function-pointer signature identity.
- Keep route-local generated names only when they are not semantic fallback
  authority.
- Any retained no-metadata bridge needs `legacy` or `deprecated` documentation
  naming owner, limitation, and removal condition.

## Proof

No new validation was required for this lifecycle-only Step 4 handoff. Use the
supervisor-selected lowerer-focused proof command for the next implementation
packet.
