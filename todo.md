Status: Active
Source Idea Path: ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Row Family And Proof Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/227_phase_e3_branch_target_helper_oracle_follow_up.md`.

## Suggested Next

Start Step 1 - Locate The Row Family And Proof Surface. Identify the exact
`find_prepared_control_flow_branch_target_labels(...)` helper-oracle row
family, map it to `BranchTargetIdentityPassContext` /
`read_agreeing_bir_branch_target_labels(...)`, and record the positive plus
fallback proof surface here before implementation begins.

## Watchouts

- Keep the slice to one branch-target helper-oracle row family.
- Preserve prepared fallback for absent context, invalid ids,
  duplicate/conflict, mismatch, non-conditional BIR, non-agreement, and
  prepared-only paths.
- Do not rewrite expected strings, baselines, wrapper output, printer/debug
  output, branch-control output, branch spelling, edge-copy scheduling, target
  policy, or emitted-output behavior.
- Reject testcase-shaped matching or fixture-name shortcuts.

## Proof

Activation-only lifecycle change. No code build or test proof required.
