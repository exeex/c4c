# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the BIR edge publication relationship surface

## Just Finished

Step 2 added the target-neutral BIR CFG edge publication/source identity query
surface in `src/backend/mir/query.hpp` and `src/backend/mir/query.cpp`.

Completed API/work item:

- Added `BirCfgEdgePublicationSourceRequest`,
  `BirCfgEdgePublicationSourceIdentity`, and
  `BirCfgEdgePublicationSourceStatus`, keyed by predecessor block/label,
  successor block/label, and destination value/name/id.
- The query derives semantic edge publication from successor PHI incoming
  identity and attaches predecessor-block source identity, source producer kind,
  producer block/instruction identity, and optional load-local
  `BirMemoryAccessIdentity`.
- Extended `BirMemoryAccessIdentity` with semantic access `byte_offset`,
  `size_bytes`, and `align_bytes` copied from BIR `MemoryAddress`.
- Kept prepared homes, move bundles, parallel-copy scheduling, destination
  storage/register facts, coalescing flags, execution phase/site/carrier facts,
  and prepared move records out of the BIR surface.
- Added focused coverage in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` comparing the new
  BIR query against prepared edge-publication oracle behavior for a normal
  named load-local edge, a named non-memory producer edge, missing publication,
  and unavailable named source producer.

## Suggested Next

Proposed Step 3 packet: add prepared-vs-BIR equivalence proof for the new edge
source identity surface before any consumer switch. Cover normal edge,
no-source/unavailable-source, optional load-local memory-source identity, and
negative predecessor/successor/destination-key paths against the prepared
oracle while keeping MIR/AArch64 consumers unchanged.

## Watchouts

- The Step 2 query intentionally remains a BIR semantic identity surface, not a
  move-routing surface. Do not add homes, move records, destination register
  names, storage-sharing/coalescing flags, execution-site/phase/carrier fields,
  or parallel-copy scheduling fields to it.
- Source value ids remain optional because raw BIR PHI incoming values do not
  carry prepared value ids. Callers that need prepared ids still need a separate
  name/id authority until consumers are switched deliberately.
- Optional memory-source coverage is included only through
  `BirMemoryAccessIdentity`; target addressing policy such as base-plus-offset
  usability and address-materialization requirements remains outside BIR.
- `clang-format` was not available in this environment; `git diff --check` was
  clean.

## Proof

Ran the delegated proof:

`cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^backend_prepared_lookup_helper$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the passing
`backend_prepared_lookup_helper` CTest run.
