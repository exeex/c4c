Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 9
Current Step Title: Prove Call And Outgoing Stack Argument Materialization

# Current Packet

## Just Finished

Step 9, call and outgoing stack argument materialization, now has focused
coverage for the prepared call-plan and move-bundle lookup seams.

`calls.cpp` now falls back to the retained `PreparedCallPlan` argument list when
immediate stack-argument binding lookup has no indexed call-plan result, and it
scans the selected before-call move bundle when frame-slot argument move lookup
has no indexed move result. The focused instruction dispatch test proves the
immediate outgoing stack argument and frame-slot stack argument routes without
binding assertions to exact scratch register names.

## Suggested Next

Next packet: isolate direct-edge `LoadLocal` publication separately, or leave it
unaccepted if no focused proof can be made without broad dispatch-only evidence.

## Watchouts

- This packet accepts only the call/outgoing stack argument materialization seam
  in `calls.cpp`, plus focused instruction dispatch coverage in
  `backend_aarch64_instruction_dispatch_test.cpp`.
- Direct-edge `LoadLocal` publication remains dirty and unaccepted.
- The focused stack-argument probes check prepared offsets, outgoing area base,
  and store-before-call ordering instead of exact temporary register names.
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
