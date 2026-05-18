Status: Active
Source Idea Path: ideas/open/281_aarch64_address_exposed_local_pointer_runtime_nonzero.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Address-Exposed Local Ownership

# Current Packet

## Just Finished

Lifecycle transition completed: idea 278 was closed after matching-scope
close-gate logs passed, and idea 281 was activated as the first focused
follow-up after the repaired `00001.c`/`00002.c`/`00003.c` route.

## Suggested Next

Execute Step 1 from `plan.md`: localize the owner for the `00004.c` and
`00005.c` AArch64 backend `[RUNTIME_NONZERO] exit=1` failures, starting from
address-exposed local storage, pointer value materialization, load/store
address formation, and prepared memory operand consumption.

## Watchouts

- Do not fold `00006.c` loop/branch control lowering into this plan; it belongs
  to `ideas/open/282_aarch64_loop_branch_control_runtime_hang.md`.
- Do not change c-testsuite expectations, allowlists, unsupported
  classifications, CTest contracts, runner files, or route diagnostics.
- Do not add filename-specific handling for `00004.c` or `00005.c`.
- Do not route proof through LLVM IR fallback while claiming AArch64 backend
  progress.
- Preserve idea 278 scalar local operand materialization for `00001.c`,
  `00002.c`, and `00003.c`.

## Proof

Close-gate command for idea 278:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. `test_before.log` and `test_after.log` both cover the focused
nine-test AArch64 backend subset plus
`c_testsuite_aarch64_backend_src_(00001|00002|00003)_c`; the parsed final
CTest summary reports 3 passed, 0 failed before and after, with zero new
failures.
