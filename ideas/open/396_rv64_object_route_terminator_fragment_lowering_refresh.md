# RV64 Object Route Terminator Fragment Lowering Refresh

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair the current RV64 prepared-object terminator-fragment bucket so prepared
control-flow exits can lower to valid RV64 object code.

## Why This Exists

The 2026-06-26 reopened 354 classification found 88 failures with:

```text
unsupported_terminator_fragment: BIR terminator requires unsupported RV64 object lowering
```

Representative case: `tests/c/external/gcc_torture/src/20020206-2.c`.

Earlier terminator work closed, but the latest full scan still exposes a
current unowned terminator-fragment family.

## In Scope

- Classify the rejected prepared terminators and their operand shapes.
- Implement reusable RV64 terminator lowering for the prepared forms.
- Preserve correct branch/jump/fallthrough semantics and relocation behavior.
- Prove representative and nearby terminator-fragment cases.

## Out Of Scope

- Reopening broad CFG ownership or adding BIR reconstruction inside object
  emission.
- Fixing semantic `lir_to_bir` scalar-control-flow failures that never reach
  prepared terminators.
- Downgrading torture cases to unsupported.

## Acceptance

- `src/20020206-2.c` no longer fails with
  `unsupported_terminator_fragment`.
- A refreshed subset shows the terminator bucket count decreases and no new
  backend runtime mismatches are introduced.
- The proof includes at least one conditional and one unconditional/return
  terminator shape if both are present in the bucket.

## Reviewer Reject Signals

- Reject filename-specific branches or single-testcase terminator shortcuts.
- Reject changes that weaken branch correctness or elide a terminator instead
  of lowering it.
- Reject proof that only checks object compilation when the lowered case can
  be linked and run through qemu.
- Reject broad route rewrites that absorb semantic CFG lowering into the RV64
  backend.
