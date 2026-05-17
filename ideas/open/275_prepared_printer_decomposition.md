# Prepared Printer Decomposition

Status: Open
Created: 2026-05-17

## Intent

Split `src/backend/prealloc/prepared_printer.cpp` into readable printer
sections after prepared schema ownership is decomposed.

The prepared printer is large because it mirrors every prepared fact family.
That is acceptable up to a point, but once schema headers and fact publishers
are split, the printer should follow the same family boundaries so prepared
dump changes are easier to review.

## Why This Exists

`prepared_printer.cpp` currently prints functions, stack layout, frame plans,
dynamic stack, call plans, storage plans, i128/f128 carriers, atomics,
intrinsics, inline asm, runtime helpers, regalloc, addressing, control flow,
and value locations from one translation unit.

That makes dump-format review noisy and causes unrelated printer edits to
rebuild the entire prepared printer.

## In Scope

- Keep `prepared_printer.hpp` as the small public print API.
- Split printer implementation by prepared fact family, using files such as:
  - `prepared_printer/core.cpp`
  - `prepared_printer/functions.cpp`
  - `prepared_printer/frame.cpp`
  - `prepared_printer/calls.cpp`
  - `prepared_printer/variadic.cpp`
  - `prepared_printer/regalloc.cpp`
  - `prepared_printer/value_locations.cpp`
  - `prepared_printer/control_flow.cpp`
  - `prepared_printer/special_carriers.cpp`
  - `prepared_printer/runtime_helpers.cpp`
  - `prepared_printer/addressing.cpp`
- Preserve exact dump output unless an explicit separate dump-format idea
  approves a change.
- Share small formatting helpers through a private printer helper header.

## Out of Scope

- Changing prepared dump text format.
- Changing prepared schema or producer behavior.
- Rewriting tests or snapshots to hide formatting drift.
- Splitting unrelated prealloc implementation files.

## Completion Criteria

- `prepared_printer.cpp` is reduced to a small public entry or removed in favor
  of family printer translation units.
- Printer file boundaries mirror prepared schema/fact families.
- Existing prepared dump tests pass with unchanged output.
- Future dump-format changes can be reviewed in the file for the affected
  prepared fact family.

## Reviewer Reject Signals

Reject the route or slice if it:

- changes prepared dump output without explicit approval;
- moves printer code before the schema/fact ownership is stable enough to name
  the sections clearly;
- creates a broad printer helper header that becomes another monolith;
- weakens tests or snapshots to accept accidental output drift;
- mixes producer behavior changes into a printer-only split.
