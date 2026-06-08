# Current Packet

Status: Active
Source Idea Path: ideas/open/129_aarch64_i128_shift_support_completeness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Complete Or Contract Shift-By-64-Or-More

## Just Finished

Step 2: Complete Or Contract Shift-By-64-Or-More completed for AArch64
immediate i128 shifts.

- `print_i128_shift()` now supports immediate counts `64..127` for `Shl`,
  `LShr`, and `AShr` while preserving the existing non-immediate diagnostic.
- `Shl >= 64` zeroes the low lane and moves or left-shifts the old low lane
  into the high lane.
- `LShr >= 64` moves or logical-shifts the old high lane into the low lane and
  zeroes the high lane.
- `AShr >= 64` moves or arithmetic-shifts the old high lane into the low lane
  and sign-fills the high lane with `asr #63`.
- Focused record, printer, and dispatch coverage now proves representative
  `== 64` and `> 64` counts for all three immediate shift kinds.

## Suggested Next

Execute Step 3 by completing or explicitly contracting variable-count i128
shifts. Keep that packet separate from the immediate-count support now covered
by Step 2.

## Watchouts

- Keep the route target-local to AArch64 i128 shifts.
- Do not weaken supported-path expectations or add named-testcase shortcuts.
- Do not expand into f128, helper ABI/resource policy, carrier ownership, or
  shared BIR/prealloc policy.
- Variable-count support still has the previous mixed contract: records can be
  built for register-backed counts, but the printer still rejects
  non-immediate counts with the existing immediate-only diagnostic.
- `clang-format` was not available in this environment, so the touched C++
  files were not tool-formatted after editing.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` contains 3/3 passing tests:
`backend_aarch64_target_instruction_records`, `backend_aarch64_machine_printer`,
and `backend_aarch64_instruction_dispatch`.
