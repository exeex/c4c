Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Recheck Representative Bucket Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 5, "Recheck Representative Bucket Cases": reran the
delegated representative RV64 string-literal/extern-call recheck with
`build/c4cll --codegen asm` for `src/00025.c` and nearby candidates `00026`,
`00056`, `00058`, `00112`, `00125`, `00131`, `00132`, `00137`, and `00138`.

`build/rv64_string_extern_recheck/summary.md` records real current outcomes for
all listed cases. Every case emitted assembly successfully. The representative
`src/00025.c` visibly emits past the old string-address/direct-call failure
mode: the summary shows `.str0`, `.byte 104, 101, 108, 108, 111, 0`, `.text`,
`main:`, `lla t0, .str0`, and `call strlen`.

## Suggested Next

Supervisor should decide whether Step 5 completes the active runbook or whether
the route now needs review against the source idea before lifecycle closure.

## Watchouts

- Do not special-case `strlen`, `hello`, `.str0`, `00025.c`, or exact source
  text.
- Do not weaken expectations, mark supported-path cases unsupported, or claim
  broader c-testsuite support beyond the recorded representative evidence.
- The delegated RV64 CTest subset still has one failing test,
  `backend_riscv_prepared_edge_publication`, reporting that the prepared module
  should emit a register edge move. That matches the known pre-existing failure
  named by the supervisor.
- The CTest subset summary remains `34 passed, 1 failed, 35 total`.
- This was an evidence-only packet; no lowering repairs or expectation changes
  were attempted.

## Proof

Ran the supervisor-selected proof exactly; `test_after.log` is preserved.

The proof exited nonzero only because the delegated `ctest -R
'backend_.*(rv64|riscv).*'` subset reports the known
`backend_riscv_prepared_edge_publication` failure. The build target completed
(`/tmp/c4c_step5_build.log` ends with `ninja: no work to do.`), the
representative recheck emitted assembly for every listed candidate, and the
summary was written to `build/rv64_string_extern_recheck/summary.md`.

```sh
cmake --build build --target c4c_backend_tests c4cll -j 2 > /tmp/c4c_step5_build.log 2>&1 && rm -rf build/rv64_string_extern_recheck && mkdir -p build/rv64_string_extern_recheck && { printf '# RV64 String/Extern Representative Recheck\n\n'; for case in 00025 00026 00056 00058 00112 00125 00131 00132 00137 00138; do src="tests/c/external/c-testsuite/src/${case}.c"; out="build/rv64_string_extern_recheck/${case}.s"; err="build/rv64_string_extern_recheck/${case}.err"; printf '## src/%s.c\n\n' "$case"; if build/c4cll --codegen asm --target riscv64-linux-gnu "$src" -o "$out" >"$err" 2>&1; then printf 'status: emit-ok\n'; { rg -n '(^main:|\.str[0-9]+:|\.byte |lla |call |does not support|unsupported|\.text)' "$out" || true; } | sed 's/^/asm: /'; else status=$?; printf 'status: emit-fail exit=%s\n' "$status"; sed -n '1,24p' "$err" | sed 's/^/diag: /'; fi; printf '\n'; done; } > build/rv64_string_extern_recheck/summary.md && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*' > test_after.log 2>&1
```

Result: build succeeded; all ten representative recheck candidates emitted
assembly successfully; CTest exited nonzero with only the known
`backend_riscv_prepared_edge_publication` failure. RV64/RISC-V subset summary:
`34 passed, 1 failed, 35 total`. Proof log: `test_after.log`; recheck summary:
`build/rv64_string_extern_recheck/summary.md`.
