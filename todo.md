Status: Active
Source Idea Path: ideas/open/658_backend_baseline_history_umbrella_triage.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build Reverse-Chronological Evidence Ledger

# Current Packet

## Just Finished

Lifecycle activation created the umbrella source idea and runbook for backend
baseline history triage.

## Suggested Next

Execute Step 1: create
`docs/backend_baseline_history_triage/evidence_timeline.md` from
`test_baseline.log`, `test_before.log`, and reverse-chronological
`build/agent_state/` evidence. Treat newer files as authoritative when history
conflicts.

## Watchouts

- Do not change implementation, tests, expectations, unsupported markers,
  allowlists, runtime behavior, or baseline acceptance state in this umbrella.
- Reconcile the older 657 Step 4 representative mismatch with the newer 657
  Step 3 representative pass before choosing repair order.
- Every follow-up idea generated later must name one owning layer and include
  reviewer reject signals.

## Proof

Lifecycle-only activation; no build or test proof required.
