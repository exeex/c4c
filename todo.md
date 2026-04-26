Status: Active
Source Idea Path: ideas/open/117_bir_printer_structured_render_context.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Printer Spelling Authority

# Current Packet

## Just Finished

Activation created this execution state for `plan.md` Step 1.

## Suggested Next

Delegate Step 1 inventory against `src/backend/bir/bir_printer.cpp`, BIR data
structures, and LIR-to-BIR handoff code.

## Watchouts

- Preserve existing `--dump-bir` text.
- Keep legacy strings as fallback until idea 118 removes them.
- Do not make `bir_printer.cpp` parse legacy type declaration bodies.

## Proof

Lifecycle-only activation; no build or test proof required yet.
