Status: Active
Source Idea Path: ideas/open/203_route4_publication_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire The Selected Reader

# Current Packet

## Just Finished

Step 3 verified that the selected AArch64 call-boundary current-block
publication source reader is wired through `prepared_call_boundary_source_value(...)`
to consume only Route 4 semantic identity from
`route4_call_boundary_source_identity(...)`.

The Route 4 path supplies only the produced BIR value when the current-block
publication reference agrees on block/program-point, prepared spelling or id,
type, producer instruction/index, and produced-value identity. Missing,
mismatched, duplicate/ambiguous, stale, or wrong-reference Route 4 facts still
fall through to the retained prepared consumption path.

Retained prepared fallback remains authoritative through
`prepare::find_prepared_current_block_publication_consumption(...)` over
`PreparedFunctionLookups::edge_publication_source_producers`; publication
mechanics, move/home/storage policy, stack-source extension, block-order
emission, wrapper/output formatting, payload spelling, and emitted/debug output
still stay outside Route 4.

## Suggested Next

Execute Step 4 from `plan.md`: add focused acceptance coverage for the selected
call-boundary adapter boundary, especially success through Route 4, prepared
fallback, rejection/fallback for mismatched Route 4 facts, and output stability
around the selected reader.

## Watchouts

No code churn was needed for Step 3: the selected reader already calls the
Route 4 adapter before the prepared fallback, and no other edge-publication
reader was migrated. Step 4 still needs focused reader-level proof that the
AArch64 call-boundary path takes Route 4 on success and preserves prepared
fallback/output behavior for absent, mismatched, duplicate/ambiguous, stale,
and wrong-reference facts.

Keep the adapter scoped to this reader. Do not replace all edge-publication
lookups or move edge-copy emission, move/home/storage policy, stack-source
extension, block-order emission, immediate publication payload spelling,
wrapper formatting, or emitted strings into BIR schema.

## Proof

Step 3 wiring verification proof passed and was written to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' > test_after.log
```

The subset covered `backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_return_lowering`, and
`backend_prepared_lookup_helper`; all 4 tests passed.
