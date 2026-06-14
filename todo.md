Status: Active
Source Idea Path: ideas/open/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Call Argument Source Inventory

# Current Packet

## Just Finished

Lifecycle activation created the runbook and canonical execution-state
skeleton for Step 1 - Call Argument Source Inventory.

## Suggested Next

Execute Step 1 by inventorying the x86 scalar named-`i32` call argument source
identity reader, the Route 6 call-use/source evidence, the prepared
`source_value_id` row, and the nearby compatibility/fail-closed proof surfaces.

## Watchouts

- Keep the active scope x86-local and limited to scalar named-`i32` call
  argument source identity.
- Do not delete, privatize, rename, or bypass public prepared call-plan APIs.
- Treat prepared-only evidence, Route 6-only evidence, mismatches,
  duplicate/conflict rows, unsupported rows, fallback rows, and
  policy-sensitive rows as fail-closed until proved otherwise.
- Do not weaken route-debug strings, wrapper baselines, helper/oracle status,
  fallback names, expected output, or `ConsumedPlans` contracts.

## Proof

Lifecycle activation only. Required check: `git diff --check -- plan.md todo.md`.
