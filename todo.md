# Current Packet

Status: Active
Source Idea Path: ideas/open/129_aarch64_i128_shift_support_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete Or Contract Variable-Count Shifts

## Just Finished

Step 3: Complete Or Contract Variable-Count Shifts completed for AArch64
i128 shifts.

- Variable-count `Shl`, `LShr`, and `AShr` keep their current record/dispatch
  contract: prepared register-backed counts are represented as
  `I128ShiftCountKind::Register` and preserve the count register as a machine
  use.
- The AArch64 i128 printer now rejects register-count shifts with the narrow
  diagnostic `i128 variable-count shifts are not supported by the current
  AArch64 i128 printer contract`.
- This packet intentionally contracts variable-count printing rather than
  emitting a partial lane route, because semantically correct variable-count
  i128 shifts require a full count-dependent cross-lane sequence rather than
  the immediate-only `lsl`/`lsr`/`asr`/`extr` route.
- Focused record, printer, and dispatch coverage now proves the explicit
  register-count contract for all three shift kinds without revisiting large
  immediate behavior.

## Suggested Next

Supervisor should decide whether the active runbook is exhausted and route
plan-owner review/closure, or delegate any remaining source-idea follow-up if
the plan still has non-complete steps.

## Watchouts

- Keep the route target-local to AArch64 i128 shifts.
- Do not weaken supported-path expectations or add named-testcase shortcuts.
- Do not expand into f128, helper ABI/resource policy, carrier ownership, or
  shared BIR/prealloc policy.
- Variable-count support is explicitly record/dispatch-only for now; printer
  support remains contracted until a semantic count-dependent lowering is
  designed.
- `clang-format` was not available in this environment, so the touched C++
  files were not tool-formatted after editing.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)' --output-on-failure > test_after.log 2>&1`

Result: passed. `test_after.log` contains 3/3 passing tests:
`backend_aarch64_target_instruction_records`, `backend_aarch64_machine_printer`,
and `backend_aarch64_instruction_dispatch`.
