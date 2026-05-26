Status: Active
Source Idea Path: ideas/open/33_riscv_prepared_edge_publication_remaining_stack_destination_sources.md
Source Plan Path: plan.md
Current Step ID: Step 2 / Step 3
Current Step Title: Define And Implement The Selected Target-Local Policy; Add Focused Positive And Negative Coverage

# Current Packet

## Just Finished

Completed Step 2 and Step 3 for the narrow RISC-V prepared edge-publication `StackSlot -> StackSlot` I32 stack-destination policy.

- `src/backend/mir/riscv/codegen/emit.cpp` now accepts only non-aliasing I32 stack sources with signed-12-bit source offsets for I32 stack destinations with signed-12-bit destination offsets.
- The accepted stack-to-stack sequence uses explicit `t0` value scratch: `lw t0, <source>(sp)` followed by `sw t0, <destination>(sp)`.
- The path remains reached only through `prepare::find_unique_indexed_prepared_edge_publication` in `consume_edge_publication_move_intent`; no RISC-V-local publication table or predecessor/successor scan was added.
- Existing `Register -> StackSlot` and `RematerializableImmediate -> StackSlot` I32 behavior remains covered and unchanged.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` now proves the positive I32 stack-to-stack path, shared lookup removal, source/destination stack provenance, and fail-closed I64, missing source offset/size, subword/aggregate source widths, large source offset, large destination offset, overlapping/same-slot, pointer-source, missing-publication, malformed-home, and non-move neighbors.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` now includes a shared lookup fixture that preserves stack-source and stack-destination homes plus move identity for a stack-to-stack publication.

## Suggested Next

Supervisor should review the completed Step 2/3 slice for route quality and decide whether the active plan is ready for closure/deactivation or needs a separate next packet for the later `PointerBasePlusOffset -> StackSlot` policy.

## Watchouts

- Large source offsets remain deliberately unsupported for stack destinations; accepting them would need an explicit address-scratch policy involving `t6` or another reserved register.
- Same-slot and byte-overlapping stack moves currently fail closed rather than being treated as no-ops.
- `PointerBasePlusOffset -> StackSlot` remains unsupported by design and should stay separate from this slice.

## Proof

Ran the supervisor-selected proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_riscv_prepared_edge_publication|backend_prepared_lookup_helper)$' | tee test_after.log`

Result: passed. `test_after.log` is the preserved proof log.
