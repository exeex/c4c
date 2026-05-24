Status: Active
Source Idea Path: ideas/open/prealloc-store-source-publication-planning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Prealloc Store-Source Plan Record

# Current Packet

## Just Finished

Completed Step 2: added `prepare::PreparedStoreSourcePublicationPlan`,
`PreparedStoreSourcePublicationInputs`,
`PreparedStoreSourcePublicationStatus`,
`PreparedStoreSourcePublicationIntent`, availability/status helpers, and
`plan_prepared_store_source_publication` in `publication_plans.*`.

The new record/helper captures target-neutral store-source facts from
Prepared/BIR inputs only:
- logical source `bir::Value`, source `PreparedValueId`, and source
  `ValueNameId`
- destination `PreparedMemoryAccess` identity, address base kind, frame-slot or
  pointer-value identity, byte offset, size/alignment, volatility, and
  base-plus-offset capability
- optional destination `PreparedFrameSlot` / `PreparedStackObject` identity
- source `PreparedValueHome` kind, storage encoding, stack slot/offset/size, and
  pointer base/byte-delta facts
- optional recovered source value plus recovered instruction index
- pending publication, stack-homes-only, duplicate-publication, and pointer
  store-writeback intent bits
- optional pointer-base home kind/slot/offset for pointer writeback adapters

Added `backend_store_source_publication_plan`, a focused backend MIR C++ test
with no AArch64 codegen headers and no machine instruction records. It covers
local frame-slot store identity, recovered source facts, pointer writeback
intent, store-global pending-publication flags, and incomplete input statuses.

## Suggested Next

Execute Step 3 with the lowest-risk AArch64 adapter slice: construct
`PreparedStoreSourcePublicationPlan` for the local store-source publication path
inside `dispatch_store_sources.cpp`, consume only neutral source/destination
facts from the plan, and preserve existing AArch64 emission behavior.

## Watchouts

- Keep AArch64 memory operand spelling, store opcode selection, scratch choice,
  global address materialization, stack-pointer sequences, and final instruction
  construction out of prealloc.
- Do not weaken store expectations or create testcase-shaped shortcuts.
- Do not move same-block scan/emission behavior into the Step 2 helper. The
  helper should describe facts supplied by the caller; AArch64 can remain
  responsible for finding recovered producers until a tighter shared query
  boundary exists.
- Step 3 should pass existing recovered-source and pointer-writeback facts into
  the plan from AArch64 instead of moving producer scans into prealloc.
- The new store-source plan intentionally reuses
  `prepared_publication_storage_encoding_from_home` to keep source-home storage
  classification aligned with scalar publication planning.

## Proof

Ran:
`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: `161/161` backend tests passed, `0` failed. `git diff --check` passed.
Proof log: `test_after.log`.
