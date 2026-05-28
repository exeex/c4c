Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove Store-Local Selected Publication Ownership

# Current Packet

## Just Finished

Step 6, store-local selected publication ownership, now has a focused MIR/unit
probe for
`future_store_local_stack_value_publication_covers_instruction`.

Added `future_store_local_stack_value_publication_proves_selected_owner()` to
`tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`.
The positive fixture builds a real prepared BIR function where instruction 0 is
a named `SelectInst` with a stack-slot home and instruction 1 is a later
`StoreLocal` whose prepared store-source publication covers that selected
producer. The same probe fails closed for no later store, value mismatch, type
mismatch, missing prepared memory access / unavailable store-source
publication, and a register-homed source.

## Suggested Next

Next packet: supervisor/reviewer should decide whether this Step 6 proof is
enough to accept only the store-local selected-publication dirty suppression
seam, then either commit the focused unit-test slice with the existing
implementation changes or request a narrower dispatch-level confirmation.

## Watchouts

- This packet only accepts the store-local selected-publication ownership
  helper/probe shape. It does not accept store-global, fused-compare,
  call/outgoing-stack, or direct-edge dirty seams.
- A route-level C probe that only shows selected local-store compilation remains
  non-evidence for this seam; the accepted evidence here is the lower-level
  MIR/unit ownership fixture.
- `c_testsuite_aarch64_backend_src_00196_c` still fails with the existing
  runtime mismatch (`joe() && fred()` cases print `1` instead of `0`).

## Proof

Ran the delegated proof command and preserved `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Result: build passed. `backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known baseline
runtime mismatch recorded above. `test_after.log` is preserved with the exact
delegated proof output.
