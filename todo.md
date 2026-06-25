Status: Active
Source Idea Path: ideas/open/350_c4c_as_file_parser_using_rv64_line_core.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Existing c4c-as Placeholder And Object Writer Surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and execution scratchpad from
idea 350. No implementation packet has run yet.

## Suggested Next

Execute Step 1 by locating the `c4c-as` placeholder, app build wiring, RV64
object writer APIs, and narrow proof command for the first code packet.

## Watchouts

- Do not start idea 351 before `c4c-as` can assemble the canonical subset.
- Do not duplicate `.insn.d`, `li`, or `ret` operand parsing in the file parser.
- Unsupported directives or instructions must fail closed, not be ignored.

## Proof

Lifecycle-only activation. No build proof required.
