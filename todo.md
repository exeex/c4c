Status: Active
Source Idea Path: ideas/open/675_post_wave_residual_baseline_failures.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Capture Candidate Delta And First Owners

# Current Packet

## Just Finished

Lifecycle activation created the post-wave residual baseline idea and runbook.
Supervisor rejected the current `test_baseline.new.log` candidate without
deleting it because it has two new-only failures relative to the accepted
`test_baseline.log`.

## Suggested Next

Execute Step 1: write
`build/agent_state/675_step1_candidate_delta/summary.md`, compare
`test_baseline.log` and `test_baseline.new.log` by stable test name, and probe
the two candidate-only RV64 CLI route failures for first owner.

## Watchouts

- Do not accept `test_baseline.new.log` while it contains candidate-only
  failures.
- Compare by stable test name, not numeric row id.
- Do not reopen closed ideas unless fresh evidence contradicts their closure
  notes.
- Do not change expectations, unsupported markers, allowlists, timeouts,
  runtime policy, or baseline accounting.

## Proof

Lifecycle-only activation; no build or test proof required.
