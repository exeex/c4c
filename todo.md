Status: Active
Source Idea Path: ideas/open/275_prepared_printer_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the Private Printer Boundary

# Current Packet

## Just Finished

Lifecycle activation created this execution state for `plan.md` Step 1.

## Suggested Next

Delegate Step 1 to an executor with a supervisor-selected compile/proof command.

## Watchouts

- Preserve prepared dump output exactly.
- Keep `prepared_printer.hpp` as the small public print API.
- Do not change prepared schema, producer behavior, tests, or snapshots.
- If using `src/backend/prealloc/prepared_printer/*.cpp`, update build wiring
  because the current backend glob only covers top-level prealloc `.cpp` files.

## Proof

Lifecycle-only activation; no build or test proof was run.
