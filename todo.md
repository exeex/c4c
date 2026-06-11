Status: Active
Source Idea Path: ideas/open/203_route4_publication_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Success, Rejection, Fallback, And Output

# Current Packet

## Just Finished

Step 4 added focused acceptance coverage for the selected AArch64
call-boundary current-block publication source reader.

`backend_aarch64_instruction_dispatch_test.cpp` now proves the public
`materialize_call_boundary_source_to_destination(...)` path can materialize the
stack-source call-boundary value from Route 4 semantic identity with prepared
producer lookups removed, preserves prepared fallback when the Route 4 owner
label is stale/wrong, and fails closed when that stale Route 4 fact has no
prepared fallback.

`backend_prepare_frame_stack_call_contract_test.cpp` now pins the Route 4
current-block reference statuses for successful validation plus missing,
mismatched type/wrong-key, duplicate reference, stale owner, and
wrong-relationship facts. Prepared publication mechanics and output-sensitive
emission still remain outside Route 4.

## Suggested Next

Execute Step 5 from `plan.md`: run the supervisor-selected broader validation
or review handoff for this first Route 4 adapter slice, then decide whether the
source idea can close or needs a follow-up idea for additional readers.

## Watchouts

Keep the adapter scoped to this reader. Do not replace all edge-publication
lookups or move edge-copy emission, move/home/storage policy, stack-source
extension, block-order emission, immediate publication payload spelling,
wrapper formatting, or emitted strings into BIR schema.

Step 5 should decide whether the targeted subset is enough for acceptance or
whether to add broader `ctest`/regression-guard coverage because this slice
touches output-sensitive AArch64 dispatch tests and BIR Route 4 reference
contracts.

## Proof

Step 4 acceptance proof passed and was written to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' > test_after.log
```

The subset covered `backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_return_lowering`, and
`backend_prepared_lookup_helper`; all 4 tests passed.
