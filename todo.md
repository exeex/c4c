Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Named Scalar Return Support For `return_add.c`

# Current Packet

## Just Finished

Step 2 from `plan.md` completed: `return_add.c` is registered in the rv64 qemu
runtime subset, the rv64 emitter handles the semantic scalar path for this
case, and `c4c_add_backend_rv64_runtime_case(...)` now supports a generic
`EXPECTED_RUN_CODE` argument that defaults to zero.

`backend_rv64_runtime_return_add` expects exit code 5 and passes under qemu
without accepting fallback BIR or LLVM text.

## Suggested Next

Execute Step 3 from `plan.md`: register `return_add_sub_chain.c` in the rv64
runtime subset, then generalize binary-op value tracking only as needed for the
selected scalar arithmetic chain.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.
- `EXPECTED_RUN_CODE` is a generic harness contract for qemu process status; it
  should not be used to hide wrong semantics.
- The supervisor-noted baseline failure
  `backend_riscv_prepared_edge_publication` still appears in the delegated
  proof subset.

## Proof

Passed for this packet with the expected widened-subset baseline failure:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime|backend_riscv|backend_codegen_route_riscv64'\'' --output-on-failure' 2>&1 | tee test_after.log`

All 12 `backend_rv64_runtime_*` cases passed, including
`backend_rv64_runtime_return_add`. The only failing test in the delegated
widened subset was the known pre-existing
`backend_riscv_prepared_edge_publication` failure. Proof log:
`test_after.log`.
