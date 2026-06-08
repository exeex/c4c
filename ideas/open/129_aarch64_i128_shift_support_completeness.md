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
