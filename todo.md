Status: Active
Source Idea Path: ideas/open/308_rv64_string_literals_and_extern_calls.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Emit String Constant Data

# Current Packet

## Just Finished

Completed `plan.md` Step 2, "Emit String Constant Data": taught RV64 prepared
global storage emission to accept prepared byte string constants that match BIR
`StringConstant` metadata and emit addressable `.rodata` labels with `.byte`
payloads plus the terminating zero.

## Suggested Next

Implement the next narrow packet by handling RV64 prepared string-constant
address materialization for prepared value `@.str0` / text `.str0`, without
broadening into call lowering.

## Watchouts

- Do not special-case `"hello"`, `strlen`, `.str0`, c-testsuite filenames, or
  exact source text.
- The prepared call plan for `strlen` is already `direct_extern_fixed_arity`.
  String storage now emits; the next blocker is RV64 handling for
  `StringConstant` address materialization in pointer local stores.
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
build succeeded and `src/00025.c` no longer hits the prepared global storage
layout diagnostic; `/tmp/rv64_00025_string_storage.s` contains `.str0:` followed
by `.byte 104, 101, 108, 108, 111, 0`. The proof exits nonzero because the
delegated `ctest -R 'backend_.*(rv64|riscv).*'` subset reports the existing
`backend_riscv_prepared_edge_publication` failure.

Supervisor regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: passed. Matching RV64/RISC-V logs both report `33 passed, 1 failed, 34
total`, with no new failing tests. Proof log: `test_after.log`.
