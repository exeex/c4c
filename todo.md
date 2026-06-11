Status: Active
Source Idea Path: ideas/open/203_route4_publication_identity_adapter.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Route 4 Identity Adapter

# Current Packet

## Just Finished

Step 2 added the smallest local Route 4 semantic identity adapter for the
selected AArch64 call-boundary current-block publication source reader:
`route4_call_boundary_source_identity(...)` in
`src/backend/mir/aarch64/codegen/calls.cpp`.

The adapter is used only by `prepared_call_boundary_source_value(...)` before
the retained prepared fallback. It builds and validates a Route 4 current-block
publication reference for the requested prepared value spelling and
before-instruction context, requires block/program-point, name, optional
prepared id, type, producer index, producer instruction, and produced-value
agreement, and rejects absent, mismatched, duplicate/ambiguous, stale, or
wrong-reference Route 4 facts by falling through to the existing prepared
consumption path.

Retained prepared fallback remains
`prepare::find_prepared_current_block_publication_consumption(...)` over
`PreparedFunctionLookups::edge_publication_source_producers`; publication
mechanics, move/home/storage policy, stack-source extension, block-order
emission, wrapper/output formatting, payload spelling, and emitted/debug output
still stay outside Route 4.

## Suggested Next

Execute Step 4 from `plan.md`: add focused acceptance coverage for the selected
call-boundary adapter boundary, especially success through Route 4, prepared
fallback, and output stability around the selected reader.

## Watchouts

The exact delegated subset passed, and existing `backend_prepared_lookup_helper`
coverage exercises the Route 4 current-block reference validation statuses used
by the adapter: available, missing, type/wrong-key, stale owner, duplicate, and
wrong-relationship. Step 4 still needs focused reader-level proof that the
AArch64 call-boundary path takes Route 4 on success and preserves prepared
fallback/output behavior for absent, mismatched, duplicate/ambiguous, and
wrong-reference facts.

Keep the adapter scoped to this reader. Do not replace all edge-publication
lookups or move edge-copy emission, move/home/storage policy, stack-source
extension, block-order emission, immediate publication payload spelling,
wrapper formatting, or emitted strings into BIR schema.

## Proof

Step 2 proof after code changes passed and was written to `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_prepared_lookup_helper)$' > test_after.log
```

The subset covered `backend_prepare_frame_stack_call_contract`,
`backend_aarch64_instruction_dispatch`, `backend_aarch64_return_lowering`, and
`backend_prepared_lookup_helper`; all 4 tests passed.
