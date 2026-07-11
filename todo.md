Status: Active
Source Idea Path: ideas/open/683_prepared_mir_view_contract_research.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Bound Proof, Diagnostic, And Equivalence Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by drafting
`docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md`
and
`docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md`.
The proof-boundary research defines `RequiredFact`, `OptionalFeatureFact`,
`VerifierFact`, `ProofCertificateFact`, `DiagnosticFact`, and
`CompatibilityFact`; classifies publication, freshness, provenance,
local-array, select-chain, and route-debug families; names current consumer
risks; proposes diagnostic-proof type boundaries; and states reviewer reject
rules. The equivalence research defines old/new producer architecture,
view-boundary equivalence, golden dump, structural comparison, verifier
comparison, compatibility exclusions, and failover to old BIR when the new
route is incomplete.

## Suggested Next

Start `plan.md` Step 4 by drafting the incremental migration and follow-up
documents:
`docs/prepared_mir_view_contract_research/06_incremental_migration_plan_for_mir_consumers.md`
and
`docs/prepared_mir_view_contract_research/07_open_questions_and_followup_implementation_ideas.md`.

## Watchouts

- Step 3 classifies current Route 6 source records and local-array proof
  artifacts as compatibility/proof material unless promoted into typed
  required or feature views. Step 4 should keep that distinction when planning
  migration checks.
- The old/new equivalence document rejects freezing raw `PreparedBirModule`,
  `notes`, `completed_phases`, private `route_`, route-debug focus filters, or
  route summary text as permanent view ABI.
- This active idea remains research and architecture documentation only. Do not
  start implementation cleanup, expectation changes, unsupported-marker edits,
  allowlist edits, or runtime behavior changes from this runbook.

## Proof

No build or test proof is required for this documentation-only research packet.
Proof passed:
`git diff --check -- docs/prepared_mir_view_contract_research/04_debug_proof_and_diagnostic_boundaries.md docs/prepared_mir_view_contract_research/05_old_bir_new_bir_equivalence_strategy.md todo.md`.
Supplemental no-index whitespace checks also passed for the two newly created
answer files while they remain untracked.
No `test_after.log` is expected because the delegated proof is a direct
documentation diff check rather than a build or CTest command.
