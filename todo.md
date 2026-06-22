Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize String Constant Addresses

# Current Packet

## Just Finished

Completed `plan.md` Step 3, "Materialize String Constant Addresses": taught
RV64 prepared local memory emission to materialize prepared string-constant
pointer addresses from prepared metadata and to publish prepared pointer
register sources into local frame slots.

## Suggested Next

Implement the next narrow direct extern-call packet. `src/00025.c` now reaches
the direct extern-call path with `.str0` materialized and stored; the next
packet should own any remaining extern-call/declaration behavior without
folding it into local-memory emission.

## Watchouts

- Do not special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or
  exact source text.
- The prepared call plan for `strlen` is already `direct_extern_fixed_arity`.
  String storage and local string-address materialization now emit; keep any
  further call/declaration work in the direct extern-call packet.
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
- Supervisor-side matching RV64/RISC-V regression guard accepted that failure
  as pre-existing: before and after both report `33 passed, 1 failed, 34
  total`, with no new failing tests.

## Proof

Ran the supervisor-selected proof exactly; `test_after.log` is preserved. The
build succeeded and `src/00025.c` no longer hits the string-address/local
pointer store blocker. `/tmp/rv64_00025_string_address.s` contains `.str0:`,
`lla t0, .str0`, `sd t0, 0(sp)`, `ld t0, 0(sp)`, and `call strlen`.

The proof exits nonzero because the delegated
`ctest -R 'backend_.*(rv64|riscv).*'` subset reports the existing
`backend_riscv_prepared_edge_publication` failure.

The RV64/RISC-V subset reports `33 passed, 1 failed, 34 total`, matching the
known `backend_riscv_prepared_edge_publication` failure noted above. Proof log:
`test_after.log`.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed. Matching RV64/RISC-V logs both report `33 passed, 1 failed, 34
total`, with no new failing tests.
