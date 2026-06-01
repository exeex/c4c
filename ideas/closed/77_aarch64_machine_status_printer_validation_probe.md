# AArch64 Machine Status And Printer Validation Probe

## Goal

Compare AArch64 machine status derivation with machine-printer validation for
the same record families, then decide whether any repeated semantic validation
should fold into a local helper or remain separately target-local.

## Why This Exists

The large-owner residue audit classified machine status and printer validation
overlap as `needs-more-evidence`. Machine records and final printer spelling
are target-local by default, but duplicated semantic validation between
`instruction.cpp` and `machine_printer.cpp` should be identified before a local
fold-back route is attempted.

## Owned Files

- Audit/probe notes in `todo.md` or a later activated plan.
- Candidate implementation files only if the probe proves duplication:
  - `src/backend/mir/aarch64/codegen/instruction.cpp`
  - `src/backend/mir/aarch64/codegen/instruction.hpp`
  - `src/backend/mir/aarch64/codegen/machine_printer.cpp`
  - any new AArch64-local validation helper beside these owners.

## In Scope

- Compare status derivation helpers against printer validation checks for the
  same record families.
- Distinguish semantic validation from final assembly text validation.
- If repeated semantic validation exists, propose or implement one
  AArch64-local helper boundary.
- Keep final mnemonic spelling, register spelling, relocation/address spelling,
  and assembly text output in the printer.

## Out Of Scope

- Moving machine record schemas or printer spelling into shared code.
- Treating all status and printer validation as duplication without a
  record-family comparison.
- Broad instruction schema cleanup or printer formatting rewrites.
- Combining this probe with scalar register helper fold-back unless the probe
  proves the same helper boundary should own both.

## Proof Expectations

- Probe notes listing the compared record families and whether each repeated
  check is semantic validation or final text validation.
- If implementation follows, focused machine status and printer-output tests
  for the affected record families.
- Regression guard logs before accepting any code slice.

## Reviewer Reject Signals

- The route folds printer checks without proving they duplicate semantic
  machine status validation.
- Final assembly spelling validation is moved out of the printer solely to
  reduce file size.
- A helper rename leaves the same repeated validation in both owners.
- Printed assembly, machine status, or unsupported-record behavior changes
  without focused proof.
- Tests are weakened or unsupported expectations are downgraded without
  explicit approval.

## Closure Note

Closed after the Step 1-3 evidence-only route. The probe compared Branch,
Scalar, Address materialization, Spill/reload, and Frame status/printer record
families, separated repeated semantic checks from printer-owned final spelling
checks, and concluded that no AArch64-local helper should be implemented. The
review in `review/idea77_status_printer_validation_review.md` found no
blockers; closure used the no-code evidence scope with no implementation or
test changes.
