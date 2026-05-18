Status: Active
Source Idea Path: ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconfirm Focused Runtime Route

# Current Packet

## Just Finished

Step 1 reconfirmed the focused AArch64 c-testsuite backend runtime route after
idea 282 closure. The focused backend tests are green, and
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c` all
pass through the real backend runtime path.

Route-stage evidence from
`tests/c/external/c-testsuite-aarch64-backend-runner.cmake`:

- compile source with `c4cll --codegen asm --target aarch64-unknown-linux-gnu`
  into the backend output path
- require backend output to exist and begin as assembly via the `.text` check,
  otherwise fail as `[BACKEND_FALLBACK_IR]`
- assemble/link with `clang --target=aarch64-unknown-linux-gnu -x assembler`
  into an executable
- run the executable directly on native AArch64, or fail closed as
  `[RUNTIME_UNAVAILABLE]` when no runner is configured for non-AArch64 hosts
- compare runtime stdout/stderr with the `.expected` sidecar and fail as
  `[RUNTIME_MISMATCH]` on differences

Generated artifacts exist for the focused route, including
`build-aarch64-scan/c_testsuite_aarch64_backend/src/00001.c.s`,
`00001.c.bin`, `00006.c.s`, and `00006.c.bin`. This proof did not use LLVM IR
fallback, c-testsuite expectation changes, allowlist changes, unsupported
classification weakening, or runner changes.

## Suggested Next

Execute Step 2 from `plan.md`: run a broad AArch64 backend inventory so idea
276 can classify the remaining route surface beyond the focused six-case
smoke. Suggested command:

```sh
set -o pipefail; { cmake --build --preset default && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not fold backend semantic repairs directly into idea 276; split remaining
  capability gaps into focused follow-up ideas.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Preserve distinct route-stage classifications.
- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, runner files, or timeout policy to claim route progress.
- `ideas/open/277_aarch64_backend_result_register_runtime_nonzero.md` remains
  open but inactive; do not reactivate it unless supervisor chooses to close or
  repair that lifecycle record separately.

## Proof

Step 1 proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_branch_control_lowering|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_operand_resolution)$' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c$'; } 2>&1 | tee test_after.log
```

Result: PASS, exit 0. The seven focused backend tests passed, and
`c_testsuite_aarch64_backend_src_(00001|00002|00003|00004|00005|00006)_c` all
passed through the AArch64 backend runtime route. `test_after.log` contains the
full command output.
