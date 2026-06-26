Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative And Route Next Boundary

# Current Packet

## Just Finished

Step 4: Rerun Representative And Route Next Boundary dump contract repair
completed before the representative rerun.

Changed files:

- `tests/backend/CMakeLists.txt`
- `todo.md`

The remaining prepared-dump failure was expectation drift after the byval
regression repair. `backend_dump_riscv64_byval_preserved_pointer_args` now
requires the current semantic contract for the second call boundary: arg0 keeps
the prepared byval `stack_copy` transport from frame slot #11 with the four
required payload chunks, and arg1 still publishes the preserved `s1` register
source to call argument `a1` while explicitly selecting
`arg.source_selection=local_frame_address_materialization` with matching
frame-slot selection metadata.

## Suggested Next

Execute the original Step 4 representative rerun for `src/20030914-2.c` through
the RV64 object route and decide whether the active idea is ready for broader
validation or needs another focused packet.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.
- This packet repaired the remaining prepared-dump contract after the
  baseline-review regression repair; it did not run `src/20030914-2.c`.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp` is the existing
  test file for the delegated `backend_riscv_object_emission` proof target,
  even though the packet named `tests/backend/bir` for optional focused tests.
- Keep future RV64 byval fixes on prepared `StackCopy` facts; do not add
  testcase-name matching or emitter-side stack-object suffix reconstruction.
- The dump expectation is intentionally stricter than the old failed snippet:
  it requires the explicit local-frame source selection and the byval stack-copy
  facts instead of reverting to `prior_preservation`.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_dump_riscv64_byval_preserved_pointer_args|backend_codegen_route_aarch64_byval_global_payload_address_call_boundary|backend_codegen_route_aarch64_hfa_global_payload_call_boundary|backend_codegen_route_aarch64_f128_hfa_global_payload_call_boundary|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_rv64_runtime_riscv64_byval_aggregate_fixed_call|backend_rv64_runtime_riscv64_byval_preserved_pointer_args|backend_rv64_runtime_riscv64_byval_formal_gpr_publication|backend_call_boundary_effect_plan|backend_riscv_object_emission)$'
```

Result: passed with the delegated 12-test scope, including
`backend_dump_riscv64_byval_preserved_pointer_args`. Canonical proof log:
`test_after.log`.
