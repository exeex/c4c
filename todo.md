Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 8
Current Step Title: Prove Fused-Compare Selected Operand Order

# Current Packet

## Just Finished

Step 8, fused-compare selected operand order, now has focused instruction
dispatch coverage and remains limited to fused-compare selected operand
materialization/order.

`lower_missing_fused_compare_operand_publication` accepts an optional preferred
target scratch, and fused-compare operand publication now prefers the second
reserved scratch for selected-value materializations when available. This keeps
the selected materialization distinct from the other compare operand while
preserving source operand order.

The instruction dispatch test now covers selected global-load materialization
feeding a fused compare as both the lhs and rhs operand. The probes derive the
selected materialization result register from the emitted select publication and
then check the final fused `cmp` operand position, avoiding assertions tied to
temporary register names.

## Suggested Next

Next packet: select one remaining dirty seam separately. Calls/outgoing-stack
publication and direct-edge `LoadLocal` publication remain unaccepted.

## Watchouts

- This packet accepts only the fused-compare selected operand materialization
  and operand-order seam in `dispatch_publication.cpp`/`.hpp` plus focused
  instruction dispatch coverage.
- Calls/outgoing-stack and direct-edge `LoadLocal` remain dirty and unaccepted.
- The fused-compare proof intentionally derives selected registers from the
  emitted publication and does not bind to concrete temporary register names.
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
