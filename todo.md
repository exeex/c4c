Status: Active
Source Idea Path: ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Prepared Aggregate va_arg Helper Facts

# Current Packet

## Just Finished

Lifecycle activation created this active runbook from
`ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.

## Suggested Next

Execute Step 1: audit the prepared aggregate `va_arg` helper facts for
`src/920908-1.c` and record the first complete or missing fact set.

## Watchouts

- Do not reopen shared LIR/BIR vararg semantics from idea 360.
- Do not reopen `va_start` destination-address or overflow-area setup from
  ideas 365 and 366.
- Do not infer aggregate layout, helper resources, or `va_list` state from
  source syntax, testcase names, raw BIR text, or diagnostics.
- Preserve fail-closed diagnostics for unsupported aggregate helper shapes.

## Proof

No implementation proof has been run for this newly activated plan.
