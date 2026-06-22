Status: Active
Source Idea Path: ideas/open/307_rv64_text_only_fail_closed_output_contract.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Recheck Secondary Representatives and Fail-Closed Behavior

# Current Packet

## Just Finished

Completed the `plan.md` Step 4 fail-closed repair in
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp`: when
`append_prepared_global_storage_asm` rejects unsupported prepared global
storage, `emit_prepared_module_text` now throws a RISC-V prepared global
storage unsupported error instead of returning successful `.text`-only output.

The four secondary representatives `src/00024.c`, `src/00025.c`, `src/00045.c`,
and `src/00119.c` now fail during `c4cll` emission with the explicit
unsupported prepared global storage diagnostic. The no-storage control
`src/00094.c` still emits `.globl main`, `main:`, `li a0, 0`, and `ret`.

## Suggested Next

Begin Step 5 closure readiness: verify the no-storage control, focused
regression, and secondary representative fail-closed behavior satisfy the
source idea acceptance criteria without claiming secondary feature completion.

## Watchouts

- `src/00024.c` represents aggregate global storage and struct field stores.
- `src/00025.c` represents string literal storage plus an external libc call.
- `src/00045.c` represents initialized scalar globals, pointer global
  initialization, and pointer dereference.
- `src/00119.c` represents floating global storage and double comparison.
- Do not implement secondary feature repairs as part of this recheck packet.
- The fail-closed change does not add aggregate, string, pointer, floating,
  scalar-global, libc-call, or full c-testsuite feature support.
- The delegated `ctest -R 'backend_.*(rv64|riscv).*'` subset currently fails
  `backend_riscv_prepared_edge_publication`; that fixture has no globals and
  fails its register edge-move expectation, not the new global-storage
  diagnostic.
- Supervisor-side matching RV64/RISC-V regression guard accepted that failure
  as pre-existing: before and after both report `33 passed, 1 failed, 34
  total`, with no new failing tests.

## Proof

Proof command delegated by the supervisor:

```sh
{ cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c -o /tmp/rv64_00094_still_ok.s && rg -n '^\s*\.globl\s+main|^main:|\bret\b|\ba0\b' /tmp/rv64_00094_still_ok.s && for n in 00024 00025 00045 00119; do src="tests/c/external/c-testsuite/src/${n}.c"; out="/tmp/rv64_${n}_unsupported.s"; log="/tmp/rv64_${n}_unsupported.log"; if ./build/c4cll --codegen asm --target riscv64-linux-gnu "$src" -o "$out" > "$log" 2>&1; then echo "unexpected success for ${n}"; sed -n '1,20p' "$out"; exit 1; fi; rg -n 'unsupported|global|storage|riscv|prepared' "$log"; done && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*'; } > test_after.log 2>&1
```

Result: failed with exit code 8 because the final CTest subset reported
`backend_riscv_prepared_edge_publication` failed its existing register
edge-move expectation. Earlier portions of the same proof command passed:
the build succeeded, `src/00094.c` emitted `main`, and all four secondary
representatives failed closed with the unsupported prepared global storage
diagnostic.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed. Matching RV64/RISC-V logs both report `33 passed, 1 failed, 34
total`, with no new failing tests. Proof log: `test_after.log`.
