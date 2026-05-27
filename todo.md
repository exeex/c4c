Status: Active
Source Idea Path: ideas/open/44_shared_prepared_dynamic_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Shared Prepared Dynamic Stack-Source Authority

# Current Packet

## Just Finished

Step 2 of `plan.md`: added shared prepared source-memory-access authority for
`LoadLocal`-produced dynamic `StackSlot` edge-publication sources.

`PreparedEdgePublication` now carries a target-neutral
`source_memory_access_status` plus copied source memory-access facts:
base kind/name, byte offset, size/alignment, address space, volatility, and
base-plus-offset/address-materialization requirement. The lookup builder
populates those facts only from the exact `PreparedMemoryAccess` at the
published `LoadLocal` producer block/instruction. Non-`LoadLocal` sources stay
`unavailable`; `LoadLocal` producers with no matching prepared access report
`missing_prepared_memory_access`; malformed/mismatched accesses report
`incomplete_prepared_memory_access`.

Focused RISC-V edge-publication coverage now proves the available dynamic
`StackSlot` source facts are exposed, missing/incomplete states remain
distinct, non-`LoadLocal` sources remain unavailable, and current RISC-V
dynamic stack-source lowering still fails closed until Step 3 consumes the
shared authority.

## Suggested Next

Step 3 should consume the shared `source_memory_access_status == available`
facts in RISC-V for the selected same-width i32 dynamic `StackSlot -> Register`
family, while preserving fail-closed behavior for unavailable, missing, and
incomplete source memory-access states.

## Watchouts

The new shared fact does not by itself authorize RISC-V lowering. Step 3 should
require `LoadLocal`, `PreparedValueHomeKind::StackSlot`, no concrete
`offset_bytes`, `source_memory_access_status == available`, matching i32 width,
valid OutOfSsa move authority, and a target-usable address contract. Keep
`PointerBasePlusOffset` as address-value materialization, not source memory
load authority.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: build passed; backend CTest passed 163/163. Proof log:
`test_after.log`.
