# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Producer And Consumer Surfaces
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

No executor packet completed yet. Lifecycle activation set the active route to
Step 1 (`Inventory Producer And Consumer Surfaces`).

## Suggested Next

Start Step 1 by inventorying the existing `PreparedRegalloc` producer data in
`src/backend/prealloc/regalloc.cpp`, the prepared-module ownership in
`src/backend/prealloc/prealloc.hpp`, and the current x86 prepared consumer
surfaces in `src/backend/mir/x86/codegen/prepared_module_emit.cpp`.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.

## Proof

Not run yet. This activation slice is lifecycle-only.
