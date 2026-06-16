# AArch64 00204 stdarg Semantic BIR Local-Memory Admission

## Goal

Repair the AArch64-target semantic BIR path for the `00204.c` stdarg fixture so
`backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold` and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
can lower through `myprintf` instead of failing in the load local-memory
semantic family.

## Why This Exists

After the fp128/HFA vararg closure, the x86 00204 semantic/prepared dump and
runtime path pass, but the AArch64-target dump still fails before producing BIR:

```text
latest function failure: semantic lir_to_bir function 'myprintf' failed in load local-memory semantic family
```

The failing blocks are the `va_arg(ap)` aggregate loads inside `myprintf`.
This is a real admission gap in the AArch64-target semantic BIR route, not a
snapshot wording drift.

## In Scope

- Identify the exact LIR load/addressing shape emitted for AArch64-target
  `myprintf` in `00204.c`.
- Repair semantic BIR lowering for the relevant `va_list` / aggregate
  local-memory load shape using existing structured type, local-slot, pointer,
  or aggregate-layout carriers.
- Preserve the existing x86 00204 semantic/prepared dump behavior.
- Keep the prepared handoff publication contract structured; do not recover
  AArch64 facts from printed dump text.

## Out of Scope

- Broad AArch64 ABI rewrites unrelated to the failing `myprintf` local-memory
  load family.
- Expectation-only updates for #134 or #136.
- Downgrading the backend CLI dump tests to unsupported or weaker contracts.
- Named-case shortcuts that only recognize `00204.c`, `myprintf`, or specific
  HFA struct names.

## Acceptance Criteria

- `ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure` passes.
- The corresponding x86 00204 semantic/prepared dump tests remain green.
- A focused backend unit or dump assertion proves the repaired local-memory
  shape without depending on a named `00204.c` function.
- The fix does not bypass semantic BIR admission gates; the route must lower
  the previously rejected load shape.

## Reviewer Reject Signals

- The patch only changes required/forbidden snippets, CTest labels, or expected
  dump text while the AArch64-target command still fails in `myprintf`.
- The patch admits `00204.c`, `myprintf`, `movi`, or HFA structs by name instead
  of lowering a semantic local-memory or `va_arg` aggregate load shape.
- The patch routes around semantic BIR by using prepared-only, printed-BIR, or
  target-specific text recovery as the source of truth.
- The patch weakens or removes the x86 00204 dump coverage to make the AArch64
  route look monotonic.
- The exact old failure remains but is hidden behind a different diagnostic,
  bucket name, or skipped test.
