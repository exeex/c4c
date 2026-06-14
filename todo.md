Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Byte-Stable Proof Rows

# Current Packet

## Just Finished

Step 4: Add Byte-Stable Proof Rows completed for idea 260's `module`
top-level printer candidate.

Implemented files:

- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `todo.md`

Completed work:

- Added focused full-output byte-stability proof rows for multiple-function BIR
  body spacing, function plus global/string compatibility blank-line behavior,
  and invariant header placement.
- Preserved the existing Step 3 rows for empty module output, function-only BIR
  body output, global-only compatibility blank-line output, string-constant-only
  compatibility blank-line output, phase header output, note header output, and
  post-body blank-line placement.
- Did not touch `prepared_printer.cpp`; the remaining proof rows did not expose
  a narrow Step 3 bug.

## Suggested Next

Execute Step 5: run closure-readiness validation for the selected top-level
prepared-printer candidate and request lifecycle review if the supervisor
accepts the proof surface.

Suggested packet:

- Objective: complete broader validation and closure-readiness review for the
  prepared-printer complete-module-text bridge.
- Owned files: `todo.md`; test or implementation files only if validation
  exposes a narrow bug in the selected printer candidate.
- Do not touch: `plan.md`,
  `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`,
  unrelated implementation files, route-debug output, target-output baselines,
  unsupported expectations, helper/oracle status names, or other idea 260
  candidates.
- Suggested proof command:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake --build build-x86 --target backend_x86_route_debug_test backend_x86_handoff_boundary_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_route_debug|backend_x86_handoff_boundary)$') > test_after.log 2>&1`

## Watchouts

- Keep this runbook limited to the selected `module` top-level printer
  candidate.
- The new helper is intentionally local to `prepared_printer.cpp`; do not turn
  it into a public BIR printer API or broad aggregate wrapper in this runbook.
- Global-only and string-constant-only rows remain compatibility rows because
  the current BIR module printer does not emit top-level global/string text.
- The compact byte-stability fixtures intentionally leave prepared name tables
  empty, so their prepared metadata tail is the existing empty-section output.
- Direct global/string text emission remains out of scope for this selected
  candidate because that would require changing the BIR module printer surface,
  not proving the prepared-printer bridge.
- Do not rewrite route-debug, target-output baselines, unsupported
  expectations, helper/oracle status names, or unrelated printer/debug strings.

## Proof

Command:

`(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

Result: passed.

Proof log: `test_after.log`.
