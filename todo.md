Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Store-Source Plan From AArch64

# Current Packet

## Just Finished

Completed the remaining narrow Step 3 AArch64 adapter slice in
`dispatch_store_sources.cpp`.

`lower_pointer_base_plus_offset_store_local_publication` now constructs
`prepare::PreparedStoreSourcePublicationPlan` through a target-local
`plan_pointer_base_plus_offset_store_local_publication` adapter. The adapter
supplies the current Prepared/BIR source value, store-local destination
`PreparedMemoryAccess`, destination frame slot/stack object facts, source
`PreparedValueHome`, and store-local publication intent.

Behavior-preserving consumption points:
- pointer-base-plus-offset publication validates the neutral destination frame
  facts and uses the plan destination stack offset plus byte offset for the
  final stack store
- direct-global relocation looks up the neutral plan source value name, with
  the pre-existing Prepared named-value fallback preserved
- pointer address materialization consumes the plan source-home kind/value facts
  while retaining the existing target-local emission helper

AArch64-local policy stayed local: memory operand spelling, store mnemonic and
scalar view selection, direct-global address materialization, pointer-address
materialization, scratch selection, stack-store text, assembler text, side
effects, and final `MachineInstruction` construction.

## Suggested Next

Step 3's planned narrow AArch64 adapter slices are complete.

Step 4 handoff: add cross-target reuse proof around the prealloc
store-source publication record/helper, preferably a focused x86/RISC-V or
record-level backend test that exercises the neutral source-home,
destination-frame, recovered-source, and pointer writeback facts without
including AArch64 codegen headers or machine instruction records.

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
- Keep global relocation, pointer address materialization, stack-store text, and
  scratch/register choices local in any additional AArch64 cleanup.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `161/161` backend tests passed, `0` failed. `git diff --check` passed.
Proof log: `test_after.log`.
