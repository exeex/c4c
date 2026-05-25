Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove The Selected Preservation Decision

# Current Packet

## Just Finished

Step 2 removed the selected local preservation reconstruction from
`find_prior_stack_preserved_value_before_instruction`. The helper no longer
refinds prepared call plans or reverse-walks `call_plans->calls`; it now
requires `context.function.call_plan_lookups` and consumes the prepared lookup
authority through
`prepare::find_latest_indexed_prior_stack_preserved_value_before_instruction`.

Added the missing prepared lookup helper shaped by `PreparedValueId`,
`block_index`, and `instruction_index`. It searches
`PreparedCallPlanLookups::prior_preserved_by_value` for the latest same-block
prior `StackSlot` preservation before the requested instruction. AArch64
emission-side validation still checks route, slot id, stack offset, nonzero
stack size, and keeps the existing alignment fallback before constructing the
`MemoryOperand`.

## Suggested Next

Review the remaining preservation reconstruction helpers for the next narrow
authority leak, keeping any follow-up packet separate from this completed
before-instruction stack-preserved lookup.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- Keep this slice to the selected before-instruction stack-preserved lookup;
  do not fold in callee-saved republication/population or block-entry
  non-call-use reconstruction.
- The new prepared helper is intentionally same-block/before-instruction to
  match the removed local walk. Dominating prior preservation remains handled
  by the separate existing lookup path.

## Proof

Passed.

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build succeeded; `ctest` reported 162/162 backend tests passed.
Proof log: `test_after.log`.
