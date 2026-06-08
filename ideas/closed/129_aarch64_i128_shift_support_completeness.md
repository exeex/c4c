# AArch64 i128 shift support completeness

## Goal

Complete or explicitly contract AArch64 i128 shift lowering support for
shift-by-64-or-more and variable-count shifts.

## Why This Exists

The AArch64 wide-value owner audit found no remaining shared BIR/prealloc
policy gap in i128 shift handling. Shift construction consumes prepared i128
pair operands and prepared shift-count storage facts, while AArch64 owns the
target shift kind, lane mechanics, machine effects, register-pair spelling, and
printer opcode spelling.

The audit did identify one target-local completeness question: whether backend
coverage and behavior for large constant shift counts and variable-count shifts
are complete enough for the current support contract.

## In Scope

- Inspect `src/backend/mir/aarch64/codegen/i128_ops.cpp` shift lowering and
  printing paths.
- Add or enable focused backend coverage for i128 shift-by-64-or-more and
  variable-count shifts where the current contract says they should be
  supported.
- If a case is intentionally unsupported, make the unsupported diagnostic
  explicit and narrow instead of silently weakening supported-path coverage.
- Prove the change with the narrow backend subset chosen by the supervisor.

## Out Of Scope

- Moving AArch64 shift opcode spelling, lane mechanics, or register-pair
  spelling into shared BIR/prealloc code.
- Reopening i128 helper ABI/resource, preservation, selected-call, carrier, or
  scalar-result ownership contracts.
- Broad rewrites of `i128_ops.cpp` unrelated to shift support completeness.
- f128 transport, helper, carrier, or printing work.

## Acceptance Criteria

- i128 shift-by-64-or-more and variable-count behavior is either supported by
  AArch64 lowering/printing with focused backend proof or rejected through an
  explicit unsupported diagnostic that matches the current capability contract.
- Any new or enabled tests exercise the relevant shift category instead of a
  single named testcase shortcut.
- The selected proof route is recorded in `todo.md` and the canonical
  regression logs remain comparable.

## Reviewer Reject Signals

- The route moves AArch64 shift opcode spelling, lane mechanics, or
  register-pair spelling into shared BIR/prealloc code.
- The route weakens expectations, marks a supported-path test unsupported, or
  reduces the test contract without explicit user approval.
- The implementation only masks one named testcase or adds testcase-shaped
  matching instead of a semantic shift lowering or explicit capability
  diagnostic.
- The route claims shared-policy progress by renaming helpers, reshaping
  diagnostics, or rewriting expectations while preserving the same unsupported
  i128 shift behavior.
- The diff expands into f128, helper ABI/resource policy, carrier ownership, or
  broad `i128_ops.cpp` cleanup outside the shift completeness scope.

## Closure Note

Closed after the active runbook completed the target-local AArch64 i128 shift
support route.

- Large immediate i128 shifts are supported through the immediate-count
  AArch64 lowering/printing route, with focused coverage for representative
  `== 64` and `> 64` cases.
- Variable-count `Shl`, `LShr`, and `AShr` remain explicitly
  printer-contracted: record/dispatch preserves register-count operands as
  `I128ShiftCountKind::Register`, and printing rejects them with the narrow
  diagnostic `i128 variable-count shifts are not supported by the current
  AArch64 i128 printer contract`.
- Touched route files were limited to
  `src/backend/mir/aarch64/codegen/i128_ops.cpp`,
  `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`,
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`, and
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, plus
  canonical `todo.md` execution state.
- Final proof command:
  `cmake --build --preset default && ctest --test-dir build -R '^backend_aarch64_' --output-on-failure > test_after.log 2>&1`.
  `test_after.log` recorded 28/28 passing `backend_aarch64_` tests.
- Close-time regression guard compared canonical `test_before.log` and
  `test_after.log` with `--allow-non-decreasing-passed`; both logs recorded
  28/28 passing tests and 0 failures.
- Reject-signal checks found no expectation downgrade, no named-testcase
  shortcut, no f128/helper ABI/resource/carrier/shared BIR/prealloc policy
  drift, and no broad `i128_ops.cpp` cleanup outside shift completeness scope.
