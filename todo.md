Status: Active
Source Idea Path: ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Write Follow-Up Recommendations And Proof Gaps

# Current Packet

## Just Finished

Step 5 from `plan.md`: finalized
`review/103_hir_post_dual_path_legacy_readiness_audit.md` with follow-up
recommendations for safe idea 104 HIR cleanup scope, separate idea 105
HIR-to-LIR bridge scope, proof gaps before demotion, focused validation subsets,
and final classification summaries for safe demotion, bridge-required,
diagnostic-only, printer-only, ABI/link-spelling, and HIR-to-LIR blocked paths.

## Suggested Next

Supervisor lifecycle review: decide whether the audit artifact satisfies idea
103 and route to the plan owner for close/deactivate/replacement-plan decision.

## Watchouts

- The artifact intentionally recommends no implementation or expectation edits
  for idea 103; route any cleanup into idea 104 or idea 105.
- Idea 104 scope should stay HIR-internal and proof-driven; idea 105 owns
  HIR-to-LIR identity handoffs and ABI/layout bridge work.
- Safe demotion candidates are narrow: complete `DeclRef` structured lookup,
  concrete IDs/link IDs, owner-aware HIR record lookup, and parity helper flips
  only after focused proof is green.
- Rendered spelling remains valid for diagnostics, printers, dumps, emitted
  link names, LLVM struct type spelling, and mangling/type fingerprints.

## Proof

Audit-only packet; no build or test command required by the supervisor.
Lightweight checks run for Step 5: re-read the Step 1-4 artifact and focused
source-idea / plan output requirements; ran `rg` over `plan.md`, the source
idea, and the review artifact for demotion, bridge, proof-gap, idea 104, and
idea 105 requirements; ran `git diff --check`. No `test_after.log` was produced
because the delegated proof explicitly did not require a build or test command.
