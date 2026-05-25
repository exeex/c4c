Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Remove the First Duplicate Planning Path

# Current Packet

## Just Finished

Step 2 in `plan.md` removed the first duplicate planning path from AArch64 call
emission. `publish_stack_preserved_call_values()` now consumes only
`prepare::first_indexed_stack_preserved_values_for_call()` through
`context.function.call_plan_lookups`; when the shared prepared lookup facts are
missing, the dispatch bridge fails closed by emitting no stack-preserved
publication instead of scanning `PreparedCallPlansFunction::calls` or raw
`call_plan.preserved_values`.

Deleted the local `first_stack_preserved_values_for_call_fallback()` helper and
removed the raw preserved-values fallback from
`src/backend/mir/aarch64/codegen/calls_dispatch_bridge.cpp`. Focused AArch64
dispatch tests that exercise stack-preserved publication now attach
`PreparedFunctionLookups`, and one test asserts the shared first-stack-preserved
lookup fact exists before proving the emitted route.

## Suggested Next

Execute Step 3 in `plan.md`: audit whether removing the stack-preserved
publication fallback makes any `calls_dispatch_bridge.*` helper boundary
emission-only and small enough to retire or merge. Keep the packet narrow; the
next likely candidates remain bridge-local helpers such as missing frame-slot
argument materialization or scalar argument producer bridging, not
`calls_printing.cpp`.

## Watchouts

- Do not activate `03_dispatch_responsibility_reduction.md` until the calls
  cleanup has established the required call-emission boundary.
- Do not claim progress from file concatenation, mechanical renames, or test
  weakening.
- Keep source-idea edits out of routine audit findings.
- `calls_printing.cpp` still owns call-boundary effect construction and
  printing/spelling. That looks like the right later Step 4 family, not the
  first Step 2 deletion, because it crosses machine-record/printer ownership.
- `materialize_missing_frame_slot_call_arguments()` and scalar argument
  producers still bridge dispatch state to prepared facts; audit them as Step 3
  candidates without mixing in call-printing/effect ownership.
- Direct `dispatch_prepared_block()` test fixtures do not automatically receive
  prepared lookup pointers from `make_function_lowering_context()`. Tests that
  need shared lookup facts must attach a `PreparedFunctionLookups` object with a
  lifetime covering the block dispatch.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan)$"; } > test_after.log 2>&1'`

Delegated proof passed: 4/4 selected tests passed after the build.
`git diff --check` passed.
Proof log path: `test_after.log`.
