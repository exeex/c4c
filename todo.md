# Current Packet

Status: Active
Source Idea Path: ideas/open/798_lir_stack_restore_lifetime_consumer_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define the selected native stack-restore authority contract

## Just Finished

- Lifecycle switch from 794: Step 1 evidence boundary is complete in
  `1cbad00d6`; 798 now owns the separate stack-restore
  producer/schema/verifier blocker.

## Suggested Next

- Execute Plan Step 1: define the one selected `LirStackRestoreOp` native
  admission and structured lifetime-consumer transition contract.

## Watchouts

- Do not absorb dynamic-VLA count work, VLA GEP, other local/lifetime rows,
  Raw-BIR/importer/734 receipt, or presentation-derived facts.

## Proof

- For this lifecycle switch: `git diff --check` and structural linkage that
  plan.md and todo.md name 798, while 794 retains its completed Step 1 and
  exact return condition.
