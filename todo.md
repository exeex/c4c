Status: Active
Source Idea Path: ideas/open/44_shared_prepared_dynamic_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Shared Dynamic Authority In RISC-V

# Current Packet

## Just Finished

Step 3 of `plan.md`: consumed shared prepared dynamic stack-source authority in
the RISC-V edge-publication helper for the selected same-width i32
`LoadLocal`-produced `StackSlot -> Register` family.

RISC-V now emits `lw` from dynamic `StackSlot` source memory only when the
shared publication is available, the source producer is `LoadLocal`, the source
home is `StackSlot` with no concrete `offset_bytes`, the shared source memory
access is `available`, the source/destination types and memory width are i32,
the move has valid OutOfSsa parallel-copy authority, and the shared address
contract is a target-usable pointer base plus signed-12-bit offset. The helper
records shared source-memory base/offset/size/alignment facts without inventing
a stack offset.

Missing, unavailable, incomplete, address-materialization-required, and non-i32
dynamic source-memory cases remain fail-closed. Clearing shared lookup or
publication authority prevents RISC-V from rediscovering the dynamic load, and
ordinary pointer-base publication still remains address-value materialization
rather than a memory load.

## Suggested Next

Step 4 should validate existing supported and fail-closed behavior around the
RISC-V edge-publication coverage, with special attention to concrete offset,
large-offset, and dynamic-neighbor guardrails.

## Watchouts

The RISC-V consumer is intentionally narrow: it only handles pointer-value
base-plus-offset source memory that fits the RISC-V load immediate and does not
require address materialization. Future dynamic source-memory families need
their own shared scratch/address-materialization contract before RISC-V should
emit them.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: build passed; backend CTest passed 163/163. Proof log:
`test_after.log`.
