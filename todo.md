Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Design the Complete-Module-Text Boundary

# Current Packet

## Just Finished

Step 2: Design the Complete-Module-Text Boundary completed for idea 260's
`module` top-level printer candidate.

Chosen helper placement:

- Keep the boundary local to `src/backend/prealloc/prepared_printer.cpp`.
- Add a small file-local helper or adapter that renders the BIR body text and
  returns whether that text is an agreed complete-module body for this prepared
  printer integration point.
- Do not add a public declaration in `prepared_printer.hpp`; the selected
  candidate is the top-level prepared-printer reader path, not a shared BIR
  printer API or aggregate wrapper.

Accepted complete-module-text rows:

- Empty module: accepted only when complete module text is the empty string and
  the prepared printer keeps `--- prepared-bir ---\n` with no extra
  post-body blank line.
- Function-only module: accepted when body text is exactly
  `bir::print(module.module)` and the prepared printer keeps the existing one
  extra blank line before prepared metadata.
- Multiple-function module: accepted when the BIR body preserves the BIR
  printer's one blank line between functions and the prepared printer adds only
  the existing single post-body blank line.
- Phase and note rows: accepted only when `completed_phases`, `invariants`, and
  `notes` remain before `--- prepared-bir ---`; the complete-module-text helper
  must not absorb or reorder prepared-only headers.
- Surrounding layout: accepted only when the full prepared dump remains
  byte-stable for section order, marker placement, BIR body bytes, and the
  prepared metadata append order.

Rejected or compatibility rows:

- Partial BIR module text, missing function text, reordered function text, or
  any helper output that differs from `bir::print(module.module)` must not be
  treated as agreement.
- Global-only or string-constant-only modules remain on the prepared-printer
  compatibility layout: current `bir::print` emits no top-level global/string
  text, but existing prepared output still adds the conditional post-BIR blank
  line when `module.globals` or `module.string_constants` are present.
- Mixed function plus global/string-constant modules are compatible only if the
  helper preserves the current function body bytes and the existing
  global/string-sensitive post-body blank-line behavior; they are not proof
  that global/string top-level text is complete.
- Any phase, invariant, note, prepared metadata section, or blank-line drift is
  a compatibility/fail-closed row for this candidate rather than an output
  rewrite opportunity.
- Raw baseline rewrites, unsupported expectation downgrades, or moving
  prepared-only sections into BIR text are out of scope.

Retained behavior:

- Prepared printer remains the owner of the header, optional phase/invariant
  and note headers, the `--- prepared-bir ---` marker, the post-BIR blank-line
  policy, and all prepared metadata sections.
- `bir::print(const bir::Module&)` remains the source of function body text;
  the Step 3 helper only makes the agreement boundary explicit for using that
  complete text in the selected prepared-printer body emission path.

Candidate implementation/test files:

- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

## Suggested Next

Execute Step 3: implement the narrow printer bridge in
`src/backend/prealloc/prepared_printer.cpp` and add focused byte-stability rows
in `tests/backend/bir/backend_prepared_printer_test.cpp`.

Suggested packet:

- Objective: implement the local complete-module-text agreement helper for the
  prepared-printer BIR body emission path.
- Owned files: `src/backend/prealloc/prepared_printer.cpp`,
  `tests/backend/bir/backend_prepared_printer_test.cpp`, `todo.md`.
- Do not touch: `plan.md`,
  `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`,
  unrelated implementation files, route-debug output, target-output baselines,
  unsupported expectations, or other idea 260 candidates.
- Required behavior: preserve byte-stable output for empty modules,
  function-only modules, global-only or string-constant-only compatibility
  blank lines, phase headers, note headers, and post-body blank-line placement.
- Focused proof command:
  `(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

## Watchouts

- Keep this runbook limited to the selected `module` top-level printer
  candidate.
- Do not treat this inventory as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared-printer section order, BIR text, blank-line behavior,
  empty-module output, function-only output, global/string-constant blank-line
  behavior, phase output, and note-header behavior.
- The current BIR printer does not emit top-level globals or string constants;
  Step 3 must preserve the existing prepared-printer compatibility blank-line
  rule instead of claiming complete global/string text agreement.
- Do not make the helper public unless implementation proves a local helper
  cannot express the boundary.
- A full-output byte comparison is preferable for the new rows because this
  packet is about marker placement and blank-line stability, not substring
  presence.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, or unrelated printer/debug strings
  to claim progress.
- Recommended narrow Step 3 proof command:
  `(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

## Proof

`git diff --check -- todo.md`

Result: passed.
