Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared Pointer-Local Inputs

# Current Packet

## Just Finished

Activation created the active runbook and initialized execution state for
plan.md Step 1.

## Suggested Next

Execute Step 1: inspect prepared BIR/debug output for
`defined_global_array_pointer.c` and `defined_global_array_pointer_store.c`;
record the prepared global-address materialization facts, local pointer home,
and later pointer-value `i32` load/store metadata before choosing the next
owner surface.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Preserve fail-closed behavior for unsupported pointer/global forms instead
  of accepting incomplete assembly that later segfaults under qemu.

## Proof

Lifecycle-only activation. No code validation was run.
