Status: Active
Source Idea Path: ideas/open/112_lir_backend_legacy_type_surface_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Record Proof Gaps And Validation Recommendations

# Current Packet

## Just Finished

Step 4 report refinement completed for the active audit. The review artifact
now records proof gaps, validation recommendations, current sufficient
evidence, and needed broader parity for aggregate, HFA, sret, variadic,
global-init, memory-addressing, GEP, initializer, `va_arg`, byval/byref,
verifier, and printer proof areas.

## Suggested Next

Delegate the final report-only packet for Step 5. The packet should perform
the final sanity pass over
`review/112_lir_backend_legacy_type_surface_readiness_audit.md`, checking that
the inventory, blocker ownership, follow-up scope, proof gaps, validation
recommendations, and must-not-remove list agree with each other.

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
- Validation recommendations are intentionally bucket-level because this packet
  was not asked to inspect or edit test sources.
- Step 5 remains and should perform the final report sanity check.

## Proof

Report-only packet; no build required. Sanity proof ran:
`git diff --name-only` showed only `todo.md` and
`review/112_lir_backend_legacy_type_surface_readiness_audit.md` changed, and
`git diff --check` was clean.
