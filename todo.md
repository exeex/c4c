Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Store-Source Plan From AArch64

# Current Packet

## Just Finished

Completed the first Step 3 AArch64 consumption slice in
`dispatch_store_sources.cpp`.

The local store-source publication path now constructs
`prepare::PreparedStoreSourcePublicationPlan` through a target-local
`plan_store_local_source_publication` adapter. The adapter supplies the current
Prepared/BIR source value, destination `PreparedMemoryAccess`, destination
frame-slot/stack-object identity, source `PreparedValueHome`, and recovered
narrow-store source facts.

Behavior-preserving consumption points:
- local publication uses the plan source value as the stored value
- the prior wide-load-from-narrow-store predicate now reuses the recovered-source
  lookup fact that is also passed into the plan
- reserved-scratch stack-home publication uses the plan source home when present
- the memory-destination branch checks the neutral destination frame-slot
  base-plus-offset facts when the plan is available

AArch64-local policy stayed local: memory operand spelling, store mnemonic and
scalar view selection, scratch selection, register alias checks, publication
emission calls, assembler text, side effects, and final `MachineInstruction`
construction.

## Suggested Next

Continue Step 3 with the next narrow AArch64 adapter slice: consume the
store-source plan in either `lower_stack_homed_pointer_store_writeback` or
`lower_pointer_base_plus_offset_store_local_publication`, starting with whichever
can pass current Prepared/BIR pointer-base and destination facts without moving
address materialization or store emission out of AArch64.

## Watchouts

- Keep AArch64 memory operand spelling, store opcode selection, scratch choice,
  global address materialization, stack-pointer sequences, and final instruction
  construction out of prealloc.
- Do not weaken store expectations or create testcase-shaped shortcuts.
- Do not move same-block scan/emission behavior into the Step 2 helper. The
  helper should describe facts supplied by the caller; AArch64 can remain
  responsible for finding recovered producers until a tighter shared query
  boundary exists.
- The current recovered-source helper still owns same-block producer scanning in
  AArch64; do not move that scan into prealloc in the next packet.
- Pointer writeback and pointer-base-plus-offset publication still need AArch64
  adapters. Keep global relocation, pointer address materialization, stack-store
  text, and scratch/register choices local.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `161/161` backend tests passed, `0` failed. `git diff --check` passed.
Proof log: `test_after.log`.
