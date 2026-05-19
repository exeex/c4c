Status: Active
Source Idea Path: ideas/open/313_aarch64_f128_transport_machine_printer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize F128 Transport Printer Requirements

# Current Packet

## Just Finished

Lifecycle activation complete: idea 313 is now the active plan. Step 1 should
localize the missing structured AArch64 `f128_transport` printer/lowering fact
before implementation work begins.

## Suggested Next

Execute Step 1: inspect the focused `00204.c` failure and local AArch64
machine-node printer/dispatch surfaces to identify the exact missing
`f128_transport` structured operand or lowering fact.

## Watchouts

- This owner is target machine-node printer/lowering work, not semantic
  local-memory prepared-handoff work from closed idea 312.
- Keep `ideas/open/314_aarch64_large_stack_offset_addressing.md` parked; do
  not fold the `00216.c` large stack-offset residual into this owner.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or semantic
  handoff contracts.
- Do not satisfy the route with filename, function-name, block-number,
  instruction-number, diagnostic-string, or c-testsuite-number special cases.

## Proof

No validation run; this was lifecycle activation only.
