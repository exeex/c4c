Status: Active
Source Idea Path: ideas/open/658_backend_baseline_history_umbrella_triage.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Generate Ordered Follow-Up Ideas

# Current Packet

## Just Finished

Step 3 - Generate Ordered Follow-Up Ideas completed. Created
`docs/backend_baseline_history_triage/follow_up_order.md` and generated ordered
follow-up source ideas `ideas/open/659_rv64_byval_prepared_call_boundary.md`
through `ideas/open/668_llvm_torture_20040709_research.md` from the Step 2
ownership classification. The order prioritizes broad byval/prepared
call-boundary, pointer-local, and prepared destination families before narrow
singleton runtime, target infrastructure, and research-only LLVM torture work.

## Suggested Next

Execute Step 4: validate the umbrella handoff, verify the generated follow-up
ideas do not duplicate or silently absorb ideas 647, 655, or 657, and add the
closure note required by `ideas/open/658_backend_baseline_history_umbrella_triage.md`.

## Watchouts

- Do not change implementation, tests, expectations, unsupported markers,
  allowlists, runtime behavior, baseline acceptance state, or existing ideas
  647, 655, and 657 in this umbrella.
- Step 4 should verify `docs/backend_baseline_history_triage/follow_up_order.md`
  agrees with the generated source ideas and with
  `docs/backend_baseline_history_triage/failure_classification.md`.
- The LLVM torture follow-up is research-only because Step 2 intentionally
  deferred rows 1941 and 1942 rather than assigning a first implementation
  owner.
- Keep the newer 2026-07-10 04:18 657 representative pass authoritative over
  the older 04:12 mismatch unless fresh evidence says otherwise.

## Proof

Docs-only lifecycle packet; no build or tests run. Inspected `plan.md`,
`todo.md`, `docs/backend_baseline_history_triage/evidence_timeline.md`,
`docs/backend_baseline_history_triage/failure_classification.md`,
`ideas/open/658_backend_baseline_history_umbrella_triage.md`, and existing
ideas 647, 655, and 657 before generating the follow-up order and source
ideas. No root-level logs were created or modified.
