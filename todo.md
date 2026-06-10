Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch One Semantic Memory Consumer If Proven

# Current Packet

## Just Finished

Step 5 switched one scalar dispatch memory consumer: the `LoadGlobalInst`
branch in `emit_value_publication_to_register`.

That branch now asks
`mir::find_bir_same_block_global_load_access_identity` for the same-block
global-load identity when the BIR instruction is representable as a global
memory access. The target emission path still uses prepared memory access data
for `emit_prepared_global_load_to_register`, preserving prepared addressing,
relocation/GOT/TLS, offset, and operand-formation policy. Non-BIR-representable
legacy global-load positives still fall back to the prepared same-block global
load access lookup.

`prepared_with_load_global_call_argument` now gives the scalar pointer
`LoadGlobalInst` a BIR global-symbol `MemoryAddress`, so the existing direct
and GOT call-argument dispatch checks exercise the BIR-representable path
without changing the expected AArch64 behavior.

## Suggested Next

Next executable packet candidate: supervisor review of the Step 5 slice, then
either close this first consumer migration or delegate a separately scoped next
consumer only if the active plan still calls for one.

## Watchouts

- Only the scalar dispatch `LoadGlobalInst` publication branch was switched.
  FP materialization, current global load materialization, local-slot/load-local
  paths, layout-dependent paths, `globals.cpp`, `alu.cpp`, and `calls.cpp` were
  left untouched.
- The prepared fallback is still necessary for legacy/non-BIR-representable
  global-load positives where the BIR instruction does not carry a
  `MemoryAddress`.
- Do not infer prepared/BIR parity for local stack-layout overlap or
  non-overlap facts from this packet.

## Proof

`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_store_source_publication_plan_test backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' > test_after.log 2>&1`

Passed. `test_after.log` contains the delegated proof log with all four
selected tests passing.
