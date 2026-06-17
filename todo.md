Status: Active
Source Idea Path: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect candidate cases and register the first in-scope target

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
`plan.md`. No implementation packet has run for this plan yet.

## Suggested Next

Execute Step 1: inspect `two_arg_local_arg.c`, inspect the prepared call plan
and value homes for its local scalar argument, register the rv64 runtime target
if it stays in scope, and capture the initial narrow proof result.

## Watchouts

- Keep the route limited to direct scalar register calls with local scalar
  argument sources.
- Do not absorb pointer parameters, member indexing, indirect calls, varargs,
  stack-passed arguments, aggregate ABI, globals, or function pointer tables
  into this plan.
- Use prepared call plans and value homes; do not match helper names,
  filenames, or exact source shapes.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Lifecycle-only activation. No build or test command was required.
