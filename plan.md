# Store Source Publication Dump Visibility Runbook

Status: Active
Source Idea: ideas/open/111_store_source_publication_dump_visibility.md

## Purpose

Expose bounded store-source publication-plan facts in prepared-module dump
output so source-producer and direct-global select-chain dependencies are
reviewable by contract tests.

Goal: prepared dumps should show the existing
`PreparedStoreSourcePublicationPlan` source-producer and direct-global
select-chain facts without creating a new semantic authority in the printer.

Core Rule: print existing prepared/publication-plan facts only. Do not
re-derive source-producer or select-chain facts inside the printer, weaken
fail-closed behavior, or expand the work into unrelated publication internals.

## Read First

- `ideas/open/111_store_source_publication_dump_visibility.md`
- `src/backend/prealloc/prepared_printer/`
- `src/backend/prealloc/prepare.cpp` or the current `prepare::print()` entry
  point
- store-source publication-plan construction and lookup code that owns
  `PreparedStoreSourcePublicationPlan`
- existing prepared-printer tests, especially call-plan and select-chain dump
  conventions

## Current Scope

Implementation targets:

- prepared-printer or prepared-module dump code that emits existing publication
  plan facts
- structured accessors only if an already-existing
  `PreparedStoreSourcePublicationPlan` field is not reachable by the printer
- focused prepared-printer tests for store-source source-producer and
  direct-global select-chain visibility

Validation targets:

- visible store-source source-producer rows
- visible direct-global select-chain dependency rows
- existing store-source missing-input fail-closed behavior
- no raw unrelated implementation pointers or unrelated publication internals
  in dump output

## Non-Goals

- Do not invent new BIR semantic source-producer facts.
- Do not rework AArch64 store-source lowering except for minimal fixture
  stabilization needed by focused dump tests.
- Do not treat `select_chain_lookups.cpp` helper authority as the problem.
- Do not add x86 or RISC-V implementation work.
- Do not broaden prepared-printer output beyond bounded store-source
  publication-plan visibility.
- Do not claim progress by changing only expectations, headers, or helper names.

## Working Model

- `PreparedStoreSourcePublicationPlan` already carries source-producer kind,
  source block and instruction coordinates, producer instruction pointers,
  direct-global select-chain source, root-is-select state, and root instruction
  index.
- Target-side planning, especially AArch64 store local/global publication
  paths, already consumes these facts.
- The active gap is dump contract visibility from `prepare::print()` or the
  nearest prepared-printer/module dump surface.
- Existing select-chain and call-argument dump sections are the formatting and
  naming model for the new bounded output.

## Execution Rules

- Inspect the existing prepared-printer dump conventions before choosing names
  or row shape.
- Keep the printer read-only with respect to semantic analysis; it may format
  existing facts but must not compute new source-producer authority.
- Preserve fail-closed behavior when store-source publication inputs are
  missing or incomplete.
- Assert concrete source-producer and direct-global select-chain fields in
  tests; a header-only contract is insufficient.
- Run `git diff --check` and the supervisor-selected build/test subset before
  accepting any code-changing step.

## Ordered Steps

### Step 1: Map Existing Store-Source Facts And Dump Conventions

Goal: identify the exact existing store-source publication facts that should
be printed and the local prepared-printer section shape to copy.

Primary targets:

- `src/backend/prealloc/prepared_printer/`
- `src/backend/prealloc/prepare.cpp` or the current `prepare::print()` entry
  point
- store-source publication-plan construction and lookup code
- existing prepared-printer tests for select-chain and call-argument dumps

Actions:

- Inspect how `prepare::print()` reaches prepared-printer/module dump code.
- Find the definition and construction sites for
  `PreparedStoreSourcePublicationPlan`.
- List the available source-producer fields and direct-global select-chain
  fields already carried by the plan.
- Compare existing select-chain and call-argument dump naming, row ordering,
  and absence handling.
- Identify the focused test fixture or nearest prepared-printer bucket for the
  new visibility contract.

Completion check:

- `todo.md` records the chosen dump surface, exact fields to print, fixture
  target, and any access gap without implementation edits.

### Step 2: Add Bounded Store-Source Publication Dump Output

Goal: emit store-source publication-plan rows from existing prepared facts.

Primary targets:

- prepared-printer/module dump code selected in Step 1
- structured accessors for `PreparedStoreSourcePublicationPlan` only if needed

Actions:

- Add a bounded store-source publication-plan section or equivalent rows using
  existing dump conventions.
- Print source-producer kind, source coordinates, producer instruction
  reference data, direct-global select-chain source, root-is-select state, and
  root instruction index when present.
- Keep absent or incomplete inputs visibly fail-closed without fabricating
  values.
- Avoid dumping raw unrelated implementation pointers or unrelated publication
  internals.

Completion check:

- Prepared dump output exposes the selected store-source facts and remains
  bounded to the source idea's visibility contract.

### Step 3: Add Focused Prepared-Printer Contracts

Goal: prove the new output is reviewable by contract tests.

Primary target:

- `tests/backend/bir/backend_prepared_printer_test.cpp` or the nearest existing
  prepared-printer test bucket

Actions:

- Add or extend a fixture that produces store-source source-producer facts.
- Add or extend a fixture that produces direct-global select-chain dependency
  facts for store-source publication.
- Assert concrete field rows, not only the presence of a section header.
- Preserve existing store-source planning tests that prove producer fields,
  direct-global select-chain fields, and missing-input fail-closed behavior.

Completion check:

- Focused tests fail without the new visibility output and pass with concrete
  source-producer and direct-global select-chain assertions.

### Step 4: Validate And Tighten Acceptance

Goal: prove the visibility work is complete without expectation downgrades or
unrelated behavior changes.

Actions:

- Run `git diff --check`.
- Run a fresh build or compile proof.
- Run the supervisor-selected prepared-printer and store-source publication
  planning subset.
- Inspect the diff for expectation weakening, header-only tests, printer-side
  re-derivation, or architecture expansion outside scope.
- Escalate to broader validation if the output path touches shared dump
  infrastructure beyond the narrow prepared-printer surface.

Completion check:

- Proof logs show the selected subset is green, tests assert concrete
  visibility fields, and no backend or printer expectation is weakened to avoid
  the contract.
