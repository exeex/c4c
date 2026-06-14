Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Narrow Printer Bridge

# Current Packet

## Just Finished

Step 3: Implement Narrow Printer Bridge completed for idea 260's `module`
top-level printer candidate.

Implemented files:

- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `todo.md`

Completed work:

- Added a file-local complete-module-text agreement helper in
  `prepared_printer.cpp` for the selected prepared-printer BIR body emission
  path.
- The helper renders through the canonical BIR module printer and records the
  explicit agreement boundary before the prepared printer consumes that body
  text.
- Preserved the prepared printer's existing ownership of section order,
  `--- prepared-bir ---` placement, prepared-only headers, prepared metadata
  sections, and post-body blank-line policy.
- Preserved the compatibility blank-line behavior for modules whose BIR body
  text remains empty while top-level globals or string constants are present.
- Added full-output byte-stability rows for empty module output,
  function-only BIR body output, global-only compatibility blank-line output,
  string-constant-only compatibility blank-line output, phase header output,
  note header output, and post-body blank-line placement.

## Suggested Next

Execute Step 4: add or review any remaining byte-stable proof rows for the
selected top-level prepared-printer candidate, then decide whether Step 5 needs
only broader validation or any additional focused fixture.

Suggested packet:

- Objective: complete focused byte-stability proof review for the
  prepared-printer complete-module-text bridge.
- Owned files: `tests/backend/bir/backend_prepared_printer_test.cpp`,
  `todo.md`.
- Do not touch: `plan.md`,
  `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`,
  unrelated implementation files, route-debug output, target-output baselines,
  unsupported expectations, helper/oracle status names, or other idea 260
  candidates.
- Focused proof command:
  `(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Keep this runbook limited to the selected `module` top-level printer
  candidate.
- The new helper is intentionally local to `prepared_printer.cpp`; do not turn
  it into a public BIR printer API or broad aggregate wrapper in this runbook.
- Global-only and string-constant-only rows remain compatibility rows because
  the current BIR module printer does not emit top-level global/string text.
- The compact byte-stability fixtures intentionally leave prepared name tables
  empty, so their prepared metadata tail is the existing empty-section output.
- Step 4 should not rewrite route-debug, target-output baselines, unsupported
  expectations, helper/oracle status names, or unrelated printer/debug strings.

## Proof

Command:

`(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

Result: passed.

Proof log: `test_after.log`.
