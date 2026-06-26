# RV64 Object Route Instruction Fragment Lowering

Status: Open
Type: Follow-up repair idea
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Goal

Repair the dominant RV64 prepared-object failure bucket where prepared BIR
instructions reach object emission but are rejected as unsupported instruction
fragments.

## Why This Exists

The 2026-06-26 reopened 354 classification found 385 failures with:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Representative case: `tests/c/external/gcc_torture/src/20000223-1.c`.

This is the largest remaining RV64 prepared-object bucket in the current
1467-case scan and fails after semantic handoff, inside target object lowering.

## In Scope

- Classify the concrete prepared instruction fragments in the 385-case bucket.
- Add semantic RV64 lowering for reusable instruction families, not named
  testcase shortcuts.
- Preserve the prepared-object contract boundary; do not move BIR ownership
  into the RV64 emitter.
- Prove at least representative cases plus nearby same-fragment cases.

## Out Of Scope

- Rewriting gcc_torture expectations or allowlists.
- Treating semantic `lir_to_bir` failures as this bucket.
- Broad assembler/object architecture rewrites unrelated to the observed
  unsupported instruction fragments.

## Acceptance

- `src/20000223-1.c` no longer fails with `unsupported_instruction_fragment`.
- A refreshed RV64 gcc_torture backend subset shows fewer failures in this
  bucket without increasing runtime mismatches.
- The implementation is covered by narrow backend proof and a matching
  before/after regression guard selected by the supervisor.

Useful proof shape:

```sh
STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

or a supervisor-selected temporary allowlist containing the representative and
same-fragment cases.

## Reviewer Reject Signals

- Reject testcase-shaped matching on `20000223-1.c` or source filename.
- Reject lowering that merely renames the diagnostic while the same prepared
  instruction family still cannot emit RV64 object code.
- Reject expectation downgrades, unsupported markers, or allowlist filtering.
- Reject changes that make instruction fragments pass by reconstructing BIR
  control/data-flow semantics inside the RV64 object emitter.
- Reject a green single case if nearby same-fragment cases still fail with the
  identical unsupported instruction diagnostic and were not examined.

## Lifecycle Notes

- 2026-06-26: Step 4 classified `src/20000403-1.c` as a producer-side scalar
  formal ABI publication gap, not an RV64 object-emitter instruction-fragment
  lowering gap. The `i16 %p.win` formal in `seqgt` and `seqgt2` reaches object
  emission as a stack-homed parameter without usable prepared scalar integer
  ABI facts. This work was split to
  `ideas/open/403_prepared_i16_formal_abi_publication.md`; do not repair it by
  reconstructing missing incoming-argument ABI policy in the RV64 object
  emitter.
