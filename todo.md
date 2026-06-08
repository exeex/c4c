# Current Packet

Status: Active
Source Idea Path: ideas/open/129_aarch64_i128_shift_support_completeness.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consolidate Proof And Guard Against Drift

## Just Finished

Step 4: Consolidate Proof And Guard Against Drift completed for the AArch64
i128 shift route.

- Final route proof ran the supervisor-selected backend AArch64 subset after
  the large-immediate and variable-count packets.
- Route-local changed files since plan activation stayed within AArch64 i128
  shift lowering and focused backend AArch64 tests:
  `src/backend/mir/aarch64/codegen/i128_ops.cpp`,
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`, and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, plus
  canonical `todo.md` execution state.
- No f128, helper ABI/resource, carrier ownership, shared BIR, or prealloc
  policy files were changed for this idea.
- Large immediate i128 shifts are now supported through the immediate-count
  AArch64 lowering/printing route, including representative `== 64` and
  `> 64` cases.
- Variable-count `Shl`, `LShr`, and `AShr` remain explicitly
  printer-contracted: record/dispatch preserves register-count operands as
  `I128ShiftCountKind::Register`, while printing rejects them with the narrow
  diagnostic `i128 variable-count shifts are not supported by the current
  AArch64 i128 printer contract`.

## Suggested Next

Recommend supervisor route the active runbook to plan-owner closure if the
source idea acceptance criteria are satisfied.

## Watchouts

- Keep closure/review target-local to AArch64 i128 shifts.
- Large immediate shifts are supported; do not later recast them as
  unsupported without explicit lifecycle/user approval.
- Variable-count support is explicitly record/dispatch-only for now; printer
  support remains contracted until a semantic count-dependent lowering is
  designed.
- No drift into f128, helper ABI/resource policy, carrier ownership, shared
  BIR, or prealloc policy was observed in the route diff.
- `clang-format` was not available during the implementation packets, so the
  touched C++ files were not tool-formatted after editing.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -R '^backend_aarch64_' --output-on-failure > test_after.log 2>&1`

Result: passed. Build completed with no work to do, and `test_after.log`
contains 28/28 passing `backend_aarch64_` tests with 0 failures.

Proof log path: `test_after.log`.
