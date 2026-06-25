Status: Active
Source Idea Path: ideas/open/359_bir_prepared_object_consumer_contract_completion.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit and Design the Shared Prepared Consumer Contract

# Current Packet

## Just Finished

Lifecycle switch only. The previous 356 runbook is paused and
`ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md` remains
open for later continuation. No implementation was started in this switch.

The uncommitted RV64 `20000113-1.c` slice is intentionally not accepted because
it exposed shared BIR/prepared consumer contract gaps rather than a purely
RV64-local missing instruction-set fix.

## Suggested Next

Execute Step 1 as audit/design only:

- Audit existing prepared move/publication/select/frame metadata and tests.
- Fill a boundary/design table for:
  - block-entry / edge-copy execution sites,
  - select / join-transfer carrier classification,
  - value-home transfer planning,
  - frame ownership including stack-homed value homes,
  - precise diagnostic taxonomy,
  - shared traversal schedule.
- Identify which shared query/helper or traversal contract is missing for each
  area.
- Do not patch RV64 `object_emission.cpp` as the primary solution.

## Watchouts

- This idea is about shared BIR/prepared contract completion, not reducing the
  full RV64 gcc torture scan failure count.
- Do not implement globals/data sections, vararg/FPR ABI, or full RV64
  instruction coverage here.
- Do not accept the retracted `20000113-1.c` slice as target-local progress.
- Keep target backends hook-shaped: concrete machine emission only, no
  rediscovery of CFG/data-flow semantics.

## Proof

Not run. Lifecycle-only switch; no code or tests were changed.
