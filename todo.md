Status: Active
Source Idea Path: ideas/open/299_rv64_runtime_pointer_param_member_index_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect metadata and register the candidate case

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
`plan.md`. No implementation packet has run for this plan yet.

## Suggested Next

Execute Step 1: inspect `local_dynamic_member_array.c`, inspect the prepared
metadata path for the case, register the rv64 runtime target if it stays in
scope, and capture the initial narrow proof result.

## Watchouts

- Keep the route limited to a direct scalar helper call that receives a pointer
  to a local aggregate and indexes a simple member array through prepared
  metadata.
- Do not accept globals, indirect calls, varargs, stack-passed arguments,
  aggregate ABI, dynamic stack support, or broad pointer provenance in this
  plan.
- Use prepared value homes, memory accesses, call plans, and address
  materialization metadata; do not match helper names, filenames, or exact
  source shapes.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Lifecycle-only activation. No build or test command was required.
