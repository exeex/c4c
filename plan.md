# Prepared Printer Decomposition Runbook

Status: Active
Source Idea: ideas/open/275_prepared_printer_decomposition.md

## Purpose

Split `src/backend/prealloc/prepared_printer.cpp` into prepared fact-family
printer units while preserving the existing public print API and exact dump
output.

Goal: make prepared dump changes reviewable in the file for the affected fact
family instead of in one large translation unit.

Core Rule: this is a printer-only decomposition; prepared dump text, prepared
schema, producer behavior, and test expectations must remain unchanged unless a
separate approved dump-format idea says otherwise.

## Read First

- `ideas/open/275_prepared_printer_decomposition.md`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/prealloc/prepared_printer.hpp`
- `src/backend/CMakeLists.txt`
- relevant prepared schema/fact headers under `src/backend/prealloc/`

## Current Targets

- Keep `src/backend/prealloc/prepared_printer.hpp` as the small public API.
- Reduce `src/backend/prealloc/prepared_printer.cpp` to a small entry point, or
  remove it if the public entry is naturally owned by a replacement core unit.
- Create family-oriented implementation files under a private printer boundary,
  such as:
  - `src/backend/prealloc/prepared_printer/core.cpp`
  - `src/backend/prealloc/prepared_printer/functions.cpp`
  - `src/backend/prealloc/prepared_printer/frame.cpp`
  - `src/backend/prealloc/prepared_printer/calls.cpp`
  - `src/backend/prealloc/prepared_printer/variadic.cpp`
  - `src/backend/prealloc/prepared_printer/regalloc.cpp`
  - `src/backend/prealloc/prepared_printer/value_locations.cpp`
  - `src/backend/prealloc/prepared_printer/control_flow.cpp`
  - `src/backend/prealloc/prepared_printer/special_carriers.cpp`
  - `src/backend/prealloc/prepared_printer/atomics.cpp`
  - `src/backend/prealloc/prepared_printer/intrinsics.cpp`
  - `src/backend/prealloc/prepared_printer/inline_asm.cpp`
  - `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
  - `src/backend/prealloc/prepared_printer/addressing.cpp`
- Share only small, printer-private formatting helpers through a private helper
  header. Do not create another monolithic printer header.

## Non-Goals

- Do not change prepared dump text format.
- Do not change prepared schema or producer behavior.
- Do not rewrite tests or snapshots to hide formatting drift.
- Do not split unrelated prealloc implementation files.
- Do not add testcase-shaped shortcuts or named-case-only printer behavior.

## Working Model

- Treat the current `prepared_printer.cpp` as the source of truth for behavior.
- First establish a private printer boundary and build wiring.
- Move one coherent prepared fact family at a time.
- Prefer small internal functions with clear ownership over a large shared
  context type. Introduce shared helper declarations only when multiple family
  files actually need them.
- Keep output validation close to every extraction step.

## Execution Rules

- Before each code-moving packet, identify the exact functions and output
  sections being moved.
- Preserve order, spacing, spelling, escaping, numeric formatting, and fallback
  text exactly.
- Keep `prepared_printer.hpp` public and narrow.
- If new files live under `src/backend/prealloc/prepared_printer/`, update build
  wiring because the current backend source glob only covers
  `src/backend/prealloc/*.cpp`.
- Run fresh compile proof after each code-moving packet.
- Run the supervisor-delegated prepared dump subset after each extraction; use
  broader backend validation before treating the decomposition as complete.
- Reject any route that weakens tests, rewrites snapshots, or mixes producer
  changes into the printer split.

## Ordered Steps

### Step 1: Establish the Private Printer Boundary

Goal: create the build-visible structure needed for family printer files
without changing dump behavior.

Primary targets:
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/prealloc/prepared_printer.hpp`
- `src/backend/CMakeLists.txt`
- new private helper/header files only if needed

Actions:
- Inspect the current printer sections and group helpers by owner:
  family-neutral formatting, names/rendering, functions, frame/stack, calls,
  variadic, storage, special carriers, runtime helpers, addressing, control
  flow, regalloc, and value locations.
- Add the private printer directory and minimal helper declarations needed by
  the first extraction.
- Update build wiring for subdirectory printer sources if the chosen file
  layout requires it.
- Keep the public `print(const PreparedBirModule&)` entry behavior unchanged.

Completion check:
- The project builds after the skeleton/build-wiring change.
- Existing prepared dump output remains unchanged for the supervisor-delegated
  narrow subset.
- `todo.md` records the exact proof command and result.

### Step 2: Extract Core, Function, Frame, Call, and Variadic Printers

Goal: move the least entangled front-half printer sections into family files.

Primary targets:
- `src/backend/prealloc/prepared_printer/core.cpp`
- `src/backend/prealloc/prepared_printer/functions.cpp`
- `src/backend/prealloc/prepared_printer/frame.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/prealloc/prepared_printer/variadic.cpp`

Actions:
- Move only printer logic and directly owned helpers for the selected families.
- Keep shared helpers private and narrowly named.
- Preserve output order by having the public/core entry call family printers in
  the same sequence as the legacy file.

Completion check:
- The project builds.
- The prepared dump subset passes with unchanged expected output.
- No test expectation or snapshot files are changed.

### Step 3: Extract Storage, Addressing, Runtime, Intrinsics, and Special Printers

Goal: separate the middle printer families that describe storage plans,
addressing, runtime helper metadata, intrinsics, inline asm, atomics, and
i128/f128 or related special carriers.

Primary targets:
- `src/backend/prealloc/prepared_printer/storage.cpp`
- `src/backend/prealloc/prepared_printer/addressing.cpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`
- `src/backend/prealloc/prepared_printer/atomics.cpp`
- `src/backend/prealloc/prepared_printer/intrinsics.cpp`
- `src/backend/prealloc/prepared_printer/inline_asm.cpp`

Actions:
- Move each family in a packet small enough to review output preservation.
- Keep numeric and quoted-text rendering identical to the legacy implementation.
- Avoid broadening the helper header when a helper belongs to one family.

Completion check:
- The project builds.
- The prepared dump subset passes with unchanged expected output.
- Shared helper growth remains justified by cross-family use.

### Step 4: Extract Regalloc, Value Location, and Control Flow Printers

Goal: move the densest prepared fact families after the easier printer
boundaries are stable.

Primary targets:
- `src/backend/prealloc/prepared_printer/regalloc.cpp`
- `src/backend/prealloc/prepared_printer/value_locations.cpp`
- `src/backend/prealloc/prepared_printer/control_flow.cpp`

Actions:
- Split regalloc printer logic along the prepared fact-family boundaries
  established by the decomposed regalloc/prealloc headers.
- Keep control-flow and value-location output ordering identical to the legacy
  dump.
- Do not repair or redesign regalloc producer behavior as part of this step.

Completion check:
- The project builds.
- The prepared dump subset passes with unchanged expected output.
- Nearby prepared dump cases for the same fact families are examined or run,
  not only one named target case.

### Step 5: Shrink the Legacy Entry and Run Completion Validation

Goal: leave the public prepared printer implementation as a small coordinator
over family printer units.

Primary targets:
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/prealloc/prepared_printer/`
- `src/backend/CMakeLists.txt`

Actions:
- Remove dead helpers and stale includes from the legacy translation unit.
- Confirm the private helper header has not become a second monolith.
- Confirm family file names and boundaries mirror prepared schema/fact
  ownership.
- Run the supervisor-chosen broader validation for the decomposition milestone.

Completion check:
- `prepared_printer.cpp` is a small public entry/coordinator or is replaced by
  an equivalent core file.
- Printer boundaries mirror prepared fact families.
- Existing prepared dump tests pass with unchanged output.
- Broader backend validation selected by the supervisor passes.
