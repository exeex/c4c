Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Store-Source Plan From AArch64

# Current Packet

## Just Finished

Completed another Step 3 AArch64 consumption slice in
`dispatch_store_sources.cpp`.

The stack-homed pointer store writeback path now constructs
`prepare::PreparedStoreSourcePublicationPlan` through a target-local
`plan_stack_homed_pointer_store_writeback` adapter. The adapter supplies the
current Prepared/BIR source value, pointer-value destination
`PreparedMemoryAccess`, source `PreparedValueHome`, pointer-base
`PreparedValueHome`, and pointer-writeback intent.

Behavior-preserving consumption points:
- pointer writeback uses the plan source value for source register lookup and
  fallback publication emission
- writeback validates the neutral pointer-value destination facts and uses the
  plan destination byte offset for the final register-indirect store
- writeback validates the neutral pointer-base stack-home facts and uses the
  plan pointer-base stack offset for the address-register reload

AArch64-local policy stayed local: memory operand spelling, store mnemonic and
scalar view selection, scratch selection, register alias checks, publication
emission calls, assembler text, side effects, and final `MachineInstruction`
construction.

## Suggested Next

Continue Step 3 with the remaining narrow AArch64 adapter slice:
`lower_pointer_base_plus_offset_store_local_publication`. It can likely pass
current Prepared/BIR source-home and destination-frame facts into the plan, then
keep direct-global relocation, pointer address materialization, stack-store text,
and scratch/register choices in AArch64.

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
- Pointer-base-plus-offset publication still needs an AArch64 adapter. Keep
  global relocation, pointer address materialization, stack-store text, and
  scratch/register choices local.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `161/161` backend tests passed, `0` failed. `git diff --check` passed.
Proof log: `test_after.log`.
