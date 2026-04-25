Status: Active
Source Idea Path: ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Record Proof Gaps And Validation Recommendations

# Current Packet

## Just Finished

Step 3 follow-up scope mapping completed for the active report-only audit. A
route quality review then found lifecycle drift: `todo.md` had marked the
runbook exhausted even though Step 4 and Step 5 remain in `plan.md`. This
lifecycle repair keeps the existing runbook and advances the active pointer to
Step 4.

## Suggested Next

Delegate the next report-only packet for Step 4. The packet should add
actionable proof gaps and validation recommendations to
`review/112_lir_backend_legacy_type_surface_readiness_audit.md`, covering
aggregate, HFA, sret, variadic, global-init, memory-addressing, GEP,
initializer, `va_arg`, byval/byref, verifier, and printer proof areas.

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
- Step 5 remains after Step 4 and should perform the final report sanity check.

## Proof

Lifecycle-only repair; no build required. Checked `plan.md` and
`review/112_route_quality_after_step3.md`; Step 4 is the next valid active
runbook step, and `plan.md` does not require a rewrite.
