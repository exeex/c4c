Status: Active
Source Idea Path: ideas/open/06_prepared_call_preservation_source_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Explicit Source Selection In Call Plans

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: extended prepared call argument
`PriorPreservation` source selections with the remaining structured
prior-preservation source facts needed by downstream consumers, including the
callee-saved save index and spill-slot placement carrier.

`src/backend/prealloc/call_plans.cpp` now copies those fields from the selected
`PreparedCallPreservedValue`, and the prepared calls printer publishes them as
explicit selection fields.

`backend_prepare_frame_stack_call_contract_test` now includes a dedicated
two-call prior-preservation source-selection fixture that proves the second
call argument carries the selected prior call position, route, register/stack
storage facts, save index, and spill-slot placement without target-local source
rediscovery.

## Suggested Next

Execute the next supervisor-selected packet for AArch64 consumption of prepared
prior-preservation source selections, using the explicit selection fields
published by Step 3 instead of local prior-source discovery.

## Watchouts

- `clang-format` is not installed in this environment; formatting was kept to
  the existing local style manually.
- AArch64 source construction still has fallback lookup paths in files outside
  this packet; this slice only publishes and proves the prepared-side authority
  fields.
- Ambiguous or incomplete prior-preservation records still produce no
  `source_selection`; consumers should treat a present `PriorPreservation`
  selection as the authoritative prepared source record.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_lookup_helper)$'`

Result: passed. The CTest subset passed 2/2 tests. Proof log:
`test_after.log`.
