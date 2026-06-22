Status: Active
Source Idea Path: ideas/open/307_rv64_text_only_fail_closed_output_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair No-Storage Prepared Function Emission

# Current Packet

## Just Finished

Implemented `plan.md` Step 2, "Repair No-Storage Prepared Function Emission":
`append_prepared_global_storage_asm` now lets non-thread-local, non-constant
extern globals with no initializer payload pass the RV64 prepared global
storage gate without emitting data. The target `src/00094.c` asm now reaches
prepared function emission and contains `.globl main`, `main:`, `li a0, 0`,
and `ret` instead of successful `.text`-only output.

## Suggested Next

Begin Step 3 by adding focused output-contract coverage for the no-storage
prepared `main` case so the old `.text`-only success cannot return unnoticed.

## Watchouts

- Do not special-case `src/00094.c`, `main`, exact source text, or c-testsuite
  case names.
- Do not broaden into string literal storage, external calls, aggregate
  globals, pointer globals, floating globals, scalar globals, libc calls, or
  full 93-case completion.
- Do not weaken expectations, mark the control unsupported, or accept empty
  `.text` as a supported result.
- The fix intentionally treats extern declaration storage as "no owned data" by
  requiring `is_extern`, no TLS, no constant/string-pool marker, and no scalar,
  symbol, or element initializer payload. It does not add data emission for
  storage definitions.
- The delegated proof's `ctest -R '^backend_'` run is blocked by
  `backend_riscv_prepared_edge_publication`, which constructs a prepared module
  directly and is outside the owned extern/global-storage surface.
- Supervisor-side matching backend regression guard accepted that failure as
  pre-existing: before and after both report `209 passed, 1 failed, 210 total`
  with no new failing tests.

## Proof

Proof command delegated by the supervisor:

```sh
{ cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c -o /tmp/rv64_00094_fixed.s && rg -n '^\s*\.globl\s+main|^main:|\bret\b|\ba0\b' /tmp/rv64_00094_fixed.s && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: build passed and the `src/00094.c` asm check passed. The delegated
`backend_` CTest subset still has the known
`backend_riscv_prepared_edge_publication` failure.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed. Matching backend logs both report `209 passed, 1 failed, 210
total`, with no new failing tests. Proof log: `test_after.log`.
