Status: Active
Source Idea Path: ideas/open/297_rv64_runtime_local_stack_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Register and classify the first local stack memory case

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and executor-compatible state
for Step 1.

## Suggested Next

Delegate Step 1 to an executor: inspect `local_array.c` and
`local_pointer_deref.c`, choose the first in-scope local frame-slot memory case,
register it as an rv64 runtime case, and capture the current narrow failure.

## Watchouts

- Keep the route limited to local frame-slot memory.
- Do not accept BIR or LLVM fallback text as runtime success.
- Stop and report a split if the candidate requires globals, calls, aggregate
  ABI, dynamic stack support, or non-local pointer provenance.

## Proof

Not run; lifecycle-only activation.
