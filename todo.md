Status: Active
Source Idea Path: ideas/open/353_c_testsuite_rv64_asm_objdump_roundtrip_scan_target.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Opt-In Build Integration

# Current Packet

## Just Finished

Step 3 added the explicit opt-in CMake target
`rv64_c_testsuite_asm_roundtrip_scan` in `tests/backend/CMakeLists.txt`.
The target depends on `c4cll`, `c4c-as`, and `c4c-objdump`, passes their
`$<TARGET_FILE:...>` paths into the Step 2 runner, uses
`tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt`, and writes
case outputs under `build/tests/backend/rv64_c_testsuite_asm_roundtrip_scan`.
It is a custom build target only and is not registered with `add_test`.

## Suggested Next

Step 4: Make the allowlist and unsupported families explicit. Suggested owned
files are `tests/backend/rv64/rv64_c_testsuite_asm_roundtrip_allowlist.txt`,
the narrowest nearby documentation or manifest file needed for unsupported
families, and `todo.md`.

## Watchouts

- Keep the large scan opt-in and out of default full CTest.
- The initial scan allowlist is intentionally tiny: `src/00001.c` and
  `src/00002.c`.
- `src/00003.c` currently reaches `c4c-as-pass1` but fails because generated
  RV64 assembly contains unsupported pseudo-instruction `mv a0, s1`; keep this
  as an explicit exclusion until assembler or printer support changes.
- The Step 3 target is intentionally not part of default CTest membership;
  `ctest --test-dir build -N | rg 'rv64_c_testsuite_asm_roundtrip_scan|c_testsuite_asm_roundtrip_scan'`
  found no matching registered test.

## Proof

Delegated Step 3 proof passed and wrote `test_after.log`:

```sh
cmake --build build --target rv64_c_testsuite_asm_roundtrip_scan >test_after.log 2>&1
```

`test_after.log` shows CMake regenerating after the target addition, then
running both allowlisted cases through `c4cll`, three `c4c-as` passes, and two
`c4c-objdump` passes. It finishes with
`[PASS][rv64-c-testsuite-roundtrip-scan] cases=2`.
