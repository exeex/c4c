Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Register And Prove `return_zero.c`

# Current Packet

## Just Finished

Step 1 from `plan.md` completed: `return_zero.c` is registered in the rv64
qemu runtime subset through `c4c_add_backend_rv64_runtime_case(...)`, and
`backend_rv64_runtime_return_zero` passed with the existing rv64 runtime cases.

## Suggested Next

Execute Step 2 from `plan.md`: register `return_add.c` in the rv64 runtime
subset, inspect the prepared BIR if it fails, and add the smallest semantic
rv64 emitter support needed for named scalar return values.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.
- No fallback BIR/LLVM acceptance was introduced in Step 1.

## Proof

Passed:
`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -R '\''^backend_rv64_runtime'\'' --output-on-failure' 2>&1 | tee test_after.log`

Proof log: `test_after.log`.
