Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consolidate The Preservation Helper Boundary

# Current Packet

## Just Finished

Step 3 closed the selected before-instruction stack-preservation helper
boundary after the Step 2 lookup-route change. The wrapper
`find_prior_stack_preserved_value_before_instruction` is no longer declared in
`calls.hpp` or implemented in `calls_preservation.cpp`; it now lives as a local
emission-only helper in `calls_moves.cpp`, next to its sole call site.

The durable lookup authority remains in
`prepare::find_latest_indexed_prior_stack_preserved_value_before_instruction`.
The local emission wrapper only validates that the prepared result is a usable
stack-slot preservation before `calls_moves.cpp` constructs the `MemoryOperand`.

## Suggested Next

Have the supervisor decide whether this Step 3 closeout is ready to commit or
whether the active plan needs reviewer/plan-owner handling before the next
packet.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark nearby preservation/publication cases
  unsupported to claim progress.
- The surviving prealloc helper is the durable prepared lookup API, not an
  AArch64 emission reconstruction boundary.
- The local `calls_moves.cpp` wrapper is intentionally emission-only because it
  performs operand-readiness validation for the sole move-emission use site.
- Do not fold in callee-saved republication/population or block-entry
  non-call-use reconstruction under this completed selected route.

## Proof

Passed.

Command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build succeeded; `ctest` reported 162/162 backend tests passed.
Proof log: `test_after.log`.
