Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Top-Level Printer Contract

# Current Packet

## Just Finished

Step 1: Inventory Top-Level Printer Contract completed for idea 260's
`module` top-level printer candidate.

Definitions/declarations/direct callers owning the current prepared-printer
top-level BIR body surface:

- Public declaration: `src/backend/prealloc/prepared_printer.hpp` exposes
  `prepare::print(const PreparedBirModule&)` as the prepared backend dump.
- Top-level definition: `src/backend/prealloc/prepared_printer.cpp` writes the
  prepared module header, optional `completed_phases`, `invariants`, and
  `notes` sections, the `--- prepared-bir ---` marker, then
  `c4c::backend::bir::print(module.module)`.
- Metadata section declarations live in
  `src/backend/prealloc/prepared_printer/private.hpp`; implementation files
  under `src/backend/prealloc/prepared_printer/*.cpp` append the prepared-only
  sections after the BIR body.
- Direct production callers include `src/backend/backend.cpp` debug/dump paths,
  `src/backend/mir/x86/module/module.cpp` fallback diagnostics, and focused
  tests in `tests/backend/bir/backend_prepared_printer_test.cpp`,
  `backend_prepare_*`, `backend_x86_route_debug_test.cpp`, and
  `backend_x86_handoff_boundary_lir_test.cpp`.

Current byte-stable behavior to preserve:

- Section order is: `prepared.module target=<triple> route=<route>`,
  optional `completed_phases`, optional `invariants`, optional `notes`,
  `--- prepared-bir ---`, complete BIR module text, then prepared metadata
  sections in the existing `append_*` order.
- BIR body text currently comes from `bir::print(module.module)`; the BIR
  printer emits functions only, with one blank line between multiple functions.
- Empty modules print no BIR body text after `--- prepared-bir ---` and do not
  add the conditional post-BIR blank line.
- Function-only modules print the `bir.func` body exactly as rendered by
  `bir::print`, followed by one extra blank line before prepared metadata.
- `PreparedBirModule::module.globals` and `string_constants` participate in the
  prepared-printer post-BIR blank-line condition even though current
  `bir::print` does not emit top-level global/string-constant text.
- Phase and note headers appear before `--- prepared-bir ---`; they must not be
  reordered into the BIR body or after prepared metadata.
- Existing tests assert the top-level module header, `--- prepared-bir ---`,
  and prepared function/control-flow sections, but they do not yet have a
  dedicated byte-for-byte row for empty-module, function-only,
  global/string-constant, phase-header, note-header, or surrounding blank-line
  behavior.

Candidate implementation files for Step 2/3:

- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/prealloc/prepared_printer.hpp` only if the agreement helper
  needs a public declaration; prefer a local helper otherwise.
- `tests/backend/bir/backend_prepared_printer_test.cpp`

## Suggested Next

Execute Step 2: design the complete-module-text agreement boundary in
`todo.md`. The next packet should specify whether a small local helper in
`prepared_printer.cpp` is enough, define accepted complete-module rows, define
compatibility/fail-closed rows for partial or layout-sensitive module text, and
name the Step 3 implementation/test ownership.

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
  Step 2 should decide whether that means the agreement boundary rejects those
  as incomplete module-text rows or preserves the existing prepared-printer
  compatibility blank-line rule.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, or unrelated printer/debug strings
  to claim progress.
- Recommended narrow Step 2/3 proof command:
  `(cmake --build --preset default --target backend_prepared_printer_test && ctest --test-dir build -R '^backend_prepared_printer$' --output-on-failure) > test_after.log 2>&1`

## Proof

`git diff --check -- todo.md`

Result: passed.
