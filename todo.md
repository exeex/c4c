Status: Active
Source Idea Path: ideas/open/bir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Structural Validation

# Current Packet

## Just Finished

Completed `plan.md` Step 5, `Final Structural Validation`.
Final BIR layout uses directory-level headers rather than one header per
implementation file:
`src/backend/bir/bir.hpp` remains the BIR public index,
`src/backend/bir/lir_to_bir.hpp` remains the exported LIR-to-BIR entry API,
and `src/backend/bir/lir_to_bir/lowering.hpp` is the private lowering
implementation index.

The memory lowering sources now live under
`src/backend/bir/lir_to_bir/memory/` as `coordinator.cpp`, `addressing.cpp`,
`local_slots.cpp`, `provenance.cpp`, and `value_materialization.cpp`.
No `memory/memory.hpp` or other `memory/*.hpp` file was introduced because the
memory split did not create memory-specific shared declarations that should be
separated from the broader private `lowering.hpp` index.

No new thin one-off headers were introduced. The current BIR header set is
`bir.hpp`, `lir_adapter_error.hpp`, `lir_to_bir.hpp`, and
`lir_to_bir/lowering.hpp`; no `bir_printer.hpp`, `bir_validate.hpp`,
`addressing.hpp`, `local_slots.hpp`, or `provenance.hpp` exists.

The source idea acceptance criteria are satisfied for the current structure:
external users can include `src/backend/bir/lir_to_bir.hpp` without private
lowering machinery, LIR-to-BIR implementation agents can use
`src/backend/bir/lir_to_bir/lowering.hpp` as the private directory index, the
thin single-purpose header count did not increase, and the memory subdirectory
has no competing one-off headers. Remaining follow-up, if the memory subdomain
later gains memory-specific shared declarations, is to add one
`memory/memory.hpp` directory index instead of per-file headers.

## Suggested Next

Hand this active runbook to the plan owner for close/deactivate decision.

## Watchouts

The runbook appears ready for lifecycle review. Do not fold future semantic
cleanup or memory-specific declaration extraction into this structural slice;
that should become a separate source idea if needed.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`.
The build completed and the full CTest suite passed: 3071 tests passed, 0
failed, with 12 disabled tests not run. Proof log: `test_after.log`.
