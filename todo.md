# Current Packet

Status: Active
Source Idea Path: ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Fail-Closed Authority

## Just Finished

Completed Step 2 and Step 3 for idea 25. RISC-V now consumes shared
`edge_publications` for the focused `StackSlot -> Register` prepared home case
when the source home has a concrete `offset_bytes` and 4-byte `size_bytes`.
The target-local emission is `lw <dst>, <offset>(sp)` after shared lookup
authority succeeds.

The implementation preserves the existing `Register -> Register` `mv` path and
`RematerializableImmediate -> Register` `li` path. Focused coverage now proves
the positive stack-source load, preserves the shared publication pointer and
prepared value ids on the intent path, and records stack-slot provenance
(`slot_id`, offset, and size) without using it to rediscover edge facts.

Step 3 fail-closed coverage remains explicit: missing shared lookups and
missing publication facts are rejected, stack sources without offset or I32
size are rejected, pointer-base sources remain unsupported, source-to-`StackSlot`
destinations remain unsupported, and non-move publications remain unsupported.

## Suggested Next

Proceed to Step 4 validation review. The focused proof is green in
`test_after.log`; the supervisor should decide whether to run a matching guard
and/or a broader backend bucket before handoff.

## Watchouts

The stack-source policy is intentionally focused to 4-byte loads from prepared
stack offsets. Pointer-base sources, non-I32 stack-source widths, missing stack
offsets, and all source-to-`StackSlot` destinations remain unsupported and
fail closed. This packet did not implement broader RISC-V frame lowering,
large-offset handling, dynamic stack addressing, or stack destinations.

## Proof

Ran the delegated focused proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: PASS, 5/5 selected tests passed.

Supervisor ran the matching regression guard against the focused
`test_before.log` and focused `test_after.log` with non-decreasing pass count
allowed because this packet extended an existing CTest binary. Result: PASS
with 5/5 before and 5/5 after, no new failures.

Supervisor then ran broader backend validation:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 163/163 backend tests passing.
This backend run is broader validation only, not the matching before/after
regression comparison for the focused packet.
