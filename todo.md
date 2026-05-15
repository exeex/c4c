Status: Active
Source Idea Path: ideas/open/242_prepared_stack_slot_preserved_value_extent.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Prepared Authority

# Current Packet

## Just Finished

Activation created the runbook from `ideas/open/242_prepared_stack_slot_preserved_value_extent.md`.

## Suggested Next

Start `plan.md` Step 1 by mapping every stack-slot branch in
`prepare::build_call_preserved_values` to its prepared size/alignment
authority, or record the missing-authority blocker in this file.

## Watchouts

- Do not infer stack-slot preserved-value size or alignment inside AArch64
  lowering.
- Do not weaken preserved-value or call tests to claim progress.
- Keep register-route preserved values behaviorally stable.

## Proof

Lifecycle activation only; no code validation run.
