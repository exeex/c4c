Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Direct Fixed-Arity Extern Calls

# Current Packet

## Just Finished

Completed `plan.md` Step 4, "Lower Direct Fixed-Arity Extern Calls": added a
focused RV64 backend route test for `tests/c/external/c-testsuite/src/00025.c`.
The existing implementation already emits string bytes, materializes `.str0`
through the local pointer slot, passes the reloaded pointer to `strlen`, and
uses the integer return value in the `strlen(p) - 5` expression.

## Suggested Next

Supervisor should decide whether Step 4 is complete enough to advance to the
next plan step or whether the route now needs review against the source idea.

## Watchouts

- Do not special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or
  exact source text.
- No call-lowering implementation change was needed for this packet; the
  focused test passed as coverage-only.
- The delegated proof target `c4c_backend_tests` was missing from the configured
  build; `tests/backend/CMakeLists.txt` now defines it as an aggregate over the
  existing backend MIR/BIR test targets so the delegated proof command can run.
- Do not make string globals look like no-storage externs; the bytes need
  emitted addressable storage.
- Do not broaden into aggregate, pointer, floating, scalar global storage,
  variadic calls, broad libc conformance, or full c-testsuite pass-count work.
- Do not weaken expectations, mark supported-path cases unsupported, or claim
  progress through classification-only changes.
- Keep scratch logs out of the repository root.
- The delegated RV64 CTest subset still has one failing test,
  `backend_riscv_prepared_edge_publication`, reporting that the prepared module
  should emit a register edge move. That appears outside this storage packet.
- The matching before run had `33 passed, 1 failed, 34 total`; after adding this
  route test the delegated subset reports `34 passed, 1 failed, 35 total`, with
  the same `backend_riscv_prepared_edge_publication` failure.

## Proof

Ran the supervisor-selected proof exactly; `test_after.log` is preserved.

The proof exits nonzero because the delegated
`ctest -R 'backend_.*(rv64|riscv).*'` subset reports the existing
`backend_riscv_prepared_edge_publication` failure.

```sh
cmake --build build --target c4c_backend_tests c4cll -j 2 > /tmp/c4c_step4_build.log 2>&1 && ctest --test-dir build -j --output-on-failure -R 'backend_.*(rv64|riscv).*' > test_after.log 2>&1
```

Result: build succeeded; CTest exited nonzero with the known
`backend_riscv_prepared_edge_publication` failure. The new
`backend_codegen_route_riscv64_external_string_literal_strlen_direct_call` test
passed. RV64/RISC-V subset summary: `34 passed, 1 failed, 35 total`. Proof log:
`test_after.log`.
