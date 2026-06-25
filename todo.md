Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make The Allowlist And Unsupported Families Explicit

# Current Packet

## Just Finished

Step 4 made the initial RV64 c-testsuite asm/objdump roundtrip scan scope
explicit. `tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt`
now names the two selected cases and points to
`tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_unsupported.md` for known
exclusions. The unsupported-family note records `src/00003.c` as excluded
because generated RV64 assembly currently contains unsupported
pseudo-instruction `mv a0, s1`, failing at `c4c-as-pass1`.

## Suggested Next

Step 5: Prove and document the opt-in scan. Suggested owned files are the
narrowest nearby README, contract, or CMake comments needed to make the manual
command discoverable, plus `todo.md`. Preserve the target as opt-in and out of
default CTest membership.

## Watchouts

- Keep the large scan opt-in and out of default full CTest.
- The initial scan allowlist remains intentionally tiny: `src/00001.c` and
  `src/00002.c`.
- The unsupported-family note is factual, not exhaustive; unlisted c-testsuite
  cases should still be classified before being added to the allowlist.
- `src/00003.c` is the current representative unsupported case for the `mv`
  pseudo-instruction gap.

## Proof

Delegated Step 4 proof passed and wrote `test_after.log`:

```sh
cmake --build build --target rv64_c_testsuite_asm_roundtrip_scan >test_after.log 2>&1
```

`test_after.log` shows both allowlisted cases running through `c4cll`, three
`c4c-as` passes, and two `c4c-objdump` passes. It finishes with
`[PASS][rv64-c-testsuite-roundtrip-scan] cases=2`.
