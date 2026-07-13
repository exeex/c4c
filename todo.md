# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Receive selected-global array-decay GEP

## Just Finished

- Completed Plan Step 4.2 with a minimal typed Raw-BIR `LoadNode`: one exact
  `GlobalObjectId` source, scalar integer `Type`, zero operands, and one
  ordinary instruction result exposed through the immutable view.
- Added atomic `LoadSpec` construction of the instruction, `InstResultDef`,
  current-function `SourceValueId` index, value order, and block order with
  rollback on every failure. The importer reuses the Store global registry and
  dispatches only native `LirTypeRef`, `LinkNameId`, and `LirValueId` facts.
- Proved two neighboring loads with misleading displays, Raw/Canonical
  source-ID lookup, cross-function lookup isolation, malformed/raw/duplicate
  rejection, whole-module rollback, and exact focused C boundary progression.

## Suggested Next

- Implement only Step 4.3's authoritative selected-global array-decay GEP row,
  reusing the imported-global registry and ordinary current-function result
  identities without admitting raw or mixed-authority indices.

## Watchouts

- Reuse typed global/result authority and ordered typed GEP indices; do not add
  a display/name side table or reinterpret raw index text.
- Load construction is deliberately atomic, so no safe public builder seam can
  stage a coherent but unresolved Load graph for verifier-negative coverage.
  Do not weaken builder validation or add test-only mutation to create one.
- Keep mixed raw indices, local/SSA bases, scalar return, remaining
  function/CFG/local-object work, and other instruction families fail-closed.

## Proof

- Fresh `cmake --build --preset default` passed.
- Focused `backend_lir_to_bir_interface` CTest passed 1/1.
- Exact `--dump-bir` boundaries: `global_load.c`,
  `lir_identity_global_store.c`, and `aarch64_return_zero_smoke.c` report
  `InvalidVoidReturn`; `lir_identity_global_array_address.c` remains
  `UnsupportedOrdinaryInstruction`.
- Full CTest passed 3033/3033 into `test_after.log`; the canonical regression
  guard reports no new failures or suspicious timeouts against
  `test_before.log`.
