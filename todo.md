Status: Active
Source Idea Path: ideas/open/307_rv64_text_only_fail_closed_output_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Output-Contract Coverage

# Current Packet

## Just Finished

Implemented `plan.md` Step 3, "Add Focused Output-Contract Coverage": added
`backend_codegen_route_riscv64_external_no_storage_main_emits_return_path`,
a focused backend route regression for `tests/c/external/c-testsuite/src/00094.c`
that requires `.globl main`, `main:`, `li a0, 0`, and `ret` in RV64 assembly.
This fails the old `.text`-only success mode without registering the full
c-testsuite or weakening any expectations.

## Suggested Next

Begin Step 4 by rechecking secondary representatives (`src/00024.c`,
`src/00025.c`, `src/00045.c`, and `src/00119.c`) so their next visible failure
modes are recorded instead of hidden behind undefined `main`.

## Watchouts

- The new test intentionally uses `src/00094.c` as the focused output-contract
  control requested for this packet; do not expand this into broad c-testsuite
  registration in the same slice.
- Do not broaden into string literal storage, external calls, aggregate
  globals, pointer globals, floating globals, scalar globals, libc calls, or
  full 93-case completion.
- Do not weaken expectations, mark the control unsupported, or accept empty
  `.text` as a supported result.
- The delegated proof's RV64/RISC-V CTest subset is still blocked by
  `backend_riscv_prepared_edge_publication`, which was already recorded as
  outside the owned extern/global-storage surface.
- The new regression itself passed in that subset as
  `backend_codegen_route_riscv64_external_no_storage_main_emits_return_path`.
- Supervisor-side matching RV64/RISC-V regression guard accepted the known
  publication failure and confirmed the new focused test adds one passing case
  with no new failures.

## Proof

Proof command delegated by the supervisor:

```sh
{ cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c -o /tmp/rv64_00094_coverage.s && rg -n '^\s*\.globl\s+main|^main:|\bret\b|\ba0\b' /tmp/rv64_00094_coverage.s && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*'; } > test_after.log 2>&1
```

Result: build passed; the direct `src/00094.c` asm check found `.globl main`,
`main:`, `li a0, 0`, and `ret`; and the new focused CTest regression passed.
The delegated CTest subset returned nonzero only because
`backend_riscv_prepared_edge_publication` still fails.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: passed. Matching RV64/RISC-V logs changed from `32 passed, 1 failed,
33 total` to `33 passed, 1 failed, 34 total`, with no new failing tests. Proof
log: `test_after.log`.
