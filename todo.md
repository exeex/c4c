Status: Active
Source Idea Path: ideas/open/104_hir_safe_legacy_lookup_demotion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The First Safe HIR-Only Demotion Packet

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Select one narrow HIR-only demotion packet from
`review/103_hir_post_dual_path_legacy_readiness_audit.md`, record the exact
classification row that authorizes it, and keep the work out of HIR-to-LIR or
codegen bridge scope.

## Watchouts

- Do not touch implementation until the selected packet is tied to a
  `safe-to-demote` or `legacy-proof-only` classification.
- Do not remove bridge-required maps or rendered spellings that idea 103 assigns
  to idea 105.
- Do not downgrade tests or expectations to prove cleanup.

## Proof

Lifecycle-only activation. No build or test command was required.
