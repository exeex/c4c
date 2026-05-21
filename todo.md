# Current Packet

Status: Active
Source Idea Path: ideas/open/366_aarch64_loaded_byte_value_reused_as_address.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Address-Carrying Boundary

## Just Finished

Lifecycle activation created the active runbook for Step 1. No executor packet
has run yet.

## Suggested Next

Supervisor should delegate Step 1 with a narrow reproduction/proof command for
`c_testsuite_aarch64_backend_src_00173_c` and any requested debug-dump flags for
prepared BIR, register allocation, and generated AArch64 evidence.

## Watchouts

- Do not special-case `00173`, a literal, a stack slot, a register, an emitted
  instruction sequence, or a loop body.
- Do not weaken expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or external test
  contracts.
- Preserve idea 365 string/global pointer publication and idea 356 dynamic
  pointer-derived byte-load behavior.
- Treat unrelated call lowering, ABI, frontend, runner, timeout, dynamic stack,
  or test-registration evidence as a lifecycle split candidate.

## Proof

Not run. Activation is lifecycle-only.
