Status: Active
Source Idea Path: ideas/open/44_shared_prepared_dynamic_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Existing Supported And Fail-Closed Behavior

# Current Packet

## Just Finished

Step 4 of `plan.md`: validated the existing supported and fail-closed behavior
for the dynamic stack-source authority work without changing implementation or
tests.

The existing RISC-V prepared edge-publication coverage records the supported
same-width i32 `LoadLocal`-produced dynamic `StackSlot -> Register` load from
shared source-memory authority, while preserving StackSlot provenance without
inventing a concrete stack offset. It also keeps the concrete-offset i32/i64
stack-source paths, large-offset i32/i64 stack-source materialization paths,
pointer-base materialization path, and stack-destination/stack-to-stack
coverage intact.

The existing negative coverage remains fail-closed for missing shared lookups,
missing publication authority, cleared publication lookup authority, dynamic
stack sources without prepared source-memory access, incomplete source-memory
facts, unavailable/address-materialization-required source-memory access,
non-i32 dynamic source-memory loads, pointer-base decorations without stack-load
authority, subword/sign-extension cases without prepared authority, aggregate
widths, unsupported homes, non-move edge publications, and stack-source or
stack-destination forms lacking concrete-offset authority where required.

## Suggested Next

Supervisor should compare `test_baseline.log` against `test_after.log` and
decide whether to close, deactivate, or extend the active lifecycle state.

## Watchouts

This packet was validation-only. No implementation files, tests, `plan.md`, or
source idea files were modified.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: build passed; backend CTest passed 163/163. Proof log:
`test_after.log`.

Then ran the delegated closure proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure | tee test_after.log`

Result: build passed; full CTest passed 3411/3411. Proof log:
`test_after.log`.
