Status: Active
Source Idea Path: ideas/open/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Row And Agreement Boundary

# Current Packet

## Just Finished

No executor packet has completed for this active plan.

## Suggested Next

Delegate Step 1 from `plan.md`: locate the exact x86 scalar `i32`
argument-source route-debug row, identify the Route 6 agreement boundary
through `ConsumedPlans` and `find_consumed_scalar_i32_call_argument_source(...)`,
and record the minimal Step 2 target files plus fallback proof gaps before
implementation edits.

## Watchouts

- Keep source intent unchanged unless durable intent actually changes.
- Do not migrate broad x86 call wrapper, ABI/call policy, call lowering,
  prepared call printer/debug, direct-call/helper-oracle families,
  `ConsumedPlans`, wrappers, expected strings, baselines, or public fallback.
- Reject testcase-shaped matching; positive proof must use Route 6 scalar
  argument-source agreement under prepared compatibility with fail-closed
  fallback.
- Preserve prepared/`ConsumedPlans` authority for absent, invalid,
  duplicate/conflict, mismatch, compatibility, wrapper, direct-call,
  helper-oracle, and public fallback paths.

## Proof

No proof has been run for this activation-only lifecycle slice.
