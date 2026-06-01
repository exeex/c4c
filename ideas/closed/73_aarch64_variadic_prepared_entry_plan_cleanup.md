# AArch64 Variadic Prepared Entry Plan Cleanup

## Goal

Make AArch64 variadic lowering consume prepared variadic entry-plan and helper
operand-home facts directly before emitting target-local va_list, save-area,
va_arg, and va_copy records.

## Why This Exists

The large-owner residue audit classified variadic entry plans and helper
operand homes as `consume-shared`, while classifying save-area instruction
selection, va_arg/va_copy record emission, and va_list layout spelling as
`target-emission`. The cleanup should narrow AArch64 variadic code to target
ABI layout and instruction records, not prepared policy reconstruction.

## Owned Files

- `src/backend/mir/aarch64/codegen/variadic.cpp`
- Shared prepared authority call sites only where needed to consume existing
  facts:
  - `src/backend/prealloc/variadic_entry_plans.*`
  - `src/backend/prealloc/variadic.hpp`
  - relevant prepared value-home/addressing lookup surfaces

## In Scope

- Consume `PreparedVariadicEntryPlanFunction` and
  `PreparedVariadicEntryHelperOperandHomes`.
- Remove local re-decision of helper operand homes or entry-plan policy.
- Preserve AArch64 GP/FP save-area instruction selection, va_arg/va_copy
  record emission, va_list layout spelling, and concrete ABI save-area
  addressing.
- Prove variadic entry, va_arg, va_copy, and helper operand-home cases still
  lower through the prepared plan.

## Out Of Scope

- Moving AArch64 va_list layout spelling or save-area instruction selection
  into shared prealloc.
- Combining this route with i128/f128 helper policy cleanup.
- Changing variadic ABI behavior to make one testcase pass.
- General memory address authority cleanup beyond the prepared facts needed by
  variadic lowering.

## Proof Expectations

- Focused AArch64 variadic tests or CTest subset covering entry plans, helper
  operand homes, GP/FP save areas, va_arg, and va_copy.
- Evidence that prepared variadic entry facts are consumed before local record
  construction.
- Regression guard logs for acceptance-sized slices.

## Reviewer Reject Signals

- Local code continues to reconstruct variadic entry-plan or helper
  operand-home policy under a renamed helper.
- The route weakens variadic expectations or marks supported cases
  unsupported without explicit approval.
- AArch64 va_list layout spelling or save-area instruction selection is moved
  into shared code.
- The implementation is a named-case shortcut for one variadic testcase.
- Unrelated special-carrier, memory, or call cleanup is mixed into this route.

## Completion Note

Closed after reviewer follow-up confirmed the implementation matches the source
idea: AArch64 variadic lowering consumes prepared entry/helper facts while
retaining target-local va_list layout, save-area instruction selection, and
va_arg/va_copy record emission.

Close proof used canonical backend regression logs:

```bash
cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Regression guard result: passed with 169/169 tests before and 169/169 tests
after, no new failures.
