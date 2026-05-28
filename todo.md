Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove Store-Local Selected Publication Ownership

# Current Packet

## Just Finished

Step 6, store-local selected publication ownership, remains unaccepted.

I tested candidate C route probes for `aarch64_store_local_selected_publication`,
but the backend case harness only produced register-homed selected values for
the local-store source. Those probes would compile without proving the
`memory.cpp` / `dispatch.cpp` future-consumer suppression path, whose ownership
requires a later store-local publication to cover an otherwise unsupported
stack-homed selected producer. No focused probe was registered because it would
have been misleading acceptance evidence.

## Suggested Next

Next packet: continue Step 6 with a lower-level MIR/unit ownership probe for
`future_store_local_stack_value_publication_covers_instruction`. The previous
C backend route probe is not acceptance evidence for this seam.

If the MIR/unit probe cannot be built without exposing inappropriate private
internals, remove or quarantine the dirty store-local suppression instead of
accepting it.

## Watchouts

- The dirty store-local future-consumer suppression in `memory.cpp` /
  `dispatch.cpp` is not acceptance-ready from this packet.
- A route-level C probe that only shows selected local-store compilation is not
  sufficient; the observed candidates did not make the selected producer
  stack-homed, so they did not isolate the intended ownership seam.
- The store-global dirty code in the same files remains outside this packet.
- `c_testsuite_aarch64_backend_src_00196_c` still fails with the existing
  runtime mismatch (`joe() && fred()` cases print `1` instead of `0`).

## Proof

Ran the delegated proof command and preserved `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_store_local_selected_publication|backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Result: build passed; `backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed; no focused
`backend_codegen_route_aarch64_store_local_selected_publication` test existed;
`c_testsuite_aarch64_backend_src_00196_c` failed with the runtime mismatch
recorded above. This proof does not accept the store-local seam.

## Lifecycle Decision

Step 6 stays active, but the C backend route is retired as the acceptance
probe for this seam. Continue only with a lower-level MIR/unit ownership probe
for `future_store_local_stack_value_publication_covers_instruction`.

Next executor packet:

- Add or extend a focused MIR/unit test, preferably
  `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  unless a closer existing MIR test is found during implementation.
- Owned implementation surface, if the test exposes a real helper defect:
  `src/backend/mir/aarch64/codegen/memory.cpp` and
  `src/backend/mir/aarch64/codegen/memory.hpp` for
  `future_store_local_stack_value_publication_covers_instruction`; only touch
  `dispatch.cpp` if needed to remove or quarantine the existing call site.
- The positive fixture must make the current instruction produce a named,
  stack-homed selected value, then make a later same-block `StoreLocal`
  prepared store-source publication cover that exact producer or its direct
  global select-chain root.
- The fail-closed fixture must reject at least these nearby cases: no later
  store, value/type mismatch, unavailable prepared store-source publication,
  and register-homed source publication.
- Proof command should include `cmake --build build -j` and the focused MIR
  test name, then rerun the supervisor-selected four-test integration subset.

If that MIR/unit probe cannot be built without exposing inappropriate private
internals, the next executor must remove or quarantine the dirty store-local
future-consumer suppression instead of accepting it. The store-global,
fused-compare, call/outgoing-stack, and direct-edge dirty seams remain under
the dirty-stack warning and must not be bundled with Step 6.
