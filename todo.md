Status: Active
Source Idea Path: ideas/open/296_rv64_runtime_scalar_local_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Register And Prove `return_zero.c`

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/296_rv64_runtime_scalar_local_foundation.md`.

## Suggested Next

Execute Step 1 from `plan.md`: register `return_zero.c` with
`c4c_add_backend_rv64_runtime_case(...)`, build as needed, and run
`ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`.

## Watchouts

- Keep qemu runtime execution as the acceptance boundary.
- Do not accept BIR or LLVM fallback text as RISC-V assembly.
- Do not add testcase-name or exact-source-shape shortcuts.

## Proof

Not run. Lifecycle activation only.
