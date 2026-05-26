Status: Active
Source Idea Path: ideas/open/04_aarch64_prior_preservation_baseline_drift.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prove the Focused Prior-Preservation Path

# Current Packet

## Just Finished

Step 2: Prove the Focused Prior-Preservation Path completed as a focused
validation packet. The supervisor-selected build plus focused AArch64 prior-
preservation CTest subset was run into the canonical `test_after.log` path and
exited 0.

`test_after.log` records 5/5 tests passed with 0 failures:
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00173_c`,
`c_testsuite_aarch64_backend_src_00179_c`,
`c_testsuite_aarch64_backend_src_00186_c`, and
`c_testsuite_aarch64_backend_src_00187_c`.

## Suggested Next

Step 3 packet: run close-grade regression guard validation using matching
canonical regression evidence (`test_before.log` and `test_after.log`) and the
supervisor-selected close-grade command before lifecycle close handling.

## Watchouts

- Treat this as validation and closure-readiness, not fresh implementation.
- Do not revive the retired broad prior-preservation fallback.
- Do not weaken c_testsuite expectations or mark tests unsupported.
- Keep `00181`, `00216`, and `00204` classified as separate families unless
  fresh proof shows they are actually prior-preservation/source-selection drift.
- Focused Step 2 proof is green, but close readiness still needs Step 3
  close-grade regression guard evidence before lifecycle closure.

## Proof

Supervisor-run Step 2 focused proof command recorded in `test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00179_c|c_testsuite_aarch64_backend_src_00186_c|c_testsuite_aarch64_backend_src_00187_c)$' >> test_after.log 2>&1
```

Result from `test_after.log`: build had no work to do, CTest reported
`100% tests passed, 0 tests failed out of 5`.

Formatting proof for this todo update passed:

```sh
git diff --check -- todo.md
```
