Status: Active
Source Idea Path: ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select One Cross-Target Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/189_phase_e_cross_target_route_view_interface_reuse.md`.

## Suggested Next

Delegate Step 1 to an executor: inspect x86 `ConsumedPlans` and riscv prepared
emission/wrapper boundaries, select one bounded target boundary that can reuse
an AArch64-proven Route 3, Route 6, or Route 7 interface, identify its fallback
target-policy path, and record the narrow proof command for the next packet.

## Watchouts

- Do not invent an x86-only or riscv-only BIR adapter before using a proven
  shared route-view interface.
- Keep x86/riscv instruction selection, formatting, frame/register allocation,
  ABI policy, and emission records target-owned.
- Do not duplicate `PreparedFunctionLookups` under a BIR-owned name.
- Do not weaken route-debug, wrapper, or target emission coverage.
- Do not claim broad prepared aggregate contraction from one selected wrapper
  thread.

## Proof

Lifecycle-only activation. No code validation run.
