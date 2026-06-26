Status: Active
Source Idea Path: ideas/open/374_rv64_object_route_non_register_param_homes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Non-Register Parameter Homes

# Current Packet

## Just Finished

Lifecycle activation created this active runbook from
`ideas/open/374_rv64_object_route_non_register_param_homes.md`.

## Suggested Next

Execute Step 1: audit prepared non-register parameter-home facts for
`src/va-arg-13.c` at `unsupported_param_home` and record the first complete or
missing fact set.

## Watchouts

- Do not infer parameter layout from source syntax, testcase names, raw BIR
  spelling, or assumed RV64 stack layouts.
- Preserve fail-closed diagnostics for unsupported stack, aggregate,
  missing-home, non-default address-space, dynamic-layout, and ABI-ambiguous
  parameter homes.
- Keep byval aggregate homes, aggregate `va_arg`, and frame-slot address call
  arguments routed to their owning ideas unless the parameter-home contract
  directly requires them.
- Do not weaken unsupported expectations, allowlists, diagnostics, or broad
  test contracts to claim progress.

## Proof

No implementation proof has been run for this newly activated plan.
