# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Route load-local materialization through prepared memory and recovered-source facts

## Just Finished

Completed Step 4's narrow ALU unpublished load-local source operand repair.
`src/backend/mir/aarch64/codegen/alu.cpp` now admits
`make_unpublished_load_local_source_operand` through prepared memory access
facts keyed by the producer load's prepared access and the current consumer
index. The stale same-block result-name producer scan and local slot/offset
alias reconstruction helper family was removed; intervening-store rejection now
uses prepared memory accesses and prepared frame-slot ranges, failing closed
when required prepared facts are missing or incomplete.

## Suggested Next

Supervisor should review and commit this Step 4 slice if the diff is accepted.

## Watchouts

`make_prepared_scalar_load_source` still has its existing unique-result-name
fallback for unrelated callers, but the Step 4 unpublished load-local route
uses the new access-keyed overload. No tests or expectations were changed.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: build succeeded; CTest passed 163/163 backend tests. CTest output is
captured in `test_after.log`.
