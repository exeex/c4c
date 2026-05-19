Status: Active
Source Idea Path: ideas/open/314_aarch64_large_stack_offset_addressing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Large Stack Offset Addressing Paths

# Current Packet

## Just Finished

Lifecycle activation complete: idea 314 is now the active plan. Step 1 should
localize the large stack-offset addressing paths for the `00216.c` stack-slot
load/store residual and the `00204.c` scalar ALU stack-publication residual
before implementation work begins.

## Suggested Next

Execute Step 1: inspect the current focused residuals and AArch64
machine-printer/dispatch surfaces to identify whether the stack-slot
load/store and scalar stack-publication failures share one large-offset repair
surface or need ordered substeps under this owner.

## Watchouts

- This owner is large stack-offset addressing/publication work, not semantic
  local-memory prepared-handoff work from closed idea 312 or f128 transport
  work from closed idea 313.
- Keep umbrella idea 295 parked; do not fold unrelated runtime, timeout,
  direct-call, vararg, address-of-local, `00164.c`, or `00214.c` residuals
  into this owner.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, semantic
  handoff contracts, or f128 transport behavior.
- Do not satisfy the route with filename, function-name, offset-literal,
  stack-slot-id, scalar-opcode, diagnostic-string, or c-testsuite-number
  special cases.

## Proof

No validation run; this was lifecycle activation only.
