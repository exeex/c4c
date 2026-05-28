Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 10
Current Step Title: Prove Direct Edge LoadLocal Prepared Memory Consumption

# Current Packet

## Just Finished

Step 10, direct-edge `LoadLocal` prepared source-memory consumption, now has
focused coverage and the code seam fails closed when the prepared publication
does not carry matching source-memory authority.

`dispatch_edge_copies.cpp` now treats a matching direct-edge `LoadLocal`
prepared publication as requiring the prepared source-memory record before it
can materialize the edge load. The focused instruction dispatch probe consumes
the prepared frame-slot source-memory address for the direct edge and verifies
that removing the prepared source-memory access rejects the route without
falling back to the source register/home.

## Suggested Next

Next packet: supervisor should decide whether the remaining active plan state is
complete enough for review/closure or whether another isolated publication seam
needs a bounded executor packet.

## Watchouts

- This packet accepts only the direct-edge `LoadLocal` prepared source-memory
  seam in `dispatch_edge_copies.cpp`; the implementation change remains limited
  to that file.
- The focused direct-edge probe checks prepared source-memory consumption and
  fail-closed behavior without binding to exact temporary register names.
- `c_testsuite_aarch64_backend_src_00196_c` still fails with the existing
  runtime mismatch (`joe() && fred()` cases print `1` instead of `0`).

## Proof

Ran the delegated proof command and preserved `test_after.log`:

```sh
(cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00196_c|c_testsuite_aarch64_backend_src_00207_c)$') > test_after.log 2>&1
```

Result: build passed. `backend_aarch64_instruction_dispatch`,
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
`c_testsuite_aarch64_backend_src_00207_c` passed.
`c_testsuite_aarch64_backend_src_00196_c` failed with the known baseline
runtime mismatch recorded above. `test_after.log` is preserved with the exact
delegated proof output.
