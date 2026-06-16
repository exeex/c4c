# Reopen 286/288 Match Load Local-Memory Admission

## Goal

Repair the reopened AArch64 286/288 CLI validation blocker so
`backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold` and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
are green again and idea 291 can return to close-time validation.

## Why This Exists

Idea 291 completed its rendered call-argument ABI suffix cleanup within its
owned boundary, but closure is blocked because the 286/288 CLI subset that the
source idea requires as close proof is red before any remaining 291 work.

The reported failure family is semantic BIR admission in the `match` load
local-memory route. The prior 286 source idea was closed with that subset green,
so this should be handled as a reopened validation blocker instead of expanding
the suffix-parser cleanup route.

## In Scope

- Reproduce the exact red 286/288 CLI subset and capture the current failure
  text.
- Identify the semantic BIR `match` load local-memory shape that now fails
  admission.
- Repair the semantic local-memory lowering or admission rule using structured
  type, local-slot, pointer, aggregate-layout, or prepared metadata already
  available to the route.
- Preserve the rendered suffix parser cleanup from idea 291; do not reintroduce
  rendered `alignstack(...)` parsing as semantic authority.
- Restore the 286/288 CLI subset as close-time proof for idea 291.

## Out of Scope

- Broad AArch64 ABI rewrites unrelated to the reopened `match` load
  local-memory admission failure.
- Prepared call-plan retirement or public prepared surface migration.
- Expectation-only updates, skipped tests, or weaker supported-path contracts.
- Named-case shortcuts for `00204.c`, `myprintf`, `match`, `movi`, or specific
  HFA struct spellings.
- Reopening the rendered call-argument ABI suffix fallback route except to prove
  it remains fenced.

## Acceptance Criteria

- The failure is reproduced with:
  `ctest --test-dir build -R '^(backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' --output-on-failure`
- A focused backend unit or dump assertion proves the repaired `match` load
  local-memory shape without depending on a named `00204.c` function.
- The exact 286/288 CLI subset passes.
- The relevant x86 00204 semantic/prepared dump coverage remains green if the
  repaired path touches shared semantic BIR lowering.
- Idea 291's suffix-parser boundary remains intact: structured call-argument
  lowering must not recover semantic layout authority from rendered
  `alignstack(...)` text.

## Reviewer Reject Signals

- The patch changes expected CLI dump text, CTest filters, unsupported markers,
  or required/forbidden snippets while the AArch64 CLI route still fails in the
  semantic BIR local-memory family.
- The patch recognizes `00204.c`, `myprintf`, `match`, `movi`, or HFA struct
  names instead of lowering a general semantic local-memory load shape.
- The patch routes around semantic BIR admission through prepared-only,
  printed-BIR, or rendered argument text recovery.
- The patch weakens or removes the x86 00204 dump coverage to make the AArch64
  route appear green.
- The patch reintroduces rendered `alignstack(...)` parsing as stronger
  authority than structured call-argument metadata.
- The exact old failure remains but is hidden behind a different diagnostic,
  bucket name, or skipped assertion.
