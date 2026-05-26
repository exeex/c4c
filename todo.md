# Current Packet

Status: Active
Source Idea Path: ideas/open/25_riscv_prepared_edge_publication_stack_source_register_consumer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff or Close

## Just Finished

Completed Step 5 for idea 25. The RISC-V stack-source route is ready for
closure with exactly scoped support for focused `StackSlot -> Register`
register-destination edge-publication moves through shared `edge_publications`.
The supported stack source must carry a concrete offset and a 4-byte size, and
the target-local emission is `lw <dst>, <offset>(sp)` after shared lookup
authority succeeds.

The stack-source slice preserves the existing RISC-V `Register -> Register`
`mv` path and `RematerializableImmediate -> Register` `li` path. Focused
coverage proves the positive stack-source load, preserves the shared
publication pointer and prepared value ids on the intent path, and records
stack-slot provenance (`slot_id`, offset, and size) without using it to
rediscover edge facts.

Validation and review evidence is sufficient for handoff: the focused RISC-V
prepared subset passed 5/5, the matching focused regression guard passed with
no new failures, broader backend validation passed 163/163, and
`review/idea25_riscv_stack_source_edge_publication_review.md` reported no
blocking findings.

## Suggested Next

Ask the plan owner to close idea 25. Any future work for pointer-base sources,
source-to-`StackSlot` destinations, non-I32 stack-source widths, missing
offsets, dynamic stack addressing, or large-offset policy should remain in
separate follow-up lifecycle packets.

## Watchouts

Do not overclaim the closure. Idea 25 covers only focused
`StackSlot -> Register` register-destination moves with concrete stack offsets
and 4-byte source size. Pointer-base sources, source-to-`StackSlot`
destinations, stack sources without offsets, non-I32 widths, dynamic stack
addressing, and large-offset policy remain unsupported and fail closed.

## Proof

No tests were run for this handoff-only packet per `docs/handoff-only`; no
`test_after.log` was created by this packet.

Previously accepted evidence for this slice:

Focused proof command:
`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_codegen_route_riscv64_.*|backend_prepared_lookup_helper|backend_prealloc_block_entry_publications)$' >> test_after.log 2>&1`

Result: PASS, 5/5 selected tests passed.

Matching focused regression guard result: PASS with 5/5 before and 5/5 after,
no new failures.

Broader backend validation command:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1`

Result: PASS. Build succeeded and CTest reported 163/163 backend tests passing.
This backend run is broader validation only, not the matching before/after
regression comparison for the focused packet.
