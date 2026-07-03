Status: Active
Source Idea Path: ideas/open/577_rv64_20000622_1_runtime_abort_after_call_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify The Runtime Abort

# Current Packet

## Just Finished

Activation only. No executor packet has run for this plan yet.

## Suggested Next

Delegate Step 1: reproduce `src/20000622-1.c`, capture 577-specific artifacts,
and classify the first post-call-lowering runtime-abort family.

## Watchouts

- Keep this route classification-first.
- Do not edit expectations, unsupported markers, allowlists, runtime comparison
  behavior, or the gcc_torture runner.
- Do not add filename-specific handling for `src/20000622-1.c`.
- Preserve the same-module call/result behavior repaired under 572.

## Proof

Lifecycle activation only; no build or test proof required.
