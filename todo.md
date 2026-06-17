Status: Active
Source Idea Path: ideas/open/298_rv64_direct_scalar_call_boundary_hardening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Register and classify the first direct scalar call case

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible scratchpad for Step 1 of
`plan.md`. No implementation packet has run for this plan yet.

## Suggested Next

Execute Step 1: inspect the first direct scalar helper-call candidates,
register the smallest in-scope rv64 runtime case, and capture the initial
rv64 runtime proof result.

## Watchouts

- Keep the route limited to direct scalar register calls and scalar integer
  returns.
- Do not accept indirect calls, varargs, stack-passed arguments, aggregate ABI,
  global function pointer tables, or pointer-heavy memory in this plan.
- Use prepared call metadata and value homes; do not match helper names,
  filenames, or exact source shapes.
- Keep review artifacts under `review/` untouched unless a supervisor-provided
  lifecycle task explicitly requires reading or rewriting around one.

## Proof

Lifecycle-only activation. No build or test command was required.
