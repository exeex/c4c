# BIR Printer Structured Render Context

Status: Closed
Created: 2026-04-26
Last Updated: 2026-04-26
Closed: 2026-04-26

Parent Ideas:
- [116_bir_layout_dual_path_coverage_and_dump_guard.md](/workspaces/c4c/ideas/closed/116_bir_layout_dual_path_coverage_and_dump_guard.md)

## Goal

Give BIR dump rendering an explicit structured render path so
`src/backend/bir/bir_printer.cpp` can print type and name spellings from
structured authority rather than depending on raw legacy type strings carried
through lowering.

## Why This Idea Exists

The BIR printer currently prints only the facts present in `bir::Module`.
`bir::Module` has functions, globals, string constants, `TypeKind`, names,
offsets, and ABI size/alignment facts, but it does not carry
`StructNameTable`, `BackendStructuredLayoutTable`, or a general type-id to text
render context.

Some dump spelling still arrives as raw strings, such as
`bir::CallInst::return_type_name`. That is acceptable while legacy text remains
active, but it blocks confident legacy removal: after `type_decls` and related
text shadows disappear, the dump path needs a structured way to recover the
same human-readable spelling.

This idea should switch printer-facing authority, not backend layout
semantics.

## Scope

Candidate work includes:

- define a small BIR render context, or extend `bir::Module`, with the minimal
  structured spelling state needed by `bir_printer.cpp`
- carry struct/type spelling state from LIR-to-BIR lowering into BIR dump
  rendering without making the printer parse legacy type declaration bodies
- make call return type rendering structured-first instead of relying on
  `CallInst::return_type_name` as the long-term authority
- keep names such as function, global, local slot, block, and string constant
  spellings stable
- add BIR printer tests that compare legacy-backed and structured-backed dump
  output
- add structured-only or legacy-shadow-absent dump fixtures where practical

Out of scope:

- Removing legacy `type_decls` and related fallback helpers.
- Broad `LirTypeRef` identity redesign outside the fields needed for BIR dump
  rendering.
- Changing prepared BIR printer contracts except where they embed semantic BIR
  output from `bir::print`.
- Migrating MIR.

## Execution Rules

- Preserve existing `--dump-bir` text unless an intentional printer contract
  change is documented and tested.
- Prefer a small explicit render context over making `bir_printer.cpp` reach
  back into LIR internals.
- Keep fallback rendering available while idea 118 has not removed legacy
  text.
- Do not parse `LirModule::type_decls` from inside the BIR printer.
- If a string field remains as fallback-only, record it explicitly as a
  removal candidate for idea 118.

## Acceptance Criteria

- `bir_printer.cpp` has a structured-first render route for BIR dump spelling
  that needs more than scalar `TypeKind`.
- `--dump-bir` output remains stable for existing CLI dump tests.
- New printer tests prove structured-backed dump output matches the previous
  legacy-backed spelling for selected aggregate-sensitive paths.
- The remaining raw-string printer fallbacks are enumerated and are not active
  authority for the covered paths.

## Closure Notes

The active runbook completed Steps 1-6. Covered aggregate sret direct call
returns now render through structured metadata instead of legacy return type
text as active authority, while preserving byte-stable BIR dump output.
Remaining raw-string fallback surfaces are inventoried in the final `todo.md`
Step 6 packet for follow-up under idea 118.

Close-scope proof used the backend-wide command
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_'; }`.
Both canonical logs report 107/107 runnable backend tests passing with 12
disabled MIR-focus backend tests not run; the regression guard passed in
non-decreasing mode with no new failures.

## Deferred

- Removing legacy type text and fallback fields belongs to idea 118.
