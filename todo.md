Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Prove Store-Global Stack Publication Ownership

# Current Packet

## Just Finished

Step 7, store-global stack publication ownership, now has a focused store-global
probe and accepted helper ownership seam.

`lower_pending_store_global_stack_value_publications` now uses the prepared
store-global publication plan to identify stack-homed pending publications,
routes them through the published-stack republish helper, and records the
published source value name. The republish helper
`lower_published_store_global_stack_value_publication` now materializes the
current producer from just after its own instruction index, so a stack-homed
selected producer can be republished after the later global store has taken
ownership.

Added `backend_codegen_route_aarch64_store_global_stack_publication` and a MIR
unit probe in
`tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`.
The unit probe proves pending publication of a stack-homed selected value,
records the published source name, proves producer republishing, and fails
closed for unpublished/register-homed cases.

## Suggested Next

Next packet: supervisor should review/commit the Step 7 store-global slice if
the route and unit probes are sufficient, then select the next remaining dirty
seam separately.

## Watchouts

- This packet only accepts the store-global stack publication/republication
  seam in `memory.cpp`, `memory.hpp`, and the dispatch call sites. It does not
  accept store-local, fused-compare, calls/outgoing-stack, or edge LoadLocal
  seams.
- The route probe checks the emitted selected-value stack publication shape
  with register-agnostic snippets: the global symbol/address materialization,
  generic word store/load operations, and stack-pointer use. The unit probe
  checks the ownership state directly.
- `c_testsuite_aarch64_backend_src_00196_c` still fails with the existing
  runtime mismatch (`joe() && fred()` cases print `1` instead of `0`).

## Proof

Ran the delegated proof command and preserved `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_store_global_stack_publication|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Result: build passed. `backend_codegen_route_aarch64_store_global_stack_publication`,
`backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known baseline
runtime mismatch recorded above. `test_after.log` is preserved with the exact
delegated proof output.

Additional focused check run while developing the unit probe:

```sh
cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_prepared_memory_operand_records$'
```

Result: passed.
