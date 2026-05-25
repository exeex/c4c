Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retire or Merge Emission-Only Shards

# Current Packet

## Just Finished

Step 3 in `plan.md` audited `calls_dispatch_bridge.*` after the
stack-preserved fallback deletion. No helper/file boundary was retired in this
packet because the remaining bridge exports are still live dispatch integration
points rather than obsolete emission-only shards:
`lower_scalar_call_argument_producers()`, `lower_call_instruction()`,
`materialize_call_boundary_source_to_destination()`,
`materialize_indirect_call_callee_to_prepared_register()`,
`record_call_result_source_register()`,
`materialize_missing_frame_slot_call_arguments()`, and
`publish_stack_preserved_call_values()` are all still called from the call path
in `dispatch.cpp`.

The removed fallback did narrow `publish_stack_preserved_call_values()` to the
shared first-stack-preserved lookup authority, but that did not make the bridge
file or a header declaration obsolete. Merging the current helper clusters into
`dispatch.cpp` would be file concatenation, not responsibility improvement:
the clusters still bridge dispatch scalar state, prepared call facts, selected
call-boundary moves, and emitted machine-node materialization at the call site.

## Suggested Next

Execute Step 4 in `plan.md`: audit call spelling/effect ownership without
mixing it into dispatch bridge cleanup. The likely next candidate is
non-emission call-boundary effect or printing logic, especially the
`calls_printing.cpp` family called out by the runbook.

## Watchouts

- Do not activate `03_dispatch_responsibility_reduction.md` until the calls
  cleanup has established the required call-emission boundary.
- Do not claim progress from file concatenation, mechanical renames, or test
  weakening; the Step 3 bridge audit explicitly rejected merging
  `calls_dispatch_bridge.*` into `dispatch.cpp` on that basis.
- Keep source-idea edits out of routine audit findings.
- `calls_printing.cpp` still owns call-boundary effect construction and
  printing/spelling. That looks like the right Step 4 family because it crosses
  machine-record/printer ownership.
- `materialize_missing_frame_slot_call_arguments()` and scalar argument
  producers still bridge dispatch state to prepared facts. They remain later
  Step 3 candidates only if a future prepared-fact consumption change makes one
  of those clusters obsolete; they are not retireable after the
  stack-preserved fallback deletion alone.
- Direct `dispatch_prepared_block()` test fixtures do not automatically receive
  prepared lookup pointers from `make_function_lowering_context()`. Tests that
  need shared lookup facts must attach a `PreparedFunctionLookups` object with a
  lifetime covering the block dispatch.

## Proof

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_call_boundary_effect_plan)$"; } > test_after.log 2>&1'`

Delegated proof passed: 4/4 selected tests passed after the build.
`git diff --check` passed.
Proof log path: `test_after.log`.
