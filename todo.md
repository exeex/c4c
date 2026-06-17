# Current Packet

Status: Active
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Prepared Global Inputs

## Just Finished

Lifecycle activation created the active runbook and initialized this executor
packet skeleton for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect prepared BIR/debug output for
`global_load.c`, `global_load_zero_init.c`, and `global_store.c`; record the
available global representation, load/store instruction facts, and smallest
owner surface needed for implementation.

## Watchouts

- Do not edit implementation files during the inspection-only packet unless the
  supervisor delegates a code-changing packet.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, or GOT-heavy
  work.

## Proof

Lifecycle-only activation. No build or runtime proof required yet.
