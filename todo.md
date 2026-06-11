Status: Active
Source Idea Path: ideas/open/203_route4_publication_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Reader And Baseline Proof

# Current Packet

## Just Finished

Step 1 selected the AArch64 call-boundary current-block publication source
reader as the bounded Route 4 adapter boundary:
`prepared_call_boundary_source_value(...)` /
`call_boundary_source_value_by_name(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

The selected reader is appropriate because it already asks
`mir::find_bir_current_block_publication_identity(...)` for the semantic
current-block publication identity before falling back to prepared current-block
publication consumption. Route 4 should supply only that semantic identity:
block/program-point availability and the produced/source value identity needed
to agree with the requested publication name before the call boundary. It must
not supply publication mechanics or emitted-output policy.

Retained prepared fallback for the implementation slice:
`PreparedFunctionLookups::edge_publication_source_producers` through
`prepare::find_prepared_current_block_publication_consumption(...)`, plus the
existing prepared call/publication mechanics for move/home/storage policy,
stack-source extension, block-order emission, wrapper/output formatting, and
emitted/debug output.

## Suggested Next

Execute Step 2 from `plan.md`: add the smallest Route 4 semantic identity
adapter for this selected call-boundary reader, keeping the prepared lookup as
the mechanics/output fallback.

## Watchouts

Missing coverage before implementation acceptance: the current baseline subset
is only a regression/proximity proof. The implementation packet still needs
focused proof for successful Route 4 semantic identity use, absent Route 4
facts, mismatched publication identity, duplicate or ambiguous identity facts,
wrong-reference rejection or fallback, retained prepared fallback behavior, and
output stability for affected call-boundary emission.

Keep the adapter scoped to this reader. Do not replace all edge-publication
lookups or move edge-copy emission, move/home/storage policy, stack-source
extension, block-order emission, immediate publication payload spelling,
wrapper formatting, or emitted strings into BIR schema.

## Proof

Baseline proof before code changes passed and was written to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' > test_after.log
```

The subset covered `backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_return_lowering`, and
`backend_prepared_lookup_helper`; all 4 tests passed.
