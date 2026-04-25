Status: Active
Source Idea Path: ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Map Follow-Up Scope For Ideas 113, 114, And 115

# Current Packet

## Just Finished

Step 3 follow-up scope mapping completed for the active report-only audit. The
review artifact now maps Step 2 findings to ideas 113, 114, and 115, separates
out-of-scope findings for each follow-up, lists only proof-only or
proof-gap-aware demotion candidates for idea 114, records HIR-to-LIR
`legacy_decl` consumers and required fallbacks for idea 115, and explicitly
states that MIR is not a migration target or blocker for these follow-ups.

## Suggested Next

Supervisor should review whether the report-only audit satisfies idea 112 and
decide whether to close, retire, or expand the active runbook.

## Watchouts

- This was a report-only packet; no implementation files were changed.
- No Step 1 surface is currently fully `safe-to-demote`; idea 114 should start
  from `legacy-proof-only` or `needs-more-parity-proof` candidates and keep
  text authority where blockers remain.
- Idea 113 owns active BIR/backend structured layout table dual-path work.
- Idea 115 owns HIR-to-LIR `legacy_decl` consumers and must keep fallback for
  incomplete structured coverage.
- MIR/aarch64 legacy consumers are `planned-rebuild` only. Do not treat MIR
  migration as required cleanup or as a blocker for BIR/LIR follow-up work.

## Proof

Report-only packet; no build required. Sanity proof commands:
`git diff --name-only` showed only
`review/112_lir_backend_legacy_type_surface_readiness_audit.md` and `todo.md`;
`git diff --check` passed clean. No `test_after.log` is required for this
supervisor-delegated report-only packet.
