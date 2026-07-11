Status: Active
Source Idea Path: ideas/open/684_bir_mir_interface_cleanup_from_prepared_mir_view_docs.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Reference-Only Core Adapter

# Current Packet

## Just Finished

Step 1 (`Add Reference-Only Core Adapter`) added a behavior-preserving
`c4c::backend::mir::prepared::PreparedMirCoreView` /
`PreparedMirFunctionView` adapter over the current
`prepare::PreparedBirModule`. The view exposes target identity, BIR/prepared
names, function traversal, globals, string constants, per-function control
flow, value locations, stack layout, addressing, and view-owned prepared
lookup caches without exposing diagnostics, notes, phases, route text, or a
raw full-module escape hatch. A focused MIR adapter test covers construction,
defined-function filtering, fail-closed missing lookups, instruction cursors,
and lookup-cache agreement.

## Suggested Next

Start Step 2 by constructing the core view inside the x86 public compatibility
wrapper and routing only the internal module-emission entry through a
view-taking surface, leaving per-function plan consumption for a later packet.

## Watchouts

- Step 1 intentionally leaves all existing x86, RV64, and AArch64 consumers on
  their old direct `PreparedBirModule` paths.
- The new function view fails closed when a defined function lacks any core
  prepared fact required by this first adapter.
- The earlier supervisor-delegated regex matched zero tests in both before and
  after logs; supervisor confirmed that was an invalid-regex artifact, not a
  code blocker.

## Proof

- Corrected proof command ran and wrote `test_after.log`:
  `set -o pipefail; (cmake --build build --target c4c_backend backend_prepared_mir_core_view_test -j && ctest --test-dir build -R '^backend_prepared_mir_core_view$|^backend_x86_shared_producer_query$|^backend_publication_plan_record$|^backend_call_boundary_effect_plan$' --output-on-failure) > test_after.log 2>&1`
- Corrected proof result: passed, 4/4 tests passed
  (`backend_prepared_mir_core_view`, `backend_x86_shared_producer_query`,
  `backend_publication_plan_record`, and
  `backend_call_boundary_effect_plan`).
