Status: Active
Source Idea Path: ideas/open/155_bir_block_entry_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define BIR Publication Identity Records

# Current Packet

## Just Finished

Completed Step 2 for `plan.md`: added BIR current-block publication identity
request/result records and `find_bir_current_block_publication_identity` on top
of existing same-block producer identity. The record carries only semantic BIR
identity fields: availability, source producer identity, instruction, produced
value/name/type, instruction index, and source-producer kind.

Added test-only prepared/BIR semantic comparison in
`backend_prepared_lookup_helper_test.cpp` for same-block available, missing
value, wrong block label, before-index/future producer, and mismatched-type
fail-closed cases. Added call-contract coverage in
`backend_prepare_frame_stack_call_contract_test.cpp` to compare the BIR identity
for `%sum` against `PreparedCurrentBlockPublicationConsumption` and to show BIR
identity remains derived from the BIR block when prepared producer facts are
intentionally corrupted.

No consumers were switched, and no hook/home/storage/register/emission/move
payload fields were added to BIR publication identity.

## Suggested Next

Next packet: add the first BIR-side block-entry publication semantic identity
shape and test-only comparison against prepared entry publication availability
and destination value identity, still without switching entry consumers or
carrying prepared move/register/storage payload into BIR.

## Watchouts

- The current-block BIR query intentionally ignores prepared producer lookup
  tables; malformed prepared facts can fail closed while BIR still reports the
  block-derived semantic identity.
- Keep entry-publication BIR identity semantic-only. Do not import
  `PreparedBlockEntryPublication` wholesale, and leave register parsing plus
  destination storage decisions in existing prepared/AArch64 consumers until a
  later explicit switch packet.
- Escalate validation if any future packet touches dispatch publication, calls,
  scalar publication planning, value homes, or emission behavior.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

```sh
cmake --build build --target backend_prepared_lookup_helper_test backend_prepare_frame_stack_call_contract_test backend_prealloc_block_entry_publications_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|prepare_frame_stack_call_contract|prealloc_block_entry_publications)' > test_after.log 2>&1
```

`test_after.log`: 3/3 targeted tests passed.
